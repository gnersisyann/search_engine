#ifndef PARALLEL_SCHEDULER_H
#define PARALLEL_SCHEDULER_H
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef void (*task_func)(void *);

typedef struct task_node {
  task_func func;
  struct task_node *next;
  void *arg;
} task_node;

typedef struct {
  pthread_t *threads;
  size_t capacity;
  task_node *task_queue_head;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int stopped;
} parallel_scheduler;

void *worker(void *arg);
parallel_scheduler *parallel_scheduler_create(size_t capacity);
void parallel_scheduler_run(parallel_scheduler *sched, task_func func,
                            void *arg);
void parallel_scheduler_destroy(parallel_scheduler *sched);
#endif