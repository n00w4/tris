#include "client_handler.h"
#include "server.h"
#include "protocol.h"
#include "game.h"

#include <stdio.h>
#include <string.h>

void handle_message(int client_socket, Message* msg) {
  switch (msg->type) {
    case MSG_CREATE_GAME:
      handle_create_games(client_socket);
      break;
    case MSG_JOIN_GAME:
      handle_join_games(client_socket, msg->game_id);
      break;
    case MSG_LIST_GAMES:
      handle_list_games(client_socket);
      break;
    case MSG_GAME_START:
      // Server must not get this message
      break;
    default: {
      Message error_msg;
      error_msg.type = MSG_ERROR;
      strcpy(error_msg.payload.error.error_message, "Unsupported message type");
      send_message(client_socket, &error_msg);
      break;
    }
  }
}

void handle_list_games(int client_socket) {
  Message response;
  memset(&response, 0, sizeof(Message));

  response.type = MSG_GAME_LIST;
  response.game_id = 0;

  pthread_mutex_lock(&games_mutex);

  int count = 0;
  Game* current = games;

  while (current != NULL && count < 10) {
    if (current->state == GAME_WAITING) {
      response.payload.game_list.game_ids[count] = current->id;
      response.payload.game_list.game_states[count] = current->state;
      count++;
    }
    current = current->next;
  }

  response.payload.game_list.game_count = count;
  pthread_mutex_unlock(&games_mutex);

  printf("[Handler] Sending game list to socket %d (%d games)\n", client_socket, count);

  if (send_message(client_socket, &response) < 0) {
    fprintf(stderr, "[Handler] Failed to send game list to socket %d\n", client_socket);
  }
}

void handle_join_games(int client_socket, uint32_t game_id) {
  pthread_mutex_lock(&games_mutex);

  Game* game = games;
  while (game != NULL) {
    if (game->id == game_id && game->state == GAME_WAITING) {
      if (game->player_o_socket != -1) {
        pthread_mutex_unlock(&games_mutex);
        Message error_msg;
        error_msg.type = MSG_ERROR;
        strcpy(error_msg.payload.error.error_message, "Game already joined by another player.");
        send_message(client_socket, &error_msg);
        return;
      }

      game->player_o_socket = client_socket;
      game->state = GAME_IN_PROGRESS;

      pthread_mutex_lock(&clients_mutex);
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_socket && clients[i].is_active) {
          clients[i].is_playing = true;
          clients[i].current_game_id = game_id;
          break;
        }
      }
      pthread_mutex_unlock(&clients_mutex);

      Message start_msg;
      start_msg.type = MSG_GAME_START;
      start_msg.game_id = game_id;

      send_message(game->player_x_socket, &start_msg);
      send_message(game->player_o_socket, &start_msg);

      pthread_mutex_unlock(&games_mutex);
      printf("[Handler] Game %u started between sockets %d and %d\n", game_id, game->player_x_socket, game->player_o_socket);
      return;
    }
    game = game->next;
  }

  pthread_mutex_unlock(&games_mutex);

  Message error_msg;
  error_msg.type = MSG_ERROR;
  strcpy(error_msg.payload.error.error_message, "Game not found or already in progress.");
  send_message(client_socket, &error_msg);
}

void handle_create_games(int client_socket) {
  Game* new_game = create_game(client_socket);
  if (!new_game) {
    Message error_msg;
    error_msg.type = MSG_ERROR;
    strcpy(error_msg.payload.error.error_message, "Failed to create game.");
    send_message(client_socket, &error_msg);
    return;
  }

  add_game_to_list(new_game);

  Message response;
  response.type = MSG_GAME_LIST;
  response.game_id = new_game->id;

  send_message(client_socket, &response);

  printf("[Handler] Game %u created by socket %d\n", new_game->id, client_socket);
}

