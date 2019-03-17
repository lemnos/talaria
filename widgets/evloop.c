#include "evloop.h"
#include "common.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/time.h>
#include <cairo/cairo-xlib.h>
#include "x11_selection.h"

static void paint_to_xwin(cairo_t *cr, Display *dpy, Window win) {
    static int w = 0;
    static int h = 0;
    static cairo_t *xcr = NULL;
    static cairo_surface_t *xsfc = NULL;
    XWindowAttributes attr;

    if(xcr == NULL) {
        xsfc = cairo_xlib_surface_create(
                dpy,
                win,
                DefaultVisual(dpy, DefaultScreen(dpy)),
                0, 0);
        xcr = cairo_create(xsfc);
    }

    cairo_set_source_surface(xcr, cairo_get_target(cr), 0, 0);
    XGetWindowAttributes(dpy, win, &attr);
    if(attr.width != w || attr.height != h) {
        h = attr.height;
        w = attr.width;

        cairo_xlib_surface_set_size (xsfc, attr.width, attr.height);
        cairo_rectangle(xcr, 0, 0, w, h);
    }

    cairo_fill_preserve(xcr);
    XFlush(dpy);
}

static long gettime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_usec/1000 + tv.tv_sec*1000;
}

static struct ui_event* create_event(XEvent *xev, cairo_t *cr) {
    char *tmp;
    KeySym sym;
    struct ui_event *ev = malloc(sizeof(struct ui_event));

    ev->cr = cr;
    ev->type = TIME_UPDATE;
    ev->time = gettime();
    if(!xev) return ev;

    switch(xev->type) {
        int shift;
        case KeyPress:
            XLookupString((XKeyEvent*)xev, ev->key.sym, sizeof ev->key.sym, &sym, NULL);

            ev->type = KEYBOARD_EV;
            ev->key.ctrl = ((XKeyEvent*)xev)->state & ControlMask;
            ev->key.alt = ((XKeyEvent*)xev)->state & Mod1Mask;
            ev->key.shift = ((XKeyEvent*)xev)->state & ShiftMask;
            shift = ((XKeyEvent*)xev)->state & ShiftMask;

            switch(sym) {
                case XK_Linefeed:
                    strcpy(ev->key.sym, "\n");
                    break;
                case XK_Escape:
                    strcpy(ev->key.sym, "<escape>");
                    break;
                case XK_Down:
                    strcpy(ev->key.sym, "<down>");
                    break;
                case XK_Left:
                    strcpy(ev->key.sym, "<left>");
                    break;
                case XK_Return:
                    strcpy(ev->key.sym, "<enter>");
                    break;
                case XK_Right:
                    strcpy(ev->key.sym, "<right>");
                    break;
                case XK_Page_Up:
                    strcpy(ev->key.sym, "<page_up>");
                    break;
                case XK_Page_Down:
                    strcpy(ev->key.sym, "<page_down>");
                    break;
                case XK_Up:
                    strcpy(ev->key.sym, "<up>");
                    break;
                case XK_ISO_Left_Tab:
                    strcpy(ev->key.sym, "⇤");
                    break;
                case XK_Tab:
                    strcpy(ev->key.sym, "\t");
                    break;
                case XK_Insert:
                    if(shift)
                        strcpy(ev->key.sym, "<paste>");
                    break;
                case XK_BackSpace:
                    strcpy(ev->key.sym, "\x08");
                    break;
                case ' ':
                    strcpy(ev->key.sym, " ");
                    break;
                case XK_Shift_L:
                case XK_Shift_R:
                    //Ignore shift.
                    ev->cr = cr;
                    ev->type = TIME_UPDATE;
                    ev->time = gettime();
                    break;
                default:
                    tmp = XKeysymToString(sym);
                    if(strlen(tmp) == 1)
                        strcpy(ev->key.sym, tmp);
                    break;
            }
            break;
    }

    return ev;
}

struct ui_evloop* ui_create_evloop(Display *dpy, Window win) {
    struct ui_evloop *evl = malloc(sizeof(struct ui_evloop));
    XWindowAttributes info;

    XGetWindowAttributes(dpy, win, &info);
    evl->cr = cairo_create(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, info.width, info.height));
    evl->win = win;
    evl->dpy = dpy;
    evl->widgets_sz = 0;
    evl->post_redraw = NULL;
    evl->pre_redraw = NULL;

    return evl;
}

char* ui_evloop_get_paste_buffer(struct ui_evloop *loop) {
    return x11_get_primary_selection(loop->dpy, loop->win);
}

void ui_evloop_add_widget(struct ui_evloop *evl, struct ui_widget* wid) {
    assert(evl->widgets_sz != MAX_WIDGETS);
    evl->widgets[evl->widgets_sz++] = wid;
}

//All resizing should be done through evloop code.
void ui_evloop_resize(struct ui_evloop *evl, int x, int y, int w, int h) {
    cairo_destroy(evl->cr);
    evl->cr = cairo_create(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h));
    XMoveResizeWindow(evl->dpy, evl->win, x, y, w, h);
    XFlush(evl->dpy);
}

void ui_evloop_run(struct ui_evloop* evl) {
    int redraw = 1;
    int xfd = XConnectionNumber(evl->dpy);

    while(1) {
        struct ui_event *ev;
        struct timeval timeout;
        fd_set fds;

        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;
        FD_ZERO(&fds);
        FD_SET(xfd, &fds);

        if(!redraw && !XPending(evl->dpy))
            select(xfd+1, &fds, NULL, NULL, &timeout);

        XGrabKeyboard(evl->dpy, evl->win, True, GrabModeAsync, GrabModeAsync, CurrentTime);

        if(XPending(evl->dpy)) {
            XEvent xev;

            XNextEvent(evl->dpy, &xev);
            ev = create_event(&xev, evl->cr);
        } else
            ev = create_event(NULL, evl->cr);

        if(ev->type == KEYBOARD_EV && !strcmp(ev->key.sym, "<escape>")) {
            XDestroyWindow(evl->dpy, evl->win);
            XFlush(evl->dpy);
            free(ev);
            return;
        }

        for(size_t i = 0;i<evl->widgets_sz;i++)
            redraw |= evl->widgets[i]->event_handler(evl->widgets[i]->ctx, ev);

        if(redraw) {
            if(evl->pre_redraw) evl->pre_redraw(evl->cr);

            for(size_t i = 0;i<evl->widgets_sz;i++)
                evl->widgets[i]->redraw(evl->cr, evl->widgets[i]->ctx);

            if(evl->post_redraw) evl->post_redraw(evl->cr);

            paint_to_xwin(evl->cr, evl->dpy, evl->win);
        }

        free(ev);
        redraw = 0;
    }
}

