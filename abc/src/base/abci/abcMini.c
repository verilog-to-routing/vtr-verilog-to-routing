/**CFile****************************************************************

  FileName    [abcMini.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Interface to the minimalistic AIG package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcMini.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "aig/miniaig/miniaig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeFanin0Copy( Abc_Ntk_t * pNtk, Vec_Int_t * vCopies, Mini_Aig_t * p, int Id )
{
    int Lit = Mini_AigNodeFanin0( p, Id );
    int AbcLit = Abc_LitNotCond( Vec_IntEntry(vCopies, Abc_Lit2Var(Lit)), Abc_LitIsCompl(Lit) );
    return Abc_ObjFromLit( pNtk, AbcLit );
}
Abc_Obj_t * Abc_NodeFanin1Copy( Abc_Ntk_t * pNtk, Vec_Int_t * vCopies, Mini_Aig_t * p, int Id )
{
    int Lit = Mini_AigNodeFanin1( p, Id );
    int AbcLit = Abc_LitNotCond( Vec_IntEntry(vCopies, Abc_Lit2Var(Lit)), Abc_LitIsCompl(Lit) );
    return Abc_ObjFromLit( pNtk, AbcLit );
}
Abc_Ntk_t * Abc_NtkFromMiniAig( Mini_Aig_t * p )
{
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pObj = NULL;
    Vec_Int_t * vCopies;
    int i, nNodes;
    // get the number of nodes
    nNodes = Mini_AigNodeNum(p);
    // create ABC network
    pNtk = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pNtk->pName = Abc_UtilStrsav( "MiniAig" );
    // create mapping from MiniAIG objects into ABC objects
    vCopies = Vec_IntAlloc( nNodes );
    Vec_IntPush( vCopies, Abc_LitNot(Abc_ObjToLit(Abc_AigConst1(pNtk))) );
    // iterate through the objects
    for ( i = 1; i < nNodes; i++ )
    {
        if ( Mini_AigNodeIsPi( p, i ) )
            pObj = Abc_NtkCreatePi(pNtk);
        else if ( Mini_AigNodeIsPo( p, i ) )
            Abc_ObjAddFanin( (pObj = Abc_NtkCreatePo(pNtk)), Abc_NodeFanin0Copy(pNtk, vCopies, p, i) );
        else if ( Mini_AigNodeIsAnd( p, i ) )
            pObj = Abc_AigAnd((Abc_Aig_t *)pNtk->pManFunc, Abc_NodeFanin0Copy(pNtk, vCopies, p, i), Abc_NodeFanin1Copy(pNtk, vCopies, p, i));
        else assert( 0 );
        Vec_IntPush( vCopies, Abc_ObjToLit(pObj) );
    }
    assert( Vec_IntSize(vCopies) == nNodes );
    Abc_AigCleanup( (Abc_Aig_t *)pNtk->pManFunc );
    Vec_IntFree( vCopies );
    Abc_NtkAddDummyPiNames( pNtk );
    Abc_NtkAddDummyPoNames( pNtk );
    if ( !Abc_NtkCheck( pNtk ) )
        fprintf( stdout, "Abc_NtkFromMini(): Network check has failed.\n" );
    // add latches
    if ( Mini_AigRegNum(p) > 0 )
    {
        extern Abc_Ntk_t * Abc_NtkRestrashWithLatches( Abc_Ntk_t * pNtk, int nLatches );
        Abc_Ntk_t * pTemp;
        pNtk = Abc_NtkRestrashWithLatches( pTemp = pNtk, Mini_AigRegNum(p) );
        Abc_NtkDelete( pTemp );
    }
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Converts the network from ABC into the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeFanin0Copy2( Abc_Obj_t * pObj )
{
    return Abc_LitNotCond( Abc_ObjFanin0(pObj)->iTemp, Abc_ObjFaninC0(pObj) );
}
int Abc_NodeFanin1Copy2( Abc_Obj_t * pObj )
{
    return Abc_LitNotCond( Abc_ObjFanin1(pObj)->iTemp, Abc_ObjFaninC1(pObj) );
}
Mini_Aig_t * Abc_NtkToMiniAig( Abc_Ntk_t * pNtk )
{
    Mini_Aig_t * p;
    Abc_Obj_t * pObj = NULL;
    int i;
    assert( Abc_NtkIsStrash(pNtk) );
    // create the manager
    p = Mini_AigStart();
    // create mapping from MiniAIG into ABC objects
    Abc_NtkCleanCopy( pNtk );
    Abc_AigConst1(pNtk)->iTemp = Mini_AigLitConst1();
    // create primary inputs
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->iTemp = Mini_AigCreatePi(p);
    // create internal nodes
    Abc_NtkForEachNode( pNtk, pObj, i )
        pObj->iTemp = Mini_AigAnd( p, Abc_NodeFanin0Copy2(pObj), Abc_NodeFanin1Copy2(pObj) );
    // create primary outputs
    Abc_NtkForEachCo( pNtk, pObj, i )
        pObj->iTemp = Mini_AigCreatePo( p, Abc_NodeFanin0Copy2(pObj) );
    // set registers
    Mini_AigSetRegNum( p, Abc_NtkLatchNum(pNtk) );
    return p;
}

/**Function*************************************************************

  Synopsis    [Procedures to update internal ABC network using AIG node array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkInputMiniAig( Abc_Frame_t * pAbc, void * p )
{
    Abc_Ntk_t * pNtk;
    if ( pAbc == NULL )
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
    pNtk = Abc_NtkFromMiniAig( (Mini_Aig_t *)p );
    Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
//    Abc_NtkDelete( pNtk );
}
void * Abc_NtkOutputMiniAig( Abc_Frame_t * pAbc )
{
    Abc_Ntk_t * pNtk;
    if ( pAbc == NULL )
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
    pNtk = Abc_FrameReadNtk( pAbc );
    if ( pNtk == NULL )
        printf( "Current network in ABC framework is not defined.\n" );
    return Abc_NtkToMiniAig( pNtk );
}
void Abc_NtkSetFlopNum( Abc_Frame_t * pAbc, int nFlops )
{
    extern void Abc_NtkMakeSeq( Abc_Ntk_t * pNtk, int nFlops );
    Abc_Ntk_t * pNtk;
    if ( pAbc == NULL )
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
    pNtk = Abc_FrameReadNtk( pAbc );
    if ( pNtk == NULL )
        printf( "Current network in ABC framework is not defined.\n" );
    Abc_NtkMakeSeq( pNtk, nFlops );
}

/**Function*************************************************************

  Synopsis    [Testing the above code.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMiniAigTest( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Mini_Aig_t * p;
    p = Abc_NtkToMiniAig( pNtk );
    pNtkNew = Abc_NtkFromMiniAig( p );
    Mini_AigStop( p );
    Abc_NtkPrintStats( pNtkNew, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
    Abc_NtkDelete( pNtkNew );

    // test dumping
    p = Abc_NtkToMiniAig( pNtk );
    Mini_AigDump( p, "miniaig.data" );
    Mini_AigPrintStats( p );
    Mini_AigStop( p );

    p = Mini_AigLoad( "miniaig.data" );
    Mini_AigPrintStats( p );
    Mini_AigStop( p );
}

/*
    if ( pNtk )
    {
        extern void Abc_NtkMiniAigTest( Abc_Ntk_t * pNtk );
        Abc_NtkMiniAigTest( pNtk );

    }
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

