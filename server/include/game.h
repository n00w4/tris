#ifndef GAME_H
#define GAME_H

#include "protocol.h"
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct GameSnapshot {
    uint32_t id;
    GameState state;
    char owner_name[32];
    int players_connected;
} GameSnapshot;

typedef struct Game {
    uint32_t id;
    int player_x_socket;
    int player_o_socket;
    int waiting_player_socket;
    char owner_username[32];
    char waiting_player_username[32];
    GameCommonState state_data;

    bool winner_stays;
    bool post_game_x_received;
    bool post_game_o_received;
    bool post_game_x_continue;
    bool post_game_o_continue;

    pthread_mutex_t lock;
    struct Game *next;
} Game;

typedef struct GameManager GameManager;

GameManager *game_manager_create(void);
void game_manager_destroy(GameManager *gm);
int game_manager_create_game(GameManager *gm, int player_x_socket, const char *owner_username, uint32_t *out_game_id);
int game_manager_get_owner_socket(GameManager *gm, uint32_t game_id);
int game_manager_join_game(GameManager *gm, uint32_t game_id, int joiner_socket, const char *joiner_username);
int game_manager_join_decision(GameManager *gm, uint32_t game_id, int accepted, Player *out_player_role);
int game_manager_move(GameManager *gm, uint32_t game_id, int player_socket, uint8_t row, uint8_t col, bool *out_finished, int *out_winner);
int game_manager_leave_game(GameManager *gm, uint32_t game_id, int player_socket);
void game_manager_list_games(GameManager *gm, GameSnapshot *out_snapshots, uint8_t *out_count);
int game_manager_get_players(GameManager *gm, uint32_t game_id, int *out_x_socket, int *out_o_socket);
int game_manager_post_game_decision(GameManager *gm, uint32_t game_id, int player_socket, int wants_to_continue, bool *out_both_decided);
int game_manager_get_game_state(GameManager *gm, uint32_t game_id, GameCommonState *out_state);
int game_manager_get_waiting_socket(GameManager *gm, uint32_t game_id, int *out_socket);

#endif
