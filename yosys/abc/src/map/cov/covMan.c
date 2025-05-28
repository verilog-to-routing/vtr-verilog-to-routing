/**CFile****************************************************************

  FileName    [covMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Mapping into network of SOPs/ESOPs.]

  Synopsis    [Decomposition manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: covMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cov.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cov_Man_t * Cov_ManAlloc( Abc_Ntk_t * pNtk, int nFaninMax, int nCubesMax )
{
    Cov_Man_t * pMan;
    Cov_Obj_t * pMem;
    Abc_Obj_t * pObj;
    int i;
    assert( pNtk->pManCut == NULL );

    // start the manager
    pMan = ABC_ALLOC( Cov_Man_t, 1 );
    memset( pMan, 0, sizeof(Cov_Man_t) );
    pMan->nFaninMax = nFaninMax;
    pMan->nCubesMax = nCubesMax;
    pMan->nWords    = Abc_BitWordNum( nFaninMax * 2 );

    // get the cubes
    pMan->vComTo0 = Vec_IntAlloc( 2*nFaninMax );
    pMan->vComTo1 = Vec_IntAlloc( 2*nFaninMax );
    pMan->vPairs0 = Vec_IntAlloc( nFaninMax );
    pMan->vPairs1 = Vec_IntAlloc( nFaninMax );
    pMan->vTriv0  = Vec_IntAlloc( 1 );  Vec_IntPush( pMan->vTriv0, -1 ); 
    pMan->vTriv1  = Vec_IntAlloc( 1 );  Vec_IntPush( pMan->vTriv1, -1 ); 

    // allocate memory for object structures
    pMan->pMemory = pMem = ABC_ALLOC( Cov_Obj_t, sizeof(Cov_Obj_t) * Abc_NtkObjNumMax(pNtk) );
    memset( pMem, 0, sizeof(Cov_Obj_t) * Abc_NtkObjNumMax(pNtk) );
    // allocate storage for the pointers to the memory
    pMan->vObjStrs = Vec_PtrAlloc( Abc_NtkObjNumMax(pNtk) );
    Vec_PtrFill( pMan->vObjStrs, Abc_NtkObjNumMax(pNtk), NULL );
    Abc_NtkForEachObj( pNtk, pObj, i )
        Vec_PtrWriteEntry( pMan->vObjStrs, i, pMem + i );
    // create the cube manager
    pMan->pManMin = Min_ManAlloc( nFaninMax );
    return pMan;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cov_ManFree( Cov_Man_t * p )
{
    Vec_Int_t * vSupp;
    int i;
    for ( i = 0; i < p->vObjStrs->nSize; i++ )
    {
        vSupp = ((Cov_Obj_t *)p->vObjStrs->pArray[i])->vSupp;
        if ( vSupp ) Vec_IntFree( vSupp );
    }

    Min_ManFree( p->pManMin );
    Vec_PtrFree( p->vObjStrs );
    Vec_IntFree( p->vFanCounts );
    Vec_IntFree( p->vTriv0 );
    Vec_IntFree( p->vTriv1 );
    Vec_IntFree( p->vComTo0 );
    Vec_IntFree( p->vComTo1 );
    Vec_IntFree( p->vPairs0 );
    Vec_IntFree( p->vPairs1 );
    ABC_FREE( p->pMemory );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Drop the covers at the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeCovDropData( Cov_Man_t * p, Abc_Obj_t * pObj )
{
    int nFanouts;
    assert( p->vFanCounts );
    nFanouts = Vec_IntEntry( p->vFanCounts, pObj->Id );
    assert( nFanouts > 0 );
    if ( --nFanouts == 0 )
    {
        Vec_IntFree( Abc_ObjGetSupp(pObj) );
        Abc_ObjSetSupp( pObj, NULL );
        Min_CoverRecycle( p->pManMin, Abc_ObjGetCover2(pObj) );
        Abc_ObjSetCover2( pObj, NULL );
        p->nSupps--;
    }
    Vec_IntWriteEntry( p->vFanCounts, pObj->Id, nFanouts );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

