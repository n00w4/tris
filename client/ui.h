#ifndef UI_H

#include <ncurses.h>
#include <stdbool.h>

#define CELL_H 3
#define CELL_W 6

#define START_X 10
#define START_Y 5

#define MAX_MENU_ITEMS 10
#define MENU_WIDTH 40

typedef struct {
  char* title;
  char* items[MAX_MENU_ITEMS];
  int item_count;
  int selected_index;
  int start_y;
  int start_x;
  int height;
  int width;
  bool border;
  WINDOW* win;
} Menu;

typedef enum {
  APP_STATE_MENU,
  APP_STATE_GAME,
  APP_STATE_CONNECTING,
  APP_STATE_LOBBY,
  APP_STATE_EXIT
} AppState;

void draw_board(void);
void init_ui(void);
void cleanup_ui(void);
void init_menu(Menu* menu, const char* title, int start_y, int start_x, int height, int width);
void draw_menu(Menu* menu);
void add_menu_item(Menu* menu, const char* item);
void cleanup_menu(Menu* menu);
int handle_menu_input(Menu* menu);
void show_main_menu(void);

#endif

