#ifndef TONE_PLAYER_H
#define TONE_PLAYER_H

typedef struct TonePlayer* TonePlayerPtr;

typedef enum {
  TONE_OK = 0,
  TONE_ERR_INVALID_ARG = -1,
  TONE_ERR_ALLOC = -2,
  TONE_ERR_QUEUE_CREATE = -3,
  TONE_ERR_BUFFER_ALLOC = -4
} ToneResult;

ToneResult TonePlayerStart(double frequency, int duration_ms,
                           TonePlayerPtr* outPlayer);
void TonePlayerStop(TonePlayerPtr p);
int TonePlayerIsFinished(TonePlayerPtr p);

#endif
