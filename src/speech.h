#ifndef SPEECH_H
#define SPEECH_H

#include <stddef.h>

#include "speech_punct.h"

typedef struct SpeechState* SpeechSynthesizerPtr;

typedef enum {
  SPEECH_OK = 0,
  SPEECH_ERR_NULL_STATE = -1,
  SPEECH_ERR_INVALID_ARG = -2,
  SPEECH_ERR_ALLOC = -3,
  SPEECH_ERR_SET_VOICE = -4,
  SPEECH_ERR_START_FAILED = -5,
  SPEECH_ERR_SET_PROPERTY = -6
} SpeechResult;

typedef enum {
  SPEECH_BOUNDARY_IMMEDIATE = 0,
  SPEECH_BOUNDARY_WORD = 1
} SpeechBoundary;

typedef void (*SpeechFinishedCallback)(void* userdata, int success);
typedef void (*SpeechWillSpeakWordCallback)(void* userdata, size_t location,
                                            size_t length,
                                            const char* fullText);
typedef void (*SpeechWillSpeakPhonemeCallback)(void* userdata,
                                               short phonemeOpcode);
typedef void (*SpeechErrorCallback)(void* userdata, size_t charIndex,
                                    const char* fullText, const char* message);

SpeechSynthesizerPtr SpeechInit(void);
SpeechSynthesizerPtr SpeechInitWithVoice(const char* voice);
SpeechResult SpeechFree(SpeechSynthesizerPtr s);

void SpeechRunLoopPump(double seconds);

SpeechResult SpeechSpeak(SpeechSynthesizerPtr s, const char* str);
SpeechResult SpeechSpeakToFile(SpeechSynthesizerPtr s, const char* str,
                               const char* path);
SpeechResult SpeechStop(SpeechSynthesizerPtr s);
SpeechResult SpeechPauseAt(SpeechSynthesizerPtr s, SpeechBoundary boundary);
SpeechResult SpeechContinueSpeaking(SpeechSynthesizerPtr s);
SpeechResult SpeechStopAt(SpeechSynthesizerPtr s, SpeechBoundary boundary);
SpeechResult SpeechWait(SpeechSynthesizerPtr s, double period_seconds);

SpeechResult SpeechSetFinishedCallback(SpeechSynthesizerPtr s,
                                       SpeechFinishedCallback cb,
                                       void* userdata);
SpeechResult SpeechSetWillSpeakWordCallback(SpeechSynthesizerPtr s,
                                            SpeechWillSpeakWordCallback cb,
                                            void* userdata);
SpeechResult SpeechSetWillSpeakPhonemeCallback(
    SpeechSynthesizerPtr s, SpeechWillSpeakPhonemeCallback cb, void* userdata);
SpeechResult SpeechSetErrorCallback(SpeechSynthesizerPtr s,
                                    SpeechErrorCallback cb, void* userdata);

SpeechResult SpeechIsSpeaking(SpeechSynthesizerPtr s, int* status);
SpeechResult SpeechSetRate(SpeechSynthesizerPtr s, float rate);
SpeechResult SpeechGetRate(SpeechSynthesizerPtr s, float* rate);
SpeechResult SpeechSetVolume(SpeechSynthesizerPtr s, float volume);
SpeechResult SpeechGetVolume(SpeechSynthesizerPtr s, float* volume);
float SpeechGetDefaultRate(SpeechSynthesizerPtr s);
float SpeechGetDefaultVolume(SpeechSynthesizerPtr s);

SpeechResult SpeechSetCharacterScale(SpeechSynthesizerPtr s, float scale);
float SpeechGetCharacterScale(SpeechSynthesizerPtr s);
SpeechResult SpeechSetSplitCaps(SpeechSynthesizerPtr s, int enabled);
int SpeechGetSplitCaps(SpeechSynthesizerPtr s);
SpeechResult SpeechSetPunctuation(SpeechSynthesizerPtr s,
                                  SpeechPunctMode punct);
SpeechPunctMode SpeechGetPunctuation(SpeechSynthesizerPtr s);

SpeechResult SpeechSetVoice(SpeechSynthesizerPtr s, const char* voice);
size_t SpeechGetVoiceAttributes(const char* voice, char* buf, size_t buf_size);
size_t SpeechGetAvailableVoices(char* buf, size_t buf_size);
size_t SpeechGetDefaultVoice(char* buf, size_t buf_size);
SpeechResult SpeechIsAnyApplicationSpeaking(int* status);

size_t SpeechGetTextProperty(SpeechSynthesizerPtr s, const char* property,
                             char* buf, size_t buf_size);
SpeechResult SpeechSetTextProperty(SpeechSynthesizerPtr s, const char* property,
                                   const char* value);
SpeechResult SpeechGetNumberProperty(SpeechSynthesizerPtr s,
                                     const char* property, double* value);
SpeechResult SpeechSetNumberProperty(SpeechSynthesizerPtr s,
                                     const char* property, double value);

SpeechResult SpeechResetToDefaults(SpeechSynthesizerPtr s);

size_t SpeechGetPhonemes(SpeechSynthesizerPtr s, const char* text, char* buf,
                         size_t buf_size);

#endif
