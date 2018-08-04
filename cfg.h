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
