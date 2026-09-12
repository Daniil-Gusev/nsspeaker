#include <stdio.h>

#include "command.h"
#include "main_loop.h"
#include "speech.h"
#include "speech_context.h"
#include "speech_queue.h"
#include "version.h"

int main(void) {
  static char rawInputBuf[CMD_READ_BUF_MAX];
  CommandInputState inputState;
  SpeechContext ctx;
  initCommandInputState(&inputState, rawInputBuf, sizeof(rawInputBuf));
  ctx.synth = SpeechInit();
  if (!ctx.synth) return 1;
  ctx.lang = LangManagerInit(ctx.synth);
  ctx.queue = SpeechQueueInit(ctx.synth, ctx.lang);
  if (!ctx.queue) {
    SpeechFree(ctx.synth);
    LangManagerFree(ctx.lang);
    return 1;
  }
  commandInputInit();
  printf("%s\n", app_version_info);
  runMainLoop(ctx, inputState);
  SpeechQueueWait(ctx.queue, POLL_INTERVAL_SEC);
  SpeechWait(ctx.synth, POLL_INTERVAL_SEC);
  LangManagerFree(ctx.lang);
  SpeechSetFinishedCallback(ctx.synth, NULL, NULL);
  SpeechQueueFree(ctx.queue);
  SpeechFree(ctx.synth);
  return 0;
}
