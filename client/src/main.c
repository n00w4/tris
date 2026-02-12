#include "ui/ui.h"
#include "ui/menu.h"

#include <ncurses.h>
#include <stdbool.h>

int main(void) {
  init_ui();
  
  int status;
  do {
    status = show_main_menu();
  } while (status != -1);
  
  cleanup_ui();
  return 0;
}
