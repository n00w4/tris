#include "utils/queue.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>

struct queue {
  void           **buffer;
  size_t           capacity;
  size_t           head;      // read index
  size_t           tail;      // write index
  size_t           count;     // number of elements
  pthread_mutex_t  lock;
  pthread_cond_t   not_full;
  pthread_cond_t   not_empty;
  atomic_int       destroyed;
};

queue_t *queue_create(size_t capacity) {
  if (capacity == 0) return NULL;

  queue_t *q = malloc(sizeof(queue_t));
  if (!q) return NULL;

  q->buffer = malloc(sizeof(void *) * capacity);
  if (!q->buffer) {
    free(q);
    return NULL;
  }

  q->capacity = capacity;
  q->head = 0;
  q->tail = 0;
  q->count = 0;
  atomic_init(&q->destroyed, 0);

  if (pthread_mutex_init(&q->lock, NULL) != 0) {
    free(q->buffer);
    free(q);
    return NULL;
  }
  if (pthread_cond_init(&q->not_full, NULL) != 0) {
    pthread_mutex_destroy(&q->lock);
    free(q->buffer);
    free(q);
    return NULL;
  }
  if (pthread_cond_init(&q->not_empty, NULL) != 0) {
    pthread_cond_destroy(&q->not_full);
    pthread_mutex_destroy(&q->lock);
    free(q->buffer);
    free(q);
    return NULL;
  }

  return q;
}

void queue_destroy(queue_t *q) {
  if (!q) return;

  pthread_mutex_lock(&q->lock);
  assert(q->count == 0 &&
      "queue_destroy: called with elements still in queue. "
      "Ensure all threads have terminated before destroying the queue.");
  atomic_store(&q->destroyed, 1);
  pthread_cond_broadcast(&q->not_full);
  pthread_cond_broadcast(&q->not_empty);
  pthread_mutex_unlock(&q->lock);

  pthread_mutex_destroy(&q->lock);
  pthread_cond_destroy(&q->not_full);
  pthread_cond_destroy(&q->not_empty);
  free(q->buffer);
  free(q);
}

static int queue_push_locked(queue_t *q, void *data, int block) {
  pthread_mutex_lock(&q->lock);
  while (q->count == q->capacity && !atomic_load(&q->destroyed)) {
    if (!block) {
      pthread_mutex_unlock(&q->lock);
      return -1;
    }
    pthread_cond_wait(&q->not_full, &q->lock);
  }
  if (atomic_load(&q->destroyed)) {
    pthread_mutex_unlock(&q->lock);
    return -1;
  }
  q->buffer[q->tail] = data;
  q->tail = (q->tail + 1) % q->capacity;
  q->count++;
  pthread_cond_signal(&q->not_empty);
  pthread_mutex_unlock(&q->lock);
  return 0;
}

static int queue_pop_locked(queue_t *q, void **data, int block) {
  pthread_mutex_lock(&q->lock);
  while (q->count == 0 && !atomic_load(&q->destroyed)) {
    if (!block) {
      pthread_mutex_unlock(&q->lock);
      return -1;
    }
    pthread_cond_wait(&q->not_empty, &q->lock);
  }
  if (q->count == 0 && atomic_load(&q->destroyed)) {
    pthread_mutex_unlock(&q->lock);
    return -1;
  }
  *data = q->buffer[q->head];
  q->head = (q->head + 1) % q->capacity;
  q->count--;
  pthread_cond_signal(&q->not_full);
  pthread_mutex_unlock(&q->lock);
  return 0;
}

int queue_push(queue_t *q, void *data) {
  return queue_push_locked(q, data, 1);
}

int queue_pop(queue_t *q, void **data) {
  return queue_pop_locked(q, data, 1);
}

int queue_try_push(queue_t *q, void *data) {
  return queue_push_locked(q, data, 0);
}

int queue_try_pop(queue_t *q, void **data) {
  return queue_pop_locked(q, data, 0);
}

size_t queue_size(queue_t *q) {
  pthread_mutex_lock(&q->lock);
  size_t sz = q->count;
  pthread_mutex_unlock(&q->lock);
  return sz;
}

int queue_is_empty(queue_t *q) {
  pthread_mutex_lock(&q->lock);
  int empty = (q->count == 0);
  pthread_mutex_unlock(&q->lock);
  return empty;
}
