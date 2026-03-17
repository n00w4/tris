#ifndef UI_EVENTS_H
#define UI_EVENTS_H

#include "protocol.h"

typedef enum {
  UI_EVENT_GAME_STATE,
  UI_EVENT_GAME_START,
  UI_EVENT_GAME_OVER,
  UI_EVENT_LOBBY_UPDATE,
  UI_EVENT_ERROR,
  UI_EVENT_JOIN_REQUEST,
  UI_EVENT_JOIN_DECISION,
  UI_EVENT_POST_GAME_OPTIONS,
  UI_EVENT_DISCONNECTED,
  UI_EVENT_GAME_STATUS_CHANGE
} UIEventType;

typedef struct {
  UIEventType type;
  union {
    GameStatePayload game_state;
    GameStartPayload game_start;
    GameOverPayload game_over;
    LobbyUpdatePayload lobby_update;
    ErrorPayload error;
    JoinRequestPayload join_request;
    JoinDecisionPayload join_decision;
    PostGameOptionsPayload post_game_options;
    GameStatusChangePayload game_status_change;
  } data;
} UIEvent;

#endif
