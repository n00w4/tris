#include "ui.h"

#include <stdbool.h>

static WINDOW* main_win = NULL;
static WINDOW* status_win = NULL;
static WINDOW* menu_win = NULL;

void draw_board() {
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

void init_ui() {
  initscr();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);

  menu_win = newwin(LINES - 4, COLS, 0, 0);
  keypad(menu_win, TRUE);

  status_win = newwin(3, COLS, LINES - 3, 0);
  main_win = stdscr;
}

void cleanup_ui() {
  if (menu_win) { delwin(menu_win); }
  if (status_win) { delwin(status_win); }
  endwin();
}

