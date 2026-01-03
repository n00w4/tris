CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g
LIBS = -pthread
SOURCES = server.c game.c protocol.c client_handler.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = tris_server

.PHONY: all clean run debug

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

server.o: server.c server.h protocol.h client_handler.h game.h
	$(CC) $(CFLAGS) -c $< -o $@

game.o: game.c game.h
	$(CC) $(CFLAGS) -c $< -o $@

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) -c $< -o $@

client_handler.o: client_handler.c client_handler.h server.h protocol.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -DDEBUG -O0
debug: clean $(TARGET)
