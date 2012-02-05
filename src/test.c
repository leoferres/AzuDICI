// -*- compile-command: "gcc -Wall -std=c99 test.c -lm -o test" -*-
/*******************************************************************************
 *    This program by L A Ferres is in the public domain and freely copyable.
 *    (or, if there were problems, in the errata  --- see
 *       http://www.udec.cl/~leo/programs.html)
 *    If you find any bugs, please report them immediately to: leo@inf.udec.cl
 *******************************************************************************
 *
 * FILE NAME: test.c
 *
 * PURPOSE: 
 *
 * Creation date: 2012-01-25-11:59; Modified: 
 *
 *******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include "clause.h"

int main(int argc, char *argv[])
{
  printf ("Bool: %lu\n",sizeof(bool));
  printf ("TClause: %lu\n",sizeof(TClause));
  printf ("NClause: %lu\n",sizeof(NClause));
  printf ("ThNClause: %lu\n",sizeof(ThNClause));
  return 0;
}
