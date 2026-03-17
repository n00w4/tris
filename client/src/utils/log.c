#include "utils/log.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static FILE *log_file = NULL;

static void log_init(void) {
  if (!log_file) {
    log_file = fopen("client.log", "a");
    if (log_file) {
      setvbuf(log_file, NULL, _IOLBF, 0); // line buffered
    }
  }
}

void log_printf(const char *fmt, ...) {
  log_init();

  if (!log_file) { return; }

  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  char time_buf[20];
  strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
  fprintf(log_file, "[%s] ", time_buf);

  va_list args;
  va_start(args, fmt);
  vfprintf(log_file, fmt, args);
  va_end(args);

  fprintf(log_file, "\n");
}
