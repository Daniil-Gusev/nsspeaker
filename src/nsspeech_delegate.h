#ifndef NSSPEECH_DELEGATE_H
#define NSSPEECH_DELEGATE_H

#include <AppKit/AppKit.h>
#include "nsspeech.h"

#if defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
#define NSSPEECH_DELEGATE_PROTOCOL <NSSpeechSynthesizerDelegate>
#else
#define NSSPEECH_DELEGATE_PROTOCOL
#endif

@interface NSSpeechDelegateBridge : NSObject
NSSPEECH_DELEGATE_PROTOCOL {
  NSSpeechFinishedCallback onFinish;
  void *finishUserdata;

  NSSpeechWillSpeakWordCallback onWillSpeakWord;
  void *wordUserdata;

  NSSpeechWillSpeakPhonemeCallback onWillSpeakPhoneme;
  void *phonemeUserdata;

  NSSpeechErrorCallback onError;
  void *errorUserdata;
}

- (void)setOnFinish:(NSSpeechFinishedCallback)cb;
- (void)setFinishUserdata:(void *)userdata;

- (void)setOnWillSpeakWord:(NSSpeechWillSpeakWordCallback)cb;
- (void)setWordUserdata:(void *)userdata;

- (void)setOnWillSpeakPhoneme:(NSSpeechWillSpeakPhonemeCallback)cb;
- (void)setPhonemeUserdata:(void *)userdata;

- (void)setOnError:(NSSpeechErrorCallback)cb;
- (void)setErrorUserdata:(void *)userdata;

@end

#endif
