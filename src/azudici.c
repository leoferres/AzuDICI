#ifndef _AZUDICI_C_
#define _AZUDICI_C_

#include "clausedb.h"
#include "azudici.h"

AzuDICI* azuDICI_init(ClauseDB* generalClauseDB){
  int i;

  AzuDICI *ad = (AzuDICI*)malloc(sizeof(AzuDICI));
  ad->cdb                = generalClauseDB;
  ad->lastUnitAdded      = ts_vec_size( db->uDB );
  ad->lastNaryAdded      = ts_vec_size( db->nDB );
  ad->randomNumberIndex  = 0;

  vec_init( lastBinariesAdded, 2*(ad->cdb->numVars+1) );
  int listSize;
  for( i=0;i < 2*(ad->cdb->numVars + 1) ; i++ ){
    ts_vec_size( listSize, vec_ith(ad->cdb->bDB, i ) );
    vec_set_ith(ad->lastBinariesAdded, i, listSize-1 );
  }
  return &ad;
}

#endif
