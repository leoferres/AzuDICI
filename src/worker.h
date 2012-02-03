#ifndef _WORKER_H_
#define _WORKER_H_

#include "clausedb.h"
#include "azudici.h"
#include "pthread.h"

/*Global structures for solving*/
static ts_vec_t(Worker) workers;
static pthread_mutex_t donemutex = PTHREAD_MUTEX_INITIALIZER;
static int done;

typedef struct Worker {
  int id;
  AzuDICI * solver;
  pthread_t thread;
  int res, fixed;
} Worker;

#endif
