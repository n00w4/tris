#include <ncurses.h>
#include <stdbool.h>

#include "ui.h"

void init_ncurses() {
  draw_board();

  getch();

  endwin();
}

int main() {
  init_ncurses();
  return 0;
}
