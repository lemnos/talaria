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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>
#include "../widgets/utf8.h"

//Allow for embedded null characters.
struct line {
    char *bytes;
    size_t sz;
};

static struct line* input_lines;
static size_t input_lines_sz;
static char **menu_items;

static int regex_flag = 0;

static char *extract_field(char *item, size_t item_sz, char delim, int field) {
    char *res;
    int cfld = 0;
    int start = 0;
    size_t i;

    for(i = 0;i<item_sz;i++) {
        if(item[i] == delim) {
            if(cfld == field) {
                res = malloc(i-start+1);
                memcpy(res, item+start, i-start);
                res[i-start] = '\0';
                return res;
            }

            cfld++;
            start=i+1;
        }
    }

    res = malloc(i-start+1);
    memcpy(res, item+start, i-start);
    res[i-start] = '\0';
    return res;
}

static void read_stdin() {
    const int max_line_len = 100000;

    size_t cap = 1000;
    input_lines = malloc(sizeof(struct line)*cap);
    input_lines_sz = 0;

    char buf[max_line_len];
    int buf_sz = 0;
    int c = 0, ln = 0;

    while(c != EOF) {
        c=fgetc(stdin);
        if(input_lines_sz == cap) {
            cap*=2;
            input_lines = realloc(input_lines, sizeof(struct line)*cap);
        }

        buf[buf_sz++]=(char)c;
        if(buf_sz == max_line_len) {
            fprintf(stderr, "ERROR: Line %d exceeds maximum of %d chars\n", ln, max_line_len);
            exit(1);
        }

        if((char)c == '\n' || c == EOF) {
            if(buf_sz == 1) {
                buf_sz = 0;
                continue;
            }

            buf[buf_sz-1] = '\0';

            input_lines[input_lines_sz].bytes = malloc(buf_sz);
            memcpy(input_lines[input_lines_sz].bytes, buf, buf_sz);
            input_lines[input_lines_sz++].sz = buf_sz;

            buf_sz = 0;
        }
    }
}

static void sort_menu_items(const char **olst, size_t olst_sz) {
    if(!olst_sz) return;
    int c = 0;
    for(int i = (int)olst_sz-1;i>=0 && c < (int)input_lines_sz;i--) {
        for(int j = 0;j<(int)input_lines_sz && c < (int)input_lines_sz;j++) {
            if(!strcmp(menu_items[j], olst[i])) {
                struct line ltmp;
                char *tmp;

                tmp =  menu_items[j];
                menu_items[j] = menu_items[c];
                menu_items[c] = tmp;

                ltmp = input_lines[j];
                input_lines[j] = input_lines[c];
                input_lines[c] = ltmp;

                c++;
            }
        }
    }
}

void stdin_filter_init(char delim, int field, int regex, const char **olist, size_t olist_sz) {
    read_stdin();

    menu_items = malloc(sizeof(char*) * input_lines_sz);

    for(size_t i = 0;i < input_lines_sz;i++) {
        menu_items[i] =
            extract_field(input_lines[i].bytes, input_lines[i].sz, delim, field);

        if(!utf8_is_valid(menu_items[i]))
            menu_items[i] = "<invalid utf8 sequence>";
    }

    sort_menu_items(olist, olist_sz);
    regex_flag = regex;
}

size_t stdin_filter_select(char *sel, char **result) {
    for(size_t i = 0; i < input_lines_sz;i++)
        if(!strcmp(menu_items[i], sel)) {
            *result = input_lines[i].bytes;
            return input_lines[i].sz - 1; //sz includes the terminating NULL byte
        }

    *result = sel;
    return strlen(sel);
}

size_t stdin_filter_filter(const char *input, char ***_items) {
    size_t j = 0;
    size_t sz = 0;
    static char **results = NULL;

    if(!results) results = malloc(sizeof(char*) * input_lines_sz);

    if(regex_flag) {
        regex_t re;
        if(regcomp(&re, input, REG_ICASE)) {
            *_items = NULL;
            return 0;
        }

        for(size_t i = 0;i < input_lines_sz;i++)
            if(!regexec(&re, menu_items[i], 0, NULL, 0))
                results[sz++] = menu_items[i];
    } else { 
        for(size_t i = 0;i < input_lines_sz;i++)
            if(strstr(menu_items[i], input))
                results[sz++] = menu_items[i];
    }

    //Prioritize items for which the input is a prefix.
    for(size_t i=0;i<sz;i++)
        if(strstr(results[i], input) == results[i]) {
            char *tmp = results[i];
            results[i] = results[j];
            results[j++] = tmp;
        }

    *_items = results;
    return sz;
}
