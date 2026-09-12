#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

void splitCamelCase(const char* in, char* out, size_t out_size);
char* dupString(const char* s);
void copyBounded(char* dst, size_t dstSize, const char* src);
size_t utf8CharLen(const char* s);
void mapMorphemeBoundaries(const char* in, char* out, size_t out_size);
const char* skipToken(const char* s, const char* delims);

#endif
