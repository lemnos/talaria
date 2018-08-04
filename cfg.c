#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "cfg.h"

static int kvp(char *line, char **key, char **val) {
  *key = NULL;
  *val = NULL;
  
  for(;*line != '\0';line++) {
    if(*line != ' ' && !*key)
      *key = line;
    
    if(*line == ':' && !*val) {
      *line++ = '\0';
      for(;isspace(*line);line++);
      *val = line;
    }
  }
  
  if(*(line - 1) == '\n')
    *(line - 1) = '\0';
  
  if(!(*val && *key))
    return -1;
  
  return 0;
}

struct cfg* parse_cfg(const char *fname) {
    char *line = NULL;
    size_t n = 0, ln = 0;
    struct cfg *cfg = malloc(sizeof(struct cfg));

    cfg->border_sz = 10;
    cfg->x_str = "auto";
    cfg->y_str = "auto";
    cfg->w_str = "auto";
    cfg->h_str = "auto";
    cfg->x = -1;
    cfg->y = -1;
    cfg->w = -1;
    cfg->h = -1;
    cfg->input_separator_height = 5;
    cfg->input_height = 40;
    cfg->border_radius = 10;
    cfg->border_color = "#000000cc";
    cfg->font_family = "Serif";
    cfg->background_color = "#000000cc";
    cfg->foreground_color = "#ffffff";
    cfg->input_separator_color = "#000000";
    cfg->cursor_color = "#ffffff";
    cfg->menu_divider_height = 1;
    cfg->menu_sel_foreground_color = cfg->background_color;
    cfg->menu_sel_background_color = cfg->foreground_color;

    FILE *fp = fopen(fname, "r");
    if(!fp) return cfg; //Return defaults if no config file xists..
    while(getline(&line, &n, fp) != -1) {
        ln++;
        char *key, *val;
        if(kvp(line, &key, &val)) {
            fprintf(stderr, "Invalid entry in %s at line %lu.\n", fname, ln);
            exit(1);
        }

        if(!strcmp(key, "border_sz"))
            cfg->border_sz = atoi(val);
        else if(!strcmp(key, "x"))
            cfg->x_str = strdup(val);
        else if(!strcmp(key, "y"))
            cfg->y_str = strdup(val);
        else if(!strcmp(key, "w"))
            cfg->w_str = strdup(val);
        else if(!strcmp(key, "h"))
            cfg->h_str = strdup(val);
        else if(!strcmp(key, "input_separator_height"))
            cfg->input_separator_height = atof(val);
        else if(!strcmp(key, "input_height"))
            cfg->input_height = atof(val);
        else if(!strcmp(key, "border_radius"))
            cfg->border_radius = atof(val);
        else if(!strcmp(key, "border_color"))
            cfg->border_color = strdup(val);
        else if(!strcmp(key, "font_family"))
            cfg->font_family = strdup(val);
        else if(!strcmp(key, "background_color"))
            cfg->background_color = strdup(val);
        else if(!strcmp(key, "foreground_color"))
            cfg->foreground_color = strdup(val);
        else if(!strcmp(key, "input_separator_color"))
            cfg->input_separator_color = strdup(val);
        else if(!strcmp(key, "cursor_color"))
            cfg->cursor_color = strdup(val);
        else if(!strcmp(key, "menu_divider_height"))
            cfg->menu_divider_height = atof(val);
        else if(!strcmp(key, "menu_sel_background_color"))
            cfg->menu_sel_background_color = strdup(val);
        else if(!strcmp(key, "menu_sel_foreground_color"))
            cfg->menu_sel_foreground_color = strdup(val);

        free(line);
        line = NULL;
        n = 0;
    }

    return cfg;
}
