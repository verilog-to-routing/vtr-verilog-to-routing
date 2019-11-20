/**CFile****************************************************************

  FileName    [abcDar.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [DAG-aware rewriting.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcDar.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "aig.h"
#include "dar.h"
#include "cnf.h"
#include "fra.h"
#include "fraig.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description [Assumes that registers are ordered after PIs/POs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fRegisters )
{
    Aig_Man_t * pMan;
    Aig_Obj_t * pObjNew;
    Abc_Obj_t * pObj;
    int i, nNodes;
    // make sure the latches follow PIs/POs
    if ( fRegisters )
    {
        assert( Abc_NtkBoxNum(pNtk) == Abc_NtkLatchNum(pNtk) );
        Abc_NtkForEachCi( pNtk, pObj, i )
            if ( i < Abc_NtkPiNum(pNtk) )
                assert( Abc_ObjIsPi(pObj) );
            else
                assert( Abc_ObjIsBo(pObj) );
        Abc_NtkForEachCo( pNtk, pObj, i )
            if ( i < Abc_NtkPoNum(pNtk) )
                assert( Abc_ObjIsPo(pObj) );
            else
                assert( Abc_ObjIsBi(pObj) );
    }
    // create the manager
    pMan = Aig_ManStart( Abc_NtkNodeNum(pNtk) + 100 );
    // save the number of registers
    if ( fRegisters )
        pMan->nRegs = Abc_NtkLatchNum(pNtk);
    // transfer the pointers to the basic nodes
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)Aig_ManConst1(pMan);
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Aig_ObjCreatePi(pMan);
    // complement the 1-values registers
    if ( fRegisters )
        Abc_NtkForEachLatch( pNtk, pObj, i )
            if ( Abc_LatchIsInit1(pObj) )
                Abc_ObjFanout0(pObj)->pCopy = Abc_ObjNot(Abc_ObjFanout0(pObj)->pCopy);
    // perform the conversion of the internal nodes (assumes DFS ordering)
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        pObj->pCopy = (Abc_Obj_t *)Aig_And( pMan, (Aig_Obj_t *)Abc_ObjChild0Copy(pObj), (Aig_Obj_t *)Abc_ObjChild1Copy(pObj) );
//        printf( "%d->%d ", pObj->Id, ((Aig_Obj_t *)pObj->pCopy)->Id );
    }
    // create the POs
    Abc_NtkForEachCo( pNtk, pObj, i )
        Aig_ObjCreatePo( pMan, (Aig_Obj_t *)Abc_ObjChild0Copy(pObj) );
    // complement the 1-valued registers
    if ( fRegisters )
        Aig_ManForEachLiSeq( pMan, pObjNew, i )
            if ( Abc_LatchIsInit1(Abc_ObjFanout0(Abc_NtkCo(pNtk,i))) )
                pObjNew->pFanin0 = Aig_Not(pObjNew->pFanin0);
    // remove dangling nodes
    if ( nNodes = Aig_ManCleanup( pMan ) )
        printf( "Abc_NtkToDar(): Unexpected %d dangling nodes when converting to AIG!\n", nNodes );
    if ( !Aig_ManCheck( pMan ) )
    {
        printf( "Abc_NtkToDar: AIG check has failed.\n" );
        Aig_ManStop( pMan );
        return NULL;
    }
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromDar( Abc_Ntk_t * pNtkOld, Aig_Man_t * pMan )
{
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkNew;
    Aig_Obj_t * pObj;
    int i;
    assert( Aig_ManRegNum(pMan) == Abc_NtkLatchNum(pNtkOld) );
    // perform strashing
    pNtkNew = Abc_NtkStartFrom( pNtkOld, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // transfer the pointers to the basic nodes
    Aig_ManConst1(pMan)->pData = Abc_AigConst1(pNtkNew);
    Aig_ManForEachPi( pMan, pObj, i )
        pObj->pData = Abc_NtkCi(pNtkNew, i);
    // rebuild the AIG
    vNodes = Aig_ManDfs( pMan );
    Vec_PtrForEachEntry( vNodes, pObj, i )
        if ( Aig_ObjIsBuf(pObj) )
            pObj->pData = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
        else
            pObj->pData = Abc_AigAnd( pNtkNew->pManFunc, (Abc_Obj_t *)Aig_ObjChild0Copy(pObj), (Abc_Obj_t *)Aig_ObjChild1Copy(pObj) );
    Vec_PtrFree( vNodes );
    // connect the PO nodes
    Aig_ManForEachPo( pMan, pObj, i )
        Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, i), (Abc_Obj_t *)Aig_ObjChild0Copy(pObj) );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkFromDar(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description [This procedure should be called after seq sweeping, 
  which changes the number of registers.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromDarSeqSweep( Abc_Ntk_t * pNtkOld, Aig_Man_t * pMan )
{
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObjNew;
    Aig_Obj_t * pObj, * pObjLo, * pObjLi;
    int i;
    assert( Aig_ManRegNum(pMan) < Abc_NtkLatchNum(pNtkOld) );
    // perform strashing
    pNtkNew = Abc_NtkStartFromNoLatches( pNtkOld, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // transfer the pointers to the basic nodes
    Aig_ManConst1(pMan)->pData = Abc_AigConst1(pNtkNew);
    Aig_ManForEachPiSeq( pMan, pObj, i )
        pObj->pData = Abc_NtkCi(pNtkNew, i);
    // create as many latches as there are registers in the manager
    Aig_ManForEachLiLoSeq( pMan, pObjLi, pObjLo, i )
    {
        pObjNew = Abc_NtkCreateLatch( pNtkNew );
        pObjLi->pData = Abc_NtkCreateBi( pNtkNew );
        pObjLo->pData = Abc_NtkCreateBo( pNtkNew );
        Abc_ObjAddFanin( pObjNew, pObjLi->pData );
        Abc_ObjAddFanin( pObjLo->pData, pObjNew );
        Abc_LatchSetInit0( pObjNew );
    }
    Abc_NtkAddDummyBoxNames( pNtkNew );
    // rebuild the AIG
    vNodes = Aig_ManDfs( pMan );
    Vec_PtrForEachEntry( vNodes, pObj, i )
        if ( Aig_ObjIsBuf(pObj) )
            pObj->pData = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
        else
            pObj->pData = Abc_AigAnd( pNtkNew->pManFunc, (Abc_Obj_t *)Aig_ObjChild0Copy(pObj), (Abc_Obj_t *)Aig_ObjChild1Copy(pObj) );
    Vec_PtrFree( vNodes );
    // connect the PO nodes
    Aig_ManForEachPo( pMan, pObj, i )
        Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, i), (Abc_Obj_t *)Aig_ObjChild0Copy(pObj) );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkFromDar(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromDarChoices( Abc_Ntk_t * pNtkOld, Aig_Man_t * pMan )
{
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkNew;
    Aig_Obj_t * pObj, * pTemp;
    int i;
    assert( pMan->pEquivs != NULL );
    assert( Aig_ManBufNum(pMan) == 0 );
    // perform strashing
    pNtkNew = Abc_NtkStartFrom( pNtkOld, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // transfer the pointers to the basic nodes
    Aig_ManConst1(pMan)->pData = Abc_AigConst1(pNtkNew);
    Aig_ManForEachPi( pMan, pObj, i )
        pObj->pData = Abc_NtkCi(pNtkNew, i);
    // rebuild the AIG
    vNodes = Aig_ManDfsChoices( pMan );
    Vec_PtrForEachEntry( vNodes, pObj, i )
    {
        pObj->pData = Abc_AigAnd( pNtkNew->pManFunc, (Abc_Obj_t *)Aig_ObjChild0Copy(pObj), (Abc_Obj_t *)Aig_ObjChild1Copy(pObj) );
        if ( pTemp = pMan->pEquivs[pObj->Id] )
        {
            Abc_Obj_t * pAbcRepr, * pAbcObj;
            assert( pTemp->pData != NULL );
            pAbcRepr = pObj->pData;
            pAbcObj  = pTemp->pData;
            pAbcObj->pData  = pAbcRepr->pData;
            pAbcRepr->pData = pAbcObj;
        }
    }
printf( "Total = %d.  Collected = %d.\n", Aig_ManNodeNum(pMan), Vec_PtrSize(vNodes) );
    Vec_PtrFree( vNodes );
/*
    {
        Abc_Obj_t * pNode;
        int k, Counter = 0;
        Abc_NtkForEachNode( pNtkNew, pNode, k )
            if ( pNode->pData != 0 )
            {
                int x = 0;
                Counter++;
            }
        printf( "Choices = %d.\n", Counter );
    }
*/
    // connect the PO nodes
    Aig_ManForEachPo( pMan, pObj, i )
        Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, i), (Abc_Obj_t *)Aig_ObjChild0Copy(pObj) );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkFromDar(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromDarSeq( Abc_Ntk_t * pNtkOld, Aig_Man_t * pMan )
{
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObjNew, * pFaninNew, * pFaninNew0, * pFaninNew1;
    Aig_Obj_t * pObj;
    int i;
//    assert( Aig_ManLatchNum(pMan) > 0 );
    // perform strashing
    pNtkNew = Abc_NtkStartFromNoLatches( pNtkOld, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // transfer the pointers to the basic nodes
    Aig_ManConst1(pMan)->pData = Abc_AigConst1(pNtkNew);
    Aig_ManForEachPi( pMan, pObj, i )
        pObj->pData = Abc_NtkPi(pNtkNew, i);
    // create latches of the new network
    Aig_ManForEachObj( pMan, pObj, i )
    {
        if ( !Aig_ObjIsLatch(pObj) )
            continue;
        pObjNew = Abc_NtkCreateLatch( pNtkNew );
        pFaninNew0 = Abc_NtkCreateBi( pNtkNew );
        pFaninNew1 = Abc_NtkCreateBo( pNtkNew );
        Abc_ObjAddFanin( pObjNew, pFaninNew0 );
        Abc_ObjAddFanin( pFaninNew1, pObjNew );
        Abc_LatchSetInit0( pObjNew );
        pObj->pData = Abc_ObjFanout0( pObjNew );
    }
    Abc_NtkAddDummyBoxNames( pNtkNew );
    // rebuild the AIG
    vNodes = Aig_ManDfs( pMan );
    Vec_PtrForEachEntry( vNodes, pObj, i )
    {
        // add the first fanin
        pObj->pData = pFaninNew0 = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
        if ( Aig_ObjIsBuf(pObj) )
            continue;
        // add the second fanin
        pFaninNew1 = (Abc_Obj_t *)Aig_ObjChild1Copy(pObj);
        // create the new node
        if ( Aig_ObjIsExor(pObj) )
            pObj->pData = pObjNew = Abc_AigXor( pNtkNew->pManFunc, pFaninNew0, pFaninNew1 );
        else
            pObj->pData = pObjNew = Abc_AigAnd( pNtkNew->pManFunc, pFaninNew0, pFaninNew1 );
    }
    Vec_PtrFree( vNodes );
    // connect the PO nodes
    Aig_ManForEachPo( pMan, pObj, i )
    {
        pFaninNew = (Abc_Obj_t *)Aig_ObjChild0Copy( pObj );
        Abc_ObjAddFanin( Abc_NtkPo(pNtkNew, i), pFaninNew );
    }
    // connect the latches
    Aig_ManForEachObj( pMan, pObj, i )
    {
        if ( !Aig_ObjIsLatch(pObj) )
            continue;
        pFaninNew = (Abc_Obj_t *)Aig_ObjChild0Copy( pObj );
        Abc_ObjAddFanin( Abc_ObjFanin0(Abc_ObjFanin0(pObj->pData)), pFaninNew );
    }
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkFromIvySeq(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Collect latch values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkGetLatchValues( Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vInits;
    Abc_Obj_t * pLatch;
    int i;
    vInits = Vec_IntAlloc( Abc_NtkLatchNum(pNtk) );
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        assert( Abc_LatchIsInit1(pLatch) == 0 );
        Vec_IntPush( vInits, Abc_LatchIsInit1(pLatch) );
    }
    return vInits;
}

/**Function*************************************************************

  Synopsis    [Performs verification after retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSecRetime( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2 )
{
    int fRemove1, fRemove2;
    Aig_Man_t * pMan1, * pMan2;
    int * pArray;

    fRemove1 = (!Abc_NtkIsStrash(pNtk1)) && (pNtk1 = Abc_NtkStrash(pNtk1, 0, 0, 0));
    fRemove2 = (!Abc_NtkIsStrash(pNtk2)) && (pNtk2 = Abc_NtkStrash(pNtk2, 0, 0, 0));


    pMan1 = Abc_NtkToDar( pNtk1, 0 );
    pMan2 = Abc_NtkToDar( pNtk2, 0 );

    Aig_ManPrintStats( pMan1 );
    Aig_ManPrintStats( pMan2 );

//    pArray = Abc_NtkGetLatchValues(pNtk1);
    pArray = NULL;
    Aig_ManSeqStrash( pMan1, Abc_NtkLatchNum(pNtk1), pArray );
    free( pArray );

//    pArray = Abc_NtkGetLatchValues(pNtk2);
    pArray = NULL;
    Aig_ManSeqStrash( pMan2, Abc_NtkLatchNum(pNtk2), pArray );
    free( pArray );

    Aig_ManPrintStats( pMan1 );
    Aig_ManPrintStats( pMan2 );

    Aig_ManStop( pMan1 );
    Aig_ManStop( pMan2 );


    if ( fRemove1 )  Abc_NtkDelete( pNtk1 );
    if ( fRemove2 )  Abc_NtkDelete( pNtk2 );
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDar( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan;//, * pTemp;
//    int * pArray;

    assert( Abc_NtkIsStrash(pNtk) );
    // convert to the AIG manager
    pMan = Abc_NtkToDar( pNtk, 0 );
    if ( pMan == NULL )
        return NULL;
    if ( !Aig_ManCheck( pMan ) )
    {
        printf( "Abc_NtkDar: AIG check has failed.\n" );
        Aig_ManStop( pMan );
        return NULL;
    }
    // perform balance
    Aig_ManPrintStats( pMan );
/*
    pArray = Abc_NtkGetLatchValues(pNtk);
    Aig_ManSeqStrash( pMan, Abc_NtkLatchNum(pNtk), pArray );
    free( pArray );
*/

//    Aig_ManDumpBlif( pMan, "aig_temp.blif" );
//    pMan->pPars = Dar_ManDefaultRwrPars();
    Dar_ManRewrite( pMan, NULL );
    Aig_ManPrintStats( pMan );
//    Dar_ManComputeCuts( pMan );

/*
{
extern Aig_Cnf_t * Aig_ManDeriveCnf( Aig_Man_t * p );
extern void Aig_CnfFree( Aig_Cnf_t * pCnf );
Aig_Cnf_t * pCnf;
pCnf = Aig_ManDeriveCnf( pMan );
Aig_CnfFree( pCnf );
}
*/

    // convert from the AIG manager
    if ( Aig_ManLatchNum(pMan) )
        pNtkAig = Abc_NtkFromDarSeq( pNtk, pMan );
    else
        pNtkAig = Abc_NtkFromDar( pNtk, pMan );
    if ( pNtkAig == NULL )
        return NULL;
    Aig_ManStop( pMan );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkAig ) )
    {
        printf( "Abc_NtkDar: The network check has failed.\n" );
        Abc_NtkDelete( pNtkAig );
        return NULL;
    }
    return pNtkAig;
}


/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarFraig( Abc_Ntk_t * pNtk, int nConfLimit, int fDoSparse, int fProve, int fTransfer, int fSpeculate, int fChoicing, int fVerbose )
{
    Fra_Par_t Pars, * pPars = &Pars; 
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0 );
    if ( pMan == NULL )
        return NULL;
    Fra_ParamsDefault( pPars );
    pPars->nBTLimitNode = nConfLimit;
    pPars->fVerbose     = fVerbose;
    pPars->fProve       = fProve;
    pPars->fDoSparse    = fDoSparse;
    pPars->fSpeculate   = fSpeculate;
    pPars->fChoicing    = fChoicing;
    pMan = Fra_FraigPerform( pTemp = pMan, pPars );
    if ( fChoicing )
        pNtkAig = Abc_NtkFromDarChoices( pNtk, pMan );
    else
        pNtkAig = Abc_NtkFromDar( pNtk, pMan );
    Aig_ManStop( pTemp );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCSweep( Abc_Ntk_t * pNtk, int nCutsMax, int nLeafMax, int fVerbose )
{
    extern Aig_Man_t * Csw_Sweep( Aig_Man_t * pAig, int nCutsMax, int nLeafMax, int fVerbose );
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0 );
    if ( pMan == NULL )
        return NULL;
    pMan = Csw_Sweep( pTemp = pMan, nCutsMax, nLeafMax, fVerbose );
    pNtkAig = Abc_NtkFromDar( pNtk, pMan );
    Aig_ManStop( pTemp );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDRewrite( Abc_Ntk_t * pNtk, Dar_RwrPar_t * pPars )
{
    Aig_Man_t * pMan, * pTemp;
    Abc_Ntk_t * pNtkAig;
    int clk;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0 );
    if ( pMan == NULL )
        return NULL;
//    Aig_ManPrintStats( pMan );
/*
//    Aig_ManSupports( pMan );
    {
        Vec_Vec_t * vParts;
        vParts = Aig_ManPartitionSmart( pMan, 50, 1, NULL );
        Vec_VecFree( vParts );
    }
*/
    Dar_ManRewrite( pMan, pPars );
//    pMan = Dar_ManBalance( pTemp = pMan, pPars->fUpdateLevel );
//    Aig_ManStop( pTemp );

clk = clock();
    pMan = Aig_ManDup( pTemp = pMan, 0 ); 
    Aig_ManStop( pTemp );
//PRT( "time", clock() - clk );

//    Aig_ManPrintStats( pMan );
    pNtkAig = Abc_NtkFromDar( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDRefactor( Abc_Ntk_t * pNtk, Dar_RefPar_t * pPars )
{
    Aig_Man_t * pMan, * pTemp;
    Abc_Ntk_t * pNtkAig;
    int clk;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0 );
    if ( pMan == NULL )
        return NULL;
//    Aig_ManPrintStats( pMan );

    Dar_ManRefactor( pMan, pPars );
//    pMan = Dar_ManBalance( pTemp = pMan, pPars->fUpdateLevel );
//    Aig_ManStop( pTemp );

clk = clock();
    pMan = Aig_ManDup( pTemp = pMan, 0 ); 
    Aig_ManStop( pTemp );
//PRT( "time", clock() - clk );

//    Aig_ManPrintStats( pMan );
    pNtkAig = Abc_NtkFromDar( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDCompress2( Abc_Ntk_t * pNtk, int fBalance, int fUpdateLevel, int fVerbose )
{
    Aig_Man_t * pMan;//, * pTemp;
    Abc_Ntk_t * pNtkAig;
    int clk;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0 );
    if ( pMan == NULL )
        return NULL;
//    Aig_ManPrintStats( pMan );

clk = clock();
    pMan = Dar_ManCompress2( pMan, fBalance, fUpdateLevel, fVerbose ); 
//    Aig_ManStop( pTemp );
//PRT( "time", clock() - clk );

//    Aig_ManPrintStats( pMan );
    pNtkAig = Abc_NtkFromDar( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDrwsat( Abc_Ntk_t * pNtk, int fBalance, int fVerbose )
{
    Aig_Man_t * pMan;//, * pTemp;
    Abc_Ntk_t * pNtkAig;
    int clk;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0 );
    if ( pMan == NULL )
        return NULL;
//    Aig_ManPrintStats( pMan );

clk = clock();
    pMan = Dar_ManRwsat( pMan, fBalance, fVerbose ); 
//    Aig_ManStop( pTemp );
//PRT( "time", clock() - clk );

//    Aig_ManPrintStats( pMan );
    pNtkAig = Abc_NtkFromDar( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkConstructFromCnf( Abc_Ntk_t * pNtk, Cnf_Man_t * p, Vec_Ptr_t * vMapped )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pNode, * pNodeNew;
    Aig_Obj_t * pObj, * pLeaf;
    Cnf_Cut_t * pCut;
    Vec_Int_t * vCover;
    unsigned uTruth;
    int i, k, nDupGates;
    // create the new network
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );
    // make the mapper point to the new network
    Aig_ManConst1(p->pManAig)->pData = Abc_NtkCreateNodeConst1(pNtkNew);
    Abc_NtkForEachCi( pNtk, pNode, i )
        Aig_ManPi(p->pManAig, i)->pData = pNode->pCopy;
    // process the nodes in topological order
    vCover = Vec_IntAlloc( 1 << 16 );
    Vec_PtrForEachEntry( vMapped, pObj, i )
    {
        // create new node
        pNodeNew = Abc_NtkCreateNode( pNtkNew );
        // add fanins according to the cut
        pCut = pObj->pData;
        Cnf_CutForEachLeaf( p->pManAig, pCut, pLeaf, k )
            Abc_ObjAddFanin( pNodeNew, pLeaf->pData );
        // add logic function
        if ( pCut->nFanins < 5 )
        {
            uTruth = 0xFFFF & *Cnf_CutTruth(pCut);
            Cnf_SopConvertToVector( p->pSops[uTruth], p->pSopSizes[uTruth], vCover );
            pNodeNew->pData = Abc_SopCreateFromIsop( pNtkNew->pManFunc, pCut->nFanins, vCover );
        }
        else
            pNodeNew->pData = Abc_SopCreateFromIsop( pNtkNew->pManFunc, pCut->nFanins, pCut->vIsop[1] );
        // save the node
        pObj->pData = pNodeNew;
    }
    Vec_IntFree( vCover );
    // add the CO drivers
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pObj = Aig_ManPo(p->pManAig, i);
        pNodeNew = Abc_ObjNotCond( Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
        Abc_ObjAddFanin( pNode->pCopy, pNodeNew );
    }

    // remove the constant node if not used
    pNodeNew = (Abc_Obj_t *)Aig_ManConst1(p->pManAig)->pData;
    if ( Abc_ObjFanoutNum(pNodeNew) == 0 )
        Abc_NtkDeleteObj( pNodeNew );
    // minimize the node
//    Abc_NtkSweep( pNtkNew, 0 );
    // decouple the PO driver nodes to reduce the number of levels
    nDupGates = Abc_NtkLogicMakeSimpleCos( pNtkNew, 1 );
//    if ( nDupGates && If_ManReadVerbose(pIfMan) )
//        printf( "Duplicated %d gates to decouple the CO drivers.\n", nDupGates );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkConstructFromCnf(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarToCnf( Abc_Ntk_t * pNtk, char * pFileName )
{
    Vec_Ptr_t * vMapped;
    Aig_Man_t * pMan;
    Cnf_Man_t * pManCnf;
    Cnf_Dat_t * pCnf;
    Abc_Ntk_t * pNtkNew = NULL;
    assert( Abc_NtkIsStrash(pNtk) );

    // convert to the AIG manager
    pMan = Abc_NtkToDar( pNtk, 0 );
    if ( pMan == NULL )
        return NULL;
    if ( !Aig_ManCheck( pMan ) )
    {
        printf( "Abc_NtkDarToCnf: AIG check has failed.\n" );
        Aig_ManStop( pMan );
        return NULL;
    }
    // perform balance
    Aig_ManPrintStats( pMan );

    // derive CNF
    pCnf = Cnf_Derive( pMan, 0 );
    pManCnf = Cnf_ManRead();

    // write the network for verification
    vMapped = Cnf_ManScanMapping( pManCnf, 1, 0 );
    pNtkNew = Abc_NtkConstructFromCnf( pNtk, pManCnf, vMapped );
    Vec_PtrFree( vMapped );

    // write CNF into a file
    Cnf_DataWriteIntoFile( pCnf, pFileName, 0 );
    Cnf_DataFree( pCnf );
    Cnf_ClearMemory();

    Aig_ManStop( pMan );
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    [Solves combinational miter using a SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDSat( Abc_Ntk_t * pNtk, sint64 nConfLimit, sint64 nInsLimit, int fVerbose )
{
    sat_solver * pSat;
    Aig_Man_t * pMan;
    Cnf_Dat_t * pCnf;
    int status, RetValue, clk = clock();
    Vec_Int_t * vCiIds;

    assert( Abc_NtkIsStrash(pNtk) );
    assert( Abc_NtkLatchNum(pNtk) == 0 );
    assert( Abc_NtkPoNum(pNtk) == 1 );

    // conver to the manager
    pMan = Abc_NtkToDar( pNtk, 0 );
    // derive CNF
//    pCnf = Cnf_Derive( pMan, 0 );
    pCnf = Cnf_DeriveSimple( pMan, 0 );
    // convert into the SAT solver
    pSat = Cnf_DataWriteIntoSolver( pCnf );
    vCiIds = Cnf_DataCollectPiSatNums( pCnf, pMan );
    Aig_ManStop( pMan );
    Cnf_DataFree( pCnf );
    // solve SAT
    if ( pSat == NULL )
    {
        Vec_IntFree( vCiIds );
//        printf( "Trivially unsat\n" );
        return 1;
    }


//    printf( "Created SAT problem with %d variable and %d clauses. ", sat_solver_nvars(pSat), sat_solver_nclauses(pSat) );
//    PRT( "Time", clock() - clk );

    // simplify the problem
    clk = clock();
    status = sat_solver_simplify(pSat);
//    printf( "Simplified the problem to %d variables and %d clauses. ", sat_solver_nvars(pSat), sat_solver_nclauses(pSat) );
//    PRT( "Time", clock() - clk );
    if ( status == 0 )
    {
        Vec_IntFree( vCiIds );
        sat_solver_delete( pSat );
//        printf( "The problem is UNSATISFIABLE after simplification.\n" );
        return 1;
    }

    // solve the miter
    clk = clock();
    if ( fVerbose )
        pSat->verbosity = 1;
    status = sat_solver_solve( pSat, NULL, NULL, (sint64)nConfLimit, (sint64)nInsLimit, (sint64)0, (sint64)0 );
    if ( status == l_Undef )
    {
//        printf( "The problem timed out.\n" );
        RetValue = -1;
    }
    else if ( status == l_True )
    {
//        printf( "The problem is SATISFIABLE.\n" );
        RetValue = 0;
    }
    else if ( status == l_False )
    {
//        printf( "The problem is UNSATISFIABLE.\n" );
        RetValue = 1;
    }
    else
        assert( 0 );
//    PRT( "SAT sat_solver time", clock() - clk );
//    printf( "The number of conflicts = %d.\n", (int)pSat->sat_solver_stats.conflicts );
 
    // if the problem is SAT, get the counterexample
    if ( status == l_True )
    {
        pNtk->pModel = Sat_SolverGetModel( pSat, vCiIds->pArray, vCiIds->nSize );
    }
    // free the sat_solver
    if ( fVerbose )
        Sat_SolverPrintStats( stdout, pSat );
//sat_solver_store_write( pSat, "trace.cnf" );
//sat_solver_store_free( pSat );
    sat_solver_delete( pSat );
    Vec_IntFree( vCiIds );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkSeqSweep( Abc_Ntk_t * pNtk, int nFramesK, int fRewrite, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 1 );
    if ( pMan == NULL )
        return NULL;
//    pMan->nRegs = Abc_NtkLatchNum(pNtk);

    pMan = Fra_FraigInduction( pTemp = pMan, nFramesK, fRewrite, fVerbose, NULL );
    Aig_ManStop( pTemp );

    if ( Aig_ManRegNum(pMan) < Abc_NtkLatchNum(pNtk) )
        pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
    else
        pNtkAig = Abc_NtkFromDar( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarSec( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nFrames, int fVerbose, int fVeryVerbose )
{
    Fraig_Params_t Params;
    Aig_Man_t * pMan;
    Abc_Ntk_t * pMiter, * pTemp;
    int RetValue;

    // get the miter of the two networks
    pMiter = Abc_NtkMiter( pNtk1, pNtk2, 0, 0 );
    if ( pMiter == NULL )
    {
        printf( "Miter computation has failed.\n" );
        return 0;
    }
    RetValue = Abc_NtkMiterIsConstant( pMiter );
    if ( RetValue == 0 )
    {
        extern void Abc_NtkVerifyReportErrorSeq( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int * pModel, int nFrames );
        printf( "Networks are NOT EQUIVALENT after structural hashing.\n" );
        // report the error
        pMiter->pModel = Abc_NtkVerifyGetCleanModel( pMiter, nFrames );
        Abc_NtkVerifyReportErrorSeq( pNtk1, pNtk2, pMiter->pModel, nFrames );
        FREE( pMiter->pModel );
        Abc_NtkDelete( pMiter );
        return 0;
    }
    if ( RetValue == 1 )
    {
        Abc_NtkDelete( pMiter );
        printf( "Networks are equivalent after structural hashing.\n" );
        return 1;
    }

    // preprocess the miter by fraiging it
    // (note that for each functional class, fraiging leaves one representative;
    // so fraiging does not reduce the number of functions represented by nodes
    Fraig_ParamsSetDefault( &Params );
    pMiter = Abc_NtkFraig( pTemp = pMiter, &Params, 0, 0 );
    Abc_NtkDelete( pTemp );

    // derive the AIG manager
    pMan = Abc_NtkToDar( pMiter, 1 );
    Abc_NtkDelete( pMiter );
    if ( pMan == NULL )
    {
        printf( "Converting miter into AIG has failed.\n" );
        return -1;
    }
    assert( pMan->nRegs > 0 );

    // perform verification
    RetValue = Fra_FraigSec( pMan, nFrames, fVerbose, fVeryVerbose );
    Aig_ManStop( pMan );
    return RetValue;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


