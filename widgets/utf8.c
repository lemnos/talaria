#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

//Functions assume valid UTF8 input. Primitives which should be thought of as
//dangerous if the input is not santized (in the same way most string functions
//are), but otherwise aim to be efficient ways of manipulating UTF8 byte
//sequences.

//Return the byte index of the nth char
size_t utf8_idx(const char *s, int n) {
    const char *start = s;

    while(*s && n--) {
        if((0xE0 & *s) == 0xC0) s+=2;
        else if((0xF0 & *s) == 0xE0) s+=3;
        else if((0xF8 & *s) == 0xF0) s+=4;
        else s++;
    }

    return s - start;
}

int utf8_len(const char *s) {
    int n = 0;
    while(*s) {
        n++;
        if((0xE0 & *s) == 0xC0) s+=2;
        else if((0xF0 & *s) == 0xE0) s+=3;
        else if((0xF8 & *s) == 0xF0) s+=4;
        else s++;
    }

    return n;
}

//Reads a utf8 character from a given string into c,
//returns the number of characters read
int utf8_read(const char *s, char c[5]) {
    int i = 0;
    c[i++] = *s++;
    while((*s & 0xC0) == 0x80)
        c[i++] = *s++;

    c[i] = '\0';
    return i;
}

//Dupe the first n characters of s (or all of it if n == 0).
char* utf8_dup(const char *s, size_t n) {
    char *res;
    int idx = n ? utf8_idx(s, n) : strlen(s);

    res = malloc(idx+1);
    memcpy(res, s, idx);
    res[idx] = '\0';
    return res;
}

//Consumes a pointer to a utf8 string and returns the first unicode 
//character as ucs4 encoded int.

uint32_t utf8_ucs4(const char *_c) {
    const unsigned char *c = (const unsigned char *)_c; //Avoid sign extensions in implicit casts.
    uint32_t res = 0;

    if((0xE0 & *c) == 0xC0) {
        res = (*c & 0x1F) << 6;
        res |= 0x3F & c[1];
    } else if((0xF0 & *c) == 0xE0) {
        res = (*c & 0x0F) << 12;
        res |= (0x3F & c[1]) << 6;
        res |= (0x3F & c[2]);
    } else if((0xF1 & *c) == 0xF0) {
        res = (*c & 0x07) << 18;
        res |= (0x3F & c[1]) << 12;
        res |= (0x3F & c[2]) << 6;
        res |= (0x3F & c[3]);
    } else
        res = c[0];

    return res;
}
