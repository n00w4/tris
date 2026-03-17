#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>

typedef struct queue queue_t;

queue_t *queue_create(size_t capacity);
void queue_destroy(queue_t *q);
int queue_push(queue_t *q, void *data);
int queue_pop(queue_t *q, void **data);
int queue_try_push(queue_t *q, void *data);
int queue_try_pop(queue_t *q, void **data);
size_t queue_size(queue_t *q);
int queue_is_empty(queue_t *q);

#endif
