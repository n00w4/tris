#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

#define FILENAME "config.conf"

typedef struct {
  char username[50];
  char ip[16];
  char port[6];
} Config;

bool read_config(Config* config);
bool save_config(Config* config);

#endif
