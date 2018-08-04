#ifndef _NOOP_H_
#define _NOOP_H_
#include <stddef.h>

size_t noop_filter(const char *s, char ***res);
size_t noop_select(char *selection, char **res);
#endif
