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

char* usage="Usage: ad <dimacs file>";

int
main(int argc, char *argv[]) {

  printf ("This is the AzuDICI SAT solver, version %d.%d\n",
	  MAJOR_VERSION,
	  MINOR_VERSION);

  printf ("(c) 2012 R. Asin, L. Ferres, and J. Olate (alphabetical!)\n");

  printf ("To report errors: <leoferres@gmail.com>\n");

  if (argc!=2) {
    printf ("%s\n",usage);
    return -1;
    }


  return 0;
}
