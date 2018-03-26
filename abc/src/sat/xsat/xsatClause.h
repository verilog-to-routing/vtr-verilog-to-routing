/**CFile****************************************************************

  FileName    [xsatClause.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [xSAT - A SAT solver written in C.
               Read the license file for more info.]

  Synopsis    [Clause data type definition.]

  Author      [Bruno Schmitt <boschmitt@inf.ufrgs.br>]

  Affiliation [UC Berkeley / UFRGS]

  Date        [Ver. 1.0. Started - November 10, 2016.]

  Revision    []

***********************************************************************/
#ifndef ABC__sat__xSAT__xsatClause_h
#define ABC__sat__xSAT__xsatClause_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include "misc/util/abc_global.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
typedef struct xSAT_Clause_t_ xSAT_Clause_t;
struct xSAT_Clause_t_
{
    unsigned fLearnt   :  1;
    unsigned fMark     :  1;
    unsigned fReallocd :  1;
    unsigned fCanBeDel :  1;
    unsigned nLBD      : 28;
    int nSize;
    union {
        int Lit;
        unsigned Act;
    } pData[0];
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////
/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int xSAT_ClauseCompare( const void * p1, const void * p2 )
{
    xSAT_Clause_t * pC1 = ( xSAT_Clause_t * ) p1;
    xSAT_Clause_t * pC2 = ( xSAT_Clause_t * ) p2;

    if ( pC1->nSize > 2 && pC2->nSize == 2 )
        return 1;
    if ( pC1->nSize == 2 && pC2->nSize > 2 )
        return 0;
    if ( pC1->nSize == 2 && pC2->nSize == 2 )
        return 0;

    if ( pC1->nLBD > pC2->nLBD )
        return 1;
    if ( pC1->nLBD < pC2->nLBD )
        return 0;

    return pC1->pData[pC1->nSize].Act < pC2->pData[pC2->nSize].Act;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void xSAT_ClausePrint( xSAT_Clause_t * pCla )
{
    int i;

    printf("{ ");
    for ( i = 0; i < pCla->nSize; i++ )
        printf("%d ", pCla->pData[i].Lit );
    printf("}\n");
}

ABC_NAMESPACE_HEADER_END

#endif
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
