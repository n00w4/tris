#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include <stdbool.h>

#define CELL_H 3
#define CELL_W 6
#define START_X 10
#define START_Y 5
#define MAX_MENU_ITEMS 10

#define MENU_BACK -2
#define MENU_QUIT -1

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

void init_ui(void);
void cleanup_ui(void);
void draw_board(void);
void get_styled_input(int y, int x, char *buffer, int max_len);

void init_generic_menu(Menu* menu, const char* title, int start_y, int start_x, int height, int width);
void draw_generic_menu(Menu* menu);
void add_menu_item(Menu* menu, const char* item);
void cleanup_generic_menu(Menu* menu);
int handle_generic_menu_input(Menu* menu, bool is_main_menu);

#endif

