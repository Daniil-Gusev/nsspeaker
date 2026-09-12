#include "lang_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define LANG_ID_MAX 64
#define LANG_RAW_DATA_MAX 256
#define LANG_MAX_VOICES 256
#define LANG_MAX_LANGS 128
#define LANG_MAX_VOICES_PER_LANG 16
#define LANG_ALIAS_MAX 32

typedef struct {
  char voiceId[LANG_ID_MAX];
  char name[LANG_NAME_MAX];
  char lang[LANG_CODE_MAX];
} VoiceEntry;

typedef struct {
  char lang[LANG_CODE_MAX];
  int voiceIdx[LANG_MAX_VOICES_PER_LANG];
  int voiceCount;
  int preferredSlot;
} LangGroup;

typedef struct {
  char alias[LANG_CODE_MAX];
  char lang[LANG_CODE_MAX];
} LangAlias;

struct LangManager {
  SpeechSynthesizerPtr synth;
  VoiceEntry voices[LANG_MAX_VOICES];
  int voiceCount;
  LangGroup groups[LANG_MAX_LANGS];
  int groupCount;
  int currentGroup;
  LangAlias aliases[LANG_ALIAS_MAX];
  int aliasCount;
};

static int findLangGroup(LangManagerPtr m, const char* lang) {
  int i;
  for (i = 0; i < m->groupCount; i++) {
    if (strcasecmp(m->groups[i].lang, lang) == 0) return i;
  }
  return -1;
}

static int findLangGroupByPrefix(LangManagerPtr m, const char* lang) {
  int i;
  size_t len = strlen(lang);
  for (i = 0; i < m->groupCount; i++) {
    if (strcasecmp(m->groups[i].lang, lang) == 0) return i;
  }
  for (i = 0; i < m->groupCount; i++) {
    if (strncasecmp(m->groups[i].lang, lang, len) == 0) return i;
  }
  return -1;
}

static int findVoiceSlot(LangManagerPtr m, int groupIdx, const char* name) {
  int slot;
  for (slot = 0; slot < m->groups[groupIdx].voiceCount; ++slot) {
    if (strcasecmp(m->voices[m->groups[groupIdx].voiceIdx[slot]].name, name) ==
        0)
      return slot;
  }
  return -1;
}

static int findVoiceSlotBySubstr(LangManagerPtr m, int groupIdx,
                                 const char* name) {
  int slot;
  for (slot = 0; slot < m->groups[groupIdx].voiceCount; ++slot) {
    if (strcasestr(m->voices[m->groups[groupIdx].voiceIdx[slot]].name, name) !=
        NULL)
      return slot;
  }
  return -1;
}

static int parseVoiceLine(const char* lineStart, size_t linelen,
                          VoiceEntry* out) {
  char line[LANG_RAW_DATA_MAX];
  char* fields[4];
  int fieldCount = 1;
  size_t i;
  if (linelen >= sizeof(line)) linelen = sizeof(line) - 1;
  memcpy(line, lineStart, linelen);
  line[linelen] = '\0';
  fields[0] = line;
  for (i = 0; i < linelen && fieldCount < 4; i++) {
    if (line[i] == ';') {
      line[i] = '\0';
      fields[fieldCount++] = line + i + 1;
    }
  }
  if (fieldCount != 4) return 0;
  copyBounded(out->voiceId, sizeof(out->voiceId), fields[0]);
  copyBounded(out->name, sizeof(out->name), fields[1]);
  copyBounded(out->lang, sizeof(out->lang), fields[2]);
  return 1;
}

static int findOrCreateGroup(LangManagerPtr m, const char* lang) {
  int gi = findLangGroup(m, lang);
  if (gi >= 0) return gi;
  if (m->groupCount >= LANG_MAX_LANGS) return -1;
  copyBounded(m->groups[m->groupCount].lang,
              sizeof(m->groups[m->groupCount].lang), lang);
  m->groups[m->groupCount].voiceCount = 0;
  m->groups[m->groupCount].preferredSlot = 0;
  return m->groupCount++;
}

static void addVoiceToGroup(LangManagerPtr m, int voiceIndex) {
  int gi = findOrCreateGroup(m, m->voices[voiceIndex].lang);
  if (gi < 0) return;
  if (m->groups[gi].voiceCount < LANG_MAX_VOICES_PER_LANG)
    m->groups[gi].voiceIdx[m->groups[gi].voiceCount++] = voiceIndex;
}

static void populateFromVoiceList(LangManagerPtr m, const char* buf) {
  const char* p = buf;
  while (*p) {
    const char* nl = strchr(p, '\n');
    size_t linelen = nl ? (size_t)(nl - p) : strlen(p);
    VoiceEntry v;
    if (parseVoiceLine(p, linelen, &v) && m->voiceCount < LANG_MAX_VOICES) {
      m->voices[m->voiceCount] = v;
      addVoiceToGroup(m, m->voiceCount);
      m->voiceCount++;
    }
    if (!nl) break;
    p = nl + 1;
  }
}

static void announce(LangManagerPtr m, const char* text, int sayIt) {
  if (sayIt) SpeechSpeak(m->synth, text);
}

static const char* resolveAlias(LangManagerPtr m, const char* alias) {
  int i;
  for (i = 0; i < m->aliasCount; i++) {
    if (strcasecmp(m->aliases[i].alias, alias) == 0) return m->aliases[i].lang;
  }
  return alias;
}

static const char* reverseResolveAlias(LangManagerPtr m, const char* lang) {
  int i;
  for (i = 0; i < m->aliasCount; i++) {
    if (strcasecmp(m->aliases[i].lang, lang) == 0) return m->aliases[i].alias;
  }
  return lang;
}

static SpeechResult applyGroup(LangManagerPtr m, int groupIndex, int sayIt) {
  int voiceIdx;
  char msg[3 + LANG_NAME_MAX + LANG_CODE_MAX];
  if (groupIndex < 0 || groupIndex >= m->groupCount)
    return SPEECH_ERR_INVALID_ARG;
  if (m->groups[groupIndex].voiceCount == 0) return SPEECH_ERR_INVALID_ARG;
  voiceIdx =
      m->groups[groupIndex].voiceIdx[m->groups[groupIndex].preferredSlot];
  if (SpeechSetVoice(m->synth, m->voices[voiceIdx].voiceId) != SPEECH_OK)
    return SPEECH_ERR_SET_VOICE;
  m->currentGroup = groupIndex;
  snprintf(msg, sizeof(msg), "%s: %s",
           reverseResolveAlias(m, m->voices[voiceIdx].lang),
           m->voices[voiceIdx].name);
  announce(m, msg, sayIt);
  return SPEECH_OK;
}

static int selectVoice(LangManagerPtr m, const char* attrs) {
  int groupIdx, slot;
  VoiceEntry voice;
  if (!parseVoiceLine(attrs, strlen(attrs), &voice)) return 0;
  groupIdx = findLangGroup(m, voice.lang);
  if (groupIdx < 0) return 0;
  slot = findVoiceSlot(m, groupIdx, voice.name);
  if (slot < 0) return 0;
  m->groups[groupIdx].preferredSlot = slot;
  applyGroup(m, groupIdx, 0);
  return 1;
}

LangManagerPtr LangManagerInit(SpeechSynthesizerPtr synth) {
  LangManagerPtr m;
  size_t needed;
  char *buf, voiceId[LANG_ID_MAX], voiceAttrs[LANG_RAW_DATA_MAX];
  if (!synth) return NULL;
  m = (LangManagerPtr)malloc(sizeof(struct LangManager));
  if (!m) return NULL;
  m->synth = synth;
  m->voiceCount = 0;
  m->groupCount = 0;
  m->aliasCount = 0;
  m->currentGroup = -1;
  needed = SpeechGetAvailableVoices(NULL, 0);
  buf = (char*)malloc(needed);
  if (!buf) return m;
  SpeechGetAvailableVoices(buf, needed);
  populateFromVoiceList(m, buf);
  free(buf);
  SpeechGetDefaultVoice(voiceId, sizeof(voiceId));
  SpeechGetVoiceAttributes(voiceId, voiceAttrs, sizeof(voiceAttrs));
  selectVoice(m, voiceAttrs);
  return m;
}

void LangManagerFree(LangManagerPtr m) {
  if (!m) return;
  free(m);
}

SpeechResult LangManagerNext(LangManagerPtr m, int sayIt) {
  if (!m || m->groupCount == 0) return SPEECH_ERR_NULL_STATE;
  if (applyGroup(m, (m->currentGroup + 1) % m->groupCount, sayIt) != SPEECH_OK)
    return SPEECH_ERR_SET_VOICE;
  return SPEECH_OK;
}

SpeechResult LangManagerPrevious(LangManagerPtr m, int sayIt) {
  int prev;
  if (!m || m->groupCount == 0) return SPEECH_ERR_NULL_STATE;
  prev = m->currentGroup - 1;
  if (prev < 0) prev = m->groupCount - 1;
  if (applyGroup(m, prev, sayIt) != SPEECH_OK) return SPEECH_ERR_SET_VOICE;
  return SPEECH_OK;
}

SpeechResult LangManagerSetLang(LangManagerPtr m, const char* spec, int sayIt) {
  char langPart[LANG_CODE_MAX];
  char voicePart[LANG_NAME_MAX];
  const char *colon, *resolvedLang;
  int groupIdx = -1, slot;
  if (!m) return SPEECH_ERR_NULL_STATE;
  if (!spec) return SPEECH_ERR_INVALID_ARG;
  colon = strchr(spec, ':');
  langPart[0] = '\0';
  voicePart[0] = '\0';
  if (colon) {
    size_t langLen = (size_t)(colon - spec);
    if (langLen >= sizeof(langPart)) langLen = sizeof(langPart) - 1;
    memcpy(langPart, spec, langLen);
    langPart[langLen] = '\0';
    copyBounded(voicePart, sizeof(voicePart), colon + 1);
  } else {
    copyBounded(langPart, sizeof(langPart), spec);
  }
  if (langPart[0] != '\0') {
    resolvedLang = resolveAlias(m, langPart);
    groupIdx = findLangGroupByPrefix(m, resolvedLang);
  }
  if (voicePart[0] != '\0') {
    if (groupIdx >= 0) {
      slot = findVoiceSlotBySubstr(m, groupIdx, voicePart);
      if (slot >= 0) {
        m->groups[groupIdx].preferredSlot = slot;
        return applyGroup(m, groupIdx, sayIt);
      }
    } else {
      int i, ownerGroup;
      for (i = 0; i < m->voiceCount; i++) {
        if (strcasestr(m->voices[i].name, voicePart) == NULL) continue;
        ownerGroup = findLangGroup(m, m->voices[i].lang);
        if (ownerGroup >= 0) {
          int ownerSlot = findVoiceSlot(m, ownerGroup, m->voices[i].name);
          int savedSlot = m->groups[ownerGroup].preferredSlot;
          if (ownerSlot >= 0) m->groups[ownerGroup].preferredSlot = ownerSlot;
          if (applyGroup(m, ownerGroup, sayIt) != SPEECH_OK) {
            m->groups[ownerGroup].preferredSlot = savedSlot;
            return SPEECH_ERR_SET_VOICE;
          }
        } else {
          if (SpeechSetVoice(m->synth, m->voices[i].voiceId) != SPEECH_OK)
            return SPEECH_ERR_SET_VOICE;
          m->currentGroup = -1;
          announce(m, m->voices[i].name, sayIt);
          return SPEECH_OK;
        }
      }
    }
  }
  if (groupIdx >= 0) return applyGroup(m, groupIdx, sayIt);
  return SPEECH_ERR_INVALID_ARG;
}

void LangManagerSetPreferredLang(LangManagerPtr m, const char* alias,
                                 const char* lang) {
  int i;
  if (!m || !alias || !lang) return;
  for (i = 0; i < m->aliasCount; i++) {
    if (strcasecmp(m->aliases[i].alias, alias) == 0) {
      copyBounded(m->aliases[i].lang, sizeof(m->aliases[i].lang), lang);
      return;
    }
  }
  if (m->aliasCount >= LANG_ALIAS_MAX) return;
  copyBounded(m->aliases[m->aliasCount].alias,
              sizeof(m->aliases[m->aliasCount].alias), alias);
  copyBounded(m->aliases[m->aliasCount].lang,
              sizeof(m->aliases[m->aliasCount].lang), lang);
  m->aliasCount++;
}
