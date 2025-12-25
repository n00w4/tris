#ifndef GAME_H
#define GAME_H

#include "stdbool.h"
#include "server.h"

typedef enum {
  GAME_WAITING,
  GAME_IN_PROGRESS,
  GAME_FINISHED
} GameState;

typedef enum {
  RESULT_WIN,
  RESULT_LOSS,
  RESULT_DRAW,
  RESULT_NONE
} GameResult;

typedef enum {
  PLAYER_X = 0,
  PLAYER_O = 1
} Player;

typedef struct Game {
  int id;
  int player_x_socket;
  int player_o_socket;
  char board[3][3];
  GameState state;
  Player current_turn;
  GameResult result_player_x;
  GameResult result_player_o;
} Game;


/**
 * TODO: add apply_move, check_winner, finalize_game (to print final results)
 * and print_board (for debug)
 */

/**
 * Initialize a game
 */
void init_game(Game* game, int game_id, int player_x_socket);

/**
 * Validate a move
 */
bool is_valid_move(Game* game, int row, int col, Player player);


#endif

