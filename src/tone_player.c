#include "tone_player.h"

#include <math.h>
#include <stdlib.h>

#include "pcm_player.h"

#define TONE_SAMPLE_RATE 44100.0
#define TONE_AMPLITUDE 0.4
#define FADE_DURATION_MS 16

struct TonePlayer {
  PcmPlayerPtr pcm;
};

typedef struct {
  double frequency, phase;
  long framesTotal, framesRemaining, fadeFrames;
} ToneSource;

static size_t toneFill(void* userdata, short* out, size_t maxFrames) {
  ToneSource* src = (ToneSource*)userdata;
  double phaseStep = 2.0 * M_PI * src->frequency / TONE_SAMPLE_RATE;
  size_t framesToWrite;
  size_t i;
  if (src->framesRemaining <= 0) return 0;
  if (src->framesRemaining < (long)maxFrames)
    framesToWrite = (size_t)src->framesRemaining;
  else
    framesToWrite = (size_t)maxFrames;
  for (i = 0; i < framesToWrite; i++) {
    long currentFrame = src->framesTotal - src->framesRemaining;
    double gain = 1.0;
    if (currentFrame < src->fadeFrames && src->fadeFrames > 0)
      gain = (double)currentFrame / src->fadeFrames;
    else if (src->framesRemaining <= src->fadeFrames && src->fadeFrames > 0)
      gain = (double)src->framesRemaining / src->fadeFrames;
    out[i] = (short)(sin(src->phase) * TONE_AMPLITUDE * gain * 32767.0);
    src->phase += phaseStep;
    if (src->phase > 2.0 * M_PI) src->phase -= 2.0 * M_PI;
    src->framesRemaining--;
  }
  return framesToWrite;
}

static void toneFreeSource(void* userdata) { free(userdata); }

ToneResult TonePlayerStart(double frequency, int duration_ms,
                           TonePlayerPtr* outPlayer) {
  ToneSource* src;
  PcmPlayerConfig config;
  PcmResult res;
  struct TonePlayer* p;
  long totalFrames = (long)(TONE_SAMPLE_RATE * duration_ms / 1000.0);
  long fadeFrames = (long)(TONE_SAMPLE_RATE * FADE_DURATION_MS / 1000.0);
  if (!outPlayer) return TONE_ERR_INVALID_ARG;
  *outPlayer = NULL;
  if (frequency <= 0.0 || duration_ms < 0) return TONE_ERR_INVALID_ARG;
  src = (ToneSource*)malloc(sizeof(ToneSource));
  if (!src) return TONE_ERR_ALLOC;
  if (fadeFrames * 2 > totalFrames) fadeFrames = totalFrames / 2;
  src->frequency = frequency;
  src->phase = 0.0;
  src->framesTotal = totalFrames;
  src->framesRemaining = totalFrames;
  src->fadeFrames = fadeFrames;
  p = (struct TonePlayer*)malloc(sizeof(struct TonePlayer));
  if (!p) {
    free(src);
    return TONE_ERR_ALLOC;
  }
  config.sampleRate = TONE_SAMPLE_RATE;
  config.channels = 1;
  config.fill = toneFill;
  config.userdata = src;
  config.freeUserdata = toneFreeSource;
  res = PcmPlayerStart(&config, &p->pcm);
  if (res != PCM_OK) {
    free(p);
    if (res == PCM_ERR_QUEUE_CREATE) return TONE_ERR_QUEUE_CREATE;
    if (res == PCM_ERR_BUFFER_ALLOC) return TONE_ERR_BUFFER_ALLOC;
    return TONE_ERR_ALLOC;
  }
  *outPlayer = p;
  return TONE_OK;
}

void TonePlayerStop(TonePlayerPtr p) {
  if (!p) return;
  PcmPlayerStop(p->pcm);
  free(p);
}

int TonePlayerIsFinished(TonePlayerPtr p) {
  if (!p) return 1;
  return PcmPlayerIsFinished(p->pcm);
}
