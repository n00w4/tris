#ifndef UI_H

#include <ncurses.h>

#define CELL_H 3
#define CELL_W 6

#define START_X 10
#define START_Y 5

#define MAX_MENU_ITEMS 10

typedef struct {
  char* title;
  char* items[MAX_MENU_ITEMS];
  int item_count;
  int selected_index;
  int start_y;
  int start_x;
  int width;
  bool border;
} Menu;

typedef enum {
  APP_STATE_MENU,
  APP_STATE_GAME,
  APP_STATE_CONNECTING,
  APP_STATE_LOBBY,
  APP_STATE_EXIT
} AppState;

void draw_board();
void init_ui();
void cleanup_ui();

#endif

