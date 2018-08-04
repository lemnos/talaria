#ifndef _PROGRAM_H_
#define _PROGRAM_H_
size_t program_filter_filter(const char *input, char ***_items);
size_t program_filter_select(char *sel, char **res);
void program_filter_init(const char *name);
#endif
