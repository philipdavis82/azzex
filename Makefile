
CC = gcc
TARGET = test_vb2
LENGTH ?= 100
EXTRA_VARS ?= 0
NO_EXTRA_FILE ?= 0
DEBUG ?= 0


CFLAGS = -Wall -Wextra -g -O2 -DTEST_RECORD_LENGTH=$(LENGTH) -DTEST_EXTRA_VARS=$(EXTRA_VARS)
ifeq ($(DEBUG), 1)
    CFLAGS += -DVB2_DEBUG_ENABLED
endif
ifeq ($(NO_EXTRA_FILE), 1)
    CFLAGS += -DTEST_NO_EXTRA_FILE
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
	rm -f test.vb2 test2.vb2 test.vb2.log test2.vb2.log