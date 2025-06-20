
CC = gcc
TARGET = test_vb2
LENGTH ?= 100
EXTRA_VARS ?= 0
DEBUG ?= 0


CFLAGS = -Wall -Wextra -O2 -DTEST_RECORD_LENGTH=$(LENGTH) -DTEST_EXTRA_VARS=$(EXTRA_VARS)
ifeq ($(DEBUG), 1)
    CFLAGS += -DVB2_DEBUG_ENABLED
endif

SOURCES = $(wildcard *.c)
SOURCES += $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

INCLUDES = -I. \
		   -I./include \

default: $(TARGET)

.PHONY: default clean

.PHONY: $(OBJECTS)
$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(OBJECTS)

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f test.vb2