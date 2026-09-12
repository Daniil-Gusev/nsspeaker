#include "pcm_player.h"

#include <AudioToolbox/AudioToolbox.h>
#include <pthread.h>
#include <stdlib.h>

#define PCM_NUM_BUFFERS 3
#define PCM_BUFFER_FRAMES 4096

struct PcmPlayer {
  AudioQueueRef queue;
  PcmFillCallback fill;
  void* userdata;
  PcmUserdataFreeFn freeUserdata;
  int channels;
  pthread_mutex_t lock;
  int finished;
  int stopRequested;
  int queueCreated;
  int pendingBuffers;
};

static int fillBuffer(struct PcmPlayer* p, AudioQueueBufferRef buf) {
  size_t frameCapacity =
      buf->mAudioDataBytesCapacity / (sizeof(short) * (size_t)p->channels);
  size_t framesWritten =
      p->fill(p->userdata, (short*)buf->mAudioData, frameCapacity);
  if (framesWritten == 0) {
    return 0;
  }
  buf->mAudioDataByteSize =
      (UInt32)(framesWritten * sizeof(short) * (size_t)p->channels);
  pthread_mutex_lock(&p->lock);
  p->pendingBuffers++;
  pthread_mutex_unlock(&p->lock);
  AudioQueueEnqueueBuffer(p->queue, buf, 0, NULL);
  return 1;
}

static void audioCallback(void* userdata, AudioQueueRef aq,
                          AudioQueueBufferRef buf) {
  struct PcmPlayer* p = (struct PcmPlayer*)userdata;
  int stop;
  (void)aq;
  pthread_mutex_lock(&p->lock);
  stop = p->stopRequested;
  p->pendingBuffers--;
  pthread_mutex_unlock(&p->lock);
  if (stop) return;
  fillBuffer(p, buf);
  pthread_mutex_lock(&p->lock);
  if (p->pendingBuffers <= 0) {
    p->finished = 1;
  }
  pthread_mutex_unlock(&p->lock);
}

static void destroyPlayer(struct PcmPlayer* p) {
  if (!p) return;
  if (p->queueCreated) {
    AudioQueueDispose(p->queue, true);
  }
  pthread_mutex_destroy(&p->lock);
  if (p->freeUserdata) p->freeUserdata(p->userdata);
  free(p);
}

PcmResult PcmPlayerStart(const PcmPlayerConfig* config,
                         PcmPlayerPtr* outPlayer) {
  struct PcmPlayer* p;
  AudioStreamBasicDescription format;
  AudioQueueBufferRef buffers[PCM_NUM_BUFFERS];
  int i, buffersAllocated, primed;
  if (!outPlayer) return PCM_ERR_INVALID_ARG;
  *outPlayer = NULL;
  if (!config || !config->fill || config->channels <= 0 ||
      config->sampleRate <= 0.0) {
    if (config && config->freeUserdata) config->freeUserdata(config->userdata);
    return PCM_ERR_INVALID_ARG;
  }
  p = (struct PcmPlayer*)malloc(sizeof(struct PcmPlayer));
  if (!p) {
    if (config->freeUserdata) config->freeUserdata(config->userdata);
    return PCM_ERR_ALLOC;
  }
  p->fill = config->fill;
  p->userdata = config->userdata;
  p->freeUserdata = config->freeUserdata;
  p->channels = config->channels;
  p->finished = 0;
  p->stopRequested = 0;
  p->queueCreated = 0;
  if (pthread_mutex_init(&p->lock, NULL) != 0) {
    if (p->freeUserdata) p->freeUserdata(p->userdata);
    free(p);
    return PCM_ERR_ALLOC;
  }
  format.mSampleRate = config->sampleRate;
  format.mFormatID = kAudioFormatLinearPCM;
  format.mFormatFlags =
      kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
  format.mChannelsPerFrame = (UInt32)config->channels;
  format.mBitsPerChannel = 16;
  format.mBytesPerFrame = (UInt32)(config->channels * (int)sizeof(short));
  format.mFramesPerPacket = 1;
  format.mBytesPerPacket = format.mBytesPerFrame;
  format.mReserved = 0;
  if (AudioQueueNewOutput(&format, audioCallback, p, NULL, NULL, 0,
                          &p->queue) != noErr) {
    destroyPlayer(p);
    return PCM_ERR_QUEUE_CREATE;
  }
  p->queueCreated = 1;
  buffersAllocated = 0;
  for (i = 0; i < PCM_NUM_BUFFERS; i++) {
    if (AudioQueueAllocateBuffer(p->queue,
                                 PCM_BUFFER_FRAMES * format.mBytesPerFrame,
                                 &buffers[i]) != noErr) {
      break;
    }
    buffersAllocated++;
  }
  if (buffersAllocated < PCM_NUM_BUFFERS) {
    destroyPlayer(p);
    return PCM_ERR_BUFFER_ALLOC;
  }
  primed = 0;
  for (i = 0; i < PCM_NUM_BUFFERS; i++) {
    if (fillBuffer(p, buffers[i])) primed++;
  }
  if (primed == 0) {
    p->finished = 1;
    *outPlayer = p;
    return PCM_OK;
  }
  if (AudioQueueStart(p->queue, NULL) != noErr) {
    destroyPlayer(p);
    return PCM_ERR_QUEUE_CREATE;
  }
  *outPlayer = p;
  return PCM_OK;
}

void PcmPlayerStop(PcmPlayerPtr p) {
  if (!p) return;
  pthread_mutex_lock(&p->lock);
  p->stopRequested = 1;
  pthread_mutex_unlock(&p->lock);
  destroyPlayer(p);
}

int PcmPlayerIsFinished(PcmPlayerPtr p) {
  int result;
  if (!p) return 1;
  pthread_mutex_lock(&p->lock);
  result = p->finished;
  pthread_mutex_unlock(&p->lock);
  return result;
}
