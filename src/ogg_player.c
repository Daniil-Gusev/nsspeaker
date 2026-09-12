#include "ogg_player.h"

#include <stdlib.h>

#define OGG_IMPL
#define VORBIS_IMPL
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <stdint.h>
#define inline
#include "third_party/minivorbis.h"
#undef inline

#include "pcm_player.h"

struct OggPlayer {
  PcmPlayerPtr pcm;
};

typedef struct {
  OggVorbis_File vf;
  int channels;
} OggSource;

static size_t oggFill(void* userdata, short* out, size_t maxFrames) {
  OggSource* src = (OggSource*)userdata;
  size_t filledSamples = 0;
  size_t maxSamples = maxFrames * (size_t)src->channels;
  while (filledSamples < maxSamples) {
    int bitstream;
    long got = ov_read(&src->vf, (char*)out + filledSamples * sizeof(short),
                       (int)((maxSamples - filledSamples) * sizeof(short)), 0,
                       2, 1, &bitstream);
    if (got <= 0) break;
    filledSamples += (size_t)got / sizeof(short);
  }
  return filledSamples / (size_t)src->channels;
}

static void oggFreeSource(void* userdata) {
  OggSource* src = (OggSource*)userdata;
  ov_clear(&src->vf);
  free(src);
}

OggResult OggPlayerStart(const char* path, OggPlayerPtr* outPlayer) {
  OggSource* src;
  vorbis_info* info;
  PcmPlayerConfig config;
  PcmResult res;
  struct OggPlayer* p;
  if (!outPlayer) return OGG_ERR_INVALID_ARG;
  *outPlayer = NULL;
  if (!path) return OGG_ERR_INVALID_ARG;
  src = (OggSource*)malloc(sizeof(OggSource));
  if (!src) return OGG_ERR_ALLOC;
  if (ov_fopen(path, &src->vf) != 0) {
    free(src);
    return OGG_ERR_OPEN_FAILED;
  }
  info = ov_info(&src->vf, -1);
  if (!info) {
    ov_clear(&src->vf);
    free(src);
    return OGG_ERR_DECODE_INFO;
  }
  src->channels = info->channels;
  p = (struct OggPlayer*)malloc(sizeof(struct OggPlayer));
  if (!p) {
    ov_clear(&src->vf);
    free(src);
    return OGG_ERR_ALLOC;
  }
  config.sampleRate = info->rate;
  config.channels = info->channels;
  config.fill = oggFill;
  config.userdata = src;
  config.freeUserdata = oggFreeSource;
  res = PcmPlayerStart(&config, &p->pcm);
  if (res != PCM_OK) {
    free(p);
    if (res == PCM_ERR_QUEUE_CREATE) return OGG_ERR_QUEUE_CREATE;
    if (res == PCM_ERR_BUFFER_ALLOC) return OGG_ERR_BUFFER_ALLOC;
    return OGG_ERR_ALLOC;
  }
  *outPlayer = p;
  return OGG_OK;
}

void OggPlayerStop(OggPlayerPtr p) {
  if (!p) return;
  PcmPlayerStop(p->pcm);
  free(p);
}

int OggPlayerIsFinished(OggPlayerPtr p) {
  if (!p) return 1;
  return PcmPlayerIsFinished(p->pcm);
}
