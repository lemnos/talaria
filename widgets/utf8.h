#ifndef _UTF_8_H_
#define _UTF_8_H_

int utf8_read(const char *s, char c[5]);
int utf8_len(const char *s);
int utf8_idx(const char *s, int n);
const char* utf8_dup(const char *s, int n);

#endif
