#ifndef UI_H
#define UI_H

#include "utils/queue.h"
#include "utils/utils.h"

void ui_run(queue_t* from_net, queue_t* to_net, Config* config);
void ui_stop(void);

#endif

