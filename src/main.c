/*******************************************************************************
 *
 * FILE NAME: main.c
 *
 * PURPOSE: Entry point for AzuDICI
 *
 * CREATION DATE: 2012-01-26-19:55; MODIFIED: 
 *
 *******************************************************************************/

#include <stdio.h>
#include "version.h"
#include "worker.h"
#include "parser.c"

char* usage="Usage: ad <dimacs file>";

static void * work (void * voidptr) {
  Worker *worker = voidptr;
  int wid = worker->id;
  dassert (0 <= wid && wid < nworkers);
  //msg (wid, 1, "running");
  dassert ( &ts_vec_ith(workers,wid) == worker );
 
  worker->res = azuDICI_solve ( worker->solver );


  // msg (wid, 1, "result %d", worker->res);
  if (pthread_mutex_lock (&donemutex))
    printf ("failed to lock 'done' mutex in worker");
  done = 1;
  if (pthread_mutex_unlock (&donemutex)) 
    printf ("failed to unlock 'done' mutex in worker");

  return worker->res ? worker : 0;
}

int main (int argc, char *argv[]) {

  printf ("This is the AzuDICI SAT solver, version %d.%d\n",
	  MAJOR_VERSION,
	  MINOR_VERSION);

  printf ("(c) 2012 R. Asin, L. Ferres, and J. Olate (alphabetical!)\n");

  printf ("To report errors: <leoferres@gmail.com>\n");

  if (argc!=2) {
    printf ("%s\n",usage);
    return -1;
    }

  char* inputFileName;
  unsigned int nThreads;
  /*

    parse parameters


   */

  unsigned int nVars;
  unsigned int nClauses;
  gzFile in;

  /*Read number of vars and clauses from the input CNF file*/
  in = gzopen(inputFileName, "rb");
  input_read_header(in, &nVars, &nClauses);
  gzclose(in);
  /**********************************/

  /*Initialize Clause DataBase and read clauses*/
  ClauseDB* cdb = init_clause_database(nVars);
  input_read_clauses(cdb, inputFileName);
  /***************************************/

  /*Initialize workers and assign each thread a new AzuDICI solver*/
  ts_vec_init(workers);
  ts_vec_resize(Worker,workers,nworkers);

  Worker *w;
  for (int i = 0; i < nworkers; i++) {
    ts_vec_ith_ma(w,workers,i);
    w->id=i;
    w->solver = azuDICI_init (cdb);
  }
  /**************/

  /*Call solving in each thread*/
  for (int i = 0; i < nworkers; i++) {
    ts_vec_ith_ma(w,workers,i);
    if ( pthread_create (&(w->thread), 0, work, (void*)w) ){
      printf("failed to create worker thread %d", i);
      exit(-1);
    }
    //    msg (-1, 2, "started worker %d", i);
  }
  /**************************/


  return 0;
}
