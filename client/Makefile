CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude -I../common/include -I../
LIBS = -lncurses -pthread

SRCDIR = src
BUILDDIR = build
TARGET = tris_client

SOURCES = $(shell find $(SRCDIR) -name '*.c') ../common/src/protocol.c

OBJECTS = $(SOURCES:%.c=$(BUILDDIR)/%.o)
DEPS = $(OBJECTS:.o=.d)

.PHONY: all clean run debug

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -ggdb -O0
debug: clean all

-include $(DEPS)
