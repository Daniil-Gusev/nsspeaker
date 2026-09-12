#ifndef SPEECH_PUNCT_H
#define SPEECH_PUNCT_H

#include <stddef.h>

typedef enum {
  PUNCT_INVALID = -1,
  PUNCT_NONE = 0,
  PUNCT_SOME = 1,
  PUNCT_ALL = 2
} SpeechPunctMode;

void applyPunctuationMode(const char* in, char* out, size_t out_size,
                          SpeechPunctMode mode);

#endif
