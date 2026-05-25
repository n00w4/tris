#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "protocol.h"

struct Game;

void handle_message(int client_socket, Message* msg);
void handle_list_game(int client_socket);
void handle_join_request(int client_socket, uint32_t game_id);
void handle_create_game(int client_socket);
void handle_move(int client_socket, Message* msg);
void handle_join_decision(int client_socket, bool accepted, uint32_t game_id);
void handle_post_game_decision(int client_socket, uint32_t game_id, bool winner_wants_to_continue);
void handle_set_username(int client_socket, const SetUsernamePayload *payload);
void handle_leave_game(int client_socket, uint32_t game_id);
void broadcast_lobby_update(void);

#endif
