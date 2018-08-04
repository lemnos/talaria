#ifndef _CX11_H_
#define _CX11_H_
#include <X11/Xlib.h>

int x11_get_active_screen(Display *dpy, int *_x, int *_y, int *_width, int *_height);
#endif
