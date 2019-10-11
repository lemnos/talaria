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

#ifndef _UI_COMMON_H_
#define _UI_COMMON_H_
#include <cairo/cairo.h>
#include <stdlib.h>

struct ui_color {
    double r;
    double g;
    double b;
    double a;
};

struct ui_event {
    enum {
        KEYBOARD_EV,
        TIME_UPDATE,
        MOUSE_CLICK,
    } type;

    int width;
    int height;
    int x;
    int y;
    int click;
    long time; //In ms
    struct ui_key {
        char sym[256];
        int ctrl;
        int alt;
        int shift;
    } key;
    cairo_t *cr;
};

struct ui_widget {
    void *ctx;

    //Return value indicates whether or not the widget should be redrawn.
    int (*event_handler) (void *ctx, struct ui_event* ev); 
    void (*redraw) (cairo_t *cr, void *ctx);
};

#endif
