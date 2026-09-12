#include "main_loop.h"

#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "command.h"
#include "commands.h"
#include "speech.h"
#include "speech_context.h"
#include "speech_queue.h"

void runMainLoop(SpeechContext ctx, CommandInputState inputState) {
  static char cmdBuf[CMD_LINE_MAX];
  int running = 1;
  while (running) {
    ReadLineStatus st;
    struct timeval tv;
    double remaining;
    long remaining_usec;
    do {
      st = readCommandLine(&inputState, cmdBuf, sizeof(cmdBuf));
      if (st == READ_LINE_READY) {
        dispatch(&ctx, cmdBuf, g_server_commands);
      } else if (st == READ_LINE_TOO_LONG) {
        fprintf(stderr, "nsspeaker: command line too long, ignored\n");
      }
    } while (st == READ_LINE_READY || st == READ_LINE_TOO_LONG);

    if (st == READ_LINE_EOF_OR_ERROR) {
      running = 0;
      break;
    }
    remaining = SpeechQueueSecondsUntilNextEvent(ctx.queue);
    if (remaining >= 0.0)
      remaining_usec = (long)(remaining * 1000000.0);
    else
      remaining_usec = POLL_INTERVAL_USEC;
    if (remaining_usec < 0) remaining_usec = 0;
    if (remaining_usec > POLL_INTERVAL_USEC)
      remaining_usec = POLL_INTERVAL_USEC;
    tv.tv_sec = 0;
    tv.tv_usec = remaining_usec;
    {
      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(STDIN_FILENO, &readfds);
      select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
    }
    SpeechRunLoopPump(0.0);
    SpeechQueueTick(ctx.queue);
  }
}
