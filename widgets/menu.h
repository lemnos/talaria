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

#ifndef _UI_MENU_H_
#define _UI_MENU_H_
#include <stdlib.h>
#include <fontconfig/fontconfig.h>
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

	FcFontSet *fonts;
    int force_redraw;
    int scroll_top; //Number of vertically hidden pixels
    size_t sel; //Index of the selected item
    char **items;
    size_t items_sz;
};

void ui_menu_set_items(struct ui_widget *_ctx, char **items, size_t items_sz);

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
        void (*selection_callback) (struct ui_menu *menu, int sel));
void ui_menu_set_height(struct ui_widget *_ctx, int height);
#endif
