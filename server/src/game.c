#include "server.h"
#include "game.h"
#include "string.h"

#include <stdlib.h>
#include <stdio.h>

void init_game(Game* game, int game_id, int player_x_socket) {
  game->id = game_id;
  game->player_x_socket = player_x_socket;
  game->player_o_socket = -1;
  memset(game->board, ' ', sizeof(game->board));
  game->state = GAME_WAITING;
  game->current_turn = PLAYER_X;
  game->result_player_x = RESULT_NONE;
  game->result_player_o = RESULT_NONE;
}

Game* create_game(int player_x_socket) {
  Game* new_game = malloc(sizeof(Game));
  if (!new_game) {
    perror("malloc failed in create_game");
    return NULL;
  }

  int game_id = rand();
  init_game(new_game, game_id, player_x_socket);

  return new_game;
}

bool is_valid_move(Game* game, int row, int col) {
  if (game->board[row][col] == ' ') { return true; }
  return false;
}



