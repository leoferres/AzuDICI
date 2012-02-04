#ifndef _PARSER_C_
#define _PARSER_C_

#include <zlib.h>
#include "clausedb.h"

#define getNext() \
  {i++;\
  if (i >= size) {\
    i  = 0;\
    size = gzread(in, buf, sizeof(buf));\
    if (!size) c=0; else c = buf[i];\
  }\
  else c = buf[i];}

void input_read_header(gzFile in, unsigned int *numVars, unsigned int *numInputClauses)
{
  char c; unsigned int n=0;
  if (in == NULL){printf("No file \n"); exit(1);}
  c=gzgetc(in); 
  while(c=='c') {while (c!='\n') c=gzgetc(in); c=gzgetc(in);} //skip comments
  if (c!='p') { printf("no line starting with p.\n"); exit(3);} c=gzgetc(in);
  while ((c>=9 && c<=13)||c==32) c=gzgetc(in);               //skip white space
  c=gzgetc(in); c=gzgetc(in); c=gzgetc(in);                  //skip "cnf"
  while ((c>=9 && c<=13)||c==32) c=gzgetc(in);               //skip white space
  while (c>='0' && c<='9'){ n=n*10+(c-'0'); c=gzgetc(in); } *numVars=n; n=0;
  while ((c>=9 && c<=13)||c==32) c=gzgetc(in);               //skip white space
  while (c>='0' && c<='9'){ n=n*10+(c-'0'); c=gzgetc(in); } *numInputClauses=n;
}


void input_read_clauses (ClauseDB* cdb, char* inputFileName){

  gzFile in;
  char  buf[4096];                          // used by getNext
  unsigned int  i=0, size=0;                        // used by getNext
  char  c='a';                              // used by getNext
  unsigned int  variable;
  bool isPos;
  unsigned int nVars,nClauses;

  in = gzopen(inputFileName, "rb");
  input_read_header(in, &nVars,&nClauses);
  dassert(nVars==cdb->numVars);
  dassert(nClauses==cdb->numInputClauses);

  bool fileEnd=false;
  while(!fileEnd) { // each iteration of this loop reads one (possibly signed) int
    getNext(); 
    while ((c>=9 && c<=13) || c==32) getNext();  // skip white space   
    if (c == 0){ // end of file
      fileEnd=true;
    }else{     
      isPos = true;
      //at this point, c is either a digit or '-' 
      if (c == '-') { isPos = false; getNext(); }
      //at this point, c is a digit for sure
      variable = 0; 
      while (c>='0' && c<='9') { variable = variable*10+(c-'0'); getNext(); }
      //at this point, variable is a variable and isPos indicates it's polarity.
      //if variable is 0, then that's the end of the clause.
      if(isPos)
	add_input_literal(cdb,variable);
      else
	add_input_literal(cdb,-variable);
    }    
  }

  gzclose(in); 
}

#endif
