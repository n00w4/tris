#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PROTOCOL_MAGIC     0x54524953U  // TRIS in ASCII 
#define PROTOCOL_VERSION   1

typedef enum MessageType {
  MSG_CREATE_GAME = 1,
  MSG_JOIN_GAME,
  MSG_MOVE,
  MSG_LEAVE_GAME,
  MSG_LIST_GAMES,
  MSG_GAME_STATE,
  MSG_GAME_LIST,
  MSG_GAME_START,
  MSG_GAME_OVER,
  MSG_JOIN_REQUEST,
  MSG_JOIN_DECISION,
  MSG_LOBBY_UPDATE,
  MSG_GAME_STATUS_CHANGE,
  MSG_POST_GAME_OPTIONS,
  MSG_ERROR
} MessageType;

typedef enum GameState {
  GAME_NEW,
  GAME_WAITING,
  GAME_IN_PROGRESS,
  GAME_FINISHED
} GameState;

typedef enum GameResult {
  RESULT_WIN,
  RESULT_LOSS,
  RESULT_DRAW,
  RESULT_NONE
} GameResult;

typedef enum Player {
  PLAYER_X = 0,
  PLAYER_O = 1
} Player;

typedef struct GameCommonState {
  char board[3][3];
  Player current_turn;
  GameState state;
  GameResult result_player_x;
  GameResult result_player_o;
  Player player_role;
} GameCommonState;

typedef struct {
  uint32_t game_id;
  uint8_t row;
  uint8_t col;
} MovePayload;

typedef struct {
  uint32_t game_id;
  GameCommonState state_data;
  char message[64];
} GameStatePayload;

typedef struct {
  char error_message[64];
} ErrorPayload;

typedef struct  GameListPayload {
  uint32_t game_ids[10];
  uint8_t game_count;
  GameState game_states[10];
} GameListPayload;

typedef struct  GameStartPayload {
  uint32_t game_id;
  Player player_role;          // X or O
  Player first_player;
} GameStartPayload;

typedef struct  GameOverPayload {
  uint32_t game_id;
  uint8_t winner;
  char message[64];
} GameOverPayload;

typedef struct  JoinRequestPayload {
  uint32_t game_id;
  uint32_t requesting_player_id;
  char requesting_player_name[32];
} JoinRequestPayload;

typedef struct {
  uint32_t game_id;
  uint8_t accepted;   // 1 = accept, 0 = refuse
} JoinDecisionPayload;

typedef struct  LobbyGameInfo {
  uint32_t game_id;
  GameState state;
  char owner_name[32];
  int players_connected; 
} LobbyGameInfo;

typedef struct  LobbyUpdatePayload {
  uint8_t game_count;
  LobbyGameInfo games[10];
} LobbyUpdatePayload;

typedef struct  GameStatusChangePayload {
  uint32_t game_id;
  GameState new_state;
  char message[128];
} GameStatusChangePayload;

typedef struct {
  uint32_t game_id;
  uint8_t winner_wants_to_continue;   // 1 = continue, 0 = not continue
} PostGameOptionsPayload;

typedef struct Message {
  MessageType type;
  union {
    MovePayload move;
    GameStatePayload game_state;
    ErrorPayload error;
    GameListPayload game_list;
    GameStartPayload game_start;
    GameOverPayload game_over;
    JoinRequestPayload join_request;
    JoinDecisionPayload join_decision;
    LobbyUpdatePayload lobby_update;
    GameStatusChangePayload status_change;
    PostGameOptionsPayload post_game_options;
  } payload;
} Message;

int send_message(int socket, const Message *msg);
int receive_message(int socket, Message *msg);

#endif // PROTOCOL_H

