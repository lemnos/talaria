#include "common.h"
#include "menu.h"
#include "draw.h"
#include <string.h>
#include <stdio.h>

static void scroll_item_into_view(struct ui_menu *ctx) {
        int min_scroll, max_scroll;

        max_scroll = ctx->sel * (ctx->item_height + ctx->divider_height);
        min_scroll = ((ctx->sel+1) * (ctx->item_height + ctx->divider_height)) - ctx->h;
        min_scroll = min_scroll < 0 ? 0 : min_scroll;

        if(ctx->scroll_top < min_scroll)
            ctx->scroll_top = min_scroll;
        else if(ctx->scroll_top > max_scroll)
            ctx->scroll_top = max_scroll;
}

static int on_event(void *_ctx, struct ui_event *ev) {
    struct ui_menu *ctx = _ctx;
    if(ev->type == KEYBOARD_EV) {
        if(ev->key.alt || ev->key.ctrl) return 0;

        if(!strcmp(ev->key.sym, "<up>")) {
            ctx->sel = ctx->sel <= 0 ?
                ctx->sel :
                ctx->sel - 1;
            scroll_item_into_view(ctx);
        } else if (!strcmp(ev->key.sym, "<down>")) {
            ctx->sel = ctx->sel >= ctx->items_sz-1 ?
                ctx->items_sz-1 :
                ctx->sel+1;
            scroll_item_into_view(ctx);
        } else if (!strcmp(ev->key.sym, "<page_up>")) {
            ctx->scroll_top -= ctx->h;
            ctx->scroll_top = ctx->scroll_top < 0 ? 0 : ctx->scroll_top;
            ctx->sel = ctx->scroll_top / (ctx->item_height + ctx->divider_height)+1;

        } else if (!strcmp(ev->key.sym, "<page_down>")) {
            int max = ctx->items_sz * (ctx->item_height + ctx->divider_height) - ctx->h;
            max = max < 0 ? 0 : max;
            ctx->scroll_top += ctx->h;
            ctx->scroll_top = ctx->scroll_top > max ? max : ctx->scroll_top;
            ctx->sel = ctx->scroll_top / (ctx->item_height + ctx->divider_height)+1;
            if(ctx->scroll_top == max)
                ctx->sel = ctx->items_sz-1;
        } else if (!strcmp(ev->key.sym, "<enter>")) {
            if(ctx->selection_callback)
                ctx->selection_callback(ctx, (int)ctx->sel);
        }


        if(ctx->force_redraw) {
            ctx->force_redraw = 0;
            return 1;
        }
        return 1;
    }

    return 0;
}

static void draw(cairo_t *cr, void *_ctx) {
    struct ui_menu *ctx = _ctx;
    size_t yoff, start, end;

    cairo_save(cr);

    cairo_rectangle(cr, ctx->x, ctx->y, ctx->w, ctx->h);
    cairo_clip(cr);

    start = ctx->scroll_top / (ctx->item_height + ctx->divider_height);
    end = start + (ctx->h / (ctx->item_height + ctx->divider_height)) + 2;
    end = end > ctx->items_sz ? ctx->items_sz : end;
    yoff = 0;

    const int scroll_off = ctx->scroll_top - start*(ctx->item_height + ctx->divider_height);
    for(size_t i = start;i<end;i++) {
        struct ui_color bgcol = ctx->bgcol;
        struct ui_color fgcol = ctx->fgcol;

        if(i == ctx->sel) {
            bgcol = ctx->selbgcol;
            fgcol = ctx->selfgcol;
        }

        const int y = ctx->y - scroll_off + yoff;

        cairo_text_box(cr,
                ctx->fonts,
                ctx->items[i],
                ctx->x,
                y,
                ctx->w,
                ctx->item_height,
                -1,-1,-1,
                (double[]){fgcol.r,fgcol.g,fgcol.b, fgcol.a},
                (double[]){bgcol.r,bgcol.g,bgcol.b, bgcol.a},
                (double[]){fgcol.r,fgcol.g,fgcol.b, fgcol.a});

        yoff += ctx->item_height;

        cairo_set_source_rgb (cr, fgcol.r, fgcol.g, fgcol.b);
        if(i == end-1)
            cairo_set_source_rgba (cr, bgcol.r, bgcol.g, bgcol.b, bgcol.a);
        cairo_rectangle(
                cr,
                ctx->x,
                ctx->y - scroll_off + yoff,
                ctx->w,
                ctx->divider_height);
        cairo_fill(cr);

        yoff += ctx->divider_height;
    }

    cairo_restore(cr);
}

void ui_menu_set_height(struct ui_widget *_ctx, int height) {
    struct ui_menu *ctx = _ctx->ctx;
    ctx->h = height;
    ctx->scroll_top = 0;
    ctx->sel = 0;
    ctx->force_redraw = 1;
}

void ui_menu_set_items(struct ui_widget *_ctx, char **items, size_t items_sz) {
    struct ui_menu *ctx = _ctx->ctx;
    ctx->scroll_top = 0;
    ctx->sel = 0;
    ctx->items = items;
    ctx->items_sz = items_sz;
    ctx->force_redraw = 1;
}

struct ui_widget* ui_create_menu(
        struct ui_evloop *loop,
        FcFontSet *fonts,
        double x,
        double y,
        double w,
        double h,
        double font_height,
        double item_height,
        double divider_height,
        struct ui_color bgcol,
        struct ui_color fgcol,
        struct ui_color selbgcol,
        struct ui_color selfgcol,
        char **items,
        size_t items_sz,
        void (*selection_callback) (struct ui_menu *menu, int sel)) {

    struct ui_widget *wid = malloc(sizeof(struct ui_widget));
    struct ui_menu *ctx = malloc(sizeof(struct ui_menu));
    ctx->loop = loop;
    ctx->x = x;
    ctx->y = y;
    ctx->w = w;
    ctx->h = h;
    ctx->bgcol = bgcol;
    ctx->fgcol = fgcol;
    ctx->selfgcol = selfgcol;
    ctx->selbgcol = selbgcol;
    ctx->sel = 0;
    ctx->scroll_top = 0;
    ctx->items = items;
    ctx->items_sz = items_sz;
    ctx->selection_callback = selection_callback;
    ctx->fonts = fonts;

    ctx->font_height = font_height;
    ctx->item_height = item_height;
    ctx->divider_height = divider_height;
    ctx->force_redraw = 0;

    wid->ctx = ctx;
    wid->event_handler = on_event;
    wid->redraw = draw;
    return wid;
}
