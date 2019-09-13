#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <stdlib.h>

char * strlncpy(char *dst, const char *src, size_t size);

char * strlncpymake(const char *src, size_t size);

char * strlnprintfmake(const char * fmt,...);

int parse_number(const char * str);

#endif // UTIL_H_INCLUDED
