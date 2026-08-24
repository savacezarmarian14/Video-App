CC      := gcc
CFLAGS  := -Wall -Wextra $(shell pkg-config --cflags gstreamer-1.0)
LDLIBS  := $(shell pkg-config --libs gstreamer-1.0)

SRCS    := $(wildcard *.c)
BINS    := $(SRCS:.c=)

.PHONY: all steps clean

all: $(BINS)

steps: $(filter 0%,$(BINS))

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -f $(BINS)