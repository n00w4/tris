CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11
LIBS = -pthread

SRCDIR = src
BUILDDIR = build

SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/server.c \
          $(SRCDIR)/game.c \
          $(SRCDIR)/protocol.c \
          $(SRCDIR)/client_handler.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

TARGET = $(BUILDDIR)/tris_server

.PHONY: all clean run debug

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -DDEBUG -O0
debug: all
