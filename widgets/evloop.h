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

#ifndef _UI_EVLOOP_H_
#define _UI_EVLOOP_H_
#include "common.h"
#include <stddef.h>
#include <X11/Xlib.h>

#define MAX_WIDGETS 100
#define UI_EVLOOP_INITIALIZER {0}

struct ui_evloop {
    struct ui_widget* widgets[MAX_WIDGETS];
    size_t widgets_sz;

    Window win;
    Display *dpy;
    cairo_t *cr;
    void (*pre_redraw)(cairo_t *cr);
    void (*post_redraw)(cairo_t *cr);
};

void ui_evloop_add_widget(struct ui_evloop *evl, struct ui_widget* wid);
void ui_evloop_run(struct ui_evloop* evl);
struct ui_evloop* ui_create_evloop(Display *dpy, Window win);
char* ui_evloop_get_paste_buffer(struct ui_evloop *loop);
void ui_evloop_resize(struct ui_evloop *evl, int x, int y, int w, int h);

#endif
