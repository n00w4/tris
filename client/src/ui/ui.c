#include "ui.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static WINDOW* main_win = NULL;
static WINDOW* status_win = NULL;
static WINDOW* menu_win = NULL;

void init_ui(void) {
  initscr();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);

  menu_win = newwin(LINES - 4, COLS, 0, 0);
  keypad(menu_win, TRUE);

  status_win = newwin(3, COLS, LINES - 3, 0);
  main_win = stdscr;
}

void cleanup_ui(void) {
  if (menu_win) { delwin(menu_win); }
  if (status_win) { delwin(status_win); }
  endwin();
}

void init_menu(Menu* menu, const char* title, int start_y, int start_x, int height, int width) {
  menu->title = strdup(title);
  menu->item_count = 0;
  menu->selected_index = 0;
  menu->start_x = start_x;
  menu->start_y = start_y;
  menu->height = height;
  menu->width = width;
  menu->border = true;

  menu->win = newwin(height, width, start_y, start_x);
  keypad(menu->win, TRUE);
}

void draw_board(void) {
  int i;
  for (i=0; i<=3; i++) {
    mvhline(START_Y + (i * CELL_H), START_X, 0, 3 * CELL_W);
    mvvline(START_Y, START_X + (i * CELL_W), 0, 3 * CELL_H);
  }

  for (int r = 0; r <= 3; r++) {
    for (int c = 0; c <= 3; c++) {
      mvaddch(START_Y + (r * CELL_H), START_X + (c * CELL_W), '+');
    }
  }
  refresh();
}

void draw_menu(Menu* menu) {
  WINDOW* win = menu_win;
  werase(win);

  if (menu->border) {
    box(win, 0, 0);
  }

  mvwprintw(win, 1, 2, "%s", menu->title);

  for (int i = 0; i < menu->item_count; i++) {
    if (i == menu->selected_index) {
      wattron(win, A_REVERSE);
    }
    mvwprintw(win, i + 3, 4, "%s", menu->items[i]);
    if (i == menu->selected_index) {
      wattroff(win, A_REVERSE);
    }
  }

  wrefresh(win);
}

void add_menu_item(Menu* menu, const char* item) {
  if (menu->item_count < MAX_MENU_ITEMS) {
    menu->items[menu->item_count] = strdup(item);
    menu->item_count++;
  }
}

void cleanup_menu(Menu* menu) {
  free(menu->title);
  for (int i = 0; i < menu->item_count; i++) {
    free(menu->items[i]);
  }
}

int handle_menu_input(Menu* menu) {
  int ch;

  while (1) {
    draw_menu(menu);
    ch = wgetch(menu->win);

    switch (ch) {
      case KEY_UP:
        if (menu->selected_index > 0) {
          menu->selected_index--;
        }
        break;

      case KEY_DOWN:
        if (menu->selected_index < menu->item_count - 1) {
          menu->selected_index++;
        }
        break;

      case '\n':  // Enter
      case KEY_ENTER:
        return menu->selected_index;

      case 27:    // ESC
      case 'q':
      case 'Q':
        return -1;  // Exit

      // Navigation with numbers
      case '1':
        if (menu->item_count >= 1) {
          menu->selected_index = 0;
          return 0;
        }
        break;
      case '2':
        if (menu->item_count >= 2) {
          menu->selected_index = 1;
          return 1;
        }
        break;
    }
  }
}

void show_main_menu(void) {
  clear();

  attron(A_BOLD);
  mvprintw(1, COLS/2 - 10, "=== TRIS CLIENT ===");
  attroff(A_BOLD);

  refresh();

  // Calculating menu dimension and position
  int menu_height = 12;
  int menu_width = 40;
  int menu_y = (LINES - menu_height) / 2;
  int menu_x = (COLS - menu_width) / 2;

  Menu main_menu;
  init_menu(&main_menu, "START MENU", menu_y, menu_x, menu_height, menu_width);

  add_menu_item(&main_menu, "Create new game");
  add_menu_item(&main_menu, "Join a game");
  add_menu_item(&main_menu, "Exit");

  int choice = handle_menu_input(&main_menu);

  cleanup_menu(&main_menu);

  switch (choice) {
    case 0:
      // Create new game
      mvprintw(LINES - 2, 2, "Create new game: not implemented yet!");
      break;
    case 1:
      // Join a Game
      mvprintw(LINES - 2, 2, "Join a game: not implemented yet!");
      break;
    case -1:
      // Esci
      mvprintw(LINES - 2, 2, "Exiting...");
      break;
  }

  refresh();
  getch();
}

