#ifndef MAIN_LOOP_H
#define MAIN_LOOP_H

#include "command.h"
#include "speech_context.h"

#define POLL_INTERVAL_USEC 25000
#define POLL_INTERVAL_SEC (POLL_INTERVAL_USEC / 1000000.0)

void runMainLoop(SpeechContext ctx, CommandInputState inputState);

#endif
