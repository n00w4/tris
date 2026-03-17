CC = gcc
CFLAGS_BASE = -Wall -Wextra -pedantic -std=c11 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -Iinclude -I../common/include
HARDENING_CFLAGS = -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security -Wshadow -Wconversion
LDFLAGS = -Wl,-z,relro,-z,now
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

.PHONY: all clean run debug release

all: release

release: CFLAGS = $(CFLAGS_BASE) $(HARDENING_CFLAGS) -O2
release: $(TARGET)

debug: CFLAGS = $(CFLAGS_BASE) -ggdb -O0 -DDEBUG
debug: clean all

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDFLAGS)

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: release
	./$(TARGET)

-include $(DEPS)
