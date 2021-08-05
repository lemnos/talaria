/*
 * ---------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ---------------------------------------------------------------------
 * 
 * Original Authors: Aetnaeus (https://github.com/lemnos)
 *
 */

#include "common.h"
#include "draw.h"
#include "input.h"
#include "evloop.h"
#include "utf8.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

/* TODO 
 * - Implement mouse support (maybe).
*/

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

static void draw(cairo_t *cr, void *_ctx) {
    int curoff;
    static int xoff = 0;
    struct ui_input *ctx = _ctx;

    if(ctx->cursor_pos == 0)
        xoff = 0;
    else {
        char *s = utf8_dup(ctx->input, ctx->cursor_pos);
        curoff = cairo_text_box_width(cr,
                ctx->fonts,
                s,
                ctx->h);
        free(s);

        if(curoff < xoff)
            xoff = curoff - 2;
        if(curoff > (xoff + ctx->w))
            xoff = curoff - ctx->w;
    }

    cairo_save(cr);

    cairo_rectangle(cr, ctx->x, ctx->y, ctx->w, ctx->h);
    cairo_clip_preserve(cr);

    cairo_text_box(cr, ctx->fonts, ctx->input, 
            -xoff + ctx->x, ctx->y, 
            ctx->w + xoff,
            ctx->h,
            ctx->cursor_pos,
            ctx->sel.start,
            ctx->sel.end,
            (double[]){ctx->fgcol.r, ctx->fgcol.g, ctx->fgcol.b, ctx->fgcol.a},
            (double[]){ctx->bgcol.r, ctx->bgcol.g, ctx->bgcol.b, ctx->bgcol.a},
			ctx->show_cursor ?
				(double[]){ctx->fgcol.r, ctx->fgcol.g, ctx->fgcol.b, ctx->fgcol.a} :
				(double[]){ctx->bgcol.r, ctx->bgcol.g, ctx->bgcol.b, ctx->bgcol.a});

    cairo_restore(cr);
}

static void set_input(struct ui_input *ctx, const char *input) {
    ctx->sel.start = -1;
    ctx->sel.end = -1;
    ctx->cursor_pos = utf8_len(input);
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
    if(!ctx->last_key.meta || (
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
    if(!ctx->last_key.meta || (
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

    char c[5];
    const int len = strlen(ctx->input);
    char *start = ctx->input + utf8_idx(ctx->input, ctx->cursor_pos-1);
    const int clen = utf8_read(start, c);

    memmove(start,
            start + clen,
            strlen(start + clen));

    ctx->input[len - clen] = '\0';
    ctx->cursor_pos--;
}

static void retreat_char(struct ui_input *ctx) {
    ctx->cursor_pos = ctx->cursor_pos == 0 ? 0 : ctx->cursor_pos-1;
}

static void advance_char(struct ui_input *ctx) {
    const size_t max = utf8_len(ctx->input);
    ctx->cursor_pos = ctx->cursor_pos >= max ? max : ctx->cursor_pos+1;
}

static void move_to_bol(struct ui_input *ctx) {
    ctx->cursor_pos = 0;
    ctx->sel.start = -1;
    ctx->sel.end = -1;
}

static void move_to_eol(struct ui_input *ctx) {
    ctx->cursor_pos = utf8_len(ctx->input);
    ctx->sel.start = -1;
    ctx->sel.end = -1;
}

const char WORD_DELIMS[] = " -./!@#$%^&*()_+";
#define INPCH(ctx, n) (ctx->input + utf8_idx(ctx->input, n))

static void advance_word(struct ui_input *ctx) {
    size_t max = utf8_len(ctx->input);

    while(ctx->cursor_pos != max &&
            *INPCH(ctx, ctx->cursor_pos) == ' ')
        ctx->cursor_pos++;

    if(strchr(WORD_DELIMS, *INPCH(ctx, ctx->cursor_pos))) {
        if(ctx->cursor_pos != max)
            ctx->cursor_pos++;
        return;
    }

    while(ctx->cursor_pos != max && 
            !strchr(WORD_DELIMS, *INPCH(ctx, ctx->cursor_pos)))
        ctx->cursor_pos++;
}

static void retreat_word(struct ui_input *ctx) {
    while(ctx->cursor_pos != 0 &&
            *INPCH(ctx, ctx->cursor_pos-1) == ' ')
        ctx->cursor_pos--;

    if(ctx->cursor_pos && 
            strchr(WORD_DELIMS, *INPCH(ctx, ctx->cursor_pos-1))) {
        ctx->cursor_pos--;
        return;
    }


    while(ctx->cursor_pos && 
            !strchr(WORD_DELIMS, *INPCH(ctx, ctx->cursor_pos-1)))
        ctx->cursor_pos--;
}

static void select_prev_word(struct ui_input *ctx) {
    ctx->sel.end = ctx->sel.end == -1 ? (ssize_t)ctx->cursor_pos-1 : ctx->sel.end;
    retreat_word(ctx);
    ctx->sel.start = ctx->cursor_pos;
}

static void select_next_word(struct ui_input *ctx) {
    ctx->sel.start = ctx->sel.start == -1 ? (int)ctx->cursor_pos : ctx->sel.start;
    advance_word(ctx);
    ctx->sel.end = ctx->cursor_pos-1;
}

static void delete_word_backward(struct ui_input *ctx) {
    char *start, *end;
    if(!ctx->cursor_pos) return;

    start = ctx->input + utf8_idx(ctx->input, ctx->cursor_pos);
    retreat_word(ctx);
    end = ctx->input + utf8_idx(ctx->input, ctx->cursor_pos);

    memmove(end, start, strlen(start)+1);
}

static void delete_selection(struct ui_input *ctx) {
        char *start, *end;

        ctx->cursor_pos = ctx->sel.start;
        start = ctx->input + utf8_idx(ctx->input, ctx->sel.start);
        end = ctx->input + utf8_idx(ctx->input, ctx->sel.end+1);

        memcpy(start, end, strlen(end)+1);

        ctx->sel.start = -1;
        ctx->sel.end = -1;
}
    
static void add_string(struct ui_input *ctx, const char *str) {
    char *tmp;

    if(ctx->sel.end != -1)
        delete_selection(ctx);

    tmp = ctx->input;
    ctx->input = insert(ctx->input, str, utf8_idx(ctx->input, ctx->cursor_pos));
    free(tmp);
    ctx->cursor_pos += utf8_len(str);
}

//Returns non zero if the associated cairo context is updated.o
static int handle_event(void *_ctx, struct ui_event *ev) {
    struct ui_input* ctx = _ctx;
    int redraw = 0;
    switch(ev->type) { 
        case KEYBOARD_EV:
        if(ev->key.ctrl && !strcmp(ev->key.sym, "a")) {
            ctx->sel.start = 0;
            ctx->sel.end = utf8_len(ctx->input)-1;
        } else if(!strcmp(ev->key.sym, "\t")) {
            complete(ctx);
        } else if(!strcmp(ev->key.sym, "⇤")) { //backtab
            backward_complete(ctx);
        } else if(!strcmp(ev->key.sym, "<paste>") || (ev->key.ctrl && !strcmp(ev->key.sym, "v")) || (ev->key.ctrl && !strcmp(ev->key.sym, "y"))) {
            char *buf = ui_evloop_get_paste_buffer(ctx->loop);
            for(char *c=buf;*c;c++)
               if(*c == '\n') *c = ' ';
            add_string(ctx, buf);
            free(buf);
        } else if(ev->key.ctrl && !strcmp(ev->key.sym, "u")) {
            set_input(ctx, "");
        } else if(ev->key.ctrl && !strcmp(ev->key.sym, "e")) {
            move_to_eol(ctx);
        } else if((ev->key.ctrl || ev->key.meta) && (!strcmp(ev->key.sym, "\x08") || !strcmp(ev->key.sym, "w"))) {
            delete_word_backward(ctx);
        } else if(!strcmp(ev->key.sym, "\x08")) { //Backspace
            if(ctx->sel.end != -1)
                delete_selection(ctx);
            else
                delete_char_backward(ctx);
        } else if(ev->key.ctrl && ev->key.shift &&
                !strcmp(ev->key.sym, "<left>"))
            select_prev_word(ctx);
        else if(ev->key.ctrl && ev->key.shift &&
                !strcmp(ev->key.sym, "<right>"))
            select_next_word(ctx);
        else if((ev->key.meta && !strcmp(ev->key.sym, "b")) || (ev->key.ctrl && !strcmp(ev->key.sym, "<left>")))
            retreat_word(ctx);
        else if((ev->key.meta && !strcmp(ev->key.sym, "f")) || (ev->key.ctrl && !strcmp(ev->key.sym, "<right>")))
            advance_word(ctx);
        else if((ev->key.ctrl && !strcmp(ev->key.sym, "f")) || !strcmp(ev->key.sym, "<right>"))
            if(ctx->sel.start != -1) {
                ctx->cursor_pos = ctx->sel.end + 1;
                ctx->sel.start = -1;
                ctx->sel.end = -1;
             } else
                advance_char(ctx);
        else if((ev->key.ctrl && !strcmp(ev->key.sym, "b")) || !strcmp(ev->key.sym, "<left>"))
            if(ctx->sel.start != -1) {
                ctx->cursor_pos = ctx->sel.start;
                ctx->sel.start = -1;
                ctx->sel.end = -1;
            } else
                retreat_char(ctx);
        else if(ev->key.meta && (!strcmp(ev->key.sym, "<up>") || !strcmp(ev->key.sym, "p")))
            history_up(ctx);
        else if(ev->key.meta && (!strcmp(ev->key.sym, "<down>") || !strcmp(ev->key.sym, "n")))
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
        //TODO maybe implement
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

struct ui_widget* ui_create_input(
        struct ui_evloop* loop,
        FcFontSet *fonts,
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
    ctx->fonts = fonts;

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
