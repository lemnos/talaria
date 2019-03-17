#include "common.h"
#include "utf8.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void set_font_height(cairo_t *cr, int h) {
    cairo_font_extents_t fe;

    cairo_set_font_size (cr, h);
    cairo_font_extents(cr, &fe);

    /* Discover the scaling factor of the font (font size doesn't
       correspond to cairo unit size) and multiply it 
       by the desired height. */
    h = (int)((double)h/(fe.ascent + fe.descent) * (double)h); 

    cairo_set_font_size (cr, h);
}

//Calculates the width fo the first n chars of txt (or all of txt if n = 0).
double ui_text_width(cairo_t *cr, const int h, const char *txt, int n) {
    const double space_width = h/2;
    cairo_text_extents_t te;
    double w = 0;
    size_t idx = 0;
    size_t len = strlen(txt);

    set_font_height(cr, h);

    while(idx<len && (!n || (idx < n))) {
        char c[5];

        txt += utf8_read(txt, c);
        cairo_text_extents(cr, c, &te);

        if(c[0] == ' ')
            w+=space_width;
        else
            w+=te.x_advance;

        idx++;
    }

    return w;
}

//Returns the x offset of the text (end of the drawn string)

double ui_draw_text_box(cairo_t *cr,
        const int x,
        const int y,
        const int w,
        const int h,
        const int padding,
        struct ui_color *bgcol,
        struct ui_color *fgcol,
        const char *_txt,
        int sel_start, int sel_end, int cursor_pos) {

    const int space_width = h/2;

    double  xoff;
    cairo_font_extents_t fe;

    set_font_height(cr, h);
    cairo_font_extents(cr, &fe);

    cairo_set_source_rgba (cr, bgcol->r, bgcol->g, bgcol->b, bgcol->a);
    cairo_rectangle (cr, x, y, w, h);
    cairo_fill (cr);

    cairo_set_source_rgba (cr, fgcol->r, fgcol->g, fgcol->b, fgcol->a);

    const char *c;
    int nc = 0;
    int idx = 0;
    xoff = x + padding;
    for(c = _txt; *c;) {
        char chr[5];
        int w;

        if(idx == cursor_pos) {
            cairo_save(cr);
            cairo_set_source_rgba (cr, fgcol->r, fgcol->g, fgcol->b, fgcol->a);
            cairo_rectangle(cr, xoff, y, 1, h);
            cairo_fill(cr);
            cairo_restore(cr);
            xoff += 1;
        }

        cairo_text_extents_t te;

        c += utf8_read(c, chr);
        cairo_text_extents (cr, chr, &te);
        w = chr[0] == ' ' ? space_width : te.x_advance;

        if(idx >= (int)sel_start && idx <= (int)sel_end) {
            cairo_set_source_rgba (cr, fgcol->r, fgcol->g, fgcol->b, bgcol->a);
            cairo_rectangle(cr, xoff, y, w, h);
            cairo_fill(cr);
            cairo_set_source_rgba (cr, bgcol->r, bgcol->g, bgcol->b, fgcol->a);
        } else
            cairo_set_source_rgba (cr, fgcol->r, fgcol->g, fgcol->b, fgcol->a);

        if(chr[0] != ' ') {
            cairo_move_to(cr,xoff,y+fe.ascent);
            cairo_show_text(cr, chr);
        }

        xoff+=w;
        cairo_fill(cr);
        nc++;
        idx++;
    }

    if(idx == cursor_pos) {
        cairo_save(cr);
        cairo_set_source_rgba (cr, fgcol->r, fgcol->g, fgcol->b, fgcol->a);
        cairo_rectangle(cr, xoff, y, 1, h);
        cairo_fill(cr);
        cairo_restore(cr);
        xoff += 1;
    }

    return xoff;
}
