/*
 * ---------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ---------------------------------------------------------------------
 * 
 * Original Authors: Aetnaeus (https://github.com/lemnos)
 *
 */

#ifndef _DRAW_H_
#define _DRAW_H_
#include <fontconfig/fontconfig.h>

FcFontSet *fc_get_fonts(const char *fc_exp);

/* Number of width required by the characters drawn with the given fontset. */
double cairo_text_box_width(cairo_t *cr, FcFontSet *fonts, const char *s, int height);

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
    double curcol[4]);
#endif
