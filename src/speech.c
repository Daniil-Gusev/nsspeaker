#include "speech.h"

#include <stdlib.h>

#include "nsspeech.h"
#include "speech_punct.h"

struct SpeechState {
  NSSpeechStatePtr raw;
  float defaultRate, defaultVolume;
  float characterScale;
  int splitCaps;
  SpeechPunctMode punct;
};

static int ToRawBoundary(SpeechBoundary b) {
  return (b == SPEECH_BOUNDARY_WORD) ? 1 : 0;
}

static SpeechSynthesizerPtr newSpeechState(NSSpeechStatePtr raw) {
  struct SpeechState* s;
  if (!raw) return NULL;
  s = (struct SpeechState*)malloc(sizeof(struct SpeechState));
  if (!s) {
    NSSpeechFree(raw);
    return NULL;
  }
  s->raw = raw;
  s->defaultRate = NSSpeechGetRate(raw);
  s->defaultVolume = NSSpeechGetVolume(raw);
  s->characterScale = 1.0f;
  s->splitCaps = 0;
  s->punct = PUNCT_SOME;
  return s;
}

SpeechSynthesizerPtr SpeechInit(void) { return newSpeechState(NSSpeechInit()); }

SpeechSynthesizerPtr SpeechInitWithVoice(const char* voice) {
  if (!voice) return NULL;
  return newSpeechState(NSSpeechInitWithVoice(voice));
}

SpeechResult SpeechFree(SpeechSynthesizerPtr s) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  NSSpeechFree(s->raw);
  free(s);
  return SPEECH_OK;
}

void SpeechRunLoopPump(double seconds) { NSSpeechRunLoopPump(seconds); }

SpeechResult SpeechSpeak(SpeechSynthesizerPtr s, const char* str) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (!str) return SPEECH_ERR_INVALID_ARG;
  if (!NSSpeechSpeak(s->raw, str)) return SPEECH_ERR_START_FAILED;
  return SPEECH_OK;
}

SpeechResult SpeechSpeakToFile(SpeechSynthesizerPtr s, const char* str,
                               const char* path) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (!str || !path) return SPEECH_ERR_INVALID_ARG;
  return NSSpeechSpeakToFile(s->raw, str, path) ? SPEECH_OK
                                                : SPEECH_ERR_START_FAILED;
}

SpeechResult SpeechStop(SpeechSynthesizerPtr s) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  NSSpeechStop(s->raw);
  return SPEECH_OK;
}

SpeechResult SpeechPauseAt(SpeechSynthesizerPtr s, SpeechBoundary boundary) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  NSSpeechPauseAt(s->raw, ToRawBoundary(boundary));
  return SPEECH_OK;
}

SpeechResult SpeechContinueSpeaking(SpeechSynthesizerPtr s) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  NSSpeechContinueSpeaking(s->raw);
  return SPEECH_OK;
}

SpeechResult SpeechStopAt(SpeechSynthesizerPtr s, SpeechBoundary boundary) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  NSSpeechStopAt(s->raw, ToRawBoundary(boundary));
  return SPEECH_OK;
}

SpeechResult SpeechIsSpeaking(SpeechSynthesizerPtr s, int* status) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (!status) return SPEECH_ERR_INVALID_ARG;
  *status = NSSpeechIsSpeaking(s->raw);
  return SPEECH_OK;
}

SpeechResult SpeechWait(SpeechSynthesizerPtr s, double period_seconds) {
  int status;
  SpeechResult res;
  if (!s) return SPEECH_ERR_NULL_STATE;
  res = SpeechIsSpeaking(s, &status);
  while (res == SPEECH_OK && status) {
    SpeechRunLoopPump(period_seconds);
    res = SpeechIsSpeaking(s, &status);
  }
  return res;
}

SpeechResult SpeechSetFinishedCallback(SpeechSynthesizerPtr s,
                                       SpeechFinishedCallback cb,
                                       void* userdata) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  NSSpeechSetFinishedCallback(s->raw, cb, userdata);
  return SPEECH_OK;
}

SpeechResult SpeechSetWillSpeakWordCallback(SpeechSynthesizerPtr s,
                                            SpeechWillSpeakWordCallback cb,
                                            void* userdata) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  NSSpeechSetWillSpeakWordCallback(s->raw, cb, userdata);
  return SPEECH_OK;
}

SpeechResult SpeechSetWillSpeakPhonemeCallback(
    SpeechSynthesizerPtr s, SpeechWillSpeakPhonemeCallback cb, void* userdata) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  NSSpeechSetWillSpeakPhonemeCallback(s->raw, cb, userdata);
  return SPEECH_OK;
}

SpeechResult SpeechSetErrorCallback(SpeechSynthesizerPtr s,
                                    SpeechErrorCallback cb, void* userdata) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  NSSpeechSetErrorCallback(s->raw, cb, userdata);
  return SPEECH_OK;
}

SpeechResult SpeechSetRate(SpeechSynthesizerPtr s, float rate) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (rate <= 0.0f) return SPEECH_ERR_INVALID_ARG;
  NSSpeechSetRate(s->raw, rate);
  return SPEECH_OK;
}

SpeechResult SpeechGetRate(SpeechSynthesizerPtr s, float* rate) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (!rate) return SPEECH_ERR_INVALID_ARG;
  *rate = NSSpeechGetRate(s->raw);
  return SPEECH_OK;
}

SpeechResult SpeechSetVolume(SpeechSynthesizerPtr s, float volume) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (volume < 0.0f || volume > 1.0f) return SPEECH_ERR_INVALID_ARG;
  NSSpeechSetVolume(s->raw, volume);
  return SPEECH_OK;
}

SpeechResult SpeechGetVolume(SpeechSynthesizerPtr s, float* volume) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (!volume) return SPEECH_ERR_INVALID_ARG;
  *volume = NSSpeechGetVolume(s->raw);
  return SPEECH_OK;
}

float SpeechGetDefaultRate(SpeechSynthesizerPtr s) {
  if (!s) return 150.0f;
  return s->defaultRate;
}

float SpeechGetDefaultVolume(SpeechSynthesizerPtr s) {
  if (!s) return 0.5f;
  return s->defaultVolume;
}

SpeechResult SpeechSetCharacterScale(SpeechSynthesizerPtr s, float scale) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (scale <= 0.0f) return SPEECH_ERR_INVALID_ARG;
  s->characterScale = scale;
  return SPEECH_OK;
}

float SpeechGetCharacterScale(SpeechSynthesizerPtr s) {
  if (!s) return 1.0f;
  return s->characterScale;
}

SpeechResult SpeechSetSplitCaps(SpeechSynthesizerPtr s, int enabled) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  s->splitCaps = enabled ? 1 : 0;
  return SPEECH_OK;
}

int SpeechGetSplitCaps(SpeechSynthesizerPtr s) {
  if (!s) return 0;
  return s->splitCaps;
}

SpeechResult SpeechSetPunctuation(SpeechSynthesizerPtr s,
                                  SpeechPunctMode punct) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (punct < PUNCT_NONE || punct > PUNCT_ALL) return SPEECH_ERR_INVALID_ARG;
  s->punct = punct;
  return SPEECH_OK;
}

SpeechPunctMode SpeechGetPunctuation(SpeechSynthesizerPtr s) {
  if (!s) return PUNCT_SOME;
  return s->punct;
}

SpeechResult SpeechSetVoice(SpeechSynthesizerPtr s, const char* voice) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (!voice) return SPEECH_ERR_INVALID_ARG;
  return NSSpeechSetVoice(s->raw, voice) ? SPEECH_OK : SPEECH_ERR_SET_VOICE;
}

size_t SpeechGetVoiceAttributes(const char* voice, char* buf, size_t buf_size) {
  return NSSpeechGetVoiceAttributes(voice, buf, buf_size);
}

size_t SpeechGetAvailableVoices(char* buf, size_t buf_size) {
  return NSSpeechGetAvailableVoices(buf, buf_size);
}

size_t SpeechGetDefaultVoice(char* buf, size_t buf_size) {
  return NSSpeechGetDefaultVoice(buf, buf_size);
}

SpeechResult SpeechIsAnyApplicationSpeaking(int* status) {
  if (!status) return SPEECH_ERR_INVALID_ARG;
  *status = NSSpeechIsAnyApplicationSpeaking();
  return SPEECH_OK;
}

size_t SpeechGetTextProperty(SpeechSynthesizerPtr s, const char* property,
                             char* buf, size_t buf_size) {
  if (!s || !property) {
    if (buf && buf_size > 0) buf[0] = '\0';
    return 1;
  }
  return NSSpeechGetTextProperty(s->raw, property, buf, buf_size);
}

SpeechResult SpeechSetTextProperty(SpeechSynthesizerPtr s, const char* property,
                                   const char* value) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (!property || !value) return SPEECH_ERR_INVALID_ARG;
  return NSSpeechSetTextProperty(s->raw, property, value)
             ? SPEECH_OK
             : SPEECH_ERR_SET_PROPERTY;
}

SpeechResult SpeechGetNumberProperty(SpeechSynthesizerPtr s,
                                     const char* property, double* value) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (!property || !value) return SPEECH_ERR_INVALID_ARG;
  return NSSpeechGetNumberProperty(s->raw, property, value)
             ? SPEECH_OK
             : SPEECH_ERR_SET_PROPERTY;
}

SpeechResult SpeechSetNumberProperty(SpeechSynthesizerPtr s,
                                     const char* property, double value) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  if (!property) return SPEECH_ERR_INVALID_ARG;
  return NSSpeechSetNumberProperty(s->raw, property, value)
             ? SPEECH_OK
             : SPEECH_ERR_SET_PROPERTY;
}

SpeechResult SpeechResetToDefaults(SpeechSynthesizerPtr s) {
  if (!s) return SPEECH_ERR_NULL_STATE;
  NSSpeechSetRate(s->raw, s->defaultRate);
  NSSpeechSetVolume(s->raw, s->defaultVolume);
  s->characterScale = 1.0f;
  s->splitCaps = 0;
  s->punct = PUNCT_SOME;
  return SPEECH_OK;
}

size_t SpeechGetPhonemes(SpeechSynthesizerPtr s, const char* text, char* buf,
                         size_t buf_size) {
  if (!s || !text) {
    if (buf && buf_size > 0) buf[0] = '\0';
    return 1;
  }
  return NSSpeechGetPhonemes(s->raw, text, buf, buf_size);
}
