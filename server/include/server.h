#ifndef SERVER_H
#define SERVER_H

#include "game.h"
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdatomic.h>

#define MAX_CLIENTS 100
#define MAX_GAMES 50
#define BUFFER_SIZE 1024
#define SERVER_PORT 8080
#define BACKLOG 10
#define MAX_CONN_MSG "Full server. Try again later.\n"

typedef struct Client {
    int socket;
    char username[50];
    int current_game_id;   // -1 if not in a game
    bool is_playing;
    bool is_active;
} Client;

extern Client clients[MAX_CLIENTS];
extern pthread_mutex_t clients_mutex;

extern atomic_int active_thread_count;

extern struct GameManager *game_manager;

Client* add_client(int client_socket);
void remove_client(int client_socket);
void initialize_server(void);
void cleanup_server(void);
int create_server_socket(int port);
void* handle_client(void* arg);
void find_username_by_socket(int socket, char *username_output, size_t out_size);
int server_get_active_clients(void);

#endif
