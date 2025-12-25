CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
SOURCES = server.c game.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = server

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -g
debug: $(TARGET)

.PHONY: clean run debug
