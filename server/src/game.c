#include "server.h"
#include "game.h"
#include "string.h"

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

bool is_valid_move(Game* game, int row, int col) {
  if (game->board[row][col] == ' ') { return true; }
  return false;
}



