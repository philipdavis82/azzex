
CC = gcc
TARGET = test_vb2
LENGTH ?= 100

CFLAGS = -Wall -Wextra -O2 -DTEST_RECORD_LENGTH=$(LENGTH)

SOURCES = $(wildcard *.c)
SOURCES += $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

INCLUDES = -I. \
		   -I./include \

default: $(TARGET)

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(OBJECTS)

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f test.vb2