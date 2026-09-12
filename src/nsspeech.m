#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

#include <stdlib.h>
#include <string.h>
#include "nsspeech.h"
#include "nsspeech_delegate.h"

struct NSSpeechState {
  NSSpeechSynthesizer *synth;
  NSSpeechDelegateBridge *delegateBridge;
};

static NSSpeechBoundary ToNSBoundary(int wordBoundary) {
  return wordBoundary ? NSSpeechWordBoundary : NSSpeechImmediateBoundary;
}

struct NSSpeechState *NSSpeechInit(void) {
  struct NSSpeechState *s;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  s = (struct NSSpeechState *)malloc(sizeof(struct NSSpeechState));
  if (!s) {
    [pool release];
    return NULL;
  }
  s->synth = [[NSSpeechSynthesizer alloc] init];
  if (!s->synth) {
    free(s);
    [pool release];
    return NULL;
  }
  s->delegateBridge = [[NSSpeechDelegateBridge alloc] init];
  [s->synth setDelegate:s->delegateBridge];
  [pool release];
  return s;
}

struct NSSpeechState *NSSpeechInitWithVoice(const char *voice) {
  struct NSSpeechState *s;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *voiceName = [NSString stringWithUTF8String:voice];
  s = (struct NSSpeechState *)malloc(sizeof(struct NSSpeechState));
  if (!s) {
    [pool release];
    return NULL;
  }
  s->synth = [[NSSpeechSynthesizer alloc] initWithVoice:voiceName];
  if (!s->synth) {
    free(s);
    [pool release];
    return NULL;
  }
  s->delegateBridge = [[NSSpeechDelegateBridge alloc] init];
  [s->synth setDelegate:s->delegateBridge];
  [pool release];
  return s;
}

void NSSpeechFree(struct NSSpeechState *s) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  [s->synth stopSpeaking];
  [s->synth setDelegate:nil];
  [s->delegateBridge release];
  [s->synth release];
  [pool release];
  free(s);
}

void NSSpeechRunLoopPump(double seconds) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                           beforeDate:[NSDate dateWithTimeIntervalSinceNow:seconds]];
  [pool release];
}

int NSSpeechSpeak(struct NSSpeechState *s, const char *str) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *nsstr = [NSString stringWithUTF8String:str];
  BOOL ok = [s->synth startSpeakingString:nsstr];
  [pool release];
  return ok ? 1 : 0;
}

int NSSpeechSpeakToFile(struct NSSpeechState *s, const char *str, const char *path) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *nsstr = [NSString stringWithUTF8String:str];
  NSString *nspath = [NSString stringWithUTF8String:path];
  NSURL *url = [NSURL fileURLWithPath:nspath];
  BOOL ok = [s->synth startSpeakingString:nsstr toURL:url];
  [pool release];
  return ok ? 1 : 0;
}

void NSSpeechStop(struct NSSpeechState *s) { [s->synth stopSpeaking]; }

void NSSpeechPauseAt(struct NSSpeechState *s, int wordBoundary) {
  [s->synth pauseSpeakingAtBoundary:ToNSBoundary(wordBoundary)];
}

void NSSpeechContinueSpeaking(struct NSSpeechState *s) { [s->synth continueSpeaking]; }

void NSSpeechStopAt(struct NSSpeechState *s, int wordBoundary) {
  [s->synth stopSpeakingAtBoundary:ToNSBoundary(wordBoundary)];
}

int NSSpeechIsSpeaking(struct NSSpeechState *s) { return [s->synth isSpeaking] ? 1 : 0; }

void NSSpeechSetRate(struct NSSpeechState *s, float rate) { [s->synth setRate:rate]; }

float NSSpeechGetRate(struct NSSpeechState *s) { return [s->synth rate]; }

void NSSpeechSetVolume(struct NSSpeechState *s, float volume) { [s->synth setVolume:volume]; }

float NSSpeechGetVolume(struct NSSpeechState *s) { return [s->synth volume]; }

int NSSpeechSetVoice(struct NSSpeechState *s, const char *voice) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *voiceName = [NSString stringWithUTF8String:voice];
  BOOL ok = [s->synth setVoice:voiceName];
  [pool release];
  return ok ? 1 : 0;
}

size_t NSSpeechGetVoiceAttributes(const char *voice, char *buf, size_t buf_size) {
  int fmt_len;
  const char *name, *lang, *gender;
  NSString *voiceKey, *nameObj, *langObj, *genderObj;
  NSDictionary *attrs;
  NSAutoreleasePool *pool;
  if (!voice || *voice == '\0') {
    if (buf && buf_size > 0) buf[0] = '\0';
    return 1;
  }
  pool = [[NSAutoreleasePool alloc] init];
  voiceKey = [NSString stringWithUTF8String:voice];
  attrs = [NSSpeechSynthesizer attributesForVoice:voiceKey];
  if (!attrs) {
    if (buf && buf_size > 0) buf[0] = '\0';
    [pool release];
    return 1;
  }
  nameObj = [attrs objectForKey:NSVoiceName];
  langObj = [attrs objectForKey:NSVoiceLocaleIdentifier];
  genderObj = [attrs objectForKey:NSVoiceGender];
  name = nameObj ? [nameObj UTF8String] : "Unknown";
  lang = langObj ? [langObj UTF8String] : "Unknown";
  gender = genderObj ? [genderObj UTF8String] : "Unknown";
  if (buf && buf_size > 0) {
    fmt_len = snprintf(buf, buf_size, "%s;%s;%s;%s", voice, name, lang, gender);
  } else {
    fmt_len = snprintf(NULL, 0, "%s;%s;%s;%s", voice, name, lang, gender);
  }
  [pool release];
  if (fmt_len < 0) {
    if (buf && buf_size > 0) buf[0] = '\0';
    return 1;
  }
  return (size_t)fmt_len + 1;
}

size_t NSSpeechGetAvailableVoices(char *buf, size_t buf_size) {
  NSArray *voices;
  NSAutoreleasePool *pool;
  NSUInteger i, count;
  char attr_buf[256], *dyn_attr;
  const char *voiceId;
  size_t attr_len, total_len = 0;
  int written;

  pool = [[NSAutoreleasePool alloc] init];
  voices = [NSSpeechSynthesizer availableVoices];
  count = [voices count];
  for (i = 0; i < count; i++) {
    voiceId = [[voices objectAtIndex:i] UTF8String];
    if (!voiceId) continue;
    attr_len = NSSpeechGetVoiceAttributes(voiceId, attr_buf, sizeof(attr_buf));
    if (attr_len > sizeof(attr_buf)) {
      dyn_attr = (char *)malloc(attr_len);
      if (!dyn_attr) continue;
      NSSpeechGetVoiceAttributes(voiceId, dyn_attr, attr_len);
      if (buf && total_len < buf_size)
        written = snprintf(buf + total_len, buf_size - total_len, "%s%s", dyn_attr,
                           (i < count - 1) ? "\n" : "");
      else
        written = snprintf(NULL, 0, "%s%s", dyn_attr, (i < count - 1) ? "\n" : "");
      free(dyn_attr);
    } else {
      if (buf && total_len < buf_size)
        written = snprintf(buf + total_len, buf_size - total_len, "%s%s", attr_buf,
                           (i < count - 1) ? "\n" : "");
      else
        written = snprintf(NULL, 0, "%s%s", attr_buf, (i < count - 1) ? "\n" : "");
    }
    if (written > 0) total_len += (size_t)written;
  }
  if (buf && buf_size > 0 && total_len == 0) buf[0] = '\0';
  [pool release];
  return total_len + 1;
}

size_t NSSpeechGetDefaultVoice(char *buf, size_t buf_size) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *voice = [NSSpeechSynthesizer defaultVoice];
  const char *cstr = voice ? [voice UTF8String] : "";
  int fmt_len;
  if (buf && buf_size > 0)
    fmt_len = snprintf(buf, buf_size, "%s", cstr);
  else
    fmt_len = snprintf(NULL, 0, "%s", cstr);
  [pool release];
  if (fmt_len < 0) {
    if (buf && buf_size > 0) buf[0] = '\0';
    return 1;
  }
  return (size_t)fmt_len + 1;
}

int NSSpeechIsAnyApplicationSpeaking(void) {
  return [NSSpeechSynthesizer isAnyApplicationSpeaking] ? 1 : 0;
}

size_t NSSpeechGetTextProperty(struct NSSpeechState *s, const char *property, char *buf,
                               size_t buf_size) {
  NSAutoreleasePool *pool;
  NSString *propKey, *strValue;
  id value;
  NSError *err = nil;
  const char *cstr;
  int fmt_len;
  pool = [[NSAutoreleasePool alloc] init];
  propKey = [NSString stringWithUTF8String:property];
  value = [s->synth objectForProperty:propKey error:&err];
  strValue = [value isKindOfClass:[NSString class]] ? (NSString *)value : nil;
  cstr = strValue ? [strValue UTF8String] : "";
  if (buf && buf_size > 0)
    fmt_len = snprintf(buf, buf_size, "%s", cstr);
  else
    fmt_len = snprintf(NULL, 0, "%s", cstr);
  [pool release];
  if (fmt_len < 0) {
    if (buf && buf_size > 0) buf[0] = '\0';
    return 1;
  }
  return (size_t)fmt_len + 1;
}

int NSSpeechSetTextProperty(struct NSSpeechState *s, const char *property, const char *value) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *propKey = [NSString stringWithUTF8String:property];
  NSString *valStr = [NSString stringWithUTF8String:value];
  NSError *err = nil;
  BOOL ok = [s->synth setObject:valStr forProperty:propKey error:&err];
  [pool release];
  return ok ? 1 : 0;
}

int NSSpeechGetNumberProperty(struct NSSpeechState *s, const char *property, double *value) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *propKey = [NSString stringWithUTF8String:property];
  NSError *err = nil;
  id obj = [s->synth objectForProperty:propKey error:&err];
  if ([obj isKindOfClass:[NSNumber class]]) {
    *value = [(NSNumber *)obj doubleValue];
    [pool release];
    return 1;
  }
  [pool release];
  return 0;
}

int NSSpeechSetNumberProperty(struct NSSpeechState *s, const char *property, double value) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *propKey = [NSString stringWithUTF8String:property];
  NSNumber *num = [NSNumber numberWithDouble:value];
  NSError *err = nil;
  BOOL ok = [s->synth setObject:num forProperty:propKey error:&err];
  [pool release];
  return ok ? 1 : 0;
}

size_t NSSpeechGetPhonemes(struct NSSpeechState *s, const char *text, char *buf, size_t buf_size) {
  NSAutoreleasePool *pool;
  NSString *nsstr, *phonemes;
  const char *cstr;
  int fmt_len;
  pool = [[NSAutoreleasePool alloc] init];
  nsstr = [NSString stringWithUTF8String:text];
  phonemes = [s->synth phonemesFromText:nsstr];
  cstr = phonemes ? [phonemes UTF8String] : "";
  if (buf && buf_size > 0)
    fmt_len = snprintf(buf, buf_size, "%s", cstr);
  else
    fmt_len = snprintf(NULL, 0, "%s", cstr);
  [pool release];
  if (fmt_len < 0) {
    if (buf && buf_size > 0) buf[0] = '\0';
    return 1;
  }
  return (size_t)fmt_len + 1;
}

void NSSpeechSetFinishedCallback(struct NSSpeechState *s, NSSpeechFinishedCallback cb,
                                 void *userdata) {
  [s->delegateBridge setOnFinish:cb];
  [s->delegateBridge setFinishUserdata:userdata];
}

void NSSpeechSetWillSpeakWordCallback(struct NSSpeechState *s, NSSpeechWillSpeakWordCallback cb,
                                      void *userdata) {
  [s->delegateBridge setOnWillSpeakWord:cb];
  [s->delegateBridge setWordUserdata:userdata];
}

void NSSpeechSetWillSpeakPhonemeCallback(struct NSSpeechState *s,
                                         NSSpeechWillSpeakPhonemeCallback cb, void *userdata) {
  [s->delegateBridge setOnWillSpeakPhoneme:cb];
  [s->delegateBridge setPhonemeUserdata:userdata];
}

void NSSpeechSetErrorCallback(struct NSSpeechState *s, NSSpeechErrorCallback cb, void *userdata) {
  [s->delegateBridge setOnError:cb];
  [s->delegateBridge setErrorUserdata:userdata];
}
