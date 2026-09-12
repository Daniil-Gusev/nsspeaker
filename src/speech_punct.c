#include "speech_punct.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

typedef struct {
  char ch;
  const char* name;
} PunctSpokenName;

static const PunctSpokenName kPunctNames[] = {
    {'!', "exclamation"},   {'"', "quote"},
    {'#', "pound"},         {'$', "dollar"},
    {'%', "percent"},       {'&', "ampersand"},
    {'\'', "apostrophe"},   {'(', "left paren"},
    {')', "right paren"},   {'*', "star"},
    {'+', "plus"},          {',', "comma"},
    {'-', "dash"},          {'.', "dot"},
    {'/', "slash"},         {':', "colon"},
    {';', "semi"},          {'<', "less than"},
    {'=', "equals"},        {'>', "greater than"},
    {'?', "question"},      {'@', "at"},
    {'[', "left bracket"},  {'\\', "backslash"},
    {']', "right bracket"}, {'^', "caret"},
    {'_', "underscore"},    {'`', "backtick"},
    {'{', "left brace"},    {'|', "pipe"},
    {'}', "right brace"},   {'~', "tilde"},
};
#define PUNCT_NAME_COUNT (sizeof(kPunctNames) / sizeof(kPunctNames[0]))

static const char* spokenNameFor(char c) {
  size_t i;
  for (i = 0; i < PUNCT_NAME_COUNT; i++) {
    if (kPunctNames[i].ch == c) return kPunctNames[i].name;
  }
  return NULL;
}

void applyPunctuationMode(const char* in, char* out, size_t out_size,
                          SpeechPunctMode mode) {
  size_t i, j = 0;
  if (out_size == 0) return;
  if (mode == PUNCT_SOME) {
    copyBounded(out, out_size, in);
    return;
  }
  for (i = 0; in[i] != '\0' && j + 1 < out_size; i++) {
    unsigned char c = (unsigned char)in[i];
    if (c < 128 && ispunct(c)) {
      if (mode == PUNCT_ALL) {
        const char* name = spokenNameFor((char)c);
        if (name) {
          size_t nlen = strlen(name);
          if (j > 0 && out[j - 1] != ' ' && j + 1 < out_size) out[j++] = ' ';
          if (j + nlen + 1 < out_size) {
            memcpy(out + j, name, nlen);
            j += nlen;
          }
          if (j + 1 < out_size) out[j++] = ' ';
          continue;
        }
      }
      if (j > 0 && out[j - 1] != ' ' && j + 1 < out_size) out[j++] = ' ';
      continue;
    }
    out[j++] = (char)c;
  }
  out[j] = '\0';
}
