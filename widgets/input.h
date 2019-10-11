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

#ifndef _UI_INPUT_H_
#define _UI_INPUT_H_
#include "common.h"
#include <stdlib.h>
#include <fontconfig/fontconfig.h>

struct ui_input {
    cairo_t *cr;
    struct ui_evloop* loop;
    double x;
    double y;
    double w;
    double h;

    void(*input_update_cb) (struct ui_input *input_widget, const char *input);
    struct ui_color cursor_color;
    struct ui_color fgcol;
    struct ui_color bgcol;
    long last_blink_update;
    int show_cursor;

    const char **history;
    size_t history_sz, last_hist_idx;

    size_t (*completion_fn) (const char *str, char ***results);
    char **completions;
    size_t completions_sz;
    int last_completion_idx;

    struct {
        ssize_t start;
        ssize_t end;
    } sel;
    size_t cursor_pos;

    FcFontSet *fonts;
    struct ui_key last_key;
    char *input, *last_input;
};

struct ui_widget* ui_create_input(
        struct ui_evloop* loop,
        FcFontSet *fonts,
        double x,
        double y,
        double w,
        double h,
        struct ui_color cursor_color,
        struct ui_color bgcol,
        struct ui_color fgcol,
        size_t (*completion_fn) (const char *str, char ***results),
        const char** history,
        size_t history_sz,
        void(*input_update_cb) (struct ui_input *input_widget, const char *input));
#endif
