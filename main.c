#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <linux/limits.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>
#include "x11.h"
#include "cfg.h"
#include "widgets/evloop.h"
#include "widgets/draw.h"
#include "widgets/menu.h"
#include "widgets/input.h"
#include "filters/stdin.h"
#include "filters/program.h"
#include "filters/noop.h"

Display *dpy;
Window win;

size_t (*selection_fn) (char *sel, char **result);
size_t (*filter_fn) (const char *input, char ***items);

struct cfg *cfg;
struct ui_evloop *evloop;
struct ui_widget *input;
struct ui_widget *menu;

char **menu_items;
size_t menu_items_sz;

//OPTS

char delim = '\0';
int field = 0;
int force_selection_flag = 0;
int input_only_flag = 0;
int use_regex_flag = 0;
char *generator_program = NULL;
char *history_file = NULL;

void print_version() {
    const char *str =
        "Talaria (v "VERSION")\n"
        "Author: Aetnaeus (2018).\n"
        "Built from git commit: "GIT_HASH"\n";
    fprintf(stderr, str);
    exit(1);
}

void calc_heights(int nitems, int *window_height, int *menu_height) {
    int max, mh, wh;

    max = cfg->h - cfg->input_separator_height - cfg->input_height - cfg->border_sz*2;
    mh = nitems * (cfg->menu_divider_height + cfg->input_height);
    mh = mh > max ? max : mh;
    wh = mh + cfg->border_sz*2 + cfg->input_height + (nitems ? cfg->input_separator_height : 0);

    if(window_height) *window_height = wh;
    if(menu_height) *menu_height = mh;
}

void resize_window(struct ui_evloop *evl) {
    int mh, wh;
    calc_heights(menu_items_sz, &wh, &mh);

    ui_evloop_resize(evl, cfg->x, cfg->y, cfg->w, wh);
    ui_menu_set_height(menu, mh);
}

long gettime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (tv.tv_sec*1000) + (long)((double)tv.tv_usec/1000);
}

int valid_hex(char *col) {
    if(strlen(col) < 7) return -1;
    return (col[0] == '#') && (col[strspn(col+1, "0123456789abcdefABCDEF")+1] == 0);
}

struct ui_color color_from_string(char *col) {
    if(!valid_hex(col)) {
        fprintf(stderr, "%s is not a valid hex string\n", col);
        exit(1);
    }

#define chrtobin(chr) ((chr > '9') ? (chr|0x20) - 'a' + 10 : chr - '0')
    col = col[0] == '#' ? col+1 : col;
    struct ui_color res;

    res.r = (chrtobin(col[0]) << 4 | chrtobin(col[1]))/256.0;
    res.g = (chrtobin(col[2]) << 4 | chrtobin(col[3]))/256.0;
    res.b = (chrtobin(col[4]) << 4 | chrtobin(col[5]))/256.0;
    res.a = (col[6] == 0 ? 256 : (chrtobin(col[6]) << 4 | chrtobin(col[7])))/256.0;

    return res;
#undef chrtobin
}

Window create_unmanaged_window(Display *dpy, int x, int y, int w, int h) {
    Window win;
    XSetWindowAttributes attr = {0};

    win = XCreateWindow(dpy,
            DefaultRootWindow(dpy),
            x, y, w, h, 0,
            XDefaultDepth(dpy, DefaultScreen(dpy)),
            InputOutput,
            DefaultVisual(dpy, DefaultScreen(dpy)),
            CopyFromParent,
            &attr);

    XSetWindowAttributes attributes = {0};
    attributes.override_redirect = True;

    XChangeWindowAttributes(dpy,
            win,
            CWOverrideRedirect,
            &attributes);

    XMapWindow(dpy, win);
    XFlush(dpy);

    return win;
}

cairo_surface_t * get_root_bitmap(Display *dpy) {
    XWindowAttributes info;
    cairo_surface_t *tmp, *xsfc;
    cairo_t *cr;
    Window root = DefaultRootWindow(dpy);

    XGetWindowAttributes(dpy, root, &info);
    tmp = cairo_xlib_surface_create(
            dpy,
            DefaultRootWindow(dpy),
            DefaultVisual(dpy, DefaultScreen(dpy)),
            info.width,
            info.height);

    xsfc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, info.width, info.height);
    cr = cairo_create(xsfc);
    cairo_set_source_surface(cr, tmp, 0, 0);
    cairo_rectangle(cr, 0, 0, info.width, info.height);
    cairo_fill(cr);
    cairo_surface_destroy(tmp);
    cairo_destroy(cr);

    return xsfc;
}

void draw_border(cairo_t *cr, int w, int h, int sz, int r, struct ui_color col) {
#define M_PI 3.14159265358979323846
    cairo_set_line_width (cr, sz);
    cairo_set_source_rgba(cr, col.r, col.g, col.b, col.a);
    cairo_arc(cr, (double)r+sz/2.0, (double)r+sz/2.0, r, M_PI, M_PI*(3.0/2.0));
    cairo_arc(cr, (double)w-r-sz/2.0, (double)r+sz/2.0, r, M_PI*(3.0/2.0), 0);
    cairo_arc(cr, (double)w-r-sz/2.0, (double)h-r-sz/2.0, r, 0, M_PI/2);
    cairo_arc(cr, (double)r+sz/2.0, (double)h-r-sz/2.0, r, M_PI/2.0, M_PI);
    cairo_close_path(cr);
    cairo_stroke(cr);
}

static void init_opts(int argc, char *argv[]) {
    XWindowAttributes info;
    XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &info);

    const char *optstr =  "f:d:x:y:w:h:u:b:mrivl:p:";
    for(char c=getopt(argc, argv, optstr);c!=-1;c=getopt(argc, argv, optstr)) {
        switch(c) {
            case 'p': generator_program = optarg; break;
            case 'v': print_version(); break;
            case 'd': delim = optarg[0]; break;
            case 'f': field = atoi(optarg); break;
            case 'm': force_selection_flag++; break;
            case 'r': use_regex_flag++; break;
            case 'i': input_only_flag++; break;
            case 'x': cfg->x_str = optarg; break;
            case 'y': cfg->y_str = optarg; break;
            case 'w': cfg->w_str = optarg; break;
            case 'h': cfg->h_str = optarg; break;
            case 'u': cfg->input_height = atoi(optarg); break;
            case 'b': cfg->border_sz = atoi(optarg); break;
            case 'l': history_file = optarg; break;
            default: exit(-1);
        }
    }
}

struct cfg* get_cfg() {
    char config_path[PATH_MAX];

    snprintf(config_path, PATH_MAX, "%s/.talariarc", getenv("HOME"));
    struct cfg *cfg = parse_cfg(config_path);

    return cfg;
}

size_t read_history(char ***_history) {
    size_t history_sz = 0;
    char **history = NULL;

    if(history_file) {
        size_t cap = 0;
        FILE *fp = fopen(history_file, "r");

        if(fp) {
            char *line = NULL;
            ssize_t n = 0;
            while((n=getline(&line, (size_t*)&n, fp)) != -1) {
                if(history_sz == cap) {
                    cap += 100;
                    history = realloc(history, sizeof(char*)*cap);
                }

                if(line[n-1] == '\n')
                    line[n-1] = '\0';

                history[history_sz++] = line;

                line = NULL;
                n = 0;
            }

            fclose(fp);
        }
    }

    *_history = history;
    return history_sz;
}

static void redraw_chrome(cairo_t *cr) {
    int wh;
    struct ui_color border_color, input_separator_color;

    border_color = color_from_string(cfg->border_color);
    input_separator_color = color_from_string(cfg->input_separator_color);

    static cairo_surface_t *rootbmp = NULL;

    calc_heights(menu_items_sz, &wh, NULL);
    if(!rootbmp) rootbmp = get_root_bitmap(dpy); //HOTSPOT (mem)
    cairo_set_source_surface(cr, rootbmp, -cfg->x, -cfg->y);
    cairo_rectangle(cr, 0,0,cfg->w,wh);
    cairo_fill(cr);

    if(menu_items_sz) {
        cairo_set_source_rgba (cr,
                input_separator_color.r,
                input_separator_color.g,
                input_separator_color.b,
                input_separator_color.a);

        cairo_rectangle(cr, 0, cfg->border_sz + cfg->input_height, 
                cfg->w, cfg->input_separator_height);
        cairo_fill(cr);
    }

    draw_border(cr, cfg->w, wh, cfg->border_sz, cfg->border_radius, border_color);
}

static void on_select(struct ui_menu *_, int idx) {
    int sz;
    char *sel;

    if(force_selection_flag && !menu_items_sz) return;

    if(menu_items_sz) {
        sel = menu_items[idx];
        sz = strlen(sel);
    } else {
        sel = ((struct ui_input*)input->ctx)->input;
        sz = strlen(sel);
    }

    sz = selection_fn(sel, &sel);

    //FIXME
    if(history_file) {
        char **history;
        size_t history_sz;
        FILE *fp;

        history_sz = read_history(&history);
        fp = fopen(history_file, "w");
        if(!fp) {
            fprintf(stderr, "Failed to write history to %s\n", history_file);
        } else {
            char *selection;
            //Store the menu option in the history entry rather than
            //the actual item (simplifies sorting)
            selection = menu_items_sz ? menu_items[idx] : sel;
            for (size_t i = 0; i < history_sz; i++) {
                if(strcmp(history[i], selection))
                    fprintf(fp, "%s\n", history[i]);
            }
            fprintf(fp, "%s\n", selection);
            fclose(fp);
        }
    }

    XUnmapWindow(dpy, win);
    XFlush(dpy);
    write(1, sel, sz); //printf ignores \0 even when sz is specified.
    write(1, "\n", 1);
    exit(1);
}

static void on_input_update(struct ui_input *inp, const char *input) {
    menu_items_sz = filter_fn(input, &menu_items);
    ui_menu_set_items(menu, menu_items, menu_items_sz);
    resize_window(inp->loop);
}

size_t file_item_completion(const char *prefix, const char *str, char ***results) {
    size_t prefix_len = strlen(prefix);
    size_t sz;
    struct dirent *ent;
    DIR *dh;
    char *dir=malloc(strlen(str)+2);
    const char *end = NULL;
    const char *partial = "";
    for(const char *c=str;*c;c++)
        if(*c == '/')
            end = c;

    if(!end) return 0;

    dir = memcpy(dir, str, end-str+1);
    dir[end-str+1] = '\0';
    partial = end+1;

    if(!(dh = opendir(dir)))
        return 0;

    sz = 0;
    while(readdir(dh)) sz++;
    *results = malloc(sizeof(char*) * sz);
    rewinddir(dh);

    sz = 0;
    while((ent=readdir(dh)))
        if(strstr(ent->d_name, partial) == ent->d_name &&
                strcmp(".", ent->d_name) &&
                strcmp("..", ent->d_name)) {
            (*results)[sz] = malloc(PATH_MAX + prefix_len);
            snprintf((*results)[sz], PATH_MAX+prefix_len+2,
                    "%s %s%s%s",
                    prefix,
                    dir,
                    ent->d_name,
                    ent->d_type == DT_DIR ? "/" : "");
            sz++;
        }

    return sz;
}

size_t menu_item_completion(const char *str, char ***results) {
    size_t n = 0;
    *results = malloc(menu_items_sz * sizeof(char*));
    for(size_t i=0;i<menu_items_sz;i++)
        if(strstr(menu_items[i], str))
            (*results)[n++] = menu_items[i];

    return n;
}

size_t completion_fn(const char *_str, char ***results) {
    size_t sz;
    char *str = strdup(_str);
    char *path = NULL;
    char *prefix = str;
    for(int i = strlen(str)-1;i>=0;i--)
        if(str[i] == ' ') {
            str[i] = '\0';
            path = str + i + 1;
            break;
        }

    if(!path)
        sz = menu_item_completion(str, results);
    else
        sz = file_item_completion(prefix, path, results);

    free(str);
    return sz;

}

void init_ui(Display *dpy, Window win, char **history, size_t history_sz) {
    struct ui_color
        fgcol,
        bgcol,
        selbgcol,
        selfgcol,
        cursor_color,
        border_color;

    int menu_item_height = cfg->input_height;

    fgcol = color_from_string(cfg->foreground_color);
    bgcol = color_from_string(cfg->background_color);
    selbgcol = color_from_string(cfg->menu_sel_background_color);
    selfgcol = color_from_string(cfg->menu_sel_foreground_color);
    border_color = color_from_string(cfg->border_color);
    cursor_color = color_from_string(cfg->cursor_color);

    evloop = ui_create_evloop(dpy, win);
    evloop->pre_redraw = redraw_chrome;
    input = ui_create_input(
            evloop,
            fc_get_fonts("Inconsolata"), //FIXME
            cfg->border_sz, cfg->border_sz,
            cfg->w-cfg->border_sz*2, cfg->input_height,
            cursor_color, bgcol, fgcol,
            completion_fn,
            (const char**)history, history_sz,
            on_input_update);

    menu = ui_create_menu(
            evloop,
            fc_get_fonts("Inconsolata"), //FIXME
            cfg->border_sz, cfg->input_height + cfg->border_sz + cfg->input_separator_height,
            cfg->w-cfg->border_sz*2, 400,
            menu_item_height * 0.75,
            menu_item_height,
            cfg->menu_divider_height,
            bgcol, fgcol,
            selbgcol, selfgcol,
            NULL, 0,
            on_select);

    //TODO cleanup evloop code.
    ui_evloop_add_widget(evloop, input);
    ui_evloop_add_widget(evloop, menu);
    on_input_update(input->ctx, "");
}

int main(int argc, char *argv[]) {
    int sh, sw, sx, sy;
    char *tmp;
    char **history;
    size_t history_sz;
    int wh;

    dpy = XOpenDisplay(NULL);
    if(!dpy) {
        perror("ERROR: Failed to open x11 display. (make sure X is running)");
        exit(1);
    }

    cfg = get_cfg();
    init_opts(argc, argv);

    //FIXME implement history properly.
    history_sz = read_history(&history);

    if(generator_program) {
        program_filter_init(generator_program);
        selection_fn = program_filter_select;
        filter_fn = program_filter_filter;
    } else if(input_only_flag) {
        selection_fn = noop_select;
        filter_fn = noop_filter;
    } else {
        stdin_filter_init(delim, field, use_regex_flag, (const char**)history, history_sz);
        selection_fn = stdin_filter_select;
        filter_fn = stdin_filter_filter;
    }

    x11_get_active_screen(dpy, &sx, &sy, &sw, &sh);

    cfg->w = !strcmp(cfg->w_str, "auto") ? sw/2 : atof(cfg->w_str);
    cfg->h = !strcmp(cfg->h_str, "auto") ? sh/2 : atof(cfg->h_str);

    menu_items_sz = filter_fn("", &menu_items);
    calc_heights(menu_items_sz, &wh, NULL);

    if(!strcmp(cfg->x_str, "auto"))
        cfg->x =  (sw-cfg->w)/2;
    else
        cfg->x = cfg->x_str[0] == '-' ? sw + atof(cfg->x_str) - cfg->w : atof(cfg->x_str);

    if(!strcmp(cfg->y_str, "auto"))
        cfg->y =  (sh-wh)/2;
    else
        cfg->y = cfg->y_str[0] == '-' ? sh + atof(cfg->y_str) - wh : atof(cfg->y_str);


    //x/y should be relative to the current screen.
    cfg->y += sy;
    cfg->x += sx;

    win = create_unmanaged_window(dpy, cfg->x, cfg->y, cfg->w, wh);

    init_ui(dpy, win, history, history_sz);
    ui_evloop_run(evloop);
    selection_fn("", &tmp);
    exit(1);
}
