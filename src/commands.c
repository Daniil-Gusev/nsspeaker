#include "commands.h"

#include <stdio.h>
#include <string.h>

#include "command.h"
#include "lang_manager.h"
#include "speech.h"
#include "speech_context.h"
#include "speech_punct.h"
#include "speech_queue.h"
#include "utils.h"
#include "version.h"

static void cmdVersion(SpeechContext* ctx, const char* arg) {
  (void)arg;
  if (!ctx) return;
  SpeechSpeak(ctx->synth, app_version_info);
}

static void cmdSpeak(SpeechContext* ctx, const char* arg) {
  static char processed[CMD_PROCESSED_MAX];
  if (!ctx) return;
  mapMorphemeBoundaries(arg, processed, sizeof(processed));
  SpeechStop(ctx->synth);
  SpeechQueueClear(ctx->queue);
  SpeechSpeak(ctx->synth, processed);
}

static void cmdStop(SpeechContext* ctx, const char* arg) {
  (void)arg;
  SpeechStop(ctx->synth);
  SpeechQueueClear(ctx->queue);
}

static void cmdRate(SpeechContext* ctx, const char* arg) {
  float rate;
  if (sscanf(arg, "%f", &rate) == 1) SpeechSetRate(ctx->synth, rate);
}

static void cmdReset(SpeechContext* ctx, const char* arg) {
  (void)arg;
  SpeechStop(ctx->synth);
  SpeechQueueClear(ctx->queue);
  SpeechResetToDefaults(ctx->synth);
}

static void cmdCharacterScale(SpeechContext* ctx, const char* arg) {
  float scale;
  if (sscanf(arg, "%f", &scale) == 1)
    SpeechSetCharacterScale(ctx->synth, scale);
}

static void cmdSplitCaps(SpeechContext* ctx, const char* arg) {
  int flag;
  if (sscanf(arg, "%d", &flag) == 1) SpeechSetSplitCaps(ctx->synth, flag);
}

static void cmdLetter(SpeechContext* ctx, const char* arg) {
  char letterBuf[5];
  size_t charLen, avail;
  float baseRate, scale;
  if (arg[0] == '\0') return;
  charLen = utf8CharLen(arg);
  avail = strlen(arg);
  if (charLen > avail) charLen = avail;
  if (charLen == 0) return;
  memcpy(letterBuf, arg, charLen);
  letterBuf[charLen] = '\0';
  SpeechStop(ctx->synth);
  SpeechQueueClear(ctx->queue);
  scale = SpeechGetCharacterScale(ctx->synth);
  if (SpeechGetRate(ctx->synth, &baseRate) == SPEECH_OK && scale != 1.0f) {
    SpeechSetRate(ctx->synth, baseRate * scale);
    SpeechSpeak(ctx->synth, letterBuf);
    SpeechSetRate(ctx->synth, baseRate);
  } else {
    SpeechSpeak(ctx->synth, letterBuf);
  }
}

static void cmdSyncState(SpeechContext* ctx, const char* arg) {
  char punct[16];
  const char *p, *lastSpace;
  size_t len;
  int splitCaps;
  SpeechPunctMode mode;
  float rate = 0.0f;
  if (!ctx) return;
  len = strcspn(arg, " ");
  if (len == 0) return;
  if (len >= sizeof(punct)) len = sizeof(punct) - 1;
  memcpy(punct, arg, len);
  punct[len] = '\0';
  p = skipToken(arg, " ");
  if (!p || sscanf(p, "%d", &splitCaps) != 1) return;
  lastSpace = strrchr(arg, ' ');
  if (!lastSpace || sscanf(lastSpace + 1, "%f", &rate) != 1) return;
  if (strcasecmp(punct, "none") == 0)
    mode = PUNCT_NONE;
  else if (strcasecmp(punct, "some") == 0)
    mode = PUNCT_SOME;
  else if (strcasecmp(punct, "all") == 0)
    mode = PUNCT_ALL;
  else
    return;
  SpeechSetPunctuation(ctx->synth, mode);
  SpeechSetSplitCaps(ctx->synth, splitCaps);
  if (rate > 0.0f) SpeechSetRate(ctx->synth, rate);
}

static void cmdPlayAudio(SpeechContext* ctx, const char* arg) {
  SpeechStop(ctx->synth);
  SpeechQueueClear(ctx->queue);
  if (SpeechQueuePushAudio(ctx->queue, arg) == SPEECH_OK) {
    SpeechQueueDispatch(ctx->queue);
  }
}

static void cmdQueueText(SpeechContext* ctx, const char* arg) {
  static char stage1[CMD_PROCESSED_MAX];
  static char processed[CMD_PROCESSED_MAX];
  const char* toQueue = arg;
  SpeechResult res;
  if (SpeechGetSplitCaps(ctx->synth)) {
    splitCamelCase(arg, stage1, sizeof(stage1));
    toQueue = stage1;
  }
  applyPunctuationMode(toQueue, processed, sizeof(processed),
                       SpeechGetPunctuation(ctx->synth));
  res = SpeechQueuePushText(ctx->queue, processed);
  if (res != SPEECH_OK) {
    fprintf(stderr, "q: failed to queue text (code %d)\n", (int)res);
  }
}

static void cmdQueueCode(SpeechContext* ctx, const char* arg) {
  SpeechResult res = SpeechQueuePushText(ctx->queue, arg);
  if (res != SPEECH_OK) {
    fprintf(stderr, "c: failed to queue codes (code %d)\n", (int)res);
  }
}

static void cmdQueueSilence(SpeechContext* ctx, const char* arg) {
  int ms;
  if (sscanf(arg, "%d", &ms) == 1) {
    SpeechQueuePushSilence(ctx->queue, ms);
  }
}

static void cmdQueueAudio(SpeechContext* ctx, const char* arg) {
  SpeechResult res = SpeechQueuePushAudio(ctx->queue, arg);
  if (res != SPEECH_OK) {
    fprintf(stderr, "a: failed to queue audio (code %d)\n", (int)res);
  }
}

static void cmdQueueTone(SpeechContext* ctx, const char* arg) {
  double freq;
  int duration;
  if (sscanf(arg, "%lf %d", &freq, &duration) == 2) {
    SpeechResult res = SpeechQueuePushTone(ctx->queue, freq, duration);
    if (res != SPEECH_OK) {
      fprintf(stderr, "t: failed to queue tone (code %d)\n", (int)res);
    }
  }
}

static void cmdDispatch(SpeechContext* ctx, const char* arg) {
  (void)arg;
  SpeechQueueDispatch(ctx->queue);
}

static void cmdSetNextLang(SpeechContext* ctx, const char* arg) {
  int sayIt = 0;
  sscanf(arg, "%d", &sayIt);
  if (LangManagerNext(ctx->lang, sayIt) == SPEECH_ERR_SET_VOICE) {
    SpeechQueuePushNextVoice(ctx->queue, sayIt);
    SpeechQueueDispatch(ctx->queue);
  }
}

static void cmdSetPreviousLang(SpeechContext* ctx, const char* arg) {
  int sayIt = 0;
  sscanf(arg, "%d", &sayIt);
  if (LangManagerPrevious(ctx->lang, sayIt) == SPEECH_ERR_SET_VOICE) {
    SpeechQueuePushPreviousVoice(ctx->queue, sayIt);
    SpeechQueueDispatch(ctx->queue);
  }
}

static void cmdSetLang(SpeechContext* ctx, const char* arg) {
  char spec[256];
  const char* next;
  size_t len;
  int sayIt = 0;
  len = strcspn(arg, " ");
  if (len >= sizeof(spec)) len = sizeof(spec) - 1;
  memcpy(spec, arg, len);
  spec[len] = '\0';
  next = skipToken(arg, " ");
  if (next) sscanf(next, "%d", &sayIt);
  if (spec[0] == '\0') return;
  if (LangManagerSetLang(ctx->lang, spec, sayIt) == SPEECH_ERR_SET_VOICE) {
    SpeechQueuePushVoice(ctx->queue, spec, sayIt);
    SpeechQueueDispatch(ctx->queue);
  }
}

static void cmdSetPreferredLang(SpeechContext* ctx, const char* arg) {
  char alias[LANG_CODE_MAX], lang[LANG_CODE_MAX];
  const char* p;
  size_t aliasLen, langLen, copyAliasLen, copyLangLen;
  aliasLen = strcspn(arg, " ");
  if (aliasLen == 0) return;
  copyAliasLen = aliasLen < sizeof(alias) ? aliasLen : sizeof(alias) - 1;
  memcpy(alias, arg, copyAliasLen);
  alias[copyAliasLen] = '\0';
  p = skipToken(arg, " ");
  if (!p) return;
  langLen = strlen(p);
  if (langLen == 0) return;
  copyLangLen = langLen < sizeof(lang) ? langLen : sizeof(lang) - 1;
  memcpy(lang, p, copyLangLen);
  lang[copyLangLen] = '\0';
  LangManagerSetPreferredLang(ctx->lang, alias, lang);
}

static void cmdSetPunctuations(SpeechContext* ctx, const char* arg) {
  int mode;
  if (!ctx) return;
  if (strcasecmp(arg, "none") == 0)
    SpeechSetPunctuation(ctx->synth, PUNCT_NONE);
  else if (strcasecmp(arg, "some") == 0)
    SpeechSetPunctuation(ctx->synth, PUNCT_SOME);
  else if (strcasecmp(arg, "all") == 0)
    SpeechSetPunctuation(ctx->synth, PUNCT_ALL);
  else if (sscanf(arg, "%d", &mode) == 1 && mode >= PUNCT_NONE &&
           mode <= PUNCT_ALL) {
    SpeechSetPunctuation(ctx->synth, mode);
  }
}

const struct CommandEntry g_server_commands[] = {
    {"q", cmdQueueText},
    {"l", cmdLetter},
    {"s", cmdStop},
    {"d", cmdDispatch},
    {"tts_sync_state", cmdSyncState},
    {"a", cmdQueueAudio},
    {"t", cmdQueueTone},
    {"p", cmdPlayAudio},
    {"sh", cmdQueueSilence},
    {"c", cmdQueueCode},
    {"tts_say", cmdSpeak},
    {"tts_set_speech_rate", cmdRate},
    {"tts_set_punctuations", cmdSetPunctuations},
    {"tts_set_character_scale", cmdCharacterScale},
    {"tts_reset", cmdReset},
    {"tts_split_caps", cmdSplitCaps},
    {"set_next_lang", cmdSetNextLang},
    {"set_previous_lang", cmdSetPreviousLang},
    {"set_lang", cmdSetLang},
    {"set_preferred_lang", cmdSetPreferredLang},
    {"version", cmdVersion},
    {NULL, NULL},
};
