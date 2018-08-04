#ifndef _UI_COMMON_H_
#define _UI_COMMON_H_
#include <cairo/cairo.h>

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
    int ctrl;
    int alt;
    int shift;
    long time; //In ms
    struct ui_key {
        char sym[256];
        int ctrl;
        int alt;
    } key;
    cairo_t *cr;
};

struct ui_widget {
    void *ctx;

    //Return value indicates whether or not the widget should be redrawn.
    int (*event_handler) (void *ctx, struct ui_event* ev); 
    void (*redraw) (cairo_t *cr, void *ctx);
};

double ui_draw_text_box(cairo_t *cr,
        const int x,
        const int y,
        const int w,
        const int h,
        const int padding,
        struct ui_color *bgcol,
        struct ui_color *fgcol,
        const char *_txt,
        int sel_start, int sel_end, int cursor_pos);

double ui_text_width(cairo_t *cr, const int h, const char *txt);
int read_utf8_char(const char *str, char c[5]);
#endif
