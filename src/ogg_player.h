#ifndef OGG_PLAYER_H
#define OGG_PLAYER_H

typedef struct OggPlayer* OggPlayerPtr;

typedef enum {
  OGG_OK = 0,
  OGG_ERR_INVALID_ARG = -1,
  OGG_ERR_ALLOC = -2,
  OGG_ERR_OPEN_FAILED = -3,
  OGG_ERR_DECODE_INFO = -4,
  OGG_ERR_QUEUE_CREATE = -5,
  OGG_ERR_BUFFER_ALLOC = -6
} OggResult;

OggResult OggPlayerStart(const char* path, OggPlayerPtr* outPlayer);
void OggPlayerStop(OggPlayerPtr p);
int OggPlayerIsFinished(OggPlayerPtr p);

#endif
