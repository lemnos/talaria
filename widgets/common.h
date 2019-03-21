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
