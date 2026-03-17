#include "utils/utils.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool read_config(Config* config) {
  if (config == NULL) {
    return false;
  }

  FILE *file = fopen(FILENAME, "r");
  if (!file) {
    Config default_config = {"Player", "127.0.0.1", "8080"};
    save_config(&default_config);

    file = fopen(FILENAME, "r");
    if (!file) {
      return false;
    }
  }

  int read = fscanf(file, " username=%49s ip=%15s port=%5s", config->username, config->ip, config->port);
  fclose(file);

  return (read == 3);
}

bool save_config(Config* config) {
  if (config == NULL) {
    return false;
  }

  FILE *file = fopen(FILENAME, "w");
  if (!file) {
    return false;
  }

  fprintf(file, "# Configuration file for Tris client\n");
  fprintf(file, "# Modify these settings as needed\n\n");
  fprintf(file, "username=%s\n", config->username);
  fprintf(file, "ip=%s\n", config->ip);
  fprintf(file, "port=%s\n", config->port);

  fclose(file);
  return true;
}
