#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>

struct GameManager {
  Game *head;
  pthread_mutex_t lock;
};

static atomic_uint next_game_id = 0;
static uint32_t generate_game_id(void) {
  return atomic_fetch_add(&next_game_id, 1);
}

static bool game_check_winner(const Game *game, int *out_winner) {
  const char (*b)[3] = game->state_data.board;
  for (int i = 0; i < 3; i++) {
    if (b[i][0] != ' ' && b[i][0] == b[i][1] && b[i][1] == b[i][2]) {
      *out_winner = (b[i][0] == 'X') ? PLAYER_X : PLAYER_O;
      return true;
    }
    if (b[0][i] != ' ' && b[0][i] == b[1][i] && b[1][i] == b[2][i]) {
      *out_winner = (b[0][i] == 'X') ? PLAYER_X : PLAYER_O;
      return true;
    }
  }
  if (b[0][0] != ' ' && b[0][0] == b[1][1] && b[1][1] == b[2][2]) {
    *out_winner = (b[0][0] == 'X') ? PLAYER_X : PLAYER_O;
    return true;
  }
  if (b[0][2] != ' ' && b[0][2] == b[1][1] && b[1][1] == b[2][0]) {
    *out_winner = (b[0][2] == 'X') ? PLAYER_X : PLAYER_O;
    return true;
  }
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      if (b[i][j] == ' ') {
        *out_winner = RESULT_NONE;
        return false;
      }
  *out_winner = RESULT_DRAW;
  return true;
}

static void game_finalize(Game *game, int winner) {
  game->state_data.state = GAME_FINISHED;
  if (winner == PLAYER_X) {
    game->state_data.result_player_x = RESULT_WIN;
    game->state_data.result_player_o = RESULT_LOSS;
  } else if (winner == PLAYER_O) {
    game->state_data.result_player_x = RESULT_LOSS;
    game->state_data.result_player_o = RESULT_WIN;
  } else if (winner == RESULT_DRAW) {
    game->state_data.result_player_x = RESULT_DRAW;
    game->state_data.result_player_o = RESULT_DRAW;
  }
}

GameManager *game_manager_create(void) {
  GameManager *gm = malloc(sizeof(GameManager));
  if (!gm) return NULL;
  gm->head = NULL;
  pthread_mutex_init(&gm->lock, NULL);
  return gm;
}

void game_manager_destroy(GameManager *gm) {
  if (!gm) return;
  pthread_mutex_lock(&gm->lock);
  Game *cur = gm->head;
  while (cur) {
    Game *next = cur->next;
    pthread_mutex_destroy(&cur->lock);
    free(cur);
    cur = next;
  }
  pthread_mutex_unlock(&gm->lock);
  pthread_mutex_destroy(&gm->lock);
  free(gm);
}

int game_manager_create_game(GameManager *gm, int player_x_socket, const char *owner_username, uint32_t *out_game_id) {
  Game *g = malloc(sizeof(Game));
  if (!g) return -1;

  memset(g, 0, sizeof(Game));
  g->id = generate_game_id();
  g->player_x_socket = player_x_socket;
  g->player_o_socket = -1;
  g->waiting_player_socket = -1;

  if (owner_username) {
    strncpy(g->owner_username, owner_username, sizeof(g->owner_username)-1);
    g->owner_username[sizeof(g->owner_username)-1] = '\0';
  }

  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      g->state_data.board[i][j] = ' ';
  g->state_data.current_turn = PLAYER_X;
  g->state_data.state = GAME_WAITING;
  g->state_data.result_player_x = RESULT_NONE;
  g->state_data.result_player_o = RESULT_NONE;

  pthread_mutex_init(&g->lock, NULL);

  pthread_mutex_lock(&gm->lock);
  g->next = gm->head;
  gm->head = g;
  pthread_mutex_unlock(&gm->lock);

  if (out_game_id) *out_game_id = g->id;
  return 0;
}

int game_manager_get_owner_socket(GameManager *gm, uint32_t game_id) {
  pthread_mutex_lock(&gm->lock);
  Game *g = gm->head;
  while (g && g->id != game_id) g = g->next;
  if (!g) {
    pthread_mutex_unlock(&gm->lock);
    return -1;
  }
  pthread_mutex_lock(&g->lock);
  pthread_mutex_unlock(&gm->lock);
  int sock = g->player_x_socket;
  pthread_mutex_unlock(&g->lock);
  return sock;
}

int game_manager_join_game(GameManager *gm, uint32_t game_id, int joiner_socket, const char *joiner_username) {
  pthread_mutex_lock(&gm->lock);
  Game *g = gm->head;
  while (g && g->id != game_id) g = g->next;
  if (!g) {
    pthread_mutex_unlock(&gm->lock);
    return -1;
  }
  pthread_mutex_lock(&g->lock);
  pthread_mutex_unlock(&gm->lock);

  if (g->state_data.state != GAME_WAITING ||
      g->player_o_socket != -1 ||
      g->waiting_player_socket != -1) {
    pthread_mutex_unlock(&g->lock);
    return -1;
  }

  g->waiting_player_socket = joiner_socket;
  if (joiner_username) {
    strncpy(g->waiting_player_username, joiner_username,
        sizeof(g->waiting_player_username)-1);
    g->waiting_player_username[sizeof(g->waiting_player_username)-1] = '\0';
  }
  pthread_mutex_unlock(&g->lock);
  return 1;   // waiting for owner decision
}

int game_manager_join_decision(GameManager *gm, uint32_t game_id, int accepted, Player *out_player_role) {
  pthread_mutex_lock(&gm->lock);
  Game *g = gm->head;
  while (g && g->id != game_id) g = g->next;
  if (!g) {
    pthread_mutex_unlock(&gm->lock);
    return -1;
  }
  pthread_mutex_lock(&g->lock);
  pthread_mutex_unlock(&gm->lock);

  if (g->waiting_player_socket == -1) {
    pthread_mutex_unlock(&g->lock);
    return -1;
  }

  if (accepted) {
    g->player_o_socket = g->waiting_player_socket;
    g->waiting_player_socket = -1;
    g->waiting_player_username[0] = '\0';
    g->state_data.state = GAME_IN_PROGRESS;
    if (out_player_role) *out_player_role = PLAYER_O;
  } else {
    g->waiting_player_socket = -1;
    g->waiting_player_username[0] = '\0';
  }
  pthread_mutex_unlock(&g->lock);
  return 0;
}

int game_manager_move(GameManager *gm, uint32_t game_id, int player_socket, uint8_t row, uint8_t col, bool *out_finished, int *out_winner) {
  pthread_mutex_lock(&gm->lock);
  Game *g = gm->head;
  while (g && g->id != game_id) g = g->next;
  if (!g) {
    pthread_mutex_unlock(&gm->lock);
    return -1;
  }
  pthread_mutex_lock(&g->lock);
  pthread_mutex_unlock(&gm->lock);

  Player player;
  if (player_socket == g->player_x_socket)
    player = PLAYER_X;
  else if (player_socket == g->player_o_socket)
    player = PLAYER_O;
  else {
    pthread_mutex_unlock(&g->lock);
    return -1;
  }

  if (g->state_data.state != GAME_IN_PROGRESS ||
      row > 2 || col > 2 ||
      g->state_data.board[row][col] != ' ' ||
      g->state_data.current_turn != player) {
    pthread_mutex_unlock(&g->lock);
    return -1;
  }

  g->state_data.board[row][col] = (player == PLAYER_X) ? 'X' : 'O';
  g->state_data.current_turn = (player == PLAYER_X) ? PLAYER_O : PLAYER_X;

  int winner;
  bool finished = game_check_winner(g, &winner);
  if (finished) {
    game_finalize(g, winner);
    if (out_winner) *out_winner = winner;
  }
  if (out_finished) *out_finished = finished;

  pthread_mutex_unlock(&g->lock);
  return 0;
}

int game_manager_leave_game(GameManager *gm, uint32_t game_id, int player_socket) {
  // find the game under gm->lock, lock it, then release gm->lock
  pthread_mutex_lock(&gm->lock);
  Game *g = gm->head;
  while (g && g->id != game_id) {
    g = g->next;
  }
  if (!g) {
    pthread_mutex_unlock(&gm->lock);
    return -1;
  }
  pthread_mutex_lock(&g->lock);
  pthread_mutex_unlock(&gm->lock);

  // Update player sockets
  if (player_socket == g->player_x_socket)
    g->player_x_socket = -1;
  else if (player_socket == g->player_o_socket)
    g->player_o_socket = -1;
  else if (player_socket == g->waiting_player_socket) {
    g->waiting_player_socket = -1;
    g->waiting_player_username[0] = '\0';
  } else {
    pthread_mutex_unlock(&g->lock);
    return -1;
  }

  // Determine if the game is empty and should be removed
  bool should_remove = (g->player_x_socket == -1 &&
      g->player_o_socket == -1 &&
      g->waiting_player_socket == -1);

  pthread_mutex_unlock(&g->lock);

  // remove the node from the global list
  if (should_remove) {
    pthread_mutex_lock(&gm->lock);
    Game **pprev = &gm->head;
    Game *curr = gm->head;
    while (curr && curr != g) {
      pprev = &curr->next;
      curr = curr->next;
    }
    if (curr == g) {
      *pprev = g->next;
      pthread_mutex_destroy(&g->lock);
      free(g);
    }
    pthread_mutex_unlock(&gm->lock);
  }

  return 0;
}

void game_manager_list_games(GameManager *gm, GameSnapshot *out_snapshots, uint8_t *out_count) {
  pthread_mutex_lock(&gm->lock);
  uint8_t cnt = 0;
  for (Game *g = gm->head; g && cnt < 10; g = g->next) {
    if (g->state_data.state == GAME_WAITING && g->player_o_socket == -1 && g->waiting_player_socket == -1) {
      out_snapshots[cnt].id = g->id;
      out_snapshots[cnt].state = g->state_data.state;
      strncpy(out_snapshots[cnt].owner_name, g->owner_username,
          sizeof(out_snapshots[cnt].owner_name)-1);
      out_snapshots[cnt].owner_name[sizeof(out_snapshots[cnt].owner_name)-1] = '\0';
      out_snapshots[cnt].players_connected = 1; // only owner present
      cnt++;
    }
  }
  *out_count = cnt;
  pthread_mutex_unlock(&gm->lock);
}

int game_manager_get_players(GameManager *gm, uint32_t game_id, int *out_x_socket, int *out_o_socket) {
  pthread_mutex_lock(&gm->lock);
  Game *g = gm->head;
  while (g && g->id != game_id) g = g->next;
  if (!g) {
    pthread_mutex_unlock(&gm->lock);
    return -1;
  }
  pthread_mutex_lock(&g->lock);
  pthread_mutex_unlock(&gm->lock);

  if (out_x_socket) { *out_x_socket = g->player_x_socket; }
  if (out_o_socket) { *out_o_socket = g->player_o_socket; }

  pthread_mutex_unlock(&g->lock);
  return 0;
}

int game_manager_post_game_decision(GameManager *gm, uint32_t game_id, int player_socket, int wants_to_continue, bool *out_both_decided) {
  pthread_mutex_lock(&gm->lock);
  Game *g = gm->head;
  while (g && g->id != game_id) g = g->next;
  if (!g) {
    pthread_mutex_unlock(&gm->lock);
    return -1;
  }
  pthread_mutex_lock(&g->lock);
  pthread_mutex_unlock(&gm->lock);

  bool is_x = (player_socket == g->player_x_socket);
  bool is_o = (player_socket == g->player_o_socket);
  if (!is_x && !is_o) {
    pthread_mutex_unlock(&g->lock);
    return -1;
  }

  if (is_x) {
    g->post_game_x_received = true;
    g->post_game_x_continue = wants_to_continue;
  } else {
    g->post_game_o_received = true;
    g->post_game_o_continue = wants_to_continue;
  }

  bool draw = (g->state_data.result_player_x == RESULT_DRAW && g->state_data.result_player_o == RESULT_DRAW);
  bool x_won = (g->state_data.result_player_x == RESULT_WIN);

  bool both_decided = false;

  if (draw) {
    both_decided = (g->post_game_x_received && g->post_game_o_received);
    if (both_decided) {
      if (g->post_game_x_continue && g->post_game_o_continue) {
        // Both want to continue: restart with same roles
        for (int i = 0; i < 3; i++)
          for (int j = 0; j < 3; j++)
            g->state_data.board[i][j] = ' ';
        g->state_data.current_turn = PLAYER_X;
        g->state_data.result_player_x = RESULT_NONE;
        g->state_data.result_player_o = RESULT_NONE;
        g->state_data.state = GAME_IN_PROGRESS;
      } else if (g->post_game_x_continue || g->post_game_o_continue) {
        // Only one wants to continue: becomes owner
        int new_owner_sock = g->post_game_x_continue ?
          g->player_x_socket : g->player_o_socket;
        g->player_x_socket = new_owner_sock;
        g->player_o_socket = -1;
        g->waiting_player_socket = -1;
        for (int i = 0; i < 3; i++)
          for (int j = 0; j < 3; j++)
            g->state_data.board[i][j] = ' ';
        g->state_data.current_turn = PLAYER_X;
        g->state_data.result_player_x = RESULT_NONE;
        g->state_data.result_player_o = RESULT_NONE;
        g->state_data.state = GAME_WAITING;
      } else {
        // No one wants to continue: mark as finished
        g->state_data.state = GAME_FINISHED;
      }
      g->post_game_x_received = g->post_game_o_received = false;
    }
  } else {
    bool winner_is_x = x_won;
    int winner_sock = winner_is_x ? g->player_x_socket : g->player_o_socket;
    if ((winner_is_x && is_o) || (!winner_is_x && is_x)) {
      pthread_mutex_unlock(&g->lock);
      return 0;
    }

    if (wants_to_continue) {
      g->player_x_socket = winner_sock;
      g->player_o_socket = -1;
      g->waiting_player_socket = -1;
      for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
          g->state_data.board[i][j] = ' ';
      g->state_data.current_turn = PLAYER_X;
      g->state_data.result_player_x = RESULT_NONE;
      g->state_data.result_player_o = RESULT_NONE;
      g->state_data.state = GAME_WAITING;
    } else {
      g->state_data.state = GAME_FINISHED;
    }
    both_decided = true;
  }

  if (out_both_decided) { *out_both_decided = both_decided; }

  pthread_mutex_unlock(&g->lock);
  return 0;
}

int game_manager_get_game_state(GameManager *gm, uint32_t game_id, GameCommonState *out_state) {
  pthread_mutex_lock(&gm->lock);
  Game *g = gm->head;
  while (g && g->id != game_id) g = g->next;
  if (!g) {
    pthread_mutex_unlock(&gm->lock);
    return -1;
  }
  pthread_mutex_lock(&g->lock);
  pthread_mutex_unlock(&gm->lock);

  *out_state = g->state_data;   // struct copy

  pthread_mutex_unlock(&g->lock);
  return 0;
}

int game_manager_get_waiting_socket(GameManager *gm, uint32_t game_id, int *out_socket) {
  pthread_mutex_lock(&gm->lock);
  Game *g = gm->head;
  while (g && g->id != game_id) g = g->next;
  if (!g) {
    pthread_mutex_unlock(&gm->lock);
    return -1;
  }
  pthread_mutex_lock(&g->lock);
  pthread_mutex_unlock(&gm->lock);

  if (out_socket) { *out_socket = g->waiting_player_socket; }

  pthread_mutex_unlock(&g->lock);
  return 0;
}

