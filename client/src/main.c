#include "ui/ui.h"
#include "utils/queue.h"
#include "utils/utils.h"
#include <stdio.h>
#include <signal.h>

int main(void) {
  signal(SIGPIPE, SIG_IGN);

  Config config;
  if (!read_config(&config)) {
    fprintf(stderr, "Failed to read config, using defaults.\n");
    snprintf(config.username, sizeof(config.username), "Player");
    snprintf(config.ip, sizeof(config.ip), "127.0.0.1");
    snprintf(config.port, sizeof(config.port), "8080");
  }

  queue_t* to_net = queue_create(64);
  queue_t* from_net = queue_create(64);
  if (!to_net || !from_net) {
    fprintf(stderr, "Failed to create queues\n");
    return 1;
  }

  ui_run(from_net, to_net, &config);

  queue_destroy(to_net);
  queue_destroy(from_net);
  return 0;
}
