#ifndef _DRAW_H_
#define _DRAW_H_
#include <fontconfig/fontconfig.h>

FcFontSet *fc_get_fonts(const char *fc_exp);

double cairo_text_box_width(cairo_t *cr, FcFontSet *fonts, const char *s, int height);

void cairo_text_box(cairo_t *cr,
	FcFontSet *fonts,
	const char *s,
	double x,
	double y,
	double height,
    size_t curpos,
    int selstart,
    int selend,
    double fgcol[4],
    double bgcol[4],
    double curcol[4]);
#endif
