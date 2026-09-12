#include "command.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "speech_context.h"

void commandInputInit(void) {
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  if (flags != -1) {
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
  }
}

void initCommandInputState(CommandInputState* st, char* buf, size_t buf_size) {
  st->buf = buf;
  st->cap = buf_size;
  st->len = 0;
  st->skipping = 0;
}

typedef enum { FILL_OK, FILL_PENDING, FILL_EOF_ERR } FillStatus;

static FillStatus fillBuffer(CommandInputState* st, size_t offset,
                             size_t maxlen) {
  ssize_t n = read(STDIN_FILENO, st->buf + offset, maxlen);
  if (n == 0) return FILL_EOF_ERR;
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) return FILL_PENDING;
    return FILL_EOF_ERR;
  }
  st->len = offset + (size_t)n;
  return FILL_OK;
}

ReadLineStatus readCommandLine(CommandInputState* st, char* out,
                               size_t out_size) {
  char* nl;
  size_t linelen, copylen;
  FillStatus fs;
  nl = (char*)memchr(st->buf, '\n', st->len);
  if (!nl) {
    if (st->skipping) {
      fs = fillBuffer(st, 0, st->cap);
    } else if (st->len >= st->cap - 1) {
      st->skipping = 1;
      fs = fillBuffer(st, 0, st->cap);
    } else {
      fs = fillBuffer(st, st->len, st->cap - st->len - 1);
    }
    if (fs == FILL_EOF_ERR) return READ_LINE_EOF_OR_ERROR;
    if (fs == FILL_PENDING) return READ_LINE_PENDING;
    nl = (char*)memchr(st->buf, '\n', st->len);
    if (!nl) return READ_LINE_PENDING;
  }
  linelen = (size_t)(nl - st->buf);
  if (st->skipping) {
    memmove(st->buf, nl + 1, st->len - linelen - 1);
    st->len -= (linelen + 1);
    st->skipping = 0;
    return READ_LINE_TOO_LONG;
  }
  if (linelen >= out_size) {
    memmove(st->buf, nl + 1, st->len - linelen - 1);
    st->len -= (linelen + 1);
    return READ_LINE_TOO_LONG;
  }
  copylen = linelen;
  memcpy(out, st->buf, copylen);
  out[copylen] = '\0';
  memmove(st->buf, nl + 1, st->len - linelen - 1);
  st->len -= (linelen + 1);
  return READ_LINE_READY;
}

struct Token {
  char* cmd;
  char* arg;
};

static struct Token parseCommand(char* str) {
  struct Token t;
  char* sp = strchr(str, ' ');
  size_t arglen;
  t.cmd = str;
  if (sp == NULL) {
    t.arg = str + strlen(str);
    return t;
  }
  *sp = '\0';
  t.arg = sp + 1;
  if (t.arg[0] == '{') {
    arglen = strlen(t.arg);
    if (arglen > 0 && t.arg[arglen - 1] == '}') {
      t.arg[arglen - 1] = '\0';
    }
    t.arg = t.arg + 1;
  }
  return t;
}

void dispatch(SpeechContext* ctx, char* str, const CommandEntry* table) {
  struct Token t;
  const CommandEntry* cmd;
  if (!table) return;
  t = parseCommand(str);
  for (cmd = table; cmd->name != NULL; ++cmd) {
    if (strcmp(t.cmd, cmd->name) == 0) {
      cmd->fn(ctx, t.arg);
      return;
    }
  }
  fprintf(stderr, "Unknown command: \"%s %s\"\n", t.cmd, t.arg);
}
