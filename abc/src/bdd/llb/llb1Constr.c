/**CFile****************************************************************

  FileName    [llb1Constr.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Computing inductive constraints.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb1Constr.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the array of constraint candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManCountEntries( Vec_Int_t * vCands )
{
    int i, Entry, Counter = 0;
    Vec_IntForEachEntry( vCands, Entry, i )
        Counter += ((Entry == 0) || (Entry == 1));
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the array of constraint candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManPrintEntries( Aig_Man_t * p, Vec_Int_t * vCands )
{
    int i, Entry;
    if ( vCands == NULL )
    {
        printf( "There is no hints.\n" );
        return;
    }
    Entry = Llb_ManCountEntries(vCands);
    printf( "\n*** Using %d hint%s:\n", Entry, (Entry != 1 ? "s":"") );
    Vec_IntForEachEntry( vCands, Entry, i )
    {
        if ( Entry != 0 && Entry != 1 )
            continue;
        printf( "%c", Entry ? '+' : '-' );
        printf( "%-6d : ", i );
        Aig_ObjPrint( p, Aig_ManObj(p, i) );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Dereference BDD nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManDerefenceBdds( Aig_Man_t * p, DdManager * dd )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p, pObj, i )
        Cudd_RecursiveDeref( dd, (DdNode *)pObj->pData );
}

/**Function*************************************************************

  Synopsis    [Returns the array of constraint candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_ManComputeIndCase_rec( Aig_Man_t * p, Aig_Obj_t * pObj, DdManager * dd, Vec_Ptr_t * vBdds )
{
    DdNode * bBdd0, * bBdd1;
    DdNode * bFunc = (DdNode *)Vec_PtrEntry( vBdds, Aig_ObjId(pObj) );
    if ( bFunc != NULL )
        return bFunc;
    assert( Aig_ObjIsNode(pObj) );
    bBdd0 = Llb_ManComputeIndCase_rec( p, Aig_ObjFanin0(pObj), dd, vBdds ); 
    bBdd1 = Llb_ManComputeIndCase_rec( p, Aig_ObjFanin1(pObj), dd, vBdds ); 
    bBdd0 = Cudd_NotCond( bBdd0, Aig_ObjFaninC0(pObj) );
    bBdd1 = Cudd_NotCond( bBdd1, Aig_ObjFaninC1(pObj) );
    bFunc = Cudd_bddAnd( dd, bBdd0, bBdd1 );  Cudd_Ref( bFunc );
    Vec_PtrWriteEntry( vBdds, Aig_ObjId(pObj), bFunc );
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Returns the array of constraint candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManComputeIndCase( Aig_Man_t * p, DdManager * dd, Vec_Int_t * vNodes )
{
    Vec_Ptr_t * vBdds;
    Aig_Obj_t * pObj;
    DdNode * bFunc;
    int i, Entry;
    vBdds = Vec_PtrStart( Aig_ManObjNumMax(p) );
    bFunc = Cudd_ReadOne(dd); Cudd_Ref( bFunc );
    Vec_PtrWriteEntry( vBdds, Aig_ObjId(Aig_ManConst1(p)), bFunc );
    Saig_ManForEachPi( p, pObj, i )
    {
        bFunc = Cudd_bddIthVar( dd, Aig_ManCiNum(p) + i );  Cudd_Ref( bFunc );
        Vec_PtrWriteEntry( vBdds, Aig_ObjId(pObj), bFunc );
    }
    Saig_ManForEachLi( p, pObj, i )
    {
        bFunc = (DdNode *)pObj->pData;  Cudd_Ref( bFunc );
        Vec_PtrWriteEntry( vBdds, Aig_ObjId(Saig_ObjLiToLo(p, pObj)), bFunc );
    }
    Vec_IntForEachEntry( vNodes, Entry, i )
    { 
        if ( Entry != 0 && Entry != 1 )
            continue;
        pObj = Aig_ManObj( p, i );
        bFunc = Llb_ManComputeIndCase_rec( p, pObj, dd, vBdds );
        if ( Entry == 0 )
        {
//            Extra_bddPrint( dd, Cudd_Not(pObj->pData) );  printf( "\n" );
//            Extra_bddPrint( dd, Cudd_Not(bFunc) );        printf( "\n" );
            if ( !Cudd_bddLeq( dd, Cudd_Not(pObj->pData), Cudd_Not(bFunc) ) )
                Vec_IntWriteEntry( vNodes, i, -1 );
        }
        else if ( Entry == 1 )
        {
//            Extra_bddPrint( dd, pObj->pData );  printf( "\n" );
//            Extra_bddPrint( dd, bFunc );        printf( "\n" );
            if ( !Cudd_bddLeq( dd, (DdNode *)pObj->pData, bFunc ) )
                Vec_IntWriteEntry( vNodes, i, -1 );
        }
    }
    Vec_PtrForEachEntry( DdNode *, vBdds, bFunc, i )
        if ( bFunc )
            Cudd_RecursiveDeref( dd, bFunc );
    Vec_PtrFree( vBdds );
}

/**Function*************************************************************

  Synopsis    [Returns the array of constraint candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Llb_ManComputeBaseCase( Aig_Man_t * p, DdManager * dd )
{
    Vec_Int_t * vNodes;
    Aig_Obj_t * pObj, * pRoot;
    int i;
    pRoot = Aig_ManCo( p, 0 );
    vNodes = Vec_IntStartFull( Aig_ManObjNumMax(p) );
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) && !Aig_ObjIsCi(pObj) )
            continue;
        if ( Cudd_bddLeq( dd, (DdNode *)pObj->pData, Cudd_Not(pRoot->pData) ) )
            Vec_IntWriteEntry( vNodes, i, 1 );
        else if ( Cudd_bddLeq( dd, Cudd_Not((DdNode *)pObj->pData), Cudd_Not(pRoot->pData) ) )
            Vec_IntWriteEntry( vNodes, i, 0 );
    }
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Constructs global BDDs for each object in the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Llb_ManConstructGlobalBdds( Aig_Man_t * p )
{
    DdManager * dd;
    DdNode * bBdd0, * bBdd1;
    Aig_Obj_t * pObj;
    int i;
    dd = Cudd_Init( Aig_ManCiNum(p), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );
    pObj = Aig_ManConst1(p);
    pObj->pData = Cudd_ReadOne(dd);  Cudd_Ref( (DdNode *)pObj->pData );
    Aig_ManForEachCi( p, pObj, i )
    {
        pObj->pData = Cudd_bddIthVar(dd, i);  Cudd_Ref( (DdNode *)pObj->pData );
    }
    Aig_ManForEachNode( p, pObj, i )
    {
        bBdd0 = Cudd_NotCond( (DdNode *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
        bBdd1 = Cudd_NotCond( (DdNode *)Aig_ObjFanin1(pObj)->pData, Aig_ObjFaninC1(pObj) );
        pObj->pData = Cudd_bddAnd( dd, bBdd0, bBdd1 );   Cudd_Ref( (DdNode *)pObj->pData );
    }
    Aig_ManForEachCo( p, pObj, i )
    {
        pObj->pData = Cudd_NotCond( (DdNode *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );  Cudd_Ref( (DdNode *)pObj->pData );
    }
    return dd;
}

/**Function*************************************************************

  Synopsis    [Derives inductive constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Llb_ManDeriveConstraints( Aig_Man_t * p )
{
    DdManager * dd;
    Vec_Int_t * vNodes;
    if ( Saig_ManPoNum(p) != 1 )
    {
        printf( "The AIG has %d property outputs.\n", Saig_ManPoNum(p) );
        return NULL;
    }
    assert( Saig_ManPoNum(p) == 1 );
    dd = Llb_ManConstructGlobalBdds( p );
    vNodes = Llb_ManComputeBaseCase( p, dd );
    if ( Llb_ManCountEntries(vNodes) > 0 )
        Llb_ManComputeIndCase( p, dd, vNodes );
    if ( Llb_ManCountEntries(vNodes) == 0 )
        Vec_IntFreeP( &vNodes );
    Llb_ManDerefenceBdds( p, dd );
    Extra_StopManager( dd );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Tests derived constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManConstrTest( Aig_Man_t * p )
{
    Vec_Int_t * vNodes;
    vNodes = Llb_ManDeriveConstraints( p );
    Llb_ManPrintEntries( p, vNodes );
    Vec_IntFreeP( &vNodes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

