#ifndef _WORKER_H_
#define _WORKER_H_

#include "clausedb.h"
#include "azudici.h"
#include "pthread.h"

typedef struct Worker {
  int id;
  AzuDICI * solver;
  pthread_t thread;
  int res, fixed;
} Worker;

/*Global structures for solving*/
static ts_vec_t(Worker) workers;



#endif
