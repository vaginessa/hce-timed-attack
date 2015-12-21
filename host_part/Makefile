# Linux
CC=gcc

OBJECTS=sample.o
SOURCE=sample.c

PCSC_CFLAGS := $(shell pkg-config --cflags libpcsclite)
LDFLAGS := $(shell pkg-config --libs libpcsclite)
CFLAGS += $(PCSC_CFLAGS) -std=c99

sample: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o sample $(LDFLAGS)

clean:
	rm -f sample