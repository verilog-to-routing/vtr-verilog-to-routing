/**CFile****************************************************************

  FileName    [cswMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cut sweeping.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 11, 2007.]

  Revision    [$Id: cswMan.c,v 1.00 2007/07/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cswInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the cut sweeping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Csw_Man_t * Csw_ManStart( Aig_Man_t * pMan, int nCutsMax, int nLeafMax, int fVerbose )
{
    Csw_Man_t * p;
    Aig_Obj_t * pObj;
    int i;
    assert( nCutsMax >= 2  );
    assert( nLeafMax <= 16 );
    // allocate the fraiging manager
    p = ABC_ALLOC( Csw_Man_t, 1 );
    memset( p, 0, sizeof(Csw_Man_t) );
    p->nCutsMax = nCutsMax;
    p->nLeafMax = nLeafMax;
    p->fVerbose = fVerbose;
    p->pManAig  = pMan;
    // create the new manager
    p->pManRes  = Aig_ManStartFrom( pMan );
    assert( Aig_ManCiNum(p->pManAig) == Aig_ManCiNum(p->pManRes) );
    // allocate room for cuts and equivalent nodes
    p->pnRefs   = ABC_ALLOC( int, Aig_ManObjNumMax(pMan) );
    p->pEquiv   = ABC_ALLOC( Aig_Obj_t *, Aig_ManObjNumMax(pMan) );
    p->pCuts    = ABC_ALLOC( Csw_Cut_t *, Aig_ManObjNumMax(pMan) );
    memset( p->pCuts, 0, sizeof(Aig_Obj_t *) * Aig_ManObjNumMax(pMan) );
    memset( p->pnRefs, 0, sizeof(int) * Aig_ManObjNumMax(pMan) );
    // allocate memory manager
    p->nTruthWords = Abc_TruthWordNum(nLeafMax);
    p->nCutSize = sizeof(Csw_Cut_t) + sizeof(int) * nLeafMax + sizeof(unsigned) * p->nTruthWords;
    p->pMemCuts = Aig_MmFixedStart( p->nCutSize * p->nCutsMax, 512 );
    // allocate hash table for cuts
    p->nTableSize = Abc_PrimeCudd( Aig_ManNodeNum(pMan) * p->nCutsMax / 2 );
    p->pTable = ABC_ALLOC( Csw_Cut_t *, p->nTableSize );
    memset( p->pTable, 0, sizeof(Aig_Obj_t *) * p->nTableSize );
    // set the pointers to the available fraig nodes
    Csw_ObjSetEquiv( p, Aig_ManConst1(p->pManAig), Aig_ManConst1(p->pManRes) );
    Aig_ManForEachCi( p->pManAig, pObj, i )
        Csw_ObjSetEquiv( p, pObj, Aig_ManCi(p->pManRes, i) );
    // room for temporary truth tables
    p->puTemp[0] = ABC_ALLOC( unsigned, 4 * p->nTruthWords );
    p->puTemp[1] = p->puTemp[0] + p->nTruthWords;
    p->puTemp[2] = p->puTemp[1] + p->nTruthWords;
    p->puTemp[3] = p->puTemp[2] + p->nTruthWords;
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Csw_ManStop( Csw_Man_t * p )
{
    if ( p->fVerbose )
    {
        int nNodesBeg = Aig_ManNodeNum(p->pManAig);
        int nNodesEnd = Aig_ManNodeNum(p->pManRes);
        printf( "Beg = %7d.  End = %7d.  (%6.2f %%)  Try = %7d.  Cuts = %8d.\n", 
            nNodesBeg, nNodesEnd, 100.0*(nNodesBeg-nNodesEnd)/nNodesBeg,
            p->nNodesTried, Csw_TableCountCuts( p ) );
        printf( "Triv0 = %6d.  Triv1 = %6d.  Triv2 = %6d.  Cut-replace = %6d.\n", 
            p->nNodesTriv0, p->nNodesTriv1, p->nNodesTriv2, p->nNodesCuts );
        ABC_PRTP( "Cuts    ", p->timeCuts,     p->timeTotal );
        ABC_PRTP( "Hashing ", p->timeHash,     p->timeTotal );
        ABC_PRTP( "Other   ", p->timeOther,    p->timeTotal );
        ABC_PRTP( "TOTAL   ", p->timeTotal,    p->timeTotal );
    }
    ABC_FREE( p->puTemp[0] );
    Aig_MmFixedStop( p->pMemCuts, 0 );
    ABC_FREE( p->pnRefs );
    ABC_FREE( p->pEquiv );
    ABC_FREE( p->pCuts );
    ABC_FREE( p->pTable );
    ABC_FREE( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

