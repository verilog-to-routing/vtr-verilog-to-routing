/**CFile****************************************************************

  FileName    [satTruth.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT solver.]

  Synopsis    [Truth table computation package.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satTruth.h,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__sat__bsat__satTruth_h
#define ABC__sat__bsat__satTruth_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "misc/util/abc_global.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Tru_Man_t_ Tru_Man_t;

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

static inline int    Tru_ManEqual( word * pOut, word * pIn, int nWords )  { int w; for ( w = 0; w < nWords; w++ ) if(pOut[w]!=pIn[w])  return 0; return 1; }
static inline int    Tru_ManEqual0( word * pOut, int nWords )             { int w; for ( w = 0; w < nWords; w++ ) if(pOut[w]!=0)       return 0; return 1; }
static inline int    Tru_ManEqual1( word * pOut, int nWords )             { int w; for ( w = 0; w < nWords; w++ ) if(pOut[w]!=~(word)0)return 0; return 1; }
static inline word * Tru_ManCopy( word * pOut, word * pIn, int nWords )   { int w; for ( w = 0; w < nWords; w++ ) pOut[w] =   pIn[w];  return pOut;        }
static inline word * Tru_ManClear( word * pOut, int nWords )              { int w; for ( w = 0; w < nWords; w++ ) pOut[w] = 0;         return pOut;        }
static inline word * Tru_ManFill( word * pOut, int nWords )               { int w; for ( w = 0; w < nWords; w++ ) pOut[w] = ~(word)0;  return pOut;        }
static inline word * Tru_ManNot( word * pOut, int nWords )                { int w; for ( w = 0; w < nWords; w++ ) pOut[w] = ~pOut[w];  return pOut;        }
static inline word * Tru_ManAnd( word * pOut, word * pIn, int nWords )    { int w; for ( w = 0; w < nWords; w++ ) pOut[w] &=  pIn[w];  return pOut;        }
static inline word * Tru_ManOr( word * pOut, word * pIn, int nWords )     { int w; for ( w = 0; w < nWords; w++ ) pOut[w] |=  pIn[w];  return pOut;        }
static inline word * Tru_ManCopyNot( word * pOut, word * pIn, int nWords ){ int w; for ( w = 0; w < nWords; w++ ) pOut[w] =  ~pIn[w];  return pOut;        }
static inline word * Tru_ManAndNot( word * pOut, word * pIn, int nWords ) { int w; for ( w = 0; w < nWords; w++ ) pOut[w] &= ~pIn[w];  return pOut;        }
static inline word * Tru_ManOrNot( word * pOut, word * pIn, int nWords )  { int w; for ( w = 0; w < nWords; w++ ) pOut[w] |= ~pIn[w];  return pOut;        }
static inline word * Tru_ManCopyNotCond( word * pOut, word * pIn, int nWords, int fCompl ){ return fCompl ? Tru_ManCopyNot(pOut, pIn, nWords) : Tru_ManCopy(pOut, pIn, nWords); }
static inline word * Tru_ManAndNotCond( word * pOut, word * pIn, int nWords, int fCompl ) { return fCompl ? Tru_ManAndNot(pOut, pIn, nWords)  : Tru_ManAnd(pOut, pIn, nWords);  }
static inline word * Tru_ManOrNotCond( word * pOut, word * pIn, int nWords, int fCompl )  { return fCompl ? Tru_ManOrNot(pOut, pIn, nWords)   : Tru_ManOr(pOut, pIn, nWords);   }


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

extern Tru_Man_t *   Tru_ManAlloc( int nVars );
extern void          Tru_ManFree( Tru_Man_t * p );
extern word *        Tru_ManVar( Tru_Man_t * p, int v );
extern word *        Tru_ManFunc( Tru_Man_t * p, int h );
extern int           Tru_ManInsert( Tru_Man_t * p, word * pTruth );
//extern int           Tru_ManHandleMax( Tru_Man_t * p );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

