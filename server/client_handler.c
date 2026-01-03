#include "client_handler.h"
#include "server.h"
#include "protocol.h"
#include "game.h"

#include <stdio.h>
#include <string.h>

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

