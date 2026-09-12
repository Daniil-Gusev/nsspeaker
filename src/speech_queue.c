#include "speech_queue.h"

#include <mach/mach_time.h>
#include <stdlib.h>

#include "lang_manager.h"
#include "ogg_player.h"
#include "speech.h"
#include "tone_player.h"
#include "utils.h"

static double monotonicNow(void) {
  static mach_timebase_info_data_t timebase;
  static int have_timebase = 0;
  uint64_t ticks;
  if (!have_timebase) {
    mach_timebase_info(&timebase);
    have_timebase = 1;
  }
  ticks = mach_absolute_time();
  return ((double)ticks * (double)timebase.numer) /
         ((double)timebase.denom * 1e9);
}

typedef enum {
  QUEUE_ITEM_TEXT,
  QUEUE_ITEM_SILENCE,
  QUEUE_ITEM_AUDIO,
  QUEUE_ITEM_TONE,
  QUEUE_ITEM_SETUP_VOICE
} QueueItemType;

typedef struct {
  double frequency;
  int duration_ms;
} ToneParams;

typedef enum {
  SET_VOICE_NEXT,
  SET_VOICE_PREVIOUS,
  SET_VOICE_BY_SPEC
} SetupVoiceType;

typedef struct {
  SetupVoiceType type;
  char* spec;
  int sayIt;
} SetupVoiceParams;

typedef struct QueueItem {
  QueueItemType type;
  union {
    char* text;
    double silenceSeconds;
    char* audioPath;
    ToneParams tone;
    SetupVoiceParams voice;
  } data;
  struct QueueItem* next;
} QueueItem;

struct SpeechQueue {
  SpeechSynthesizerPtr synth;
  LangManagerPtr lang;
  QueueItem* head;
  QueueItem* tail;
  OggPlayerPtr activePlayer;
  TonePlayerPtr activeTone;
  double silenceDeadline;
  int dispatching;
  int silencePending;
  int speakingText;
};

static void freeItem(QueueItem* item) {
  if (item->type == QUEUE_ITEM_TEXT) {
    free(item->data.text);
  } else if (item->type == QUEUE_ITEM_AUDIO) {
    free(item->data.audioPath);
  } else if (item->type == QUEUE_ITEM_SETUP_VOICE) {
    free(item->data.voice.spec);
  }
  free(item);
}

static void advance(SpeechQueuePtr q) {
  QueueItem* item;
  if (!q) return;
  for (;;) {
    item = q->head;
    if (!item) {
      q->dispatching = 0;
      return;
    }
    q->head = item->next;
    if (!q->head) q->tail = NULL;
    if (item->type == QUEUE_ITEM_TEXT) {
      SpeechSpeak(q->synth, item->data.text);
      q->speakingText = 1;
      freeItem(item);
      return;
    } else if (item->type == QUEUE_ITEM_SILENCE) {
      double seconds = item->data.silenceSeconds;
      freeItem(item);
      q->silenceDeadline = monotonicNow() + seconds;
      q->silencePending = 1;
      return;
    } else if (item->type == QUEUE_ITEM_AUDIO) {
      OggResult res = OggPlayerStart(item->data.audioPath, &q->activePlayer);
      freeItem(item);
      if (res == OGG_OK) return;
      q->activePlayer = NULL;
    } else if (item->type == QUEUE_ITEM_TONE) {
      ToneParams params = item->data.tone;
      ToneResult res;
      freeItem(item);
      res =
          TonePlayerStart(params.frequency, params.duration_ms, &q->activeTone);
      if (res == TONE_OK) return;
      q->activeTone = NULL;
    } else {
      int speaking = 0;
      SpeechResult res;
      if (SpeechIsSpeaking(q->synth, &speaking) == SPEECH_OK && speaking) {
        item->next = q->head;
        q->head = item;
        if (!q->tail) q->tail = item;
        return;
      }
      if (item->data.voice.type == SET_VOICE_NEXT)
        res = LangManagerNext(q->lang, item->data.voice.sayIt);
      else if (item->data.voice.type == SET_VOICE_PREVIOUS)
        res = LangManagerPrevious(q->lang, item->data.voice.sayIt);
      else
        res = LangManagerSetLang(q->lang, item->data.voice.spec,
                                 item->data.voice.sayIt);
      if (res == SPEECH_ERR_SET_VOICE) {
        item->next = q->head;
        q->head = item;
        if (!q->tail) q->tail = item;
        return;
      }
      freeItem(item);
    }
  }
}

void SpeechQueueTick(SpeechQueuePtr q) {
  int speaking;
  if (!q) return;
  if (q->speakingText) {
    if (SpeechIsSpeaking(q->synth, &speaking) == SPEECH_OK && !speaking) {
      q->speakingText = 0;
      advance(q);
    }
    return;
  }
  if (q->silencePending && monotonicNow() >= q->silenceDeadline) {
    q->silencePending = 0;
    advance(q);
    return;
  }
  if (q->activePlayer && OggPlayerIsFinished(q->activePlayer)) {
    OggPlayerStop(q->activePlayer);
    q->activePlayer = NULL;
    advance(q);
    return;
  }
  if (q->activeTone && TonePlayerIsFinished(q->activeTone)) {
    TonePlayerStop(q->activeTone);
    q->activeTone = NULL;
    advance(q);
    return;
  }
  if (q->head) advance(q);
}

double SpeechQueueSecondsUntilNextEvent(SpeechQueuePtr q) {
  if (!q || !q->silencePending) return -1.0;
  return q->silenceDeadline - monotonicNow();
}

SpeechQueuePtr SpeechQueueInit(SpeechSynthesizerPtr synth,
                               LangManagerPtr lang) {
  SpeechQueuePtr q;
  if (!synth || !lang) return NULL;
  q = (SpeechQueuePtr)malloc(sizeof(struct SpeechQueue));
  if (!q) return NULL;
  q->synth = synth;
  q->lang = lang;
  q->head = NULL;
  q->tail = NULL;
  q->activePlayer = NULL;
  q->activeTone = NULL;
  q->dispatching = 0;
  q->silencePending = 0;
  q->silenceDeadline = 0.0;
  q->speakingText = 0;
  return q;
}

void SpeechQueueFree(SpeechQueuePtr q) {
  if (!q) return;
  SpeechQueueClear(q);
  free(q);
}

SpeechResult SpeechQueuePushText(SpeechQueuePtr q, const char* text) {
  QueueItem* item;
  char* copy;
  if (!q) return SPEECH_ERR_NULL_STATE;
  if (!text) return SPEECH_ERR_INVALID_ARG;
  copy = dupString(text);
  if (!copy) return SPEECH_ERR_ALLOC;
  item = (QueueItem*)malloc(sizeof(QueueItem));
  if (!item) {
    free(copy);
    return SPEECH_ERR_ALLOC;
  }
  item->type = QUEUE_ITEM_TEXT;
  item->data.text = copy;
  item->next = NULL;
  if (q->tail) {
    q->tail->next = item;
  } else {
    q->head = item;
  }
  q->tail = item;
  return SPEECH_OK;
}

SpeechResult SpeechQueuePushSilence(SpeechQueuePtr q, int duration_ms) {
  QueueItem* item;
  if (!q) return SPEECH_ERR_NULL_STATE;
  if (duration_ms < 0) return SPEECH_ERR_INVALID_ARG;
  item = (QueueItem*)malloc(sizeof(QueueItem));
  if (!item) return SPEECH_ERR_ALLOC;
  item->type = QUEUE_ITEM_SILENCE;
  item->data.silenceSeconds = duration_ms / 1000.0;
  item->next = NULL;
  if (q->tail) {
    q->tail->next = item;
  } else {
    q->head = item;
  }
  q->tail = item;
  return SPEECH_OK;
}

SpeechResult SpeechQueuePushAudio(SpeechQueuePtr q, const char* path) {
  QueueItem* item;
  char* copy;
  if (!q) return SPEECH_ERR_NULL_STATE;
  if (!path) return SPEECH_ERR_INVALID_ARG;
  copy = dupString(path);
  if (!copy) return SPEECH_ERR_ALLOC;
  item = (QueueItem*)malloc(sizeof(QueueItem));
  if (!item) {
    free(copy);
    return SPEECH_ERR_ALLOC;
  }
  item->type = QUEUE_ITEM_AUDIO;
  item->data.audioPath = copy;
  item->next = NULL;
  if (q->tail) {
    q->tail->next = item;
  } else {
    q->head = item;
  }
  q->tail = item;
  return SPEECH_OK;
}

SpeechResult SpeechQueuePushTone(SpeechQueuePtr q, double frequency,
                                 int duration_ms) {
  QueueItem* item;
  if (!q) return SPEECH_ERR_NULL_STATE;
  if (frequency <= 0.0 || duration_ms < 0) return SPEECH_ERR_INVALID_ARG;
  item = (QueueItem*)malloc(sizeof(QueueItem));
  if (!item) return SPEECH_ERR_ALLOC;
  item->type = QUEUE_ITEM_TONE;
  item->data.tone.frequency = frequency;
  item->data.tone.duration_ms = duration_ms;
  item->next = NULL;
  if (q->tail) {
    q->tail->next = item;
  } else {
    q->head = item;
  }
  q->tail = item;
  return SPEECH_OK;
}

static SpeechResult PushSelectVoice(SpeechQueuePtr q, int next, int sayIt) {
  QueueItem* item;
  if (!q) return SPEECH_ERR_NULL_STATE;
  item = (QueueItem*)malloc(sizeof(QueueItem));
  if (!item) return SPEECH_ERR_ALLOC;
  item->type = QUEUE_ITEM_SETUP_VOICE;
  if (next)
    item->data.voice.type = SET_VOICE_NEXT;
  else
    item->data.voice.type = SET_VOICE_PREVIOUS;
  item->data.voice.sayIt = sayIt;
  item->data.voice.spec = NULL;
  item->next = NULL;
  if (q->tail) {
    q->tail->next = item;
  } else {
    q->head = item;
  }
  q->tail = item;
  return SPEECH_OK;
}

SpeechResult SpeechQueuePushNextVoice(SpeechQueuePtr q, int sayIt) {
  return PushSelectVoice(q, 1, sayIt);
}

SpeechResult SpeechQueuePushPreviousVoice(SpeechQueuePtr q, int sayIt) {
  return PushSelectVoice(q, 0, sayIt);
}

SpeechResult SpeechQueuePushVoice(SpeechQueuePtr q, const char* voice,
                                  int sayIt) {
  QueueItem* item;
  char* copy;
  if (!q) return SPEECH_ERR_NULL_STATE;
  if (!voice) return SPEECH_ERR_INVALID_ARG;
  copy = dupString(voice);
  if (!copy) return SPEECH_ERR_ALLOC;
  item = (QueueItem*)malloc(sizeof(QueueItem));
  if (!item) {
    free(copy);
    return SPEECH_ERR_ALLOC;
  }
  item->type = QUEUE_ITEM_SETUP_VOICE;
  item->data.voice.type = SET_VOICE_BY_SPEC;
  item->data.voice.spec = copy;
  item->data.voice.sayIt = sayIt;
  item->next = NULL;
  if (q->tail) {
    q->tail->next = item;
  } else {
    q->head = item;
  }
  q->tail = item;
  return SPEECH_OK;
}

SpeechResult SpeechQueueDispatch(SpeechQueuePtr q) {
  if (!q) return SPEECH_ERR_NULL_STATE;
  if (q->dispatching) return SPEECH_OK;
  q->dispatching = 1;
  advance(q);
  return SPEECH_OK;
}

SpeechResult SpeechQueueClear(SpeechQueuePtr q) {
  QueueItem *item, *next;
  if (!q) return SPEECH_ERR_NULL_STATE;
  if (q->activePlayer) {
    OggPlayerStop(q->activePlayer);
    q->activePlayer = NULL;
  }
  if (q->activeTone) {
    TonePlayerStop(q->activeTone);
    q->activeTone = NULL;
  }
  q->silencePending = 0;
  q->speakingText = 0;
  item = q->head;
  while (item) {
    next = item->next;
    freeItem(item);
    item = next;
  }
  q->head = NULL;
  q->tail = NULL;
  q->dispatching = 0;
  return SPEECH_OK;
}

void SpeechQueueWait(SpeechQueuePtr q, double period_seconds) {
  if (!q) return;
  while (q->dispatching || q->head || q->silencePending || q->activePlayer ||
         q->activeTone) {
    SpeechRunLoopPump(period_seconds);
    SpeechQueueTick(q);
  }
}
