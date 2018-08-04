#include "common.h"
#include "input.h"
#include "evloop.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

//TODO properly implement selection and mouse support (maybe).

static char *insert(const char *dst, const char *str, size_t pos) {
    const size_t lstr = str ? strlen(str) : 0;
    const size_t ldst = dst ? strlen(dst) : 0;
    char *res = malloc(lstr + ldst + 1);
    char *c = res;

    for(size_t i=0;i<pos;i++)
        *c++ = dst[i];
    for(size_t i=0;i<lstr;i++)
        *c++ = str[i];
    for(size_t i=pos;i<ldst;i++)
        *c++ = dst[i];
    *c = '\0';

    return res;
}


//Returns the number of a character at position x as where would be drawn in
//the given string using the provided cairo context.

static int xth_char(cairo_t *cr, const char *input, const int x, const int h) {
    size_t i;

    char *tmp = strdup(input);

    for(i=0;i < strlen(input);i++) {
        strcpy(tmp, input);
        tmp[i] = '\0';
        if(ui_text_width(cr, h, tmp) >= x) {
            return i-1;
        }
    }

    free(tmp);
    return -1;
}

static void draw(cairo_t *cr, void *_ctx) {
    struct ui_input *ctx = _ctx;
    const int cursor_sz = 1;
    const int tw = ui_text_width(cr, ctx->h, ctx->input);

    int xoff = tw+cursor_sz > ctx->w ?
        ctx->x - tw + ctx->w - cursor_sz:
        ctx->x;

    cairo_save(cr);

    cairo_rectangle(cr, ctx->x, ctx->y, ctx->w, ctx->h);
    cairo_clip_preserve(cr);

    xoff = ui_draw_text_box(
            cr,
            xoff, ctx->y, 
            ctx->w, ctx->h,
            0,
            &ctx->bgcol,
            &ctx->fgcol,
            ctx->input, ctx->sel.start, ctx->sel.end, ctx->show_cursor ? (int)ctx->cursor_pos : -1);

    cairo_restore(cr);
}

static void set_input(struct ui_input *ctx, const char *input) {
    ctx->sel.start = -1;
    ctx->sel.end = -1;
    ctx->cursor_pos = strlen(input);
    free(ctx->input);
    ctx->input = strdup(input);
}

static void backward_complete(struct ui_input *ctx) {
    if(strcmp(ctx->last_key.sym, "⇤") && 
            strcmp(ctx->last_key.sym, "\t")) return;

    ctx->last_completion_idx--;
    ctx->last_completion_idx = ctx->last_completion_idx < 0 ? 
        0 : ctx->last_completion_idx;
    set_input(ctx, ctx->completions[ctx->last_completion_idx]);
}

static void complete(struct ui_input *ctx) {
    if(strcmp(ctx->last_key.sym, "⇤") && 
            strcmp(ctx->last_key.sym, "\t")) {
        free(ctx->completions);
        ctx->completions_sz = ctx->completion_fn(ctx->input, &ctx->completions);
        ctx->last_completion_idx = -1;
    }

    if(!ctx->completions_sz) return;

    ctx->last_completion_idx++;

    if((size_t)ctx->last_completion_idx == ctx->completions_sz) {
        //Restart completions on the last entry.
        ctx->last_key.sym[0] = ' ';
        ctx->last_key.sym[1] = '\0';
        complete(ctx);
    }

    set_input(ctx, ctx->completions[ctx->last_completion_idx]);
}

static void history_up(struct ui_input *ctx) {
    if(!ctx->history_sz) return;
    if(!ctx->last_key.alt || (
                strcmp(ctx->last_key.sym, "<down>") &&
                strcmp(ctx->last_key.sym, "<up>") &&
                strcmp(ctx->last_key.sym, "n") &&
                strcmp(ctx->last_key.sym, "p")))
        ctx->last_hist_idx = ctx->history_sz;

    ctx->last_hist_idx = ctx->last_hist_idx == 0 ? 0 : ctx->last_hist_idx - 1;
    set_input(ctx, ctx->history[ctx->last_hist_idx]);
}

static void history_down(struct ui_input *ctx) {
    if(!ctx->history_sz) return;
    if(!ctx->last_key.alt || (
                strcmp(ctx->last_key.sym, "<down>") &&
                strcmp(ctx->last_key.sym, "<up>") &&
                strcmp(ctx->last_key.sym, "n") &&
                strcmp(ctx->last_key.sym, "p")))
        ctx->last_hist_idx = ctx->history_sz-1;

    ctx->last_hist_idx = (ctx->last_hist_idx == ctx->history_sz-1) ? ctx->last_hist_idx : ctx->last_hist_idx + 1;
    set_input(ctx, ctx->history[ctx->last_hist_idx]);
}

static void delete_char_backward(struct ui_input *ctx) {
    if(!ctx->cursor_pos) return;

    char *start = ctx->input + ctx->cursor_pos - 1;
    while(((*start & 0xC0) == 0x80) && start != 0)
        start--;

    memmove(start,
            ctx->input+ctx->cursor_pos,
            strlen(ctx->input)-ctx->cursor_pos+1);

    ctx->cursor_pos=start-ctx->input;
}

static void delete_word_backward(struct ui_input *ctx) {
    if(!ctx->cursor_pos) return;
    size_t start = ctx->cursor_pos-1;
    for(;start > 0 && ctx->input[start] == ' ';start--);
    for(;start > 0 && ctx->input[start] != ' ';start--);
    if(ctx->input[start] == ' ')
        start++;
    memmove(ctx->input+start,
            ctx->input+ctx->cursor_pos,
            strlen(ctx->input)-ctx->cursor_pos+1);
    ctx->cursor_pos = start;
}

static void retreat_char(struct ui_input *ctx) {
    while(ctx->cursor_pos && ((ctx->input[ctx->cursor_pos-1] & 0xC0) == 0x80))
        ctx->cursor_pos--;

    ctx->cursor_pos = ctx->cursor_pos ? ctx->cursor_pos - 1 : ctx->cursor_pos;
}

static void advance_char(struct ui_input *ctx) {
    if(ctx->cursor_pos == strlen(ctx->input)) return;
    char c[5];
    ctx->cursor_pos += read_utf8_char(ctx->input + ctx->cursor_pos, c);
}

static void move_to_bol(struct ui_input *ctx) {
    ctx->cursor_pos = 0;
    ctx->sel.start = -1;
    ctx->sel.end = -1;
}

static void move_to_eol(struct ui_input *ctx) {
    ctx->cursor_pos = strlen(ctx->input);
    ctx->sel.start = -1;
    ctx->sel.end = -1;
}

static void advance_word(struct ui_input *ctx) {
    size_t max = strlen(ctx->input);
    while(ctx->cursor_pos != max && ctx->input[ctx->cursor_pos] == ' ')
        ctx->cursor_pos++;
    while(ctx->cursor_pos != max && ctx->input[ctx->cursor_pos] != ' ')
        ctx->cursor_pos++;
}

static void retreat_word(struct ui_input *ctx) {
    if(!ctx->cursor_pos) return;

    while(ctx->cursor_pos && ctx->input[ctx->cursor_pos-1] == ' ')
        ctx->cursor_pos--;
    while(ctx->cursor_pos && ctx->input[ctx->cursor_pos-1] != ' ')
        ctx->cursor_pos--;
}

static void add_string(struct ui_input *ctx, const char *str) {
    char *tmp;

    if(ctx->sel.end != -1) {
        ctx->sel.start = -1;
        ctx->sel.end = -1;
        ctx->cursor_pos = 0;
        if(ctx->input)
            ctx->input[0] = '\0';
    }

    tmp = ctx->input;
    ctx->input = insert(ctx->input, str, ctx->cursor_pos);
    free(tmp);
    ctx->cursor_pos+=strlen(str);
}

//Returns non zero if the associated cairo context is updated.o
static int handle_event(void *_ctx, struct ui_event *ev) {
    struct ui_input* ctx = _ctx;
    int redraw = 0;
    switch(ev->type) { 
        int i, start, end;

        case KEYBOARD_EV:
        if(ev->key.ctrl && !strcmp(ev->key.sym, "a")) {
            ctx->sel.start = 0;
            ctx->sel.end = (int)strlen(ctx->input);
        } else if(!strcmp(ev->key.sym, "\t")) {
            complete(ctx);
        } else if(!strcmp(ev->key.sym, "⇤")) { //backtab
            backward_complete(ctx);
        } else if(!strcmp(ev->key.sym, "<paste>") || (ev->key.ctrl && !strcmp(ev->key.sym, "v"))) {
            char *buf = ui_evloop_get_paste_buffer(ctx->loop);
            for(char *c=buf;*c;c++)
               if(*c == '\n') *c = ' ';
            add_string(ctx, buf);
            free(buf);
        } else if(ev->key.ctrl && !strcmp(ev->key.sym, "u")) {
            set_input(ctx, "");
        } else if(ev->key.ctrl && !strcmp(ev->key.sym, "e")) {
            ctx->sel.start = -1;
            ctx->sel.end = -1;
            ctx->cursor_pos = strlen(ctx->input);
        } else if((ev->key.ctrl || ev->key.alt) && (!strcmp(ev->key.sym, "\x08") || !strcmp(ev->key.sym, "w"))) {
            delete_word_backward(ctx);
        } else if(!strcmp(ev->key.sym, "\x08")) {
            if(ctx->sel.end != -1)
                set_input(ctx, "");
            else
                delete_char_backward(ctx);
        }
        else if((ev->key.alt && !strcmp(ev->key.sym, "b")) || (ev->key.ctrl && !strcmp(ev->key.sym, "<left>")))
            retreat_word(ctx);
        else if((ev->key.alt && !strcmp(ev->key.sym, "f")) || (ev->key.ctrl && !strcmp(ev->key.sym, "<right>")))
            advance_word(ctx);
        else if((ev->key.ctrl && !strcmp(ev->key.sym, "f")) || !strcmp(ev->key.sym, "<right>"))
            if(ctx->sel.start != -1)
                move_to_eol(ctx);
            else
                advance_char(ctx);
        else if((ev->key.ctrl && !strcmp(ev->key.sym, "b")) || !strcmp(ev->key.sym, "<left>"))
            if(ctx->sel.start != -1)
                move_to_bol(ctx);
            else
                retreat_char(ctx);
        else if(ev->key.alt && (!strcmp(ev->key.sym, "<up>") || !strcmp(ev->key.sym, "p")))
            history_up(ctx);
        else if(ev->key.alt && (!strcmp(ev->key.sym, "<down>") || !strcmp(ev->key.sym, "n")))
            history_down(ctx);
        else if(!strcmp(ev->key.sym, "<page_up>") ||
                !strcmp(ev->key.sym, "<page_down>") ||
                !strcmp(ev->key.sym, "<up>") ||
                !strcmp(ev->key.sym, "<enter>") ||
                !strcmp(ev->key.sym, "<down>"))
        {}
        else
            add_string(ctx, ev->key.sym);

        ctx->last_blink_update = ev->time;
        ctx->show_cursor = 1;

        ctx->last_key = ev->key;

        redraw = 1;

        //Cleanup, keep track of changes with a flag.
        if(ctx->input_update_cb && strcmp(ctx->last_input, ctx->input)) 
            ctx->input_update_cb(ctx, ctx->input);

        free(ctx->last_input);
        ctx->last_input = strdup(ctx->input);
        break;
        case MOUSE_CLICK:
        i = xth_char(ev->cr, ctx->input, ev->x - ctx->x, ctx->h);

        if(i == -1) {
            ctx->sel.start = -1;
            ctx->sel.end = -1;
            redraw = 1;
        } else if(ctx->input[i] == ' ') 
            return 0;
        else {
            for(start = i;
                    start != 0 && ctx->input[start-1] != ' ';
                    start--);
            for(end = i;
                    end != (int)strlen(ctx->input)-1 && ctx->input[end+1] != ' ';
                    end++);

            ctx->sel.start = start;
            ctx->sel.end = end;

            redraw = 1;
        }
        break;
        case TIME_UPDATE:
        break;
    }

    if((ev->time - ctx->last_blink_update) > 500) {
        ctx->last_blink_update = ev->time;
        ctx->show_cursor = !ctx->show_cursor;

        redraw = 1;
    }

    return redraw;
}

char* ui_input_get_input(struct ui_widget *w) {
    return ((struct ui_input*)w->ctx)->input;
}

struct ui_widget* ui_create_input(
        struct ui_evloop* loop,
        double x,
        double y,
        double w,
        double h,
        struct ui_color cursor_color,
        struct ui_color bgcol,
        struct ui_color fgcol,
        size_t (*completion_fn) (const char *str, char ***results),
        const char** history,
        size_t history_sz,
        void(*input_update_cb) (struct ui_input *input_widget, const char *input)) {

    struct ui_widget* wid = malloc(sizeof(struct ui_widget));
    struct ui_input* ctx = malloc(sizeof(struct ui_input));
    ctx->loop = loop;
    ctx->x = x;
    ctx->y = y;
    ctx->w = w;
    ctx->h = h;

    ctx->cursor_color = cursor_color;
    ctx->fgcol = fgcol;
    ctx->bgcol = bgcol;

    ctx->cursor_pos = 0;

    ctx->completions = NULL;
    ctx->last_completion_idx = 0;
    ctx->completions_sz = 0;
    ctx->completion_fn = completion_fn;

    ctx->sel.start = -1;
    ctx->sel.end = -1;
    ctx->last_blink_update = 0;
    ctx->show_cursor = 0;
    ctx->last_input = strdup("");
    ctx->input = strdup("");
    ctx->input_update_cb = input_update_cb;

    ctx->history = history;
    ctx->history_sz = history_sz;

    wid->ctx = ctx;
    wid->event_handler = handle_event;
    wid->redraw = draw;
    return wid;
}
