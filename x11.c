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

#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <stdlib.h>
#include <stdio.h>

/* Obtain the physical boundaries of the active screen (assumes xinerama). */
void x11_get_active_screen(Display *dpy, int *_x, int *_y, int *_width, int *_height) {
    Window win, chld, root;
    int n, x, y, _;
    unsigned int _u;
    XineramaScreenInfo *screens;

    /* Obtain absolute pointer coordinates */
    XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &chld, &x, &y, &_, &_, &_u);

    screens = XineramaQueryScreens(dpy, &n);
    for (int i = 0; i < n; i++) {
        if((x >= screens[i].x_org) && (x <= (screens[i].x_org + screens[i].width)) &&
                (y >= screens[i].y_org) && (y <= (screens[i].y_org + screens[i].height))) {
            *_x = screens[i].x_org;
            *_y = screens[i].y_org;
            *_width = screens[i].width;
            *_height = screens[i].height;
            return;
        }
    }

    fprintf(stderr, "X11: Could not find the active screen!");
    exit(1);
}
