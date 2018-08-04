#include "common.h"
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

//Returns -1 if input is not a valid utf8 string.
int read_utf8_char(const char *str, char c[5]) {
    if((*str & 0xF8) == 0xF0) {
        if(((str[1] & 0xC0) != 0x80) ||
                ((str[2] & 0xC0) != 0x80) ||
                ((str[3] & 0xC0) != 0x80))
            return -1;

        c[0] = str[0];
        c[1] = str[1];
        c[2] = str[2];
        c[3] = str[3];
        c[4] = '\0';
        return 4;
    }
    else if((*str & 0xF0) == 0xE0) {
        if(((str[1] & 0xC0) != 0x80) ||
                ((str[2] & 0xC0) != 0x80))
            return -1;

        c[0] = str[0];
        c[1] = str[1];
        c[2] = str[2];
        c[3] = '\0';
        return 3;
    }
    else if((*str & 0xE0) == 0xC0) {
        if(((str[1] & 0xC0) != 0x80))
            return -1;
        c[0] = str[0];
        c[1] = str[1];
        c[2] = '\0';
        return 2;
    } else {
        c[0] = str[0];
        c[1] = '\0';
        return 1;
    }
}

double ui_text_width(cairo_t *cr, const int h, const char *txt) {
    const double space_width = h/2;
    cairo_text_extents_t te;
    double w = 0;
    size_t i = 0;

    set_font_height(cr, h);

    while(i<strlen(txt)) {
        char str[5];

        i+=read_utf8_char(txt+i, str);
        cairo_text_extents(cr, str, &te);

        if(str[0] == ' ')
            w+=space_width;
        else
            w+=te.x_advance;
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
    xoff = x + padding;
    for(c = _txt; *c;) {
        if(c-_txt == cursor_pos) {
            cairo_save(cr);
            cairo_set_source_rgba (cr, fgcol->r, fgcol->g, fgcol->b, fgcol->a);
            cairo_rectangle(cr, xoff, y, 1, h);
            cairo_fill(cr);
            cairo_restore(cr);
            xoff += 1;
        }

        char chr[5];
        int w, chr_sz, is_space;
        cairo_text_extents_t te;

        chr_sz = read_utf8_char(c, chr);
        cairo_text_extents (cr, chr, &te);
        is_space = ((chr_sz == 1) && (chr[0] == ' '));
        w = is_space ? space_width : te.x_advance;
        c += chr_sz;
        if(chr_sz == -1) {
            fprintf(stderr, "Invalid utf8 input..\n");
            exit(1);
        }

        if(c-_txt >= (int)sel_start && c-_txt <= (int)sel_end) {
            cairo_set_source_rgba (cr, fgcol->r, fgcol->g, fgcol->b, bgcol->a);
            cairo_rectangle(cr, xoff, y, w, h);
            cairo_fill(cr);
            cairo_set_source_rgba (cr, bgcol->r, bgcol->g, bgcol->b, fgcol->a);
        } else
            cairo_set_source_rgba (cr, fgcol->r, fgcol->g, fgcol->b, fgcol->a);

        if(!is_space) {
            cairo_move_to(cr,xoff,y+fe.ascent);
            cairo_show_text(cr, chr);
        }

        xoff+=w;
        cairo_fill(cr);
        nc++;
    }

    if(c-_txt == cursor_pos) {
        cairo_save(cr);
        cairo_set_source_rgba (cr, fgcol->r, fgcol->g, fgcol->b, fgcol->a);
        cairo_rectangle(cr, xoff, y, 1, h);
        cairo_fill(cr);
        cairo_restore(cr);
        xoff += 1;
    }

    return xoff;
}
