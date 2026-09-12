#ifndef LANG_MANAGER_H
#define LANG_MANAGER_H

#include "speech.h"

#define LANG_NAME_MAX 64
#define LANG_CODE_MAX 16

typedef struct LangManager* LangManagerPtr;

LangManagerPtr LangManagerInit(SpeechSynthesizerPtr synth);
void LangManagerFree(LangManagerPtr m);

SpeechResult LangManagerNext(LangManagerPtr m, int sayIt);
SpeechResult LangManagerPrevious(LangManagerPtr m, int sayIt);
SpeechResult LangManagerSetLang(LangManagerPtr m, const char* spec, int sayIt);
void LangManagerSetPreferredLang(LangManagerPtr m, const char* alias,
                                 const char* lang);

#endif
