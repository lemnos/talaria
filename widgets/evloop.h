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
