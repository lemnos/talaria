#include <string.h>
#include "noop.h"

size_t noop_filter(const char *s, char ***res) {
    *res = NULL;
    return 0;
}

size_t noop_select(char *selection, char **res) {
    *res = selection;
    return strlen(*res);
}
