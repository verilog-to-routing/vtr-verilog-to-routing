/**CFile****************************************************************

  FileName    [giaSupMin.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Support minimization for AIGs with don't-cares.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaSupMin.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// decomposition manager
typedef struct Gia_ManSup_t_ Gia_ManSup_t;
struct Gia_ManSup_t_ 
{
    int             nVarsMax;     // the max number of variables
    int             nWordsMax;    // the max number of words
    Vec_Ptr_t *     vTruthVars;   // elementary truth tables
    Vec_Ptr_t *     vTruthNodes;  // internal truth tables
    // current problem
    Gia_Man_t *     pGia;
    int             iData;
    int             iCare;
    Vec_Int_t *     vConeCare;
    Vec_Int_t *     vConeData;
    unsigned *      pTruthIn;
    unsigned *      pTruthOut;
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts Decmetry manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_ManSup_t * Gia_ManSupStart( int nVarsMax )
{
    Gia_ManSup_t * p;
    assert( nVarsMax <= 20 );
    p = ABC_CALLOC( Gia_ManSup_t, 1 );
    p->nVarsMax    = nVarsMax;
    p->nWordsMax   = Kit_TruthWordNum( p->nVarsMax );
    p->vTruthVars  = Vec_PtrAllocTruthTables( p->nVarsMax );
    p->vTruthNodes = Vec_PtrAllocSimInfo( 512, p->nWordsMax );
    p->vConeCare   = Vec_IntAlloc( 512 );
    p->vConeData   = Vec_IntAlloc( 512 );
    p->pTruthIn    = ABC_ALLOC( unsigned, p->nWordsMax );
    p->pTruthOut   = ABC_ALLOC( unsigned, p->nWordsMax );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops Decmetry manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSupStop( Gia_ManSup_t * p )
{
    ABC_FREE( p->pTruthIn );
    ABC_FREE( p->pTruthOut );
    Vec_IntFreeP( &p->vConeCare );
    Vec_IntFreeP( &p->vConeData );
    Vec_PtrFreeP( &p->vTruthVars );
    Vec_PtrFreeP( &p->vTruthNodes );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSupExperimentOne( Gia_ManSup_t * p, Gia_Obj_t * pData, Gia_Obj_t * pCare )
{
    int iData = Gia_ObjId( p->pGia, Gia_Regular(pData) );
    int iCare = Gia_ObjId( p->pGia, Gia_Regular(pCare) );
    if ( !Gia_ObjIsAnd(Gia_Regular(pCare)) )
    {
        Abc_Print( 1, "Enable is not an AND.\n" );
        return;
    }
    Abc_Print( 1, "DataSupp = %6d. DataCone = %6d.   CareSupp = %6d. CareCone = %6d.", 
        Gia_ManSuppSize( p->pGia, &iData, 1 ),
        Gia_ManConeSize( p->pGia, &iData, 1 ),
        Gia_ManSuppSize( p->pGia, &iCare, 1 ),
        Gia_ManConeSize( p->pGia, &iCare, 1 ) );
    Abc_Print( 1, "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSupExperiment( Gia_Man_t * pGia, Vec_Int_t * vPairs )
{
    Gia_ManSup_t * p;
    Gia_Obj_t * pData, * pCare;
    int i;
    p = Gia_ManSupStart( 16 );
    p->pGia = pGia;
    assert( Vec_IntSize(vPairs) % 2 == 0 );
    for ( i = 0; i < Vec_IntSize(vPairs)/2; i++ )
    {
        Abc_Print( 1, "%6d : ", i );
        pData = Gia_ManPo( pGia, Vec_IntEntry(vPairs, 2*i+0) );
        pCare = Gia_ManPo( pGia, Vec_IntEntry(vPairs, 2*i+1) );
        Gia_ManSupExperimentOne( p, Gia_ObjChild0(pData), Gia_ObjChild0(pCare) );
    }
    Gia_ManSupStop( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

