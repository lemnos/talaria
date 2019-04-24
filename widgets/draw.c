#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <fontconfig/fontconfig.h>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <fontconfig/fontconfig.h>
#include "utf8.h"

#define CURSOR_WIDTH 1

//Return the FcPattern* corresponding to a font in the given font set which
//is a truetype font with the given utf8 character.

static FcPattern *get_font_with_char(FcFontSet *candidates, const char *c) {
    for (int i = 0; i < candidates->nfont; i++) {
        const char *format;

        FcPatternGetString(candidates->fonts[i], FC_FONTFORMAT, 0, (FcChar8**)&format);

        if(!strcmp(format, "TrueType")) {
            FcValue v;
            const FcCharSet *charset;
            FcPatternGet(candidates->fonts[i], FC_CHARSET, 0, &v);
            assert(v.type == FcTypeCharSet);

            charset = v.u.c;
            if(FcCharSetHasChar(charset, utf8_ucs4(c)))
                return candidates->fonts[i];
        }
    }

    return NULL;
}

FcFontSet *fc_get_fonts(const char *fc_exp) {
    FcFontSet *candidates;
    FcResult r;
    FcPattern *pat = FcNameParse((FcChar8*)fc_exp);

    FcConfigSubstitute(NULL, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    candidates = FcFontSort(NULL, pat, 0, NULL, &r);
    assert(r == FcResultMatch);

    return candidates;
}

static void set_cairo_font(cairo_t *cr, FcPattern *font, int height, cairo_font_extents_t *_fe) {
    cairo_font_extents_t fe;
    cairo_font_face_t *cf = cairo_ft_font_face_create_for_pattern(font);

    assert(cf);
    cairo_set_font_face(cr, cf);
    cairo_set_font_size(cr, height);
    cairo_font_extents(cr, &fe);

    cairo_set_font_size(cr, height * height / fe.height);
    if(_fe) *_fe = fe;
}

/* Number of width required by the characters drawn with the given fontset. */
double cairo_text_box_width(cairo_t *cr, FcFontSet *fonts, const char *s, int height) {
    size_t w = CURSOR_WIDTH; //Init val == cursor width
    size_t idx = 0;

    while (*s) {
        cairo_text_extents_t te;
        FcPattern *font;
        char c[5];

        s += utf8_read(s, c);
        font = get_font_with_char(fonts, c);
        if(!font) {
            strcpy(c, "\uFFFD");
            font = get_font_with_char(fonts, c);
        }

        set_cairo_font(cr, font, height, NULL);

        cairo_text_extents(cr, c, &te);

        w += te.x_advance;
        idx++;
    }

    return w;
}

void cairo_text_box(cairo_t *cr,
	FcFontSet *fonts,
	const char *s,
	double x,
	double y,
	double width,
	double height,
    size_t curpos,
    int selstart,
    int selend,
    double fgcol[4],
    double bgcol[4],
    double curcol[4]) {
    cairo_new_path(cr);

    int idx = 0;
    double xoff = x;

    while(*s) {
        cairo_font_extents_t fe;
        cairo_text_extents_t te;
        FcPattern *font;
        char c[5];

        if(idx == (int)curpos) {
            cairo_set_source_rgba(cr, curcol[0],curcol[1],curcol[2],curcol[3]);
            cairo_rectangle(cr, xoff, y, 1, height);
            cairo_fill(cr);
            xoff += 1;
        }

        s += utf8_read(s, c);
        font = get_font_with_char(fonts, c);

        //Glyph not found in the given font set.
        if(!font) {
            strcpy(c, "\uFFFD"); //Unknown char unicode code point
            font = get_font_with_char(fonts, c);
        }

        set_cairo_font(cr, font, height, &fe);

        cairo_text_extents(cr, c, &te);

        if(idx >= selstart && idx <= selend) {
            cairo_set_source_rgba(cr, fgcol[0],fgcol[1],fgcol[2],fgcol[3]);
            cairo_rectangle(cr, xoff, y, te.x_advance, height);
            cairo_fill(cr);
            cairo_set_source_rgba(cr, bgcol[0],bgcol[1],bgcol[2],bgcol[3]);
        } else {
            cairo_set_source_rgba(cr, bgcol[0],bgcol[1],bgcol[2],bgcol[3]);
            cairo_rectangle(cr, xoff, y, te.x_advance, height);
            cairo_fill(cr);
            cairo_set_source_rgba(cr, fgcol[0],fgcol[1],fgcol[2],fgcol[3]);
        }

        cairo_move_to(cr, xoff, y + fe.ascent + (height - fe.height));
        cairo_show_text(cr, c);

        xoff += te.x_advance;
        idx++;
    }

    if(idx == (int)curpos) {
        cairo_set_source_rgba(cr, curcol[0],curcol[1],curcol[2],curcol[3]);
        cairo_rectangle(cr, xoff, y, 1, height);
        cairo_fill(cr);
        xoff += 1;
    }

    cairo_set_source_rgba(cr, bgcol[0],bgcol[1],bgcol[2],bgcol[3]);
    cairo_rectangle(cr, xoff, y, width - (xoff-x), height);
    cairo_fill(cr);
}
