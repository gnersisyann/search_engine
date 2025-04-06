#include "parallel_scheduler.h"

static task_node *task_queue = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int stopped = 0;

void *worker(void *arg) {
  (void)arg;
  while (1) {
    pthread_mutex_lock(&mutex);

    while (task_queue == NULL && !stopped) {
      pthread_cond_wait(&cond, &mutex);
    }

    if (stopped && task_queue == NULL) {
      pthread_mutex_unlock(&mutex);
      break;
    }

    task_node *node = task_queue;
    task_queue = task_queue->next;
    pthread_mutex_unlock(&mutex);

    if (node->func)
      node->func(node->arg);

    free(node);
  }
  return NULL;
}

parallel_scheduler *parallel_scheduler_create(size_t capacity) {
  parallel_scheduler *sched = malloc(sizeof(parallel_scheduler));
  sched->capacity = capacity;
  sched->threads = malloc(capacity * sizeof(pthread_t));
  sched->stopped = 0;

  for (size_t i = 0; i < capacity; i++)
    pthread_create(&sched->threads[i], NULL, worker, NULL);

  return sched;
}

void parallel_scheduler_run(parallel_scheduler *sched, task_func func,
                            void *arg) {
  (void)sched;
  task_node *node = malloc(sizeof(task_node));
  node->func = func;
  node->arg = arg;
  node->next = NULL;

  pthread_mutex_lock(&mutex);
  if (task_queue == NULL) {
    task_queue = node;
  } else {
    task_node *current = task_queue;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = node;
  }
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
}

void parallel_scheduler_destroy(parallel_scheduler *sched) {
  pthread_mutex_lock(&mutex);
  stopped = 1;
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);

  for (size_t i = 0; i < sched->capacity; i++)
    pthread_join(sched->threads[i], NULL);

  free(sched->threads);

  task_node *current = task_queue;
  while (current != NULL) {
    task_node *next = current->next;
    free(current);
    current = next;
  }

  free(sched);
}