#ifndef _UI_MENU_H_
#define _UI_MENU_H_
#include <stdlib.h>
#include "common.h"

struct ui_menu {
    cairo_t *cr;
    struct ui_evloop *loop;
    double x;
    double y;
    double w;
    double h;

    int font_height;
    int item_height;
    int divider_height;

    void (*selection_callback) (struct ui_menu *menu, int sel);

    struct ui_color bgcol;
    struct ui_color fgcol;
    struct ui_color selfgcol;
    struct ui_color selbgcol;

    int force_redraw;
    int scroll_top; //Number of vertically hidden pixels
    size_t sel; //Index of the selected item
    char **items;
    size_t items_sz;
};

void ui_menu_set_items(struct ui_widget *_ctx, char **items, size_t items_sz);

struct ui_widget* ui_create_menu(
        struct ui_evloop *loop,
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
        void (*selection_callback) (struct ui_menu *menu, int sel));
void ui_menu_set_height(struct ui_widget *_ctx, int height);
#endif
