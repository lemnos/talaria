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

#ifndef _CFG_H_
#define _CFG_H_

struct cfg {
    int border_sz;
    double x;
    double y;
    double w;
    double h;
    double input_separator_height;
    double input_height;
    double border_radius;
    char* border_color;
    char* font_family;
    char* background_color;
    char* foreground_color;
    char* menu_sel_background_color;
    char* menu_sel_foreground_color;
    char* input_separator_color;
    char* cursor_color;
    char* x_str;
    char* y_str;
    char* w_str;
    char* h_str;
    double menu_divider_height;
};

struct cfg* parse_cfg(const char *fname);

#endif
