#ifndef CONTEXT_H
#define CONTEXT_H

#include "lang_manager.h"
#include "speech.h"
#include "speech_queue.h"

typedef struct {
  SpeechSynthesizerPtr synth;
  SpeechQueuePtr queue;
  LangManagerPtr lang;
} SpeechContext;

#endif
