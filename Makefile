CC   ?= cc
OBJC ?= $(CC)
RM   ?= rm -f

SRC_DIR := src

BIN     := nsspeaker

APP_NAME ?= $(BIN)
VERSION ?= 0.1
BUILD_DATE := $(shell date +'%Y-%m-%d %H:%M:%S')

DEPLOY_TARGET := 10.5

CFLAGS := -std=c89 -Wall -Wextra -pedantic -Werror -Wno-deprecated-declarations -Os
LDFLAGS   :=
LDLIBS    := -framework AppKit -framework AudioToolbox

CC_SUPPORTS_FNO_OBJC_ARC := $(shell echo 'int main(void){return 0;}' | \
	$(CC) -x objective-c -fno-objc-arc -c -o /dev/null - >/dev/null 2>&1 && echo yes)

ifeq ($(CC_SUPPORTS_FNO_OBJC_ARC),yes)
OBJCFLAGS := -fno-objc-arc
else
OBJCFLAGS :=
endif

CFLAGS += -mmacosx-version-min=$(DEPLOY_TARGET)
LDFLAGS += -mmacosx-version-min=$(DEPLOY_TARGET)

CFLAGS += -DAPP_NAME='"$(APP_NAME)"'
CFLAGS += -DAPP_VERSION='"$(VERSION)"'
CFLAGS += -DAPP_BUILD_DATE='"$(BUILD_DATE)"'

ifdef DEBUG
CFLAGS += -g -O0 -DDEBUG
endif

SRCS := $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*.m)
HEADERS := $(wildcard $(SRC_DIR)/*.h)

.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(SRCS)
	$(CC) $(CFLAGS) $(OBJCFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@
ifndef DEBUG
	strip $(BIN)
endif

fmt: $(SRCS) $(HEADERS)
	clang-format -i --style=Google $^

clean:
	$(RM) $(BIN)

install: all
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(BIN) $(DESTDIR)/usr/local/bin

uninstall:
	$(RM) $(DESTDIR)/usr/local/bin/$(BIN)
