/*******************************************************************************
 *
 * FILE NAME: main.c
 *
 * PURPOSE: Entry point for AzuDICI
 *
 * CREATION DATE: 2012-01-26-19:55; MODIFIED: 
 *
 *******************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include "version.h"
#include "worker.h"
#include "parser.c"


char* usage="Usage: ad <dimacs file>";
unsigned int nworkers; //the number of Threads

static void * work (void * voidptr) {
  Worker *worker = (Worker *)voidptr;
  int wid = worker->id;
  dassert (0 <= wid && wid < nworkers);
  //msg (wid, 1, "running");
  Worker *dassertW;
  ts_vec_ith_ma(dassertW,workers,wid);
  dassert ( dassertW == worker ); 
 
  worker->res = azuDICI_solve ( worker->solver );

  if(worker->res == 10){
    printf("SAT\n");
  }else if(worker->res==20){
    printf("UNSAT\n");
  }

 printf("|%*d|%*lu|%*lu|%*d|%*d|%*d|%*d|%*d|%*d|%*d|%*d|%*d|\n", 
	 3,worker->solver->wId, 15,worker->solver->stats.numDecisions, 
	 15,worker->solver->stats.numProps, 10,worker->solver->stats.numConflicts, 
	 10,worker->solver->lastUnitAdded, 10,worker->solver->lastBinaryAdded, 
	 10,worker->solver->lastNaryAdded,
	 10,0, 10,0, 10,0, 10,0, 10,0);

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
  
  if (argc<2) {
    printf ("%s\n",usage);
    //return -1;
    }

  char* inputFileName;
  
  
/* Starting parse parameters */

 int c;
 opterr = 0;
  while ((c = getopt(argc, argv, "t:f:")) != -1){
        switch (c) {
            case 't':
                nworkers = optarg[0] - '0';
                break;
            case 'f':
                inputFileName = optarg;
                break;
            case '?':
                if (optopt == 'c')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
                return 1;
            default:
                abort();
        }
  }

  /*Ending parse parameters*/
  
  //inputFileName="../../cores-benchmarks/hoons-vbmc-s04-06.cnf.gz";
  //nworkers = 1;
  unsigned int nVars;
  unsigned int nClauses;
  gzFile in;

  /*Read number of vars and clauses from the input CNF file*/
  in = gzopen(inputFileName, "rb");
  input_read_header(in, &nVars, &nClauses);
  printf("Header read with nVars %d and nClauses %d\n",nVars, nClauses);
  gzclose(in);
  /**********************************/

  /*Initialize Clause DataBase and read clauses*/
  printf("About to init clause deb\n");
  ClauseDB* cdb = init_clause_database(nVars,nworkers);
  printf("Clause db initialized\n");
  input_read_clauses(cdb, inputFileName);
  printf("Clause db loaded\n");
  /***************************************/

  /*Initialize workers and assign each thread a new AzuDICI solver*/
  ts_vec_init(workers);
  ts_vec_resize(Worker,workers,nworkers);

  Worker *w;
  for (int i = 0; i < nworkers; i++) {
    ts_vec_ith_ma(w,workers,i);
    w->id=i;
    w->solver = azuDICI_init (cdb,w->id);
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

      printf("|                                   GENERAL STATS                               |              CLEANUP STATS                           |\n");
      printf("|WID|   nDecisions  |     nProps    |nConflicts|   units  |    bins  | nary_Cls | newUnits |  newBins |delTrueCls|   nDels  | nRealDels|\n");

  for(int i=0;i<nworkers;i++){
      ts_vec_ith_ma(w,workers,i);
      pthread_join(w->thread, NULL);
  }
  
  return 0;
}
