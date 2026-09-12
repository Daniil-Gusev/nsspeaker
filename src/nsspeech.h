#ifndef NSSPEECH_H
#define NSSPEECH_H

#include <stddef.h>

typedef struct NSSpeechState* NSSpeechStatePtr;

typedef void (*NSSpeechFinishedCallback)(void* userdata, int success);
typedef void (*NSSpeechWillSpeakWordCallback)(void* userdata, size_t location,
                                              size_t length,
                                              const char* fullText);
typedef void (*NSSpeechWillSpeakPhonemeCallback)(void* userdata,
                                                 short phonemeOpcode);
typedef void (*NSSpeechErrorCallback)(void* userdata, size_t charIndex,
                                      const char* fullText,
                                      const char* message);

NSSpeechStatePtr NSSpeechInit(void);
NSSpeechStatePtr NSSpeechInitWithVoice(const char* voice);
void NSSpeechFree(NSSpeechStatePtr s);

void NSSpeechRunLoopPump(double seconds);

int NSSpeechSpeak(NSSpeechStatePtr s, const char* str);
int NSSpeechSpeakToFile(NSSpeechStatePtr s, const char* str, const char* path);
void NSSpeechStop(NSSpeechStatePtr s);
void NSSpeechPauseAt(NSSpeechStatePtr s, int wordBoundary);
void NSSpeechContinueSpeaking(NSSpeechStatePtr s);
void NSSpeechStopAt(NSSpeechStatePtr s, int wordBoundary);
int NSSpeechIsSpeaking(NSSpeechStatePtr s);

void NSSpeechSetRate(NSSpeechStatePtr s, float rate);
float NSSpeechGetRate(NSSpeechStatePtr s);
void NSSpeechSetVolume(NSSpeechStatePtr s, float volume);
float NSSpeechGetVolume(NSSpeechStatePtr s);

int NSSpeechSetVoice(NSSpeechStatePtr s, const char* voice);

size_t NSSpeechGetVoiceAttributes(const char* voice, char* buf,
                                  size_t buf_size);
size_t NSSpeechGetAvailableVoices(char* buf, size_t buf_size);
size_t NSSpeechGetDefaultVoice(char* buf, size_t buf_size);
int NSSpeechIsAnyApplicationSpeaking(void);

size_t NSSpeechGetTextProperty(NSSpeechStatePtr s, const char* property,
                               char* buf, size_t buf_size);
int NSSpeechSetTextProperty(NSSpeechStatePtr s, const char* property,
                            const char* value);
int NSSpeechGetNumberProperty(NSSpeechStatePtr s, const char* property,
                              double* value);
int NSSpeechSetNumberProperty(NSSpeechStatePtr s, const char* property,
                              double value);

size_t NSSpeechGetPhonemes(NSSpeechStatePtr s, const char* text, char* buf,
                           size_t buf_size);

void NSSpeechSetFinishedCallback(NSSpeechStatePtr s,
                                 NSSpeechFinishedCallback cb, void* userdata);
void NSSpeechSetWillSpeakWordCallback(NSSpeechStatePtr s,
                                      NSSpeechWillSpeakWordCallback cb,
                                      void* userdata);
void NSSpeechSetWillSpeakPhonemeCallback(NSSpeechStatePtr s,
                                         NSSpeechWillSpeakPhonemeCallback cb,
                                         void* userdata);
void NSSpeechSetErrorCallback(NSSpeechStatePtr s, NSSpeechErrorCallback cb,
                              void* userdata);

#endif
