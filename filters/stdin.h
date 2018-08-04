#ifndef _STDIN_H_
#define _STDIN_H_
void stdin_filter_init(char delim, int field, int regex, const char **olist, size_t olist_sz);
size_t stdin_filter_select(char *sel, char **result);
size_t stdin_filter_filter(const char *input, char ***items);
#endif
