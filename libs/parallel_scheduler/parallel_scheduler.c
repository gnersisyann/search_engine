#include "parallel_scheduler.h"

void *worker(void *arg) {
  parallel_scheduler *sched = (parallel_scheduler *)arg;
  while (1) {
    pthread_mutex_lock(&sched->mutex);

    while (sched->task_queue_head == NULL && !sched->stopped) {
      pthread_cond_wait(&sched->cond, &sched->mutex);
    }

    if (sched->stopped && sched->task_queue_head == NULL) {
      pthread_mutex_unlock(&sched->mutex);
      break;
    }

    task_node *node = sched->task_queue_head;
    sched->task_queue_head = sched->task_queue_head->next;
    pthread_mutex_unlock(&sched->mutex);

    if (node->func)
      node->func(node->arg);

    free(node);
  }
  return NULL;
}

parallel_scheduler *parallel_scheduler_create(size_t capacity) {
  parallel_scheduler *sched =
      (parallel_scheduler *)malloc(sizeof(parallel_scheduler));
  sched->capacity = capacity;
  sched->threads = (pthread_t *)malloc(capacity * sizeof(pthread_t));
  sched->task_queue_head = NULL;
  pthread_mutex_init(&sched->mutex, NULL);
  pthread_cond_init(&sched->cond, NULL);
  sched->stopped = 0;

  for (size_t i = 0; i < capacity; i++)
    pthread_create(&sched->threads[i], NULL, worker, sched);

  return sched;
}

void parallel_scheduler_run(parallel_scheduler *sched, task_func func,
                            void *arg) {
  task_node *node = (task_node *)malloc(sizeof(task_node));
  node->func = func;
  node->arg = arg;
  node->next = NULL;

  pthread_mutex_lock(&sched->mutex);
  if (sched->task_queue_head == NULL) {
    sched->task_queue_head = node;
  } else {
    task_node *current = sched->task_queue_head;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = node;
  }
  pthread_cond_signal(&sched->cond);
  pthread_mutex_unlock(&sched->mutex);
}

void parallel_scheduler_destroy(parallel_scheduler *sched) {
  pthread_mutex_lock(&sched->mutex);
  sched->stopped = 1;
  pthread_cond_broadcast(&sched->cond);
  pthread_mutex_unlock(&sched->mutex);

  for (size_t i = 0; i < sched->capacity; i++)
    pthread_join(sched->threads[i], NULL);

  free(sched->threads);

  task_node *current = sched->task_queue_head;
  while (current != NULL) {
    task_node *next = current->next;
    free(current);
    current = next;
  }

  pthread_mutex_destroy(&sched->mutex);
  pthread_cond_destroy(&sched->cond);

  free(sched);
}