#ifndef PCM_PLAYER_H
#define PCM_PLAYER_H

#include <stddef.h>

typedef struct PcmPlayer* PcmPlayerPtr;

typedef enum {
  PCM_OK = 0,
  PCM_ERR_INVALID_ARG = -1,
  PCM_ERR_ALLOC = -2,
  PCM_ERR_QUEUE_CREATE = -3,
  PCM_ERR_BUFFER_ALLOC = -4
} PcmResult;

typedef size_t (*PcmFillCallback)(void* userdata, short* out, size_t maxFrames);
typedef void (*PcmUserdataFreeFn)(void* userdata);

typedef struct {
  double sampleRate;
  int channels;
  PcmFillCallback fill;
  void* userdata;
  PcmUserdataFreeFn freeUserdata;
} PcmPlayerConfig;

PcmResult PcmPlayerStart(const PcmPlayerConfig* config,
                         PcmPlayerPtr* outPlayer);
void PcmPlayerStop(PcmPlayerPtr p);
int PcmPlayerIsFinished(PcmPlayerPtr p);

#endif
