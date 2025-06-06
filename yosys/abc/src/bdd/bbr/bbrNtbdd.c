/**CFile****************************************************************

  FileName    [bbrNtbdd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD-based reachability analysis.]

  Synopsis    [Procedures to construct global BDDs for the network.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bbrNtbdd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bbr.h"
//#include "bar.h"

ABC_NAMESPACE_IMPL_START


typedef char ProgressBar;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline void     Aig_ObjSetGlobalBdd( Aig_Obj_t * pObj, DdNode * bFunc )   { pObj->pData = bFunc; }
static inline void     Aig_ObjCleanGlobalBdd( DdManager * dd, Aig_Obj_t * pObj ) { Cudd_RecursiveDeref( dd, (DdNode *)pObj->pData ); pObj->pData = NULL; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the global BDD for one AIG node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Bbr_NodeGlobalBdds_rec( DdManager * dd, Aig_Obj_t * pNode, int nBddSizeMax, int fDropInternal, ProgressBar * pProgress, int * pCounter, int fVerbose )
{
    DdNode * bFunc, * bFunc0, * bFunc1;
    assert( !Aig_IsComplement(pNode) );
    if ( Cudd_ReadKeys(dd)-Cudd_ReadDead(dd) > (unsigned)nBddSizeMax )
    {
//        Extra_ProgressBarStop( pProgress );
        if ( fVerbose )
        printf( "The number of live nodes reached %d.\n", nBddSizeMax );
        fflush( stdout );
        return NULL;
    }
    // if the result is available return
    if ( Aig_ObjGlobalBdd(pNode) == NULL )
    {
        // compute the result for both branches
        bFunc0 = Bbr_NodeGlobalBdds_rec( dd, Aig_ObjFanin0(pNode), nBddSizeMax, fDropInternal, pProgress, pCounter, fVerbose ); 
        if ( bFunc0 == NULL )
            return NULL;
        Cudd_Ref( bFunc0 );
        bFunc1 = Bbr_NodeGlobalBdds_rec( dd, Aig_ObjFanin1(pNode), nBddSizeMax, fDropInternal, pProgress, pCounter, fVerbose ); 
        if ( bFunc1 == NULL )
            return NULL;
        Cudd_Ref( bFunc1 );
        bFunc0 = Cudd_NotCond( bFunc0, Aig_ObjFaninC0(pNode) );
        bFunc1 = Cudd_NotCond( bFunc1, Aig_ObjFaninC1(pNode) );
        // get the final result
        bFunc = Cudd_bddAnd( dd, bFunc0, bFunc1 );   Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( dd, bFunc0 );
        Cudd_RecursiveDeref( dd, bFunc1 );
        // add the number of used nodes
        (*pCounter)++;
        // set the result
        assert( Aig_ObjGlobalBdd(pNode) == NULL );
        Aig_ObjSetGlobalBdd( pNode, bFunc );
        // increment the progress bar
//        if ( pProgress )
//            Extra_ProgressBarUpdate( pProgress, *pCounter, NULL );
    }
    // prepare the return value
    bFunc = Aig_ObjGlobalBdd(pNode);
    // dereference BDD at the node
    if ( --pNode->nRefs == 0 && fDropInternal )
    {
        Cudd_Deref( bFunc );
        Aig_ObjSetGlobalBdd( pNode, NULL );
    }
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Frees the global BDDs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManFreeGlobalBdds( Aig_Man_t * p, DdManager * dd ) 
{ 
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p, pObj, i )
        if ( Aig_ObjGlobalBdd(pObj) )
            Aig_ObjCleanGlobalBdd( dd, pObj );
}

/**Function*************************************************************

  Synopsis    [Returns the shared size of global BDDs of the COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManSizeOfGlobalBdds( Aig_Man_t * p ) 
{
    Vec_Ptr_t * vFuncsGlob;
    Aig_Obj_t * pObj;
    int RetValue, i;
    // complement the global functions
    vFuncsGlob = Vec_PtrAlloc( Aig_ManCoNum(p) );
    Aig_ManForEachCo( p, pObj, i )
        Vec_PtrPush( vFuncsGlob, Aig_ObjGlobalBdd(pObj) );
    RetValue = Cudd_SharingSize( (DdNode **)Vec_PtrArray(vFuncsGlob), Vec_PtrSize(vFuncsGlob) );
    Vec_PtrFree( vFuncsGlob );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Recursively computes global BDDs for the AIG in the manager.]

  Description [On exit, BDDs are stored in the pNode->pData fields.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Aig_ManComputeGlobalBdds( Aig_Man_t * p, int nBddSizeMax, int fDropInternal, int fReorder, int fVerbose )
{
    ProgressBar * pProgress = NULL;
    Aig_Obj_t * pObj;
    DdManager * dd;
    DdNode * bFunc;
    int i, Counter;
    // start the manager
    dd = Cudd_Init( Aig_ManCiNum(p), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // set reordering
    if ( fReorder )
        Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
    // prepare to construct global BDDs
    Aig_ManCleanData( p );
    // assign the constant node BDD
    Aig_ObjSetGlobalBdd( Aig_ManConst1(p), dd->one );   Cudd_Ref( dd->one );
    // set the elementary variables
    Aig_ManForEachCi( p, pObj, i )
    {
        Aig_ObjSetGlobalBdd( pObj, dd->vars[i] );  Cudd_Ref( dd->vars[i] );
    }

    // collect the global functions of the COs
    Counter = 0;
    // construct the BDDs
//    pProgress = Extra_ProgressBarStart( stdout, Aig_ManNodeNum(p) );
    Aig_ManForEachCo( p, pObj, i )
    {
        bFunc = Bbr_NodeGlobalBdds_rec( dd, Aig_ObjFanin0(pObj), nBddSizeMax, fDropInternal, pProgress, &Counter, fVerbose );
        if ( bFunc == NULL )
        {
            if ( fVerbose )
            printf( "Constructing global BDDs is aborted.\n" );
            Aig_ManFreeGlobalBdds( p, dd );
            Cudd_Quit( dd ); 
            // reset references
            Aig_ManResetRefs( p );
            return NULL;
        }
        bFunc = Cudd_NotCond( bFunc, Aig_ObjFaninC0(pObj) );  Cudd_Ref( bFunc ); 
        Aig_ObjSetGlobalBdd( pObj, bFunc );
    }
//    Extra_ProgressBarStop( pProgress );
    // reset references
    Aig_ManResetRefs( p );
    // reorder one more time
    if ( fReorder )
    {
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
        Cudd_AutodynDisable( dd );
    }
//    Cudd_PrintInfo( dd, stdout );
    return dd;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

