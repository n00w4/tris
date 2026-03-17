CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -Iinclude -I../common/include 
LIBS = -lncurses -pthread

SRCDIR = src
COMMONDIR = ../common/src
BUILDDIR = build
TARGET = tris_client

SRC_FILES = $(shell find $(SRCDIR) -name '*.c')
COMMON_FILES = $(shell find $(COMMONDIR) -name '*.c')

OBJECTS = $(patsubst %.c,$(BUILDDIR)/%.o,$(notdir $(SRC_FILES) $(COMMON_FILES)))
DEPS = $(OBJECTS:.o=.d)

VPATH_DIRS = $(shell find $(SRCDIR) -type d | tr '\n' ':')
VPATH_DIRS += $(COMMONDIR)
vpath %.c $(VPATH_DIRS)

.PHONY: all clean run debug

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -ggdb -O0
debug: clean all

-include $(DEPS)
