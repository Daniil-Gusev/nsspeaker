#ifndef SPEECH_QUEUE_H
#define SPEECH_QUEUE_H

#include "lang_manager.h"
#include "speech.h"

typedef struct SpeechQueue* SpeechQueuePtr;

SpeechQueuePtr SpeechQueueInit(SpeechSynthesizerPtr synth, LangManagerPtr lang);
void SpeechQueueFree(SpeechQueuePtr q);
SpeechResult SpeechQueuePushText(SpeechQueuePtr q, const char* text);
SpeechResult SpeechQueuePushSilence(SpeechQueuePtr q, int duration_ms);
SpeechResult SpeechQueuePushAudio(SpeechQueuePtr q, const char* path);
SpeechResult SpeechQueuePushTone(SpeechQueuePtr q, double frequency,
                                 int duration_ms);
SpeechResult SpeechQueuePushNextVoice(SpeechQueuePtr q, int sayIt);
SpeechResult SpeechQueuePushPreviousVoice(SpeechQueuePtr q, int sayIt);
SpeechResult SpeechQueuePushVoice(SpeechQueuePtr q, const char* voice,
                                  int sayIt);
SpeechResult SpeechQueueDispatch(SpeechQueuePtr q);
SpeechResult SpeechQueueClear(SpeechQueuePtr q);
void SpeechQueueWait(SpeechQueuePtr q, double period_seconds);

void SpeechQueueTick(SpeechQueuePtr q);
double SpeechQueueSecondsUntilNextEvent(SpeechQueuePtr q);

#endif
