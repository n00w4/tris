CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -Iinclude -I../common/include
LIBS = -pthread

SRCDIR = src
COMMONDIR = ../common/src
BUILDDIR = build

SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/server.c \
          $(SRCDIR)/game.c \
          $(SRCDIR)/client_handler.c \
          $(COMMONDIR)/protocol.c

OBJECTS = $(patsubst %.c,$(BUILDDIR)/%.o,$(notdir $(SOURCES)))
DEPS = $(OBJECTS:.o=.d)

TARGET = tris_server

.PHONY: all clean run debug

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILDDIR)/protocol.o: $(COMMONDIR)/protocol.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -ggdb -O0 -DDEBUG
debug: clean all

-include $(DEPS)
