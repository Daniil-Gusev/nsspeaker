#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void splitCamelCase(const char* in, char* out, size_t out_size) {
  size_t i, j = 0;
  for (i = 0; in[i] != '\0' && j + 1 < out_size; i++) {
    if (i > 0 && isupper((unsigned char)in[i]) &&
        !isupper((unsigned char)in[i - 1])) {
      if (j + 1 < out_size) out[j++] = ' ';
    }
    out[j++] = in[i];
  }
  out[j] = '\0';
}

char* dupString(const char* s) {
  size_t len = strlen(s) + 1;
  char* copy = (char*)malloc(len);
  if (copy) strcpy(copy, s);
  return copy;
}

void copyBounded(char* dst, size_t dstSize, const char* src) {
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
}

size_t utf8CharLen(const char* s) {
  unsigned char c;
  if (!s || s[0] == '\0') return 0;
  c = (unsigned char)s[0];
  if (c < 128u)
    return 1;
  else if ((c & 224u) == 192u && c > 193)
    return 2;
  else if ((c & 240u) == 224u)
    return 3;
  else if ((c & 248u) == 240u && c < 245)
    return 4;
  return 1;
}

void mapMorphemeBoundaries(const char* in, char* out, size_t out_size) {
  size_t i = 0, j = 0;
  while (in[i] != '\0' && j + 1 < out_size) {
    if (in[i] == '[' && in[i + 1] == '*' && in[i + 2] == ']') {
      i += 3;
      continue;
    }
    out[j++] = in[i++];
  }
  out[j] = '\0';
}

const char* skipToken(const char* s, const char* delims) {
  s += strcspn(s, delims);
  if (*s == '\0') return NULL;
  return s + 1;
}
