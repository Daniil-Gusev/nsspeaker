#include "nsspeech_delegate.h"

@implementation NSSpeechDelegateBridge

- (void)setOnFinish:(NSSpeechFinishedCallback)cb {
  onFinish = cb;
}
- (void)setFinishUserdata:(void *)userdata {
  finishUserdata = userdata;
}

- (void)setOnWillSpeakWord:(NSSpeechWillSpeakWordCallback)cb {
  onWillSpeakWord = cb;
}
- (void)setWordUserdata:(void *)userdata {
  wordUserdata = userdata;
}

- (void)setOnWillSpeakPhoneme:(NSSpeechWillSpeakPhonemeCallback)cb {
  onWillSpeakPhoneme = cb;
}
- (void)setPhonemeUserdata:(void *)userdata {
  phonemeUserdata = userdata;
}

- (void)setOnError:(NSSpeechErrorCallback)cb {
  onError = cb;
}
- (void)setErrorUserdata:(void *)userdata {
  errorUserdata = userdata;
}

- (void)speechSynthesizer:(NSSpeechSynthesizer *)sender didFinishSpeaking:(BOOL)finishedSpeaking {
  (void)sender;
  if (onFinish) {
    onFinish(finishUserdata, finishedSpeaking ? 1 : 0);
  }
}

- (void)speechSynthesizer:(NSSpeechSynthesizer *)sender
            willSpeakWord:(NSRange)characterRange
                 ofString:(NSString *)string {
  (void)sender;
  if (onWillSpeakWord) {
    const char *full = [string UTF8String];
    onWillSpeakWord(wordUserdata, characterRange.location, characterRange.length, full);
  }
}

- (void)speechSynthesizer:(NSSpeechSynthesizer *)sender willSpeakPhoneme:(short)phonemeOpcode {
  (void)sender;
  if (onWillSpeakPhoneme) {
    onWillSpeakPhoneme(phonemeUserdata, phonemeOpcode);
  }
}

- (void)speechSynthesizer:(NSSpeechSynthesizer *)sender
    didEncounterErrorAtIndex:(NSUInteger)characterIndex
                    ofString:(NSString *)string
                     message:(NSString *)message {
  (void)sender;
  if (onError) {
    const char *full = [string UTF8String];
    const char *msg = [message UTF8String];
    onError(errorUserdata, characterIndex, full, msg);
  }
}

@end
