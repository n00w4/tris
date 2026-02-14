CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -D_POSIX_C_SOURCE=200809L -I../common/include
LIBS = -lncurses -pthread

SRCDIR = src
COMMONDIR = ../common/src
BUILDDIR = build

SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/ui/ui.c \
          $(SRCDIR)/ui/menu.c \
          $(SRCDIR)/ui/screens.c \
          $(SRCDIR)/utils/utils.c \
          $(SRCDIR)/network/network.c \
          $(COMMONDIR)/protocol.c

OBJECTS = $(patsubst %.c,$(BUILDDIR)/%.o,$(notdir $(SOURCES)))
DEPS = $(OBJECTS:.o=.d)

TARGET = tris_client

.PHONY: all clean run debug

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/*/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILDDIR)/protocol.o: $(COMMONDIR)/protocol.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -ggdb -O0
debug: clean all

-include $(DEPS)
