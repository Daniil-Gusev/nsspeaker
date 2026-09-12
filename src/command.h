#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

#include "speech_context.h"

#define CMD_READ_BUF_MAX (512 * 1024)
#define CMD_LINE_MAX (128 * 1024)
#define CMD_PROCESSED_MAX (CMD_LINE_MAX * 2)

typedef struct {
  char* buf;
  size_t cap, len;
  int skipping;
} CommandInputState;

typedef enum {
  READ_LINE_READY = 1,
  READ_LINE_PENDING = 0,
  READ_LINE_EOF_OR_ERROR = -1,
  READ_LINE_TOO_LONG = -2
} ReadLineStatus;

typedef void (*CommandFn)(SpeechContext* ctx, const char* arg);

typedef struct CommandEntry {
  const char* name;
  CommandFn fn;
} CommandEntry;

void commandInputInit(void);
void initCommandInputState(CommandInputState* st, char* buf, size_t buf_size);
ReadLineStatus readCommandLine(CommandInputState* st, char* out,
                               size_t out_size);
void dispatch(SpeechContext* ctx, char* str, const CommandEntry* table);

#endif
