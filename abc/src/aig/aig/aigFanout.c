/**CFile****************************************************************

  FileName    [aigFanout.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Fanout manipulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigFanout.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


// 0: first iFan
// 1: prev iFan0
// 2: prev iFan1
// 3: next iFan0
// 4: next iFan1

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int   Aig_FanoutCreate( int FanId, int Num )    { assert( Num < 2 ); return (FanId << 1) | Num;   } 
static inline int * Aig_FanoutObj( int * pData, int ObjId )   { return pData + 5*ObjId;                         }
static inline int * Aig_FanoutPrev( int * pData, int iFan )   { return pData + 5*(iFan >> 1) + 1 + (iFan & 1);  }
static inline int * Aig_FanoutNext( int * pData, int iFan )   { return pData + 5*(iFan >> 1) + 3 + (iFan & 1);  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create fanout for all objects in the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManFanoutStart( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    assert( Aig_ManBufNum(p) == 0 );
    // allocate fanout datastructure
    assert( p->pFanData == NULL );
    p->nFansAlloc = 2 * Aig_ManObjNumMax(p);
    if ( p->nFansAlloc < (1<<12) )
        p->nFansAlloc = (1<<12);
    p->pFanData = ABC_ALLOC( int, 5 * p->nFansAlloc );
    memset( p->pFanData, 0, sizeof(int) * 5 * p->nFansAlloc );
    // add fanouts for all objects
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjChild0(pObj) )
            Aig_ObjAddFanout( p, Aig_ObjFanin0(pObj), pObj );
        if ( Aig_ObjChild1(pObj) )
            Aig_ObjAddFanout( p, Aig_ObjFanin1(pObj), pObj );
    }
}

/**Function*************************************************************

  Synopsis    [Deletes fanout for all objects in the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManFanoutStop( Aig_Man_t * p )
{
    assert( p->pFanData != NULL );
    ABC_FREE( p->pFanData );
    p->nFansAlloc = 0;
}

/**Function*************************************************************

  Synopsis    [Adds fanout (pFanout) of node (pObj).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjAddFanout( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFanout )
{
    int iFan, * pFirst, * pPrevC, * pNextC, * pPrev, * pNext;
    assert( p->pFanData );
    assert( !Aig_IsComplement(pObj) && !Aig_IsComplement(pFanout) );
    assert( pFanout->Id > 0 );
    if ( pObj->Id >= p->nFansAlloc || pFanout->Id >= p->nFansAlloc )
    {
        int nFansAlloc = 2 * Abc_MaxInt( pObj->Id, pFanout->Id ); 
        p->pFanData = ABC_REALLOC( int, p->pFanData, 5 * nFansAlloc );
        memset( p->pFanData + 5 * p->nFansAlloc, 0, sizeof(int) * 5 * (nFansAlloc - p->nFansAlloc) );
        p->nFansAlloc = nFansAlloc;
    }
    assert( pObj->Id < p->nFansAlloc && pFanout->Id < p->nFansAlloc );
    iFan   = Aig_FanoutCreate( pFanout->Id, Aig_ObjWhatFanin(pFanout, pObj) );
    pPrevC = Aig_FanoutPrev( p->pFanData, iFan );
    pNextC = Aig_FanoutNext( p->pFanData, iFan );
    pFirst = Aig_FanoutObj( p->pFanData, pObj->Id );
    if ( *pFirst == 0 )
    {
        *pFirst = iFan;
        *pPrevC = iFan;
        *pNextC = iFan;
    }
    else
    {
        pPrev = Aig_FanoutPrev( p->pFanData, *pFirst );
        pNext = Aig_FanoutNext( p->pFanData, *pPrev );
        assert( *pNext == *pFirst );
        *pPrevC = *pPrev;
        *pNextC = *pFirst;
        *pPrev  = iFan;
        *pNext  = iFan;
    }
}

/**Function*************************************************************

  Synopsis    [Removes fanout (pFanout) of node (pObj).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjRemoveFanout( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFanout )
{
    int iFan, * pFirst, * pPrevC, * pNextC, * pPrev, * pNext;
    assert( p->pFanData && pObj->Id < p->nFansAlloc && pFanout->Id < p->nFansAlloc );
    assert( !Aig_IsComplement(pObj) && !Aig_IsComplement(pFanout) );
    assert( pFanout->Id > 0 );
    iFan   = Aig_FanoutCreate( pFanout->Id, Aig_ObjWhatFanin(pFanout, pObj) );
    pPrevC = Aig_FanoutPrev( p->pFanData, iFan );
    pNextC = Aig_FanoutNext( p->pFanData, iFan );
    pPrev  = Aig_FanoutPrev( p->pFanData, *pNextC );
    pNext  = Aig_FanoutNext( p->pFanData, *pPrevC );
    assert( *pPrev == iFan );
    assert( *pNext == iFan );
    pFirst = Aig_FanoutObj( p->pFanData, pObj->Id );
    assert( *pFirst > 0 );
    if ( *pFirst == iFan )
    {
        if ( *pNextC == iFan )
        {
            *pFirst = 0;
            *pPrev  = 0;
            *pNext  = 0;
            *pPrevC = 0;
            *pNextC = 0;
            return;
        }
        *pFirst = *pNextC;
    }
    *pPrev  = *pPrevC;
    *pNext  = *pNextC;
    *pPrevC = 0;
    *pNextC = 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

