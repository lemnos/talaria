#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <stdlib.h>
#include <stdio.h>

/* Obtain the physical boundaries of the active screen (assumes xinerama). */
void x11_get_active_screen(Display *dpy, int *_x, int *_y, int *_width, int *_height) {
    XWindowAttributes attr;
    Window win;
    int n, x, y, _;
    XineramaScreenInfo *screens;

    XGetInputFocus(dpy, &win, &_);
    XGetWindowAttributes(dpy, win, &attr);

    /* Obtain absolute window coorindates */
    XTranslateCoordinates(dpy, win, attr.root, 0, 0,&x, &y, &win); 

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
