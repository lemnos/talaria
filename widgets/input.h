#ifndef _UI_INPUT_H_
#define _UI_INPUT_H_
#include "common.h"
#include <stdlib.h>

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
        int start;
        int end;
    } sel;
    size_t cursor_pos;

    struct ui_key last_key;
    char *input, *last_input;
};

struct ui_widget* ui_create_input(
        struct ui_evloop* loop,
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

char* ui_input_get_input(struct ui_widget *w);
#endif
