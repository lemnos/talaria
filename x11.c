#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <stdlib.h>
#include <stdio.h>

/* Obtain the physical boundaries of the active screen (assumes xinerama). */
void x11_get_active_screen(Display *dpy, int *_x, int *_y, int *_width, int *_height) {
    Window win, *chld, *root;
    int n, x, y, _;
    XineramaScreenInfo *screens;

    /* Obtain absolute pointer coordinates */
    XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &chld, &x, &y, &_, &_, &_);

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
