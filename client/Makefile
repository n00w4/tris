CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -D_POSIX_C_SOURCE=200809L
LIBS = -lncurses -pthread

SRCDIR = src
BUILDDIR = build

SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/ui/ui.c \
					$(SRCDIR)/ui/menu.c \
					$(SRCDIR)/utils/utils.c \
					$(SRCDIR)/network/protocol.c \
					$(SRCDIR)/network/network.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
DEPS = $(OBJECTS:.o=.d)
TARGET = tris_client

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -ggdb -O0
debug: clean $(TARGET)

.PHONY: all clean run debug

-include $(DEPS)
