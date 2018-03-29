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

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "aig/gia/giaAig.h"
#include "opt/dar/dar.h"
#include "sat/cnf/cnf.h"
#include "proof/fra/fra.h"
#include "proof/fraig/fraig.h"
#include "proof/int/int.h"
#include "proof/dch/dch.h"
#include "proof/ssw/ssw.h"
#include "opt/cgt/cgt.h"
#include "bdd/bbr/bbr.h"
#include "aig/gia/gia.h"
#include "proof/cec/cec.h"
#include "opt/csw/csw.h"
#include "proof/pdr/pdr.h"
#include "sat/bmc/bmc.h"
#include "map/mio/mio.h"

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
int Abc_ObjCompareById( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 )
{
    return Abc_ObjId(Abc_ObjRegular(*pp1)) - Abc_ObjId(Abc_ObjRegular(*pp2));
}

/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_CollectTopOr_rec( Abc_Obj_t * pObj, Vec_Ptr_t * vSuper )
{
    if ( Abc_ObjIsComplement(pObj) || !Abc_ObjIsNode(pObj) )
    {
        Vec_PtrPush( vSuper, pObj );
        return;
    }
    // go through the branches
    Abc_CollectTopOr_rec( Abc_ObjChild0(pObj), vSuper );
    Abc_CollectTopOr_rec( Abc_ObjChild1(pObj), vSuper );
}
void Abc_CollectTopOr( Abc_Obj_t * pObj, Vec_Ptr_t * vSuper )
{
    Vec_PtrClear( vSuper );
    if ( Abc_ObjIsComplement(pObj) )
    {
        Abc_CollectTopOr_rec( Abc_ObjNot(pObj), vSuper );
        Vec_PtrUniqify( vSuper, (int (*)())Abc_ObjCompareById );
    }
    else
        Vec_PtrPush( vSuper, Abc_ObjNot(pObj) );
}

/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description [The returned map maps new PO IDs into old ones.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Abc_NtkToDarBmc( Abc_Ntk_t * pNtk, Vec_Int_t ** pvMap )
{
    Aig_Man_t * pMan;
    Abc_Obj_t * pObj, * pTemp;
    Vec_Ptr_t * vDrivers;
    Vec_Ptr_t * vSuper;
    int i, k, nDontCares;

    // print warning about initial values
    nDontCares = 0;
    Abc_NtkForEachLatch( pNtk, pObj, i )
        if ( Abc_LatchIsInitDc(pObj) )
        {
            Abc_LatchSetInit0(pObj);
            nDontCares++;
        }
    if ( nDontCares )
    {
        Abc_Print( 1, "Warning: %d registers in this network have don't-care init values.\n", nDontCares );
        Abc_Print( 1, "The don't-care are assumed to be 0. The result may not verify.\n" );
        Abc_Print( 1, "Use command \"print_latch\" to see the init values of registers.\n" );
        Abc_Print( 1, "Use command \"zero\" to convert or \"init\" to change the values.\n" );
    }

    // collect the drivers
    vSuper   = Vec_PtrAlloc( 100 );
    vDrivers = Vec_PtrAlloc( 100 );
    if ( pvMap ) 
    *pvMap   = Vec_IntAlloc( 100 );
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        if ( pNtk->nConstrs && i >= pNtk->nConstrs )
        {
            Vec_PtrPush( vDrivers, Abc_ObjNot(Abc_ObjChild0(pObj)) );
            if ( pvMap )
            Vec_IntPush( *pvMap, i );
            continue;
        }
        Abc_CollectTopOr( Abc_ObjChild0(pObj), vSuper );
        Vec_PtrForEachEntry( Abc_Obj_t *, vSuper, pTemp, k )
        {
            Vec_PtrPush( vDrivers, pTemp );
            if ( pvMap )
            Vec_IntPush( *pvMap, i );
        }       
    }
    Vec_PtrFree( vSuper );

    // create network
    pMan = Aig_ManStart( Abc_NtkNodeNum(pNtk) + 100 );
    pMan->nConstrs = pNtk->nConstrs;
    pMan->nBarBufs = pNtk->nBarBufs;
    pMan->pName = Extra_UtilStrsav( pNtk->pName );
    pMan->pSpec = Extra_UtilStrsav( pNtk->pSpec );
    // transfer the pointers to the basic nodes
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)Aig_ManConst1(pMan);
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Aig_ObjCreateCi(pMan);
    // create flops
    Abc_NtkForEachLatch( pNtk, pObj, i )
        Abc_ObjFanout0(pObj)->pCopy = Abc_ObjNotCond( Abc_ObjFanout0(pObj)->pCopy, Abc_LatchIsInit1(pObj) );
    // copy internal nodes
    Abc_NtkForEachNode( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Aig_And( pMan, (Aig_Obj_t *)Abc_ObjChild0Copy(pObj), (Aig_Obj_t *)Abc_ObjChild1Copy(pObj) );
    // create the POs
    Vec_PtrForEachEntry( Abc_Obj_t *, vDrivers, pTemp, k )
        Aig_ObjCreateCo( pMan, (Aig_Obj_t *)Abc_ObjNotCond(Abc_ObjRegular(pTemp)->pCopy, !Abc_ObjIsComplement(pTemp)) );
    Vec_PtrFree( vDrivers );
    // create flops
    Abc_NtkForEachLatchInput( pNtk, pObj, i )
        Aig_ObjCreateCo( pMan, (Aig_Obj_t *)Abc_ObjNotCond(Abc_ObjChild0Copy(pObj), Abc_LatchIsInit1(Abc_ObjFanout0(pObj))) );

    // remove dangling nodes
    Aig_ManSetRegNum( pMan, Abc_NtkLatchNum(pNtk) );
    Aig_ManCleanup( pMan );
    if ( !Aig_ManCheck( pMan ) )
    {
        Abc_Print( 1, "Abc_NtkToDarBmc: AIG check has failed.\n" );
        Aig_ManStop( pMan );
        return NULL;
    }
    return pMan;
}


/**Function*************************************************************

  Synopsis    [Collects information about what flops have unknown values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkFindDcLatches( Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vUnknown;
    Abc_Obj_t * pObj;
    int i;
    vUnknown = Vec_IntStart( Abc_NtkLatchNum(pNtk) );
    Abc_NtkForEachLatch( pNtk, pObj, i )
        if ( Abc_LatchIsInitDc(pObj) )
        {
            Vec_IntWriteEntry( vUnknown, i, 1 );
            Abc_LatchSetInit0(pObj);
        }
    return vUnknown;
}

/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description [Assumes that registers are ordered after PIs/POs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters )
{
    Vec_Ptr_t * vNodes;
    Aig_Man_t * pMan;
    Aig_Obj_t * pObjNew;
    Abc_Obj_t * pObj;
    int i, nNodes, nDontCares;
    // make sure the latches follow PIs/POs
    if ( fRegisters ) 
    { 
        assert( Abc_NtkBoxNum(pNtk) == Abc_NtkLatchNum(pNtk) );
        Abc_NtkForEachCi( pNtk, pObj, i )
            if ( i < Abc_NtkPiNum(pNtk) )
            {
                assert( Abc_ObjIsPi(pObj) );
                if ( !Abc_ObjIsPi(pObj) )
                    Abc_Print( 1, "Abc_NtkToDar(): Temporary bug: The PI ordering is wrong!\n" );
            }
            else
                assert( Abc_ObjIsBo(pObj) );
        Abc_NtkForEachCo( pNtk, pObj, i )
            if ( i < Abc_NtkPoNum(pNtk) )
            {
                assert( Abc_ObjIsPo(pObj) );
                if ( !Abc_ObjIsPo(pObj) )
                    Abc_Print( 1, "Abc_NtkToDar(): Temporary bug: The PO ordering is wrong!\n" );
            }
            else
                assert( Abc_ObjIsBi(pObj) );
        // print warning about initial values
        nDontCares = 0;
        Abc_NtkForEachLatch( pNtk, pObj, i )
            if ( Abc_LatchIsInitDc(pObj) )
            {
                Abc_LatchSetInit0(pObj);
                nDontCares++;
            }
        if ( nDontCares )
        {
            Abc_Print( 1, "Warning: %d registers in this network have don't-care init values.\n", nDontCares );
            Abc_Print( 1, "The don't-care are assumed to be 0. The result may not verify.\n" );
            Abc_Print( 1, "Use command \"print_latch\" to see the init values of registers.\n" );
            Abc_Print( 1, "Use command \"zero\" to convert or \"init\" to change the values.\n" );
        }
    }
    // create the manager
    pMan = Aig_ManStart( Abc_NtkNodeNum(pNtk) + 100 );
    pMan->fCatchExor = fExors;
    pMan->nConstrs = pNtk->nConstrs;
    pMan->nBarBufs = pNtk->nBarBufs;
    pMan->pName = Extra_UtilStrsav( pNtk->pName );
    pMan->pSpec = Extra_UtilStrsav( pNtk->pSpec );
    // transfer the pointers to the basic nodes
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)Aig_ManConst1(pMan);
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        pObj->pCopy = (Abc_Obj_t *)Aig_ObjCreateCi(pMan);
        // initialize logic level of the CIs
        ((Aig_Obj_t *)pObj->pCopy)->Level = pObj->Level;
    }

    // complement the 1-values registers
    if ( fRegisters ) {
        Abc_NtkForEachLatch( pNtk, pObj, i )
            if ( Abc_LatchIsInit1(pObj) )
                Abc_ObjFanout0(pObj)->pCopy = Abc_ObjNot(Abc_ObjFanout0(pObj)->pCopy);
    }
    // perform the conversion of the internal nodes (assumes DFS ordering)
//    pMan->fAddStrash = 1;
    vNodes = Abc_NtkDfs( pNtk, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
//    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        pObj->pCopy = (Abc_Obj_t *)Aig_And( pMan, (Aig_Obj_t *)Abc_ObjChild0Copy(pObj), (Aig_Obj_t *)Abc_ObjChild1Copy(pObj) );
//        Abc_Print( 1, "%d->%d ", pObj->Id, ((Aig_Obj_t *)pObj->pCopy)->Id );
    }
    Vec_PtrFree( vNodes );
    pMan->fAddStrash = 0;
    // create the POs
    Abc_NtkForEachCo( pNtk, pObj, i )
        Aig_ObjCreateCo( pMan, (Aig_Obj_t *)Abc_ObjChild0Copy(pObj) );
    // complement the 1-valued registers
    Aig_ManSetRegNum( pMan, Abc_NtkLatchNum(pNtk) );
    if ( fRegisters )
        Aig_ManForEachLiSeq( pMan, pObjNew, i )
            if ( Abc_LatchIsInit1(Abc_ObjFanout0(Abc_NtkCo(pNtk,i))) )
                pObjNew->pFanin0 = Aig_Not(pObjNew->pFanin0);
    // remove dangling nodes
    nNodes = (Abc_NtkGetChoiceNum(pNtk) == 0)? Aig_ManCleanup( pMan ) : 0;
    if ( !fExors && nNodes )
        Abc_Print( 1, "Abc_NtkToDar(): Unexpected %d dangling nodes when converting to AIG!\n", nNodes );
//Aig_ManDumpVerilog( pMan, "test.v" );
    // save the number of registers
    if ( fRegisters )
    {
        Aig_ManSetRegNum( pMan, Abc_NtkLatchNum(pNtk) );
        pMan->vFlopNums = Vec_IntStartNatural( pMan->nRegs );
//        pMan->vFlopNums = NULL;
//        pMan->vOnehots = Abc_NtkConverLatchNamesIntoNumbers( pNtk );
        if ( pNtk->vOnehots )
            pMan->vOnehots = (Vec_Ptr_t *)Vec_VecDupInt( (Vec_Vec_t *)pNtk->vOnehots );
    }
    if ( !Aig_ManCheck( pMan ) )
    {
        Abc_Print( 1, "Abc_NtkToDar: AIG check has failed.\n" );
        Aig_ManStop( pMan );
        return NULL;
    }
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description [Assumes that registers are ordered after PIs/POs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Abc_NtkToDarChoices( Abc_Ntk_t * pNtk )
{
    Aig_Man_t * pMan;
    Abc_Obj_t * pObj, * pPrev, * pFanin;
    Vec_Ptr_t * vNodes;
    int i;
    vNodes = Abc_AigDfs( pNtk, 0, 0 );
    // create the manager
    pMan = Aig_ManStart( Abc_NtkNodeNum(pNtk) + 100 );
    pMan->nConstrs = pNtk->nConstrs;
    pMan->nBarBufs = pNtk->nBarBufs;
    pMan->pName = Extra_UtilStrsav( pNtk->pName );
    pMan->pSpec = Extra_UtilStrsav( pNtk->pSpec );
    if ( Abc_NtkGetChoiceNum(pNtk) )
    {
        pMan->pEquivs = ABC_ALLOC( Aig_Obj_t *, Abc_NtkObjNum(pNtk) );
        memset( pMan->pEquivs, 0, sizeof(Aig_Obj_t *) * Abc_NtkObjNum(pNtk) );
    }
    // transfer the pointers to the basic nodes
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)Aig_ManConst1(pMan);
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Aig_ObjCreateCi(pMan);
    // perform the conversion of the internal nodes (assumes DFS ordering)
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        pObj->pCopy = (Abc_Obj_t *)Aig_And( pMan, (Aig_Obj_t *)Abc_ObjChild0Copy(pObj), (Aig_Obj_t *)Abc_ObjChild1Copy(pObj) );
//        Abc_Print( 1, "%d->%d ", pObj->Id, ((Aig_Obj_t *)pObj->pCopy)->Id );
        if ( Abc_AigNodeIsChoice( pObj ) )
        {
            for ( pPrev = pObj, pFanin = (Abc_Obj_t *)pObj->pData; pFanin; pPrev = pFanin, pFanin = (Abc_Obj_t *)pFanin->pData )
                Aig_ObjSetEquiv( pMan, (Aig_Obj_t *)pPrev->pCopy, (Aig_Obj_t *)pFanin->pCopy );
//            Aig_ManCreateChoice( pIfMan, (Aig_Obj_t *)pNode->pCopy );
        }
    }
    Vec_PtrFree( vNodes );
    // create the POs
    Abc_NtkForEachCo( pNtk, pObj, i )
        Aig_ObjCreateCo( pMan, (Aig_Obj_t *)Abc_ObjChild0Copy(pObj) );
    // complement the 1-valued registers
    Aig_ManSetRegNum( pMan, 0 );
    if ( !Aig_ManCheck( pMan ) )
    {
        Abc_Print( 1, "Abc_NtkToDar: AIG check has failed.\n" );
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
    assert( pMan->nAsserts == 0 );
//    assert( Aig_ManRegNum(pMan) == Abc_NtkLatchNum(pNtkOld) );
    // perform strashing
    pNtkNew = Abc_NtkStartFrom( pNtkOld, ABC_NTK_STRASH, ABC_FUNC_AIG );
    pNtkNew->nConstrs = pMan->nConstrs;
    pNtkNew->nBarBufs = pNtkOld->nBarBufs;
    // transfer the pointers to the basic nodes
    Aig_ManConst1(pMan)->pData = Abc_AigConst1(pNtkNew);
    Aig_ManForEachCi( pMan, pObj, i )
    {
        pObj->pData = Abc_NtkCi(pNtkNew, i);
        // initialize logic level of the CIs
        ((Abc_Obj_t *)pObj->pData)->Level = pObj->Level;
    }
    // rebuild the AIG
    vNodes = Aig_ManDfs( pMan, 1 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        if ( Aig_ObjIsBuf(pObj) )
            pObj->pData = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
        else
            pObj->pData = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, (Abc_Obj_t *)Aig_ObjChild0Copy(pObj), (Abc_Obj_t *)Aig_ObjChild1Copy(pObj) );
    Vec_PtrFree( vNodes );
    // connect the PO nodes
    Aig_ManForEachCo( pMan, pObj, i )
    {
        if ( pMan->nAsserts && i == Aig_ManCoNum(pMan) - pMan->nAsserts )
            break;
        Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, i), (Abc_Obj_t *)Aig_ObjChild0Copy(pObj) );
    }
    // if there are assertions, add them
    if ( !Abc_NtkCheck( pNtkNew ) )
        Abc_Print( 1, "Abc_NtkFromDar(): Network check has failed.\n" );
//Abc_NtkPrintCiLevels( pNtkNew );
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
    Abc_Obj_t * pObjNew, * pLatch;
    Aig_Obj_t * pObj, * pObjLo, * pObjLi;
    int i, iNodeId, nDigits; 
    assert( pMan->nAsserts == 0 );
    assert( pNtkOld->nBarBufs == 0 );
//    assert( Aig_ManRegNum(pMan) != Abc_NtkLatchNum(pNtkOld) );
    // perform strashing
    pNtkNew = Abc_NtkStartFromNoLatches( pNtkOld, ABC_NTK_STRASH, ABC_FUNC_AIG );
    pNtkNew->nConstrs = pMan->nConstrs;
    pNtkNew->nBarBufs = pMan->nBarBufs;
    // consider the case of target enlargement
    if ( Abc_NtkCiNum(pNtkNew) < Aig_ManCiNum(pMan) - Aig_ManRegNum(pMan) )
    {
        for ( i = Aig_ManCiNum(pMan) - Aig_ManRegNum(pMan) - Abc_NtkCiNum(pNtkNew); i > 0; i-- )
        {
            pObjNew = Abc_NtkCreatePi( pNtkNew );
            Abc_ObjAssignName( pObjNew, Abc_ObjName(pObjNew), NULL );
        }
        Abc_NtkOrderCisCos( pNtkNew );
    }
    assert( Abc_NtkCiNum(pNtkNew) == Aig_ManCiNum(pMan) - Aig_ManRegNum(pMan) );
    assert( Abc_NtkCoNum(pNtkNew) == Aig_ManCoNum(pMan) - Aig_ManRegNum(pMan) );
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
        Abc_ObjAddFanin( pObjNew, (Abc_Obj_t *)pObjLi->pData );
        Abc_ObjAddFanin( (Abc_Obj_t *)pObjLo->pData, pObjNew );
        Abc_LatchSetInit0( pObjNew );
    }
    // rebuild the AIG
    vNodes = Aig_ManDfs( pMan, 1 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        if ( Aig_ObjIsBuf(pObj) )
            pObj->pData = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
        else
            pObj->pData = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, (Abc_Obj_t *)Aig_ObjChild0Copy(pObj), (Abc_Obj_t *)Aig_ObjChild1Copy(pObj) );
    Vec_PtrFree( vNodes );
    // connect the PO nodes
    Aig_ManForEachCo( pMan, pObj, i )
    {
//        if ( pMan->nAsserts && i == Aig_ManCoNum(pMan) - pMan->nAsserts )
//            break;
        iNodeId = Nm_ManFindIdByNameTwoTypes( pNtkNew->pManName, Abc_ObjName(Abc_NtkCo(pNtkNew, i)), ABC_OBJ_PI, ABC_OBJ_BO );
        if ( iNodeId >= 0 )
            pObjNew = Abc_NtkObj( pNtkNew, iNodeId );
        else
            pObjNew = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
        Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, i), pObjNew );
    }
    if ( pMan->vFlopNums == NULL )
        Abc_NtkAddDummyBoxNames( pNtkNew );
    else
    {
/*
        {
            int i, k, iFlop, Counter = 0;
            FILE * pFile;
            pFile = fopen( "out.txt", "w" );
            fAbc_Print( 1, pFile, "The total of %d registers were removed (out of %d):\n", 
                Abc_NtkLatchNum(pNtkOld)-Vec_IntSize(pMan->vFlopNums), Abc_NtkLatchNum(pNtkOld) );
            for ( i = 0; i < Abc_NtkLatchNum(pNtkOld); i++ )
            {
                Vec_IntForEachEntry( pMan->vFlopNums, iFlop, k )
                {
                    if ( i == iFlop )
                        break;
                }
                if ( k == Vec_IntSize(pMan->vFlopNums) )
                    fAbc_Print( 1, pFile, "%6d (%6d)  :  %s\n", ++Counter, i, Abc_ObjName( Abc_ObjFanout0(Abc_NtkBox(pNtkOld, i)) ) );
            }
            fclose( pFile );
            //Abc_Print( 1, "\n" );
        }
*/
        assert( Abc_NtkBoxNum(pNtkOld) == Abc_NtkLatchNum(pNtkOld) );
        nDigits = Abc_Base10Log( Abc_NtkLatchNum(pNtkNew) );
        Abc_NtkForEachLatch( pNtkNew, pObjNew, i )
        {
            pLatch = Abc_NtkBox( pNtkOld, Vec_IntEntry( pMan->vFlopNums, i ) );
            iNodeId = Nm_ManFindIdByName( pNtkNew->pManName, Abc_ObjName(Abc_ObjFanout0(pLatch)), ABC_OBJ_PO );
            if ( iNodeId >= 0 )
            {
                Abc_ObjAssignName( pObjNew, Abc_ObjNameDummy("l", i, nDigits), NULL );
                Abc_ObjAssignName( Abc_ObjFanin0(pObjNew), Abc_ObjNameDummy("li", i, nDigits), NULL );
                Abc_ObjAssignName( Abc_ObjFanout0(pObjNew), Abc_ObjNameDummy("lo", i, nDigits), NULL );
//Abc_Print( 1, "happening   %s -> %s\n", Abc_ObjName(Abc_ObjFanin0(pObjNew)), Abc_ObjName(Abc_ObjFanout0(pObjNew)) );
                continue;
            }
            Abc_ObjAssignName( pObjNew, Abc_ObjName(pLatch), NULL );
            Abc_ObjAssignName( Abc_ObjFanin0(pObjNew),  Abc_ObjName(Abc_ObjFanin0(pLatch)), NULL );
            Abc_ObjAssignName( Abc_ObjFanout0(pObjNew), Abc_ObjName(Abc_ObjFanout0(pLatch)), NULL );
        }
    }
    // if there are assertions, add them
    if ( !Abc_NtkCheck( pNtkNew ) )
        Abc_Print( 1, "Abc_NtkFromDar(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description [This procedure should be called after seq sweeping, 
  which changes the number of registers.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromAigPhase( Aig_Man_t * pMan )
{
    Vec_Ptr_t * vNodes; 
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObjNew;
    Aig_Obj_t * pObj, * pObjLo, * pObjLi;
    int i; 
    assert( pMan->nAsserts == 0 );
    // perform strashing
    pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pNtkNew->nConstrs = pMan->nConstrs;
    pNtkNew->nBarBufs = pMan->nBarBufs;
    // duplicate the name and the spec
//    pNtkNew->pName = Extra_UtilStrsav(pMan->pName);
//    pNtkNew->pSpec = Extra_UtilStrsav(pMan->pSpec);
    Aig_ManConst1(pMan)->pData = Abc_AigConst1(pNtkNew);
    // create PIs
    Aig_ManForEachPiSeq( pMan, pObj, i )
    {
        pObjNew = Abc_NtkCreatePi( pNtkNew );
//        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObjNew), NULL );
        pObj->pData = pObjNew;
    }
    // create POs
    Aig_ManForEachPoSeq( pMan, pObj, i )
    {
        pObjNew = Abc_NtkCreatePo( pNtkNew );
//        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObjNew), NULL );
        pObj->pData = pObjNew;
    }
    assert( Abc_NtkCiNum(pNtkNew) == Aig_ManCiNum(pMan) - Aig_ManRegNum(pMan) );
    assert( Abc_NtkCoNum(pNtkNew) == Aig_ManCoNum(pMan) - Aig_ManRegNum(pMan) );
    // create as many latches as there are registers in the manager
    Aig_ManForEachLiLoSeq( pMan, pObjLi, pObjLo, i )
    {
        pObjNew = Abc_NtkCreateLatch( pNtkNew );
        pObjLi->pData = Abc_NtkCreateBi( pNtkNew );
        pObjLo->pData = Abc_NtkCreateBo( pNtkNew );
        Abc_ObjAddFanin( pObjNew, (Abc_Obj_t *)pObjLi->pData );
        Abc_ObjAddFanin( (Abc_Obj_t *)pObjLo->pData, pObjNew );
        Abc_LatchSetInit0( pObjNew );
//        Abc_ObjAssignName( (Abc_Obj_t *)pObjLi->pData, Abc_ObjName((Abc_Obj_t *)pObjLi->pData), NULL );
//        Abc_ObjAssignName( (Abc_Obj_t *)pObjLo->pData, Abc_ObjName((Abc_Obj_t *)pObjLo->pData), NULL );
    }
    // rebuild the AIG
    vNodes = Aig_ManDfs( pMan, 1 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        if ( Aig_ObjIsBuf(pObj) )
            pObj->pData = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
        else
            pObj->pData = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, (Abc_Obj_t *)Aig_ObjChild0Copy(pObj), (Abc_Obj_t *)Aig_ObjChild1Copy(pObj) );
    Vec_PtrFree( vNodes );
    // connect the PO nodes
    Aig_ManForEachCo( pMan, pObj, i )
    {
        pObjNew = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
        Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, i), pObjNew );
    }

    Abc_NtkAddDummyPiNames( pNtkNew );
    Abc_NtkAddDummyPoNames( pNtkNew );
    Abc_NtkAddDummyBoxNames( pNtkNew );

    // check the resulting AIG
    if ( !Abc_NtkCheck( pNtkNew ) )
        Abc_Print( 1, "Abc_NtkFromAigPhase(): Network check has failed.\n" );
    return pNtkNew;
}



/**Function*************************************************************

  Synopsis    [Creates local function of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Abc_ObjHopFromGia_rec( Hop_Man_t * pHopMan, Gia_Man_t * p, int Id, Vec_Ptr_t * vCopies )
{
    Gia_Obj_t * pObj;
    Hop_Obj_t * gFunc, * gFunc0, * gFunc1;
    if ( Gia_ObjIsTravIdCurrentId(p, Id) )
        return (Hop_Obj_t *)Vec_PtrEntry( vCopies, Id );
    Gia_ObjSetTravIdCurrentId(p, Id);
    pObj = Gia_ManObj(p, Id);
    assert( Gia_ObjIsAnd(pObj) );
    // compute the functions of the children
    gFunc0 = Abc_ObjHopFromGia_rec( pHopMan, p, Gia_ObjFaninId0(pObj, Id), vCopies );
    gFunc1 = Abc_ObjHopFromGia_rec( pHopMan, p, Gia_ObjFaninId1(pObj, Id), vCopies );
    // get the function of the cut
    gFunc  = Hop_And( pHopMan, Hop_NotCond(gFunc0, Gia_ObjFaninC0(pObj)), Hop_NotCond(gFunc1, Gia_ObjFaninC1(pObj)) );  
    Vec_PtrWriteEntry( vCopies, Id, gFunc );
    return gFunc;
}
Hop_Obj_t * Abc_ObjHopFromGia( Hop_Man_t * pHopMan, Gia_Man_t * p, int GiaId, Vec_Ptr_t * vCopies )
{
    int k, iFan;
    assert( Gia_ObjIsLut(p, GiaId) );
    assert( Gia_ObjLutSize(p, GiaId) > 0 );
    Gia_ManIncrementTravId( p );
    Gia_LutForEachFanin( p, GiaId, iFan, k )
    {
        Gia_ObjSetTravIdCurrentId(p, iFan);
        Vec_PtrWriteEntry( vCopies, iFan, Hop_IthVar(pHopMan, k) );
    }
    return Abc_ObjHopFromGia_rec( pHopMan, p, GiaId, vCopies );
}

/**Function*************************************************************

  Synopsis    [Converts the network from the mapped GIA manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromMappedGia( Gia_Man_t * p )
{
    int fVerbose = 0;
    int fDuplicate = 0;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObjNew, * pObjNewLi, * pObjNewLo, * pConst0 = NULL;
    Gia_Obj_t * pObj, * pObjLi, * pObjLo;
    Vec_Ptr_t * vReflect;
    int i, k, iFan, nDupGates; 
    assert( Gia_ManHasMapping(p) || p->pMuxes );
    pNtkNew = Abc_NtkAlloc( ABC_NTK_LOGIC, Gia_ManHasMapping(p) ? ABC_FUNC_AIG : ABC_FUNC_SOP, 1 );
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav(p->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(p->pSpec);
    Gia_ManFillValue( p );
    // create constant
    pConst0 = Abc_NtkCreateNodeConst0( pNtkNew );
    Gia_ManConst0(p)->Value = Abc_ObjId(pConst0);
    // create PIs
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Abc_ObjId( Abc_NtkCreatePi( pNtkNew ) );
    // create POs
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Abc_ObjId( Abc_NtkCreatePo( pNtkNew ) );
    // create as many latches as there are registers in the manager
    Gia_ManForEachRiRo( p, pObjLi, pObjLo, i )
    {
        pObjNew = Abc_NtkCreateLatch( pNtkNew );
        pObjNewLi = Abc_NtkCreateBi( pNtkNew );
        pObjNewLo = Abc_NtkCreateBo( pNtkNew );
        Abc_ObjAddFanin( pObjNew, pObjNewLi );
        Abc_ObjAddFanin( pObjNewLo, pObjNew );
        pObjLi->Value = Abc_ObjId( pObjNewLi );
        pObjLo->Value = Abc_ObjId( pObjNewLo );
        Abc_LatchSetInit0( pObjNew );
    }
    // rebuild the AIG
    if ( p->pMuxes )
    {
        Gia_ManForEachAnd( p, pObj, i )
        {
            pObjNew = Abc_NtkCreateNode( pNtkNew );
            if ( Gia_ObjIsMuxId(p, i) )
            {
                Abc_ObjAddFanin( pObjNew, Abc_NtkObj(pNtkNew, Gia_ObjValue(Gia_ObjFanin2(p, pObj))) );
                Abc_ObjAddFanin( pObjNew, Abc_NtkObj(pNtkNew, Gia_ObjValue(Gia_ObjFanin1(pObj))) );
                Abc_ObjAddFanin( pObjNew, Abc_NtkObj(pNtkNew, Gia_ObjValue(Gia_ObjFanin0(pObj))) );
                pObjNew->pData = Abc_SopCreateMux( (Mem_Flex_t *)pNtkNew->pManFunc );
                if ( Gia_ObjFaninC2(p, pObj) )  Abc_SopComplementVar( (char *)pObjNew->pData, 0 );
                if ( Gia_ObjFaninC1(pObj) )     Abc_SopComplementVar( (char *)pObjNew->pData, 1 );
                if ( Gia_ObjFaninC0(pObj) )     Abc_SopComplementVar( (char *)pObjNew->pData, 2 );
            }
            else if ( Gia_ObjIsXor(pObj) )
            {
                Abc_ObjAddFanin( pObjNew, Abc_NtkObj(pNtkNew, Gia_ObjValue(Gia_ObjFanin0(pObj))) );
                Abc_ObjAddFanin( pObjNew, Abc_NtkObj(pNtkNew, Gia_ObjValue(Gia_ObjFanin1(pObj))) );
                pObjNew->pData = Abc_SopCreateXor( (Mem_Flex_t *)pNtkNew->pManFunc, 2 );
                if ( Gia_ObjFaninC0(pObj) )  Abc_SopComplementVar( (char *)pObjNew->pData, 0 );
                if ( Gia_ObjFaninC1(pObj) )  Abc_SopComplementVar( (char *)pObjNew->pData, 1 );
            }
            else 
            {
                Abc_ObjAddFanin( pObjNew, Abc_NtkObj(pNtkNew, Gia_ObjValue(Gia_ObjFanin0(pObj))) );
                Abc_ObjAddFanin( pObjNew, Abc_NtkObj(pNtkNew, Gia_ObjValue(Gia_ObjFanin1(pObj))) );
                pObjNew->pData = Abc_SopCreateAnd( (Mem_Flex_t *)pNtkNew->pManFunc, 2, NULL );
                if ( Gia_ObjFaninC0(pObj) )  Abc_SopComplementVar( (char *)pObjNew->pData, 0 );
                if ( Gia_ObjFaninC1(pObj) )  Abc_SopComplementVar( (char *)pObjNew->pData, 1 );
            }
            pObj->Value = Abc_ObjId( pObjNew );
        }
    }
    else
    {
        vReflect = Vec_PtrStart( Gia_ManObjNum(p) );
        Gia_ManForEachLut( p, i )
        {
            pObj = Gia_ManObj(p, i);
            assert( pObj->Value == ~0 );
            if ( Gia_ObjLutSize(p, i) == 0 )
            {
                pObj->Value = Abc_ObjId(pConst0);
                continue;
            }
            pObjNew = Abc_NtkCreateNode( pNtkNew );
            Gia_LutForEachFanin( p, i, iFan, k )
                Abc_ObjAddFanin( pObjNew, Abc_NtkObj(pNtkNew, Gia_ObjValue(Gia_ManObj(p, iFan))) );
            pObjNew->pData = Abc_ObjHopFromGia( (Hop_Man_t *)pNtkNew->pManFunc, p, i, vReflect );
            pObj->Value = Abc_ObjId( pObjNew );
        }
        Vec_PtrFree( vReflect );
    }
    // connect the PO nodes
    Gia_ManForEachCo( p, pObj, i )
    {
        pObjNew = Abc_NtkObj( pNtkNew, Gia_ObjValue(Gia_ObjFanin0(pObj)) );
        Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, i), Abc_ObjNotCond( pObjNew, Gia_ObjFaninC0(pObj) ) );
    }
    // create names
    Abc_NtkAddDummyPiNames( pNtkNew );
    Abc_NtkAddDummyPoNames( pNtkNew );
    Abc_NtkAddDummyBoxNames( pNtkNew );

    // decouple the PO driver nodes to reduce the number of levels
    nDupGates = Abc_NtkLogicMakeSimpleCos( pNtkNew, fDuplicate );
    if ( fVerbose && nDupGates && !Abc_FrameReadFlag("silentmode") )
    {
        if ( !fDuplicate )
            printf( "Added %d buffers/inverters to decouple the CO drivers.\n", nDupGates );
        else
            printf( "Duplicated %d gates to decouple the CO drivers.\n", nDupGates );
    }
    // remove const node if it is not used
    if ( !Abc_ObjIsNone(pConst0) && Abc_ObjFanoutNum(pConst0) == 0 )
        Abc_NtkDeleteObj( pConst0 );

    assert( Gia_ManPiNum(p) == Abc_NtkPiNum(pNtkNew) );
    assert( Gia_ManPoNum(p) == Abc_NtkPoNum(pNtkNew) );
    assert( Gia_ManRegNum(p) == Abc_NtkLatchNum(pNtkNew) );

    // check the resulting AIG
    if ( !Abc_NtkCheck( pNtkNew ) )
        Abc_Print( 1, "Abc_NtkFromMappedGia(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Converts the network from the mapped GIA manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_NtkFromCellWrite( Vec_Int_t * vCopyLits, int i, int c, int Id )
{
    Vec_IntWriteEntry( vCopyLits, Abc_Var2Lit(i, c), Id );
}
static inline Abc_Obj_t * Abc_NtkFromCellRead( Abc_Ntk_t * p, Vec_Int_t * vCopyLits, int i, int c )
{
    Abc_Obj_t * pObjNew;
    int iObjNew = Vec_IntEntry( vCopyLits, Abc_Var2Lit(i, c) );
    if ( iObjNew >= 0 )
        return Abc_NtkObj(p, iObjNew);
    // opposite phase should be already constructed
    assert( 0 );
    if ( i == 0 )
        pObjNew = c ? Abc_NtkCreateNodeConst1(p) : Abc_NtkCreateNodeConst0(p);
    else
    {
        iObjNew = Vec_IntEntry( vCopyLits, Abc_Var2Lit(i, !c) );   assert( iObjNew >= 0 );
        pObjNew = Abc_NtkCreateNodeInv( p, Abc_NtkObj(p, iObjNew) );
    }
    Abc_NtkFromCellWrite( vCopyLits, i, c, Abc_ObjId(pObjNew) );
    return pObjNew;
}
Abc_Ntk_t * Abc_NtkFromCellMappedGia( Gia_Man_t * p )
{
    int fFixDrivers = 1;
    int fDuplicate = 1;
    int fVerbose = 0;
    Abc_Ntk_t * pNtkNew;
    Vec_Int_t * vCopyLits;
    Abc_Obj_t * pObjNew, * pObjNewLi, * pObjNewLo;
    Gia_Obj_t * pObj, * pObjLi, * pObjLo;
    int i, k, iLit, iFanLit, nCells, fNeedConst[2] = {0}; 
    Mio_Cell2_t * pCells = Mio_CollectRootsNewDefault2( 6, &nCells, 0 );
    assert( Gia_ManHasCellMapping(p) );
    // start network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_MAP, 1 );
    pNtkNew->pName = Extra_UtilStrsav(p->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(p->pSpec);
    assert( pNtkNew->pManFunc == Abc_FrameReadLibGen() );
    vCopyLits = Vec_IntStartFull( 2*Gia_ManObjNum(p) );
    // create PIs
    Gia_ManForEachPi( p, pObj, i )
        Abc_NtkFromCellWrite( vCopyLits, Gia_ObjId(p, pObj), 0, Abc_ObjId( Abc_NtkCreatePi( pNtkNew ) ) );
    // create POs
    Gia_ManForEachPo( p, pObj, i )
        Abc_NtkFromCellWrite( vCopyLits, Gia_ObjId(p, pObj), 0, Abc_ObjId( Abc_NtkCreatePo( pNtkNew ) ) );
    // create as many latches as there are registers in the manager
    Gia_ManForEachRiRo( p, pObjLi, pObjLo, i )
    {
        pObjNew = Abc_NtkCreateLatch( pNtkNew );
        pObjNewLi = Abc_NtkCreateBi( pNtkNew );
        pObjNewLo = Abc_NtkCreateBo( pNtkNew );
        Abc_ObjAddFanin( pObjNew, pObjNewLi );
        Abc_ObjAddFanin( pObjNewLo, pObjNew );
//        pObjLi->Value = Abc_ObjId( pObjNewLi );
//        pObjLo->Value = Abc_ObjId( pObjNewLo );
        Abc_NtkFromCellWrite( vCopyLits, Gia_ObjId(p, pObjLi), 0, Abc_ObjId( pObjNewLi ) );
        Abc_NtkFromCellWrite( vCopyLits, Gia_ObjId(p, pObjLo), 0, Abc_ObjId( pObjNewLo ) );
        Abc_LatchSetInit0( pObjNew );
    }

    // create constants
    Gia_ManForEachCo( p, pObj, i )
        if ( Gia_ObjFaninId0p(p, pObj) == 0 )
            fNeedConst[Gia_ObjFaninC0(pObj)] = 1;
    Gia_ManForEachBuf( p, pObj, i )
        if ( Gia_ObjFaninId0p(p, pObj) == 0 )
            fNeedConst[Gia_ObjFaninC0(pObj)] = 1;
    if ( fNeedConst[0] )
        Abc_NtkFromCellWrite( vCopyLits, 0, 0, Abc_ObjId(Abc_NtkCreateNodeConst0(pNtkNew)) );
    if ( fNeedConst[1] )
        Abc_NtkFromCellWrite( vCopyLits, 0, 1, Abc_ObjId(Abc_NtkCreateNodeConst1(pNtkNew)) );

    // rebuild the AIG
    Gia_ManForEachCell( p, iLit )
    {
        int fSkip = 0;
        if ( Gia_ObjIsCellBuf(p, iLit) )
        {
            assert( !Abc_LitIsCompl(iLit) );
            // build buffer
            pObjNew = Abc_NtkCreateNode( pNtkNew );
            iFanLit = Gia_ObjFaninLit0p( p, Gia_ManObj(p, Abc_Lit2Var(iLit)) );
            Abc_ObjAddFanin( pObjNew, Abc_NtkFromCellRead(pNtkNew, vCopyLits, Abc_Lit2Var(iFanLit), Abc_LitIsCompl(iFanLit)) );
            pObjNew->pData = NULL; // barrier buffer
            assert( Abc_ObjIsBarBuf(pObjNew) );
            pNtkNew->nBarBufs2++;
        }
        else if ( Gia_ObjIsCellInv(p, iLit) )
        {
            int iLitNot = Abc_LitNot(iLit);
            if ( !Abc_LitIsCompl(iLit) ) // positive phase
            {
                // build negative phase
                assert( Vec_IntEntry(vCopyLits, iLitNot) == -1 );
                assert( Gia_ObjCellId(p, iLitNot) > 0 );
                pObjNew = Abc_NtkCreateNode( pNtkNew );
                Gia_CellForEachFanin( p, iLitNot, iFanLit, k )
                    Abc_ObjAddFanin( pObjNew, Abc_NtkFromCellRead(pNtkNew, vCopyLits, Abc_Lit2Var(iFanLit), Abc_LitIsCompl(iFanLit)) );
                pObjNew->pData = Mio_LibraryReadGateByName( (Mio_Library_t *)pNtkNew->pManFunc, pCells[Gia_ObjCellId(p, iLitNot)].pName, NULL );
                Abc_NtkFromCellWrite( vCopyLits, Abc_Lit2Var(iLitNot), Abc_LitIsCompl(iLitNot), Abc_ObjId(pObjNew) );
                fSkip = 1;
            }
            else // negative phase
            {
                // positive phase is available
                assert( Vec_IntEntry(vCopyLits, iLitNot) != -1 );
            }
            // build inverter
            pObjNew = Abc_NtkCreateNode( pNtkNew );
            Abc_ObjAddFanin( pObjNew, Abc_NtkFromCellRead(pNtkNew, vCopyLits, Abc_Lit2Var(iLit), Abc_LitIsCompl(iLitNot)) );
            pObjNew->pData = Mio_LibraryReadGateByName( (Mio_Library_t *)pNtkNew->pManFunc, pCells[3].pName, NULL );
        }
        else
        {
            assert( Gia_ObjCellId(p, iLit) >= 0 );
            pObjNew = Abc_NtkCreateNode( pNtkNew );
            Gia_CellForEachFanin( p, iLit, iFanLit, k )
                Abc_ObjAddFanin( pObjNew, Abc_NtkFromCellRead(pNtkNew, vCopyLits, Abc_Lit2Var(iFanLit), Abc_LitIsCompl(iFanLit)) );
            pObjNew->pData = Mio_LibraryReadGateByName( (Mio_Library_t *)pNtkNew->pManFunc, pCells[Gia_ObjCellId(p, iLit)].pName, NULL );
        }
        assert( Vec_IntEntry(vCopyLits, iLit) == -1 );
        Abc_NtkFromCellWrite( vCopyLits, Abc_Lit2Var(iLit), Abc_LitIsCompl(iLit), Abc_ObjId(pObjNew) );
        // skip next
        iLit += fSkip;
    }

    // connect the PO nodes
    Gia_ManForEachCo( p, pObj, i )
    {
        pObjNew = Abc_NtkFromCellRead( pNtkNew, vCopyLits, Gia_ObjFaninId0p(p, pObj), Gia_ObjFaninC0(pObj) );
        Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, i), pObjNew );
    }
    // create names
    Abc_NtkAddDummyPiNames( pNtkNew );
    Abc_NtkAddDummyPoNames( pNtkNew );
    Abc_NtkAddDummyBoxNames( pNtkNew );

    // decouple the PO driver nodes to reduce the number of levels
    if ( fFixDrivers )
    {
        int nDupGates = Abc_NtkLogicMakeSimpleCos( pNtkNew, fDuplicate );
        if ( fVerbose && nDupGates && !Abc_FrameReadFlag("silentmode") )
        {
            if ( !fDuplicate )
                printf( "Added %d buffers/inverters to decouple the CO drivers.\n", nDupGates );
            else
                printf( "Duplicated %d gates to decouple the CO drivers.\n", nDupGates );
        }
    }

    assert( Gia_ManPiNum(p) == Abc_NtkPiNum(pNtkNew) );
    assert( Gia_ManPoNum(p) == Abc_NtkPoNum(pNtkNew) );
    assert( Gia_ManRegNum(p) == Abc_NtkLatchNum(pNtkNew) );
    Vec_IntFree( vCopyLits );
    ABC_FREE( pCells );

    // check the resulting AIG
    if ( !Abc_NtkCheck( pNtkNew ) )
        Abc_Print( 1, "Abc_NtkFromMappedGia(): Network check has failed.\n" );
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description [This procedure should be called after seq sweeping, 
  which changes the number of registers.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkAfterTrim( Aig_Man_t * pMan, Abc_Ntk_t * pNtkOld )
{
    Vec_Ptr_t * vNodes; 
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObjNew, * pObjOld;
    Aig_Obj_t * pObj, * pObjLo, * pObjLi;
    int i; 
    assert( pMan->nAsserts == 0 );
    assert( pNtkOld->nBarBufs == 0 );
    assert( Aig_ManRegNum(pMan) <= Abc_NtkLatchNum(pNtkOld) );
    assert( Saig_ManPiNum(pMan) <= Abc_NtkCiNum(pNtkOld) );
    assert( Saig_ManPoNum(pMan) == Abc_NtkPoNum(pNtkOld) );
    assert( pMan->vCiNumsOrig != NULL );
    // perform strashing
    pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pNtkNew->nConstrs = pMan->nConstrs;
    pNtkNew->nBarBufs = pMan->nBarBufs;
    // duplicate the name and the spec
//    pNtkNew->pName = Extra_UtilStrsav(pMan->pName);
//    pNtkNew->pSpec = Extra_UtilStrsav(pMan->pSpec);
    Aig_ManConst1(pMan)->pData = Abc_AigConst1(pNtkNew);
    // create PIs
    Aig_ManForEachPiSeq( pMan, pObj, i )
    {
        pObjNew = Abc_NtkCreatePi( pNtkNew );
        pObj->pData = pObjNew;
        // find the name
        pObjOld = Abc_NtkCi( pNtkOld, Vec_IntEntry(pMan->vCiNumsOrig, i) );
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObjOld), NULL );
    }
    // create POs
    Aig_ManForEachPoSeq( pMan, pObj, i )
    {
        pObjNew = Abc_NtkCreatePo( pNtkNew );
        pObj->pData = pObjNew;
        // find the name
        pObjOld = Abc_NtkCo( pNtkOld, i );
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObjOld), NULL );
    }
    assert( Abc_NtkCiNum(pNtkNew) == Aig_ManCiNum(pMan) - Aig_ManRegNum(pMan) );
    assert( Abc_NtkCoNum(pNtkNew) == Aig_ManCoNum(pMan) - Aig_ManRegNum(pMan) );
    // create as many latches as there are registers in the manager
    Aig_ManForEachLiLoSeq( pMan, pObjLi, pObjLo, i )
    {
        pObjNew = Abc_NtkCreateLatch( pNtkNew );
        pObjLi->pData = Abc_NtkCreateBi( pNtkNew );
        pObjLo->pData = Abc_NtkCreateBo( pNtkNew );
        Abc_ObjAddFanin( pObjNew, (Abc_Obj_t *)pObjLi->pData );
        Abc_ObjAddFanin( (Abc_Obj_t *)pObjLo->pData, pObjNew );
        Abc_LatchSetInit0( pObjNew );
        // find the name
        pObjOld = Abc_NtkCi( pNtkOld, Vec_IntEntry(pMan->vCiNumsOrig, Saig_ManPiNum(pMan)+i) );
        Abc_ObjAssignName( (Abc_Obj_t *)pObjLo->pData, Abc_ObjName(pObjOld), NULL );
        // find the name
        pObjOld = Abc_NtkCo( pNtkOld, Saig_ManPoNum(pMan)+i );
        Abc_ObjAssignName( (Abc_Obj_t *)pObjLi->pData, Abc_ObjName(pObjOld), NULL );
    }
    // rebuild the AIG
    vNodes = Aig_ManDfs( pMan, 1 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        if ( Aig_ObjIsBuf(pObj) )
            pObj->pData = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
        else
            pObj->pData = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, (Abc_Obj_t *)Aig_ObjChild0Copy(pObj), (Abc_Obj_t *)Aig_ObjChild1Copy(pObj) );
    Vec_PtrFree( vNodes );
    // connect the PO nodes
    Aig_ManForEachCo( pMan, pObj, i )
    {
        pObjNew = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
        Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, i), pObjNew );
    }
    // check the resulting AIG
    if ( !Abc_NtkCheck( pNtkNew ) )
        Abc_Print( 1, "Abc_NtkAfterTrim(): Network check has failed.\n" );
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
    Abc_Ntk_t * pNtkNew;
    Aig_Obj_t * pObj, * pTemp;
    int i;
    assert( pMan->pEquivs != NULL );
    assert( Aig_ManBufNum(pMan) == 0 );
    // perform strashing
    pNtkNew = Abc_NtkStartFrom( pNtkOld, ABC_NTK_STRASH, ABC_FUNC_AIG );
    pNtkNew->nConstrs = pMan->nConstrs;
    pNtkNew->nBarBufs = pNtkOld->nBarBufs;
    // transfer the pointers to the basic nodes
    Aig_ManCleanData( pMan );
    Aig_ManConst1(pMan)->pData = Abc_AigConst1(pNtkNew);
    Aig_ManForEachCi( pMan, pObj, i )
        pObj->pData = Abc_NtkCi(pNtkNew, i);
    // rebuild the AIG
    Aig_ManForEachNode( pMan, pObj, i )
    {
        pObj->pData = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, (Abc_Obj_t *)Aig_ObjChild0Copy(pObj), (Abc_Obj_t *)Aig_ObjChild1Copy(pObj) );
        if ( (pTemp = Aig_ObjEquiv(pMan, pObj)) )
        {
            assert( pTemp->pData != NULL );
            ((Abc_Obj_t *)pObj->pData)->pData = ((Abc_Obj_t *)pTemp->pData);
        }
    }
    // connect the PO nodes
    Aig_ManForEachCo( pMan, pObj, i )
        Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, i), (Abc_Obj_t *)Aig_ObjChild0Copy(pObj) );
    if ( !Abc_NtkCheck( pNtkNew ) )
        Abc_Print( 1, "Abc_NtkFromDar(): Network check has failed.\n" );

    // verify topological order
    if ( 0 )
    {
        Abc_Obj_t * pNode;
        Abc_NtkForEachNode( pNtkNew, pNode, i )
            if ( Abc_AigNodeIsChoice( pNode ) )
            {
                int Counter = 0;
                for ( pNode = Abc_ObjEquiv(pNode); pNode; pNode = Abc_ObjEquiv(pNode) )
                    Counter++;
                printf( "%d ", Counter );
            }
        printf( "\n" );
    }
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
    assert( pNtkOld->nBarBufs == 0 );
    // perform strashing
    pNtkNew = Abc_NtkStartFromNoLatches( pNtkOld, ABC_NTK_STRASH, ABC_FUNC_AIG );
    pNtkNew->nConstrs = pMan->nConstrs;
    pNtkNew->nBarBufs = pMan->nBarBufs;
    // transfer the pointers to the basic nodes
    Aig_ManConst1(pMan)->pData = Abc_AigConst1(pNtkNew);
    Aig_ManForEachCi( pMan, pObj, i )
        pObj->pData = Abc_NtkPi(pNtkNew, i);
    // create latches of the new network
    Aig_ManForEachObj( pMan, pObj, i )
    {
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
    vNodes = Aig_ManDfs( pMan, 1 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        // add the first fanin
        pObj->pData = pFaninNew0 = (Abc_Obj_t *)Aig_ObjChild0Copy(pObj);
        if ( Aig_ObjIsBuf(pObj) )
            continue;
        // add the second fanin
        pFaninNew1 = (Abc_Obj_t *)Aig_ObjChild1Copy(pObj);
        // create the new node
        if ( Aig_ObjIsExor(pObj) )
            pObj->pData = pObjNew = Abc_AigXor( (Abc_Aig_t *)pNtkNew->pManFunc, pFaninNew0, pFaninNew1 );
        else
            pObj->pData = pObjNew = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, pFaninNew0, pFaninNew1 );
    }
    Vec_PtrFree( vNodes );
    // connect the PO nodes
    Aig_ManForEachCo( pMan, pObj, i )
    {
        pFaninNew = (Abc_Obj_t *)Aig_ObjChild0Copy( pObj );
        Abc_ObjAddFanin( Abc_NtkPo(pNtkNew, i), pFaninNew );
    }
    // connect the latches
    Aig_ManForEachObj( pMan, pObj, i )
    {
        pFaninNew = (Abc_Obj_t *)Aig_ObjChild0Copy( pObj );
        Abc_ObjAddFanin( Abc_ObjFanin0(Abc_ObjFanin0((Abc_Obj_t *)pObj->pData)), pFaninNew );
    }
    if ( !Abc_NtkCheck( pNtkNew ) )
        Abc_Print( 1, "Abc_NtkFromIvySeq(): Network check has failed.\n" );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Collects CI of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkCollectCiNames( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Vec_Ptr_t * vNames;
    vNames = Vec_PtrAlloc( 100 );
    Abc_NtkForEachCi( pNtk, pObj, i )
        Vec_PtrPush( vNames, Extra_UtilStrsav(Abc_ObjName(pObj)) );
    return vNames;
}

/**Function*************************************************************

  Synopsis    [Collects CO of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkCollectCoNames( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Vec_Ptr_t * vNames;
    vNames = Vec_PtrAlloc( 100 );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_PtrPush( vNames, Extra_UtilStrsav(Abc_ObjName(pObj)) );
    return vNames;
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
        if ( Abc_LatchIsInit0(pLatch) )
            Vec_IntPush( vInits, 0 );
        else if ( Abc_LatchIsInit1(pLatch) )
            Vec_IntPush( vInits, 1 );
        else if ( Abc_LatchIsInitDc(pLatch) )
            Vec_IntPush( vInits, 2 );
        else
            assert( 0 );
    }
    return vInits;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDar( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkAig = NULL;
    Aig_Man_t * pMan;
    extern void Fra_ManPartitionTest( Aig_Man_t * p, int nComLim );

    assert( Abc_NtkIsStrash(pNtk) );
    // convert to the AIG manager
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
    if ( pMan == NULL )
        return NULL;

    // perform computation
//    Fra_ManPartitionTest( pMan, 4 );
    pNtkAig = Abc_NtkFromDar( pNtk, pMan );
    Aig_ManStop( pMan );

    // make sure everything is okay
    if ( pNtkAig && !Abc_NtkCheck( pNtkAig ) )
    {
        Abc_Print( 1, "Abc_NtkDar: The network check has failed.\n" );
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
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
    if ( pMan == NULL )
        return NULL;
    Fra_ParamsDefault( pPars );
    pPars->nBTLimitNode = nConfLimit;
    pPars->fChoicing    = fChoicing;
    pPars->fDoSparse    = fDoSparse;
    pPars->fSpeculate   = fSpeculate;
    pPars->fProve       = fProve;
    pPars->fVerbose     = fVerbose;
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
Abc_Ntk_t * Abc_NtkDarFraigPart( Abc_Ntk_t * pNtk, int nPartSize, int nConfLimit, int nLevelMax, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
    if ( pMan == NULL )
        return NULL;
    pMan = Aig_ManFraigPartitioned( pTemp = pMan, nPartSize, nConfLimit, nLevelMax, fVerbose );
    Aig_ManStop( pTemp );
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
Abc_Ntk_t * Abc_NtkCSweep( Abc_Ntk_t * pNtk, int nCutsMax, int nLeafMax, int fVerbose )
{
//    extern Aig_Man_t * Csw_Sweep( Aig_Man_t * pAig, int nCutsMax, int nLeafMax, int fVerbose );
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
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
    abctime clk;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
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

clk = Abc_Clock();
    pMan = Aig_ManDupDfs( pTemp = pMan ); 
    Aig_ManStop( pTemp );
//ABC_PRT( "time", Abc_Clock() - clk );

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
    abctime clk;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
    if ( pMan == NULL )
        return NULL;
//    Aig_ManPrintStats( pMan );

    Dar_ManRefactor( pMan, pPars );
//    pMan = Dar_ManBalance( pTemp = pMan, pPars->fUpdateLevel );
//    Aig_ManStop( pTemp );

clk = Abc_Clock();
    pMan = Aig_ManDupDfs( pTemp = pMan ); 
    Aig_ManStop( pTemp );
//ABC_PRT( "time", Abc_Clock() - clk );

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
Abc_Ntk_t * Abc_NtkDC2( Abc_Ntk_t * pNtk, int fBalance, int fUpdateLevel, int fFanout, int fPower, int fVerbose )
{
    Aig_Man_t * pMan, * pTemp;
    Abc_Ntk_t * pNtkAig;
    abctime clk;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
    if ( pMan == NULL )
        return NULL;
//    Aig_ManPrintStats( pMan );

clk = Abc_Clock();
    pMan = Dar_ManCompress2( pTemp = pMan, fBalance, fUpdateLevel, fFanout, fPower, fVerbose ); 
    Aig_ManStop( pTemp );
//ABC_PRT( "time", Abc_Clock() - clk );

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
Abc_Ntk_t * Abc_NtkDChoice( Abc_Ntk_t * pNtk, int fBalance, int fUpdateLevel, int fConstruct, int nConfMax, int nLevelMax, int fVerbose )
{
    Aig_Man_t * pMan, * pTemp;
    Abc_Ntk_t * pNtkAig;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
    if ( pMan == NULL )
        return NULL;
    pMan = Dar_ManChoice( pTemp = pMan, fBalance, fUpdateLevel, fConstruct, nConfMax, nLevelMax, fVerbose );
    Aig_ManStop( pTemp );
    pNtkAig = Abc_NtkFromDarChoices( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDch( Abc_Ntk_t * pNtk, Dch_Pars_t * pPars )
{
    extern Gia_Man_t * Dar_NewChoiceSynthesis( Aig_Man_t * pAig, int fBalance, int fUpdateLevel, int fPower, int fLightSynth, int fVerbose );
    extern Aig_Man_t * Cec_ComputeChoices( Gia_Man_t * pGia, Dch_Pars_t * pPars );

    Aig_Man_t * pMan, * pTemp;
    Abc_Ntk_t * pNtkAig;
    Gia_Man_t * pGia;
    abctime clk;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
    if ( pMan == NULL )
        return NULL;
clk = Abc_Clock();
    if ( pPars->fSynthesis )
        pGia = Dar_NewChoiceSynthesis( pMan, 1, 1, pPars->fPower, pPars->fLightSynth, pPars->fVerbose );
    else
    {
        pGia = Gia_ManFromAig( pMan );
        Aig_ManStop( pMan );
    }
pPars->timeSynth = Abc_Clock() - clk;
    if ( pPars->fUseGia )
        pMan = Cec_ComputeChoices( pGia, pPars );
    else
    {
        pMan = Gia_ManToAigSkip( pGia, 3 );
        Gia_ManStop( pGia );
        pMan = Dch_ComputeChoices( pTemp = pMan, pPars );
        Aig_ManStop( pTemp );
    }
    pNtkAig = Abc_NtkFromDarChoices( pNtk, pMan );
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
    Aig_Man_t * pMan, * pTemp;
    Abc_Ntk_t * pNtkAig;
    abctime clk;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
    if ( pMan == NULL )
        return NULL;
//    Aig_ManPrintStats( pMan );

clk = Abc_Clock();
    pMan = Dar_ManRwsat( pTemp = pMan, fBalance, fVerbose ); 
    Aig_ManStop( pTemp );
//ABC_PRT( "time", Abc_Clock() - clk );

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
        Aig_ManCi(p->pManAig, i)->pData = pNode->pCopy;
    // process the nodes in topological order
    vCover = Vec_IntAlloc( 1 << 16 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vMapped, pObj, i )
    {
        // create new node
        pNodeNew = Abc_NtkCreateNode( pNtkNew );
        // add fanins according to the cut
        pCut = (Cnf_Cut_t *)pObj->pData;
        Cnf_CutForEachLeaf( p->pManAig, pCut, pLeaf, k )
            Abc_ObjAddFanin( pNodeNew, (Abc_Obj_t *)pLeaf->pData );
        // add logic function
        if ( pCut->nFanins < 5 )
        {
            uTruth = 0xFFFF & *Cnf_CutTruth(pCut);
            Cnf_SopConvertToVector( p->pSops[uTruth], p->pSopSizes[uTruth], vCover );
            pNodeNew->pData = Abc_SopCreateFromIsop( (Mem_Flex_t *)pNtkNew->pManFunc, pCut->nFanins, vCover );
        }
        else
            pNodeNew->pData = Abc_SopCreateFromIsop( (Mem_Flex_t *)pNtkNew->pManFunc, pCut->nFanins, pCut->vIsop[1] );
        // save the node
        pObj->pData = pNodeNew;
    }
    Vec_IntFree( vCover );
    // add the CO drivers
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pObj = Aig_ManCo(p->pManAig, i);
        pNodeNew = Abc_ObjNotCond( (Abc_Obj_t *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
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
//        Abc_Print( 1, "Duplicated %d gates to decouple the CO drivers.\n", nDupGates );
    if ( !Abc_NtkCheck( pNtkNew ) )
        Abc_Print( 1, "Abc_NtkConstructFromCnf(): Network check has failed.\n" );
    return pNtkNew;
}
 
/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarToCnf( Abc_Ntk_t * pNtk, char * pFileName, int fFastAlgo, int fChangePol, int fVerbose )
{
//    Vec_Ptr_t * vMapped = NULL;
    Aig_Man_t * pMan;
//    Cnf_Man_t * pManCnf = NULL;
    Cnf_Dat_t * pCnf;
    Abc_Ntk_t * pNtkNew = NULL;
    abctime clk = Abc_Clock();
    assert( Abc_NtkIsStrash(pNtk) );

    // convert to the AIG manager
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
    if ( pMan == NULL )
        return NULL;
    if ( !Aig_ManCheck( pMan ) )
    {
        Abc_Print( 1, "Abc_NtkDarToCnf: AIG check has failed.\n" );
        Aig_ManStop( pMan );
        return NULL;
    }
    // perform balance
    if ( fVerbose )
    Aig_ManPrintStats( pMan );

    // derive CNF
    if ( fFastAlgo )
        pCnf = Cnf_DeriveFast( pMan, 0 );
    else
        pCnf = Cnf_Derive( pMan, 0 );

    // adjust polarity
    if ( fChangePol )
        Cnf_DataTranformPolarity( pCnf, 0 );

    // print stats
//    if ( fVerbose )
    {
        Abc_Print( 1, "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d.   ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }

/*
    // write the network for verification
    pManCnf = Cnf_ManRead();
    vMapped = Cnf_ManScanMapping( pManCnf, 1, 0 );
    pNtkNew = Abc_NtkConstructFromCnf( pNtk, pManCnf, vMapped );
    Vec_PtrFree( vMapped );
*/
    // write CNF into a file
    Cnf_DataWriteIntoFile( pCnf, pFileName, 0, NULL, NULL );
    Cnf_DataFree( pCnf );
    Cnf_ManFree();
    Aig_ManStop( pMan );
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    [Solves combinational miter using a SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDSat( Abc_Ntk_t * pNtk, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, int nLearnedStart, int nLearnedDelta, int nLearnedPerce, int fAlignPol, int fAndOuts, int fNewSolver, int fVerbose )
{
    Aig_Man_t * pMan;
    int RetValue;//, clk = Abc_Clock();
    assert( Abc_NtkIsStrash(pNtk) );
    assert( Abc_NtkLatchNum(pNtk) == 0 );
//    assert( Abc_NtkPoNum(pNtk) == 1 );
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
    RetValue = Fra_FraigSat( pMan, nConfLimit, nInsLimit, nLearnedStart, nLearnedDelta, nLearnedPerce, fAlignPol, fAndOuts, fNewSolver, fVerbose ); 
    pNtk->pModel = (int *)pMan->pData, pMan->pData = NULL;
    Aig_ManStop( pMan );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Solves combinational miter using a SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkPartitionedSat( Abc_Ntk_t * pNtk, int nAlgo, int nPartSize, int nConfPart, int nConfTotal, int fAlignPol, int fSynthesize, int fVerbose )
{
    extern int Aig_ManPartitionedSat( Aig_Man_t * pNtk, int nAlgo, int nPartSize, int nConfPart, int nConfTotal, int fAlignPol, int fSynthesize, int fVerbose );
    Aig_Man_t * pMan;
    int RetValue;//, clk = Abc_Clock();
    assert( Abc_NtkIsStrash(pNtk) );
    assert( Abc_NtkLatchNum(pNtk) == 0 );
    pMan = Abc_NtkToDar( pNtk, 0, 0 );
    RetValue = Aig_ManPartitionedSat( pMan, nAlgo, nPartSize, nConfPart, nConfTotal, fAlignPol, fSynthesize, fVerbose ); 
    pNtk->pModel = (int *)pMan->pData, pMan->pData = NULL;
    Aig_ManStop( pMan );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarCec( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nConfLimit, int fPartition, int fVerbose )
{
    Aig_Man_t * pMan, * pMan1, * pMan2;
    Abc_Ntk_t * pMiter;
    int RetValue;
    abctime clkTotal = Abc_Clock();
/*
    {
    extern void Cec_ManVerifyTwoAigs( Aig_Man_t * pAig0, Aig_Man_t * pAig1, int fVerbose );
    Aig_Man_t * pAig0 = Abc_NtkToDar( pNtk1, 0, 0 );
    Aig_Man_t * pAig1 = Abc_NtkToDar( pNtk2, 0, 0 );
    Cec_ManVerifyTwoAigs( pAig0, pAig1, 1 );
    Aig_ManStop( pAig0 );
    Aig_ManStop( pAig1 );
    return 1;
    }
*/
    // cannot partition if it is already a miter
    if ( pNtk2 == NULL && fPartition == 1 )
    {
        Abc_Print( 1, "Abc_NtkDarCec(): Switching to non-partitioned CEC for the miter.\n" );
        fPartition = 0;
    }

    // if partitioning is selected, call partitioned CEC
    if ( fPartition )
    {
        pMan1 = Abc_NtkToDar( pNtk1, 0, 0 );
        pMan2 = Abc_NtkToDar( pNtk2, 0, 0 );
        RetValue = Fra_FraigCecPartitioned( pMan1, pMan2, nConfLimit, 100, 1, fVerbose );
        Aig_ManStop( pMan1 );
        Aig_ManStop( pMan2 );
        goto finish;
    }

    if ( pNtk2 != NULL )
    {
        // get the miter of the two networks
        pMiter = Abc_NtkMiter( pNtk1, pNtk2, 0, 0, 0, 0 );
        if ( pMiter == NULL )
        {
            Abc_Print( 1, "Miter computation has failed.\n" );
            return 0;
        }
    }
    else
    {
        pMiter = Abc_NtkDup( pNtk1 );
    }
    RetValue = Abc_NtkMiterIsConstant( pMiter );
    if ( RetValue == 0 )
    {
//        extern void Abc_NtkVerifyReportErrorSeq( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int * pModel, int nFrames );
        Abc_Print( 1, "Networks are NOT EQUIVALENT after structural hashing.\n" );
        // report the error
        if ( pNtk2 == NULL )
            pNtk1->pModel = Abc_NtkVerifyGetCleanModel( pNtk1, 1 );
//        pMiter->pModel = Abc_NtkVerifyGetCleanModel( pMiter, nFrames );
//        Abc_NtkVerifyReportErrorSeq( pNtk1, pNtk2, pMiter->pModel, nFrames );
//        ABC_FREE( pMiter->pModel );
        Abc_NtkDelete( pMiter );
        return 0;
    }
    if ( RetValue == 1 )
    {
        Abc_NtkDelete( pMiter );
        Abc_Print( 1, "Networks are equivalent after structural hashing.\n" );
        return 1;
    }

    // derive the AIG manager
    pMan = Abc_NtkToDar( pMiter, 0, 0 );
    Abc_NtkDelete( pMiter );
    if ( pMan == NULL )
    {
        Abc_Print( 1, "Converting miter into AIG has failed.\n" );
        return -1;
    }
    // perform verification
    RetValue = Fra_FraigCec( &pMan, 100000, fVerbose );
    // transfer model if given
    if ( pNtk2 == NULL )
        pNtk1->pModel = (int *)pMan->pData, pMan->pData = NULL;
    Aig_ManStop( pMan );

finish:
    // report the miter
    if ( RetValue == 1 )
    {
        Abc_Print( 1, "Networks are equivalent.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    else if ( RetValue == 0 )
    {
        Abc_Print( 1, "Networks are NOT EQUIVALENT.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    else
    {
        Abc_Print( 1, "Networks are UNDECIDED.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    fflush( stdout );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarSeqSweep( Abc_Ntk_t * pNtk, Fra_Ssw_t * pPars )
{
    Fraig_Params_t Params;
    Abc_Ntk_t * pNtkAig = NULL, * pNtkFraig;
    Aig_Man_t * pMan, * pTemp;
    abctime clk = Abc_Clock();

    // preprocess the miter by fraiging it
    // (note that for each functional class, fraiging leaves one representative;
    // so fraiging does not reduce the number of functions represented by nodes
    Fraig_ParamsSetDefault( &Params );
    Params.nBTLimit = 100000;
    if ( pPars->fFraiging && pPars->nPartSize == 0 )
    {
        pNtkFraig = Abc_NtkFraig( pNtk, &Params, 0, 0 );
if ( pPars->fVerbose ) 
{
ABC_PRT( "Initial fraiging time", Abc_Clock() - clk );
}
    }
    else
        pNtkFraig = Abc_NtkDup( pNtk );

    pMan = Abc_NtkToDar( pNtkFraig, 0, 1 );
    Abc_NtkDelete( pNtkFraig );
    if ( pMan == NULL )
        return NULL;

//    pPars->TimeLimit = 5.0;
    pMan = Fra_FraigInduction( pTemp = pMan, pPars );
    Aig_ManStop( pTemp );
    if ( pMan )
    {
        if ( Aig_ManRegNum(pMan) < Abc_NtkLatchNum(pNtk) )
            pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
        else
        {
            Abc_Obj_t * pObj;
            int i;
            pNtkAig = Abc_NtkFromDar( pNtk, pMan );
            Abc_NtkForEachLatch( pNtkAig, pObj, i )
                Abc_LatchSetInit0( pObj );
        }
        Aig_ManStop( pMan );
    }
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Print Latch Equivalence Classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintLatchEquivClasses( Abc_Ntk_t * pNtk, Aig_Man_t * pAig )
{
    int header_dumped = 0;
    int num_orig_latches = Abc_NtkLatchNum(pNtk);
    char **pNames = ABC_ALLOC( char *, num_orig_latches );
    int *p_irrelevant = ABC_ALLOC( int, num_orig_latches );
    char * pFlopName, * pReprName;
    Aig_Obj_t * pFlop, * pRepr;
    Abc_Obj_t * pNtkFlop; 
    int repr_idx;
    int i;

    Abc_NtkForEachLatch( pNtk, pNtkFlop, i )
    {
        char *temp_name = Abc_ObjName( Abc_ObjFanout0(pNtkFlop) );
        pNames[i] = ABC_ALLOC( char , strlen(temp_name)+1);
        strcpy(pNames[i], temp_name);
    }
    i = 0;
    
    Aig_ManSetCioIds( pAig );
    Saig_ManForEachLo( pAig, pFlop, i )
    {
        p_irrelevant[i] = false;
        
        pFlopName = pNames[i];

        pRepr = Aig_ObjRepr(pAig, pFlop);

        if ( pRepr == NULL )
        {
            // Abc_Print( 1, "Nothing equivalent to flop %s\n", pFlopName);
//            p_irrelevant[i] = true;
            continue;
        }

        if (!header_dumped)
        {
            Abc_Print( 1, "Here are the flop equivalences:\n");
            header_dumped = true;
        }

        // pRepr is representative of the equivalence class, to which pFlop belongs
        if ( Aig_ObjIsConst1(pRepr) )
        {
            Abc_Print( 1, "Original flop %s is proved equivalent to constant.\n", pFlopName );
            // Abc_Print( 1, "Original flop # %d is proved equivalent to constant.\n", i );
            continue;
        }

        assert( Saig_ObjIsLo( pAig, pRepr ) );
        repr_idx = Aig_ObjCioId(pRepr) - Saig_ManPiNum(pAig);
        pReprName = pNames[repr_idx];
        Abc_Print( 1, "Original flop %s is proved equivalent to flop %s.\n",  pFlopName, pReprName );
        // Abc_Print( 1, "Original flop # %d is proved equivalent to flop # %d.\n",  i, repr_idx );
    }

    header_dumped = false;
    for (i=0; i<num_orig_latches; ++i)
    {
        if (p_irrelevant[i])
        {
            if (!header_dumped)
            {
                Abc_Print( 1, "The following flops have been deemed irrelevant:\n");
                header_dumped = true;
            }
            Abc_Print( 1, "%s ", pNames[i]);
        }
        
        ABC_FREE(pNames[i]);
    }
    if (header_dumped)
        Abc_Print( 1, "\n");
    
    ABC_FREE(pNames);
    ABC_FREE(p_irrelevant);
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarSeqSweep2( Abc_Ntk_t * pNtk, Ssw_Pars_t * pPars )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;

    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;

    pMan = Ssw_SignalCorrespondence( pTemp = pMan, pPars );

    if ( pPars->fFlopVerbose )
        Abc_NtkPrintLatchEquivClasses(pNtk, pTemp);

    Aig_ManStop( pTemp );
    if ( pMan == NULL )
        return NULL;

    if ( Aig_ManRegNum(pMan) < Abc_NtkLatchNum(pNtk) )
        pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
    else
    {
        Abc_Obj_t * pObj;
        int i;
        pNtkAig = Abc_NtkFromDar( pNtk, pMan );
        Abc_NtkForEachLatch( pNtkAig, pObj, i )
            Abc_LatchSetInit0( pObj );
    }
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Computes latch correspondence.]

  Description [] 
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
Abc_Ntk_t * Abc_NtkDarLcorr( Abc_Ntk_t * pNtk, int nFramesP, int nConfMax, int fVerbose )
{
    Aig_Man_t * pMan, * pTemp;
    Abc_Ntk_t * pNtkAig = NULL;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    pMan = Fra_FraigLatchCorrespondence( pTemp = pMan, nFramesP, nConfMax, 0, fVerbose, NULL, 0.0 );
    Aig_ManStop( pTemp );
    if ( pMan )
    {
        if ( Aig_ManRegNum(pMan) < Abc_NtkLatchNum(pNtk) )
            pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
        else
        {
            Abc_Obj_t * pObj;
            int i;
            pNtkAig = Abc_NtkFromDar( pNtk, pMan );
            Abc_NtkForEachLatch( pNtkAig, pObj, i )
                Abc_LatchSetInit0( pObj );
        }
        Aig_ManStop( pMan );
    }
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Computes latch correspondence.]

  Description [] 
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
Abc_Ntk_t * Abc_NtkDarLcorrNew( Abc_Ntk_t * pNtk, int nVarsMax, int nConfMax, int fVerbose )
{
    Ssw_Pars_t Pars, * pPars = &Pars;
    Aig_Man_t * pMan, * pTemp;
    Abc_Ntk_t * pNtkAig = NULL;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    Ssw_ManSetDefaultParams( pPars );
    pPars->fLatchCorrOpt = 1;
    pPars->nBTLimit      = nConfMax;
    pPars->nSatVarMax    = nVarsMax;
    pPars->fVerbose      = fVerbose;
    pMan = Ssw_SignalCorrespondence( pTemp = pMan, pPars );
    Aig_ManStop( pTemp );
    if ( pMan )
    {
        if ( Aig_ManRegNum(pMan) < Abc_NtkLatchNum(pNtk) )
            pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
        else
        {
            Abc_Obj_t * pObj;
            int i;
            pNtkAig = Abc_NtkFromDar( pNtk, pMan );
            Abc_NtkForEachLatch( pNtkAig, pObj, i )
                Abc_LatchSetInit0( pObj );
        }
        Aig_ManStop( pMan );
    }
    return pNtkAig;
}

/*
#include <signal.h>
#include "misc/util/utilMem.h"
static void sigfunc( int signo ) 
{
    if (signo == SIGINT) {
        Abc_Print( 1, "SIGINT received!\n");
        s_fInterrupt = 1;
    }
}
*/

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarBmc( Abc_Ntk_t * pNtk, int nStart, int nFrames, int nSizeMax, int nNodeDelta, int nTimeOut, int nBTLimit, int nBTLimitAll, int fRewrite, int fNewAlgo, int fOrDecomp, int nCofFanLit, int fVerbose, int * piFrames, int fUseSatoko )
{
    Aig_Man_t * pMan;
    Vec_Int_t * vMap = NULL;
    int status, RetValue = -1;
    abctime clk = Abc_Clock();
    abctime nTimeLimit = nTimeOut ? nTimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0;
    // derive the AIG manager
    if ( fOrDecomp )
        pMan = Abc_NtkToDarBmc( pNtk, &vMap );
    else
        pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
    {
        Abc_Print( 1, "Converting miter into AIG has failed.\n" );
        return RetValue;
    }
    assert( pMan->nRegs > 0 );
    assert( vMap == NULL || Vec_IntSize(vMap) == Saig_ManPoNum(pMan) );
    if ( fVerbose && vMap && Abc_NtkPoNum(pNtk) != Saig_ManPoNum(pMan) ) 
        Abc_Print( 1, "Expanded %d outputs into %d outputs using OR decomposition.\n", Abc_NtkPoNum(pNtk), Saig_ManPoNum(pMan) );

    // perform verification
    if ( fNewAlgo ) // command 'bmc'
    {
        int iFrame;
        RetValue = Saig_ManBmcSimple( pMan, nFrames, nSizeMax, nBTLimit, fRewrite, fVerbose, &iFrame, nCofFanLit, fUseSatoko );
        if ( piFrames )
            *piFrames = iFrame;
        ABC_FREE( pNtk->pModel );
        ABC_FREE( pNtk->pSeqModel );
        pNtk->pSeqModel = pMan->pSeqModel; pMan->pSeqModel = NULL;
        if ( RetValue == 1 )
            Abc_Print( 1, "Incorrect return value.  " );
        else if ( RetValue == -1 )
        {
            Abc_Print( 1, "No output asserted in %d frames. Resource limit reached ", Abc_MaxInt(iFrame+1,0) );
            if ( nTimeLimit && Abc_Clock() > nTimeLimit )
                Abc_Print( 1, "(timeout %d sec). ", nTimeLimit );
            else
                Abc_Print( 1, "(conf limit %d). ", nBTLimit );
        }
        else // if ( RetValue == 0 )
        {
            Abc_Cex_t * pCex = pNtk->pSeqModel;
            Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d. ", pCex->iPo, pNtk->pName, pCex->iFrame );
        }
ABC_PRT( "Time", Abc_Clock() - clk );
    }
    else
    { 
        RetValue = Saig_BmcPerform( pMan, nStart, nFrames, nNodeDelta, nTimeOut, nBTLimit, nBTLimitAll, fVerbose, 0, piFrames, 0, fUseSatoko );
        ABC_FREE( pNtk->pModel );
        ABC_FREE( pNtk->pSeqModel );
        pNtk->pSeqModel = pMan->pSeqModel; pMan->pSeqModel = NULL;
    }
    // verify counter-example
    if ( pNtk->pSeqModel ) 
    {
        status = Saig_ManVerifyCex( pMan, pNtk->pSeqModel );
        if ( status == 0 )
            Abc_Print( 1, "Abc_NtkDarBmc(): Counter-example verification has FAILED.\n" );
    }
    Aig_ManStop( pMan );
    // update the counter-example
    if ( pNtk->pSeqModel && vMap )
        pNtk->pSeqModel->iPo = Vec_IntEntry( vMap, pNtk->pSeqModel->iPo );
    Vec_IntFreeP( &vMap );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarBmc3( Abc_Ntk_t * pNtk, Saig_ParBmc_t * pPars, int fOrDecomp )
{
    Aig_Man_t * pMan;
    Vec_Int_t * vMap = NULL;
    int status, RetValue = -1;
    abctime clk = Abc_Clock();
    abctime nTimeOut = pPars->nTimeOut ? pPars->nTimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0;
    if ( fOrDecomp && !pPars->fSolveAll )
        pMan = Abc_NtkToDarBmc( pNtk, &vMap );
    else
        pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
    {
        Abc_Print( 1, "Converting miter into AIG has failed.\n" );
        return RetValue;
    }
    assert( pMan->nRegs > 0 );
    if ( pPars->fVerbose && vMap && Abc_NtkPoNum(pNtk) != Saig_ManPoNum(pMan) ) 
        Abc_Print( 1, "Expanded %d outputs into %d outputs using OR decomposition.\n", Abc_NtkPoNum(pNtk), Saig_ManPoNum(pMan) );

    RetValue = Saig_ManBmcScalable( pMan, pPars );
    ABC_FREE( pNtk->pModel );
    ABC_FREE( pNtk->pSeqModel );
    pNtk->pSeqModel = pMan->pSeqModel; pMan->pSeqModel = NULL;
    if ( !pPars->fSilent )
    {
        if ( RetValue == 1 )
        {
            Abc_Print( 1, "Explored all reachable states after completing %d frames.  ", 1<<Aig_ManRegNum(pMan) );
        }
        else if ( RetValue == -1 )
        {
            if ( pPars->nFailOuts == 0 )
            {
                Abc_Print( 1, "No output asserted in %d frames. Resource limit reached ", Abc_MaxInt(pPars->iFrame+1,0) );
                if ( nTimeOut && Abc_Clock() > nTimeOut )
                    Abc_Print( 1, "(timeout %d sec). ", pPars->nTimeOut );
                else
                    Abc_Print( 1, "(conf limit %d). ", pPars->nConfLimit );
            }
            else
            {
                Abc_Print( 1, "The total of %d outputs asserted in %d frames. Resource limit reached ", pPars->nFailOuts, pPars->iFrame );
                if ( Abc_Clock() > nTimeOut )
                    Abc_Print( 1, "(timeout %d sec). ", pPars->nTimeOut );
                else
                    Abc_Print( 1, "(conf limit %d). ", pPars->nConfLimit );
            }
        }
        else // if ( RetValue == 0 )
        {
            if ( !pPars->fSolveAll )
            {
                Abc_Cex_t * pCex = pNtk->pSeqModel;
                Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d. ", pCex->iPo, pNtk->pName, pCex->iFrame );
            }
            else
            {
                int nOutputs = Saig_ManPoNum(pMan) - Saig_ManConstrNum(pMan);
                if ( pMan->vSeqModelVec == NULL || Vec_PtrCountZero(pMan->vSeqModelVec) == nOutputs )
                    Abc_Print( 1, "None of the %d outputs is found to be SAT", nOutputs );
                else if ( Vec_PtrCountZero(pMan->vSeqModelVec) == 0 )
                    Abc_Print( 1, "All %d outputs are found to be SAT", nOutputs );
                else
                {
                    Abc_Print( 1, "Some outputs are SAT (%d out of %d)", nOutputs - Vec_PtrCountZero(pMan->vSeqModelVec), nOutputs );
                    if ( pPars->nDropOuts )
                        Abc_Print( 1, " while others timed out (%d out of %d)", pPars->nDropOuts, nOutputs );
                }
                Abc_Print( 1, " after %d frames", pPars->iFrame+2 );
                Abc_Print( 1, ".   " );
            }
        }
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    if ( RetValue == 0 && pPars->fSolveAll )
    {
        if ( pNtk->vSeqModelVec )
            Vec_PtrFreeFree( pNtk->vSeqModelVec );
        pNtk->vSeqModelVec = pMan->vSeqModelVec;  pMan->vSeqModelVec = NULL;
    }
    if ( pNtk->pSeqModel ) 
    {
        status = Saig_ManVerifyCex( pMan, pNtk->pSeqModel );
        if ( status == 0 )
            Abc_Print( 1, "Abc_NtkDarBmc3(): Counter-example verification has FAILED.\n" );
    }
    Aig_ManStop( pMan );
    // update the counter-example
    if ( pNtk->pSeqModel && vMap )
        pNtk->pSeqModel->iPo = Vec_IntEntry( vMap, pNtk->pSeqModel->iPo );
    Vec_IntFreeP( &vMap );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarBmcInter_int( Aig_Man_t * pMan, Inter_ManParams_t * pPars, Aig_Man_t ** ppNtkRes )
{
    int RetValue = -1, iFrame;
    abctime clk = Abc_Clock();
    int nTotalProvedSat = 0;
    assert( pMan->nRegs > 0 );
    if ( ppNtkRes )
        *ppNtkRes = NULL;
    if ( pPars->fUseSeparate )
    {
        Aig_Man_t * pTemp, * pAux;
        Aig_Obj_t * pObjPo;
        int i, Counter = 0;
        Saig_ManForEachPo( pMan, pObjPo, i )
        {
            if ( Aig_ObjFanin0(pObjPo) == Aig_ManConst1(pMan) )
                continue;
            if ( pPars->fVerbose )
                Abc_Print( 1, "Solving output %2d (out of %2d):\n", i, Saig_ManPoNum(pMan) );
            pTemp = Aig_ManDupOneOutput( pMan, i, 1 );
            pTemp = Aig_ManScl( pAux = pTemp, 1, 1, 0, -1, -1, 0, 0 );
            Aig_ManStop( pAux );
            if ( Aig_ManRegNum(pTemp) == 0 )
            {
                pTemp->pSeqModel = NULL;
                RetValue = Fra_FraigSat( pTemp, pPars->nBTLimit, 0, 0, 0, 0, 0, 0, 0, 0 ); 
                if ( pTemp->pData )
                    pTemp->pSeqModel = Abc_CexCreate( Aig_ManRegNum(pMan), Saig_ManPiNum(pMan), (int *)pTemp->pData, 0, i, 1 );
//                pNtk->pModel = pTemp->pData, pTemp->pData = NULL;
            }
            else
                RetValue = Inter_ManPerformInterpolation( pTemp, pPars, &iFrame );
            if ( pTemp->pSeqModel )
            {
                if ( pPars->fDropSatOuts )
                {
                    Abc_Print( 1, "Output %d proved SAT in frame %d (replacing by const 0 and continuing...)\n", i, pTemp->pSeqModel->iFrame );
                    Aig_ObjPatchFanin0( pMan, pObjPo, Aig_ManConst0(pMan) );
                    Aig_ManStop( pTemp );
                    nTotalProvedSat++;
                    continue;
                }
                else
                {
                    Abc_Cex_t * pCex;
                    pCex = pMan->pSeqModel = pTemp->pSeqModel; pTemp->pSeqModel = NULL;
                    pCex->iPo = i;
                    Aig_ManStop( pTemp );
                    break;
                }
            }
            // if solved, remove the output
            if ( RetValue == 1 )
            {
                Aig_ObjPatchFanin0( pMan, pObjPo, Aig_ManConst0(pMan) );
//                    Abc_Print( 1, "Output %3d : Solved ", i );
            }
            else
            {
                Counter++;
//                    Abc_Print( 1, "Output %3d : Undec  ", i );
            }
//                Aig_ManPrintStats( pTemp );
            Aig_ManStop( pTemp );
            Abc_Print( 1, "Solving output %3d (out of %3d) using interpolation.\r", i, Saig_ManPoNum(pMan) );
        }
        Aig_ManCleanup( pMan );
        if ( pMan->pSeqModel == NULL )
        {
            Abc_Print( 1, "Interpolation left %d (out of %d) outputs unsolved              \n", Counter, Saig_ManPoNum(pMan) );
            if ( Counter )
                RetValue = -1;
        }
        if ( ppNtkRes )
        {
            pTemp = Aig_ManDupUnsolvedOutputs( pMan, 1 );
            *ppNtkRes = Aig_ManScl( pTemp, 1, 1, 0, -1, -1, 0, 0 );
            Aig_ManStop( pTemp );
        }
    }
    else
    {    
        RetValue = Inter_ManPerformInterpolation( pMan, pPars, &iFrame );
    }
    if ( nTotalProvedSat )
        Abc_Print( 1, "The total of %d outputs proved SAT and replaced by const 0 in this run.\n", nTotalProvedSat );
    if ( RetValue == 1 )
        Abc_Print( 1, "Property proved.  " );
    else if ( RetValue == 0 )
        Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d.  ", pMan->pSeqModel ? pMan->pSeqModel->iPo : -1, pMan->pName, iFrame );
    else if ( RetValue == -1 )
        Abc_Print( 1, "Property UNDECIDED.  " );
    else
        assert( 0 );
ABC_PRT( "Time", Abc_Clock() - clk );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarBmcInter( Abc_Ntk_t * pNtk, Inter_ManParams_t * pPars, Abc_Ntk_t ** ppNtkRes )
{
    Aig_Man_t * pMan;
    int RetValue;
    if ( ppNtkRes )
        *ppNtkRes = NULL;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
    {
        Abc_Print( 1, "Converting miter into AIG has failed.\n" );
        return -1;
    }
    if ( pPars->fUseSeparate && ppNtkRes )
    {
        Aig_Man_t * pManNew;
        RetValue = Abc_NtkDarBmcInter_int( pMan, pPars, &pManNew );
        *ppNtkRes = Abc_NtkFromAigPhase( pManNew );
        Aig_ManStop( pManNew );
    }
    else
    {
        RetValue = Abc_NtkDarBmcInter_int( pMan, pPars, NULL );
    }
    ABC_FREE( pNtk->pModel );
    ABC_FREE( pNtk->pSeqModel );
    pNtk->pSeqModel = pMan->pSeqModel; pMan->pSeqModel = NULL;
    Aig_ManStop( pMan );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarDemiter( Abc_Ntk_t * pNtk )
{ 
    char * pFileNameGeneric, pFileName0[1000], pFileName1[1000];
    Aig_Man_t * pMan, * pPart0, * pPart1;//, * pMiter;
    // derive the AIG manager
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
    {
        Abc_Print( 1, "Converting network into AIG has failed.\n" );
        return 0;
    }
//    if ( !Saig_ManDemiterSimple( pMan, &pPart0, &pPart1 ) )
    if ( !Saig_ManDemiterSimpleDiff( pMan, &pPart0, &pPart1 ) )
    {
        Aig_ManStop( pMan );
        Abc_Print( 1, "Demitering has failed.\n" );
        return 0;
    }
    // create file names
    pFileNameGeneric = Extra_FileNameGeneric( pNtk->pSpec ? pNtk->pSpec : pNtk->pName );
//    sprintf( pFileName0,  "%s%s",  pFileNameGeneric, "_part0.aig" ); 
//    sprintf( pFileName1,  "%s%s",  pFileNameGeneric, "_part1.aig" ); 
    sprintf( pFileName0,  "%s",  "part0.aig" ); 
    sprintf( pFileName1,  "%s",  "part1.aig" ); 
    ABC_FREE( pFileNameGeneric );
    // dump files
    Ioa_WriteAiger( pPart0, pFileName0, 0, 0 );
    Ioa_WriteAiger( pPart1, pFileName1, 0, 0 );
    Abc_Print( 1, "Demitering produced two files \"%s\" and \"%s\".\n", pFileName0, pFileName1 );
    // create two-level miter
//    pMiter = Saig_ManCreateMiterTwo( pPart0, pPart1, 2 );
//    Aig_ManDumpBlif( pMiter, "miter01.blif", NULL, NULL );
//    Aig_ManStop( pMiter );
//    Abc_Print( 1, "The new miter is written into file \"%s\".\n", "miter01.blif" );
    Aig_ManStop( pPart0 );
    Aig_ManStop( pPart1 );
    Aig_ManStop( pMan );
    return 1;
} 

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarDemiterNew( Abc_Ntk_t * pNtk )
{ 
    char * pFileNameGeneric, pFileName0[1000], pFileName1[1000];
    Aig_Man_t * pMan, * pPart0, * pPart1;//, * pMiter;
    // derive the AIG manager
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
    {
        Abc_Print( 1, "Converting network into AIG has failed.\n" );
        return 0;
    }

    Saig_ManDemiterNew( pMan );
    Aig_ManStop( pMan );
    return 1;

//    if ( !Saig_ManDemiterSimple( pMan, &pPart0, &pPart1 ) )
    if ( !Saig_ManDemiterSimpleDiff( pMan, &pPart0, &pPart1 ) )
    {
        Abc_Print( 1, "Demitering has failed.\n" );
        return 0;
    }
    // create file names
    pFileNameGeneric = Extra_FileNameGeneric( pNtk->pSpec );
    sprintf( pFileName0,  "%s%s",  pFileNameGeneric, "_part0.aig" ); 
    sprintf( pFileName1,  "%s%s",  pFileNameGeneric, "_part1.aig" ); 
    ABC_FREE( pFileNameGeneric );
    // dump files
    Ioa_WriteAiger( pPart0, pFileName0, 0, 0 );
    Ioa_WriteAiger( pPart1, pFileName1, 0, 0 );
    Abc_Print( 1, "Demitering produced two files \"%s\" and \"%s\".\n", pFileName0, pFileName1 );
    // create two-level miter
//    pMiter = Saig_ManCreateMiterTwo( pPart0, pPart1, 2 );
//    Aig_ManDumpBlif( pMiter, "miter01.blif", NULL, NULL );
//    Aig_ManStop( pMiter );
//    Abc_Print( 1, "The new miter is written into file \"%s\".\n", "miter01.blif" );
    Aig_ManStop( pPart0 );
    Aig_ManStop( pPart1 );
    Aig_ManStop( pMan );
    return 1;
} 

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarDemiterDual( Abc_Ntk_t * pNtk, int fVerbose )
{ 
    char * pFileNameGeneric, pFileName0[1000], pFileName1[1000];
    Aig_Man_t * pMan, * pPart0, * pPart1;//, * pMiter;
    if ( (Abc_NtkPoNum(pNtk) & 1) )
    {
        Abc_Print( 1, "The number of POs should be even.\n" );
        return 0;
    }
    // derive the AIG manager
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
    {
        Abc_Print( 1, "Converting network into AIG has failed.\n" );
        return 0;
    }
//    if ( !Saig_ManDemiterSimple( pMan, &pPart0, &pPart1 ) )
    if ( !Saig_ManDemiterDual( pMan, &pPart0, &pPart1 ) )
    {
        Abc_Print( 1, "Demitering has failed.\n" );
        return 0;
    }
    // create new AIG
    ABC_FREE( pPart0->pName );
    pPart0->pName = Abc_UtilStrsav( "part0" );
    // create new AIGs
    ABC_FREE( pPart1->pName );
    pPart1->pName = Abc_UtilStrsav( "part1" );
    // create file names
    pFileNameGeneric = Extra_FileNameGeneric( pNtk->pSpec );
//    sprintf( pFileName0,  "%s%s",  pFileNameGeneric, "_part0.aig" ); 
//    sprintf( pFileName1,  "%s%s",  pFileNameGeneric, "_part1.aig" ); 
    sprintf( pFileName0,  "%s",  "part0.aig" ); 
    sprintf( pFileName1,  "%s",  "part1.aig" ); 
    ABC_FREE( pFileNameGeneric );
    Ioa_WriteAiger( pPart0, pFileName0, 0, 0 );
    Ioa_WriteAiger( pPart1, pFileName1, 0, 0 );
    Abc_Print( 1, "Demitering produced two files \"%s\" and \"%s\".\n", pFileName0, pFileName1 );
    // dump files
    if ( fVerbose )
    {
//        Abc_Print( 1, "Init:  " );
        Aig_ManPrintStats( pMan );
//        Abc_Print( 1, "Part1: " );
        Aig_ManPrintStats( pPart0 );
//        Abc_Print( 1, "Part2: " );
        Aig_ManPrintStats( pPart1 );
    }
    // create two-level miter
//    pMiter = Saig_ManCreateMiterTwo( pPart0, pPart1, 2 );
//    Aig_ManDumpBlif( pMiter, "miter01.blif", NULL, NULL );
//    Aig_ManStop( pMiter );
//    Abc_Print( 1, "The new miter is written into file \"%s\".\n", "miter01.blif" );
    Aig_ManStop( pPart0 );
    Aig_ManStop( pPart1 );
    Aig_ManStop( pMan );
    return 1;
} 
 
/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarProve( Abc_Ntk_t * pNtk, Fra_Sec_t * pSecPar, int nBmcFramesMax, int nBmcConfMax )
{
    Aig_Man_t * pMan;
    int iFrame = -1, RetValue = -1;
    abctime clkTotal = Abc_Clock();
    if ( pSecPar->fTryComb || Abc_NtkLatchNum(pNtk) == 0 )
    {
        Prove_Params_t Params, * pParams = &Params;
        Abc_Ntk_t * pNtkComb;
        int RetValue;
        abctime clk = Abc_Clock();
        if ( Abc_NtkLatchNum(pNtk) == 0 )
            Abc_Print( 1, "The network has no latches. Running CEC.\n" );
        // create combinational network
        pNtkComb = Abc_NtkDup( pNtk );
        Abc_NtkMakeComb( pNtkComb, 1 );
        // solve it using combinational equivalence checking
        Prove_ParamsSetDefault( pParams );
        pParams->fVerbose = 1;
        RetValue = Abc_NtkIvyProve( &pNtkComb, pParams );
        // transfer model if given
//        pNtk->pModel = pNtkComb->pModel; pNtkComb->pModel = NULL;
        if ( RetValue == 0  && (Abc_NtkLatchNum(pNtk) == 0) )
        {
            pNtk->pModel = pNtkComb->pModel; pNtkComb->pModel = NULL;
            Abc_Print( 1, "Networks are not equivalent.\n" );
            ABC_PRT( "Time", Abc_Clock() - clk );
            if ( pSecPar->fReportSolution )
            {
                Abc_Print( 1, "SOLUTION: FAIL       " );
                ABC_PRT( "Time", Abc_Clock() - clkTotal );
            }
            return RetValue;
        }
        Abc_NtkDelete( pNtkComb );
        // return the result, if solved
        if ( RetValue == 1 )
        {
            Abc_Print( 1, "Networks are equivalent after CEC.   " );
            ABC_PRT( "Time", Abc_Clock() - clk );
            if ( pSecPar->fReportSolution )
            {
            Abc_Print( 1, "SOLUTION: PASS       " );
            ABC_PRT( "Time", Abc_Clock() - clkTotal );
            }
            return RetValue;
        }
    }
    // derive the AIG manager
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
    {
        Abc_Print( 1, "Converting miter into AIG has failed.\n" );
        return -1;
    }
    assert( pMan->nRegs > 0 );

    if ( pSecPar->fTryBmc )
    {
        RetValue = Saig_BmcPerform( pMan, 0, nBmcFramesMax, 2000, 0, nBmcConfMax, 0, pSecPar->fVerbose, 0, &iFrame, 0, 0 );
        if ( RetValue == 0 )
        {
            Abc_Print( 1, "Networks are not equivalent.\n" );
            if ( pSecPar->fReportSolution )
            {
                Abc_Print( 1, "SOLUTION: FAIL       " );
                ABC_PRT( "Time", Abc_Clock() - clkTotal );
            }
            // return the counter-example generated
            ABC_FREE( pNtk->pModel );
            ABC_FREE( pNtk->pSeqModel );
            pNtk->pSeqModel = pMan->pSeqModel; pMan->pSeqModel = NULL;
            Aig_ManStop( pMan );
            return RetValue;
        }
    } 
    // perform verification
    if ( pSecPar->fUseNewProver )
    {
        RetValue = Ssw_SecGeneralMiter( pMan, NULL );
    }
    else
    {
        RetValue = Fra_FraigSec( pMan, pSecPar, NULL );
        ABC_FREE( pNtk->pModel );
        ABC_FREE( pNtk->pSeqModel );
        pNtk->pSeqModel = pMan->pSeqModel; pMan->pSeqModel = NULL;
        if ( pNtk->pSeqModel )
        {
            Abc_Cex_t * pCex = pNtk->pSeqModel;
            Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d.\n", pCex->iPo, pNtk->pName, pCex->iFrame );
            if ( !Saig_ManVerifyCex( pMan, pNtk->pSeqModel ) )
                Abc_Print( 1, "Abc_NtkDarProve(): Counter-example verification has FAILED.\n" );
        }
    }
    Aig_ManStop( pMan );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarSec( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Fra_Sec_t * pSecPar )
{
//    Fraig_Params_t Params;
    Aig_Man_t * pMan;
    Abc_Ntk_t * pMiter;//, * pTemp;
    int RetValue;
 
    // get the miter of the two networks
    pMiter = Abc_NtkMiter( pNtk1, pNtk2, 0, 0, 0, 0 );
    if ( pMiter == NULL )
    {
        Abc_Print( 1, "Miter computation has failed.\n" );
        return 0;
    }
    RetValue = Abc_NtkMiterIsConstant( pMiter );
    if ( RetValue == 0 )
    {
        extern void Abc_NtkVerifyReportErrorSeq( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int * pModel, int nFrames );
        Abc_Print( 1, "Networks are NOT EQUIVALENT after structural hashing.\n" );
        // report the error
        pMiter->pModel = Abc_NtkVerifyGetCleanModel( pMiter, pSecPar->nFramesMax );
//        Abc_NtkVerifyReportErrorSeq( pNtk1, pNtk2, pMiter->pModel, pSecPar->nFramesMax );
        ABC_FREE( pMiter->pModel );
        Abc_NtkDelete( pMiter );
        return 0;
    }
    if ( RetValue == 1 )
    {
        Abc_NtkDelete( pMiter );
        Abc_Print( 1, "Networks are equivalent after structural hashing.\n" );
        return 1;
    }

    // commented out because sometimes the problem became non-inductive
/*
    // preprocess the miter by fraiging it
    // (note that for each functional class, fraiging leaves one representative;
    // so fraiging does not reduce the number of functions represented by nodes
    Fraig_ParamsSetDefault( &Params );
    Params.nBTLimit = 100000;
    pMiter = Abc_NtkFraig( pTemp = pMiter, &Params, 0, 0 );
    Abc_NtkDelete( pTemp );
    RetValue = Abc_NtkMiterIsConstant( pMiter );
    if ( RetValue == 0 )
    {
        extern void Abc_NtkVerifyReportErrorSeq( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int * pModel, int nFrames );
        Abc_Print( 1, "Networks are NOT EQUIVALENT after structural hashing.\n" );
        // report the error
        pMiter->pModel = Abc_NtkVerifyGetCleanModel( pMiter, nFrames );
        Abc_NtkVerifyReportErrorSeq( pNtk1, pNtk2, pMiter->pModel, nFrames );
        ABC_FREE( pMiter->pModel );
        Abc_NtkDelete( pMiter );
        return 0;
    }
    if ( RetValue == 1 )
    {
        Abc_NtkDelete( pMiter );
        Abc_Print( 1, "Networks are equivalent after structural hashing.\n" );
        return 1;
    }
*/
    // derive the AIG manager
    pMan = Abc_NtkToDar( pMiter, 0, 1 );
    Abc_NtkDelete( pMiter );
    if ( pMan == NULL )
    {
        Abc_Print( 1, "Converting miter into AIG has failed.\n" );
        return -1;
    }
    assert( pMan->nRegs > 0 );

    // perform verification
    RetValue = Fra_FraigSec( pMan, pSecPar, NULL );
    Aig_ManStop( pMan );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarPdr( Abc_Ntk_t * pNtk, Pdr_Par_t * pPars )
{
    int RetValue = -1;
    abctime clk = Abc_Clock();
    Aig_Man_t * pMan;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
    {
        Abc_Print( 1, "Converting network into AIG has failed.\n" );
        return -1;
    }
    RetValue = Pdr_ManSolve( pMan, pPars );
    pPars->nDropOuts = Saig_ManPoNum(pMan) - pPars->nProveOuts - pPars->nFailOuts;
    if ( !pPars->fSilent )
    {
        if ( pPars->fSolveAll )
            Abc_Print( 1, "Properties:  All = %d. Proved = %d. Disproved = %d. Undecided = %d.   ", 
                Saig_ManPoNum(pMan), pPars->nProveOuts, pPars->nFailOuts, pPars->nDropOuts );
        else if ( RetValue == 1 )
            Abc_Print( 1, "Property proved.  " );
        else 
        {
            if ( RetValue == 0 )
            {
                if ( pMan->pSeqModel == NULL )
                    Abc_Print( 1, "Abc_NtkDarPdr(): Counter-example is not available.\n" );
                else
                {
                    Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d.  ", pMan->pSeqModel->iPo, pNtk->pName, pMan->pSeqModel->iFrame );
                    if ( !Saig_ManVerifyCex( pMan, pMan->pSeqModel ) )
                        Abc_Print( 1, "Abc_NtkDarPdr(): Counter-example verification has FAILED.\n" );
                }
            }
            else if ( RetValue == -1 )
                Abc_Print( 1, "Property UNDECIDED.  " );
            else
                assert( 0 );
        }
        ABC_PRT( "Time", Abc_Clock() - clk );
/*
        Abc_Print( 1, "Status: " );
        if ( pPars->pOutMap )
        {
            int i;
            for ( i = 0; i < Saig_ManPoNum(pMan); i++ )
                if ( pPars->pOutMap[i] == 1 )
                    Abc_Print( 1, "%d=%s ", i, "unsat" );
                else if ( pPars->pOutMap[i] == 0 )
                    Abc_Print( 1, "%d=%s ", i, "sat" );
                else if ( pPars->pOutMap[i] < 0 )
                    Abc_Print( 1, "%d=%s ", i, "undec" );
                else assert( 0 );
        }
        Abc_Print( 1, "\n" );
*/
    }
    ABC_FREE( pNtk->pSeqModel );
    pNtk->pSeqModel = pMan->pSeqModel;
    pMan->pSeqModel = NULL;
    if ( pNtk->vSeqModelVec )
        Vec_PtrFreeFree( pNtk->vSeqModelVec );
    pNtk->vSeqModelVec = pMan->vSeqModelVec;
    pMan->vSeqModelVec = NULL;
    Aig_ManStop( pMan );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Performs BDD-based reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarAbSec( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nFrames, int fVerbose )
{
    Aig_Man_t * pMan1, * pMan2 = NULL;
    int RetValue;
    // derive AIG manager
    pMan1 = Abc_NtkToDar( pNtk1, 0, 1 );
    if ( pMan1 == NULL )
    {
        Abc_Print( 1, "Converting miter into AIG has failed.\n" );
        return -1;
    }
    assert( Aig_ManRegNum(pMan1) > 0 );
    // derive AIG manager
    if ( pNtk2 )
    {
        pMan2 = Abc_NtkToDar( pNtk2, 0, 1 );
        if ( pMan2 == NULL )
        {
            Aig_ManStop( pMan1 );
            Abc_Print( 1, "Converting miter into AIG has failed.\n" );
            return -1;
        }
        assert( Aig_ManRegNum(pMan2) > 0 );
        if ( Saig_ManPiNum(pMan1) != Saig_ManPiNum(pMan2) )
        {
            Aig_ManStop( pMan1 );
            Aig_ManStop( pMan2 );
            Abc_Print( 1, "The networks have different number of PIs.\n" );
            return -1;
        }
        if ( Saig_ManPoNum(pMan1) != Saig_ManPoNum(pMan2) )
        {
            Aig_ManStop( pMan1 );
            Aig_ManStop( pMan2 );
            Abc_Print( 1, "The networks have different number of POs.\n" );
            return -1;
        }
        if ( Aig_ManRegNum(pMan1) != Aig_ManRegNum(pMan2) )
        {
            Aig_ManStop( pMan1 );
            Aig_ManStop( pMan2 );
            Abc_Print( 1, "The networks have different number of flops.\n" );
            return -1;
        }
    }
    // perform verification
    RetValue = Ssw_SecSpecialMiter( pMan1, pMan2, nFrames, fVerbose );
    Aig_ManStop( pMan1 );
    if ( pMan2 )
        Aig_ManStop( pMan2 );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarSimSec( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Ssw_Pars_t * pPars )
{
    Aig_Man_t * pMan1, * pMan2 = NULL;
    int RetValue;
    // derive AIG manager
    pMan1 = Abc_NtkToDar( pNtk1, 0, 1 );
    if ( pMan1 == NULL )
    {
        Abc_Print( 1, "Converting miter into AIG has failed.\n" );
        return -1;
    }
    assert( Aig_ManRegNum(pMan1) > 0 );
    // derive AIG manager
    if ( pNtk2 )
    {
        pMan2 = Abc_NtkToDar( pNtk2, 0, 1 );
        if ( pMan2 == NULL )
        {
            Abc_Print( 1, "Converting miter into AIG has failed.\n" );
            return -1;
        }
        assert( Aig_ManRegNum(pMan2) > 0 );
    }

    // perform verification
    RetValue = Ssw_SecWithSimilarity( pMan1, pMan2, pPars );
    Aig_ManStop( pMan1 );
    if ( pMan2 )
        Aig_ManStop( pMan2 );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarMatch( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nDist, int fVerbose )
{
    extern Vec_Int_t * Saig_StrSimPerformMatching( Aig_Man_t * p0, Aig_Man_t * p1, int nDist, int fVerbose, Aig_Man_t ** ppMiter );
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan1, * pMan2 = NULL, * pManRes;
    Vec_Int_t * vPairs;
    assert( Abc_NtkIsStrash(pNtk1) );
    // derive AIG manager
    pMan1 = Abc_NtkToDar( pNtk1, 0, 1 );
    if ( pMan1 == NULL )
    {
        Abc_Print( 1, "Converting miter into AIG has failed.\n" );
        return NULL;
    }
    assert( Aig_ManRegNum(pMan1) > 0 );
    // derive AIG manager
    if ( pNtk2 )
    {
        pMan2 = Abc_NtkToDar( pNtk2, 0, 1 );
        if ( pMan2 == NULL )
        {
            Abc_Print( 1, "Converting miter into AIG has failed.\n" );
            return NULL;
        }
        assert( Aig_ManRegNum(pMan2) > 0 );
    }

    // perform verification
    vPairs = Saig_StrSimPerformMatching( pMan1, pMan2, nDist, 1, &pManRes );
    pNtkAig = Abc_NtkFromAigPhase( pManRes );
    if ( vPairs )
        Vec_IntFree( vPairs );
    if ( pManRes )
        Aig_ManStop( pManRes );
    Aig_ManStop( pMan1 );
    if ( pMan2 )
        Aig_ManStop( pMan2 );
    return pNtkAig;
}


/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarLatchSweep( Abc_Ntk_t * pNtk, int fLatchConst, int fLatchEqual, int fSaveNames, int fUseMvSweep, int nFramesSymb, int nFramesSatur, int fVerbose, int fVeryVerbose )
{
    extern void Aig_ManPrintControlFanouts( Aig_Man_t * p );
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    if ( fSaveNames )
    {
        Aig_ManSeqCleanup( pMan );
        if ( fLatchConst && pMan->nRegs )
            pMan = Aig_ManConstReduce( pMan, fUseMvSweep, nFramesSymb, nFramesSatur, fVerbose, fVeryVerbose );
        if ( fLatchEqual && pMan->nRegs )
            pMan = Aig_ManReduceLaches( pMan, fVerbose );
    }
    else
    {
        if ( pMan->vFlopNums )
            Vec_IntFree( pMan->vFlopNums );
        pMan->vFlopNums = NULL;
        pMan = Aig_ManScl( pTemp = pMan, fLatchConst, fLatchEqual, fUseMvSweep, nFramesSymb, nFramesSatur, fVerbose, fVeryVerbose );
        Aig_ManStop( pTemp );
    }

    pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
//Aig_ManPrintControlFanouts( pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarRetime( Abc_Ntk_t * pNtk, int nStepsMax, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
//    Aig_ManReduceLachesCount( pMan );
    if ( pMan->vFlopNums )
        Vec_IntFree( pMan->vFlopNums ); 
    pMan->vFlopNums = NULL;

    pMan = Rtm_ManRetime( pTemp = pMan, 1, nStepsMax, fVerbose );
    Aig_ManStop( pTemp );

//    pMan = Aig_ManReduceLaches( pMan, 1 );
//    pMan = Aig_ManConstReduce( pMan, 1, 0 );

    pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarRetimeF( Abc_Ntk_t * pNtk, int nStepsMax, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
//    Aig_ManReduceLachesCount( pMan );
    if ( pMan->vFlopNums )
        Vec_IntFree( pMan->vFlopNums ); 
    pMan->vFlopNums = NULL;

    pMan = Aig_ManRetimeFrontier( pTemp = pMan, nStepsMax );
    Aig_ManStop( pTemp );

//    pMan = Aig_ManReduceLaches( pMan, 1 );
//    pMan = Aig_ManConstReduce( pMan, 1, 0 );

    pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarRetimeMostFwd( Abc_Ntk_t * pNtk, int nMaxIters, int fVerbose )
{
    extern Aig_Man_t * Saig_ManRetimeForward( Aig_Man_t * p, int nIters, int fVerbose );

    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
//    Aig_ManReduceLachesCount( pMan );
    if ( pMan->vFlopNums )
        Vec_IntFree( pMan->vFlopNums ); 
    pMan->vFlopNums = NULL;

    pMan = Saig_ManRetimeForward( pTemp = pMan, nMaxIters, fVerbose );
    Aig_ManStop( pTemp );

//    pMan = Aig_ManReduceLaches( pMan, 1 );
//    pMan = Aig_ManConstReduce( pMan, 1, 0 );

    pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarRetimeMinArea( Abc_Ntk_t * pNtk, int nMaxIters, int fForwardOnly, int fBackwardOnly, int fInitial, int fVerbose )
{
    extern Aig_Man_t * Saig_ManRetimeMinArea( Aig_Man_t * p, int nMaxIters, int fForwardOnly, int fBackwardOnly, int fInitial, int fVerbose );
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    if ( pMan->vFlopNums )
        Vec_IntFree( pMan->vFlopNums ); 
    pMan->vFlopNums = NULL;

    pMan = Saig_ManRetimeMinArea( pTemp = pMan, nMaxIters, fForwardOnly, fBackwardOnly, fInitial, fVerbose );
    Aig_ManStop( pTemp );

    pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarRetimeStep( Abc_Ntk_t * pNtk, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    if ( pMan->vFlopNums )
        Vec_IntFree( pMan->vFlopNums ); 
    pMan->vFlopNums = NULL;

    Aig_ManPrintStats(pMan);
    Saig_ManRetimeSteps( pMan, 1000, 1, 0 );
    Aig_ManPrintStats(pMan);

    pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
Abc_Ntk_t * Abc_NtkDarHaigRecord( Abc_Ntk_t * pNtk, int nIters, int nSteps, int fRetimingOnly, int fAddBugs, int fUseCnf, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    if ( pMan->vFlopNums )
        Vec_IntFree( pMan->vFlopNums ); 
    pMan->vFlopNums = NULL;

    pMan = Saig_ManHaigRecord( pTemp = pMan, nIters, nSteps, fRetimingOnly, fAddBugs, fUseCnf, fVerbose );
    Aig_ManStop( pTemp );

    pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}
*/

/**Function*************************************************************

  Synopsis    [Performs random simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarSeqSim( Abc_Ntk_t * pNtk, int nFrames, int nWords, int TimeOut, int fNew, int fMiter, int fVerbose, char * pFileSim )
{
    Aig_Man_t * pMan;
    Abc_Cex_t * pCex;
    int status, RetValue = -1;
    abctime clk = Abc_Clock();
    if ( Abc_NtkGetChoiceNum(pNtk) )
    {
        Abc_Print( 1, "Removing %d choices from the AIG.\n", Abc_NtkGetChoiceNum(pNtk) );
        Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);
    }
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( fNew )
    {
        Gia_Man_t * pGia;
        Gia_ParSim_t Pars, * pPars = &Pars;
        Gia_ManSimSetDefaultParams( pPars );
        pPars->nWords = nWords;
        pPars->nIters = nFrames;
        pPars->TimeLimit = TimeOut;
        pPars->fCheckMiter = fMiter;
        pPars->fVerbose = fVerbose;
        pGia = Gia_ManFromAig( pMan );
        if ( Gia_ManSimSimulate( pGia, pPars ) )
        { 
            if ( pGia->pCexSeq )
            {
                Abc_Print( 1, "Simulation of %d frames with %d words asserted output %d in frame %d. ", 
                    nFrames, nWords, pGia->pCexSeq->iPo, pGia->pCexSeq->iFrame );
                status = Saig_ManVerifyCex( pMan, pGia->pCexSeq );
                if ( status == 0 )
                    Abc_Print( 1, "Abc_NtkDarSeqSim(): Counter-example verification has FAILED.\n" );
            }
            ABC_FREE( pNtk->pModel );
            ABC_FREE( pNtk->pSeqModel );
            pNtk->pSeqModel = pGia->pCexSeq; pGia->pCexSeq = NULL;
            RetValue = 0;
        }
        else
        {
            Abc_Print( 1, "Simulation of %d frames with %d words did not assert the outputs.    ", 
                nFrames, nWords );
        }
        Gia_ManStop( pGia );
    }
    else // comb/seq simulator
    {
        Fra_Sml_t * pSml;
        if ( pFileSim != NULL )
        {
            assert( Abc_NtkLatchNum(pNtk) == 0 );
            pSml = Fra_SmlSimulateCombGiven( pMan, pFileSim, fMiter, fVerbose );
        }
        else if ( Abc_NtkLatchNum(pNtk) == 0 )
            pSml = Fra_SmlSimulateComb( pMan, nWords, fMiter );
        else
            pSml = Fra_SmlSimulateSeq( pMan, 0, nFrames, nWords, fMiter );
        if ( pSml->fNonConstOut )
        {
            pCex = Fra_SmlGetCounterExample( pSml );
            if ( pCex )
            {
                Abc_Print( 1, "Simulation of %d frame%s with %d word%s asserted output %d in frame %d. ", 
                    pSml->nFrames, pSml->nFrames == 1 ? "": "s", 
                    pSml->nWordsFrame, pSml->nWordsFrame == 1 ? "": "s", 
                    pCex->iPo, pCex->iFrame );
                status = Saig_ManVerifyCex( pMan, pCex );
                if ( status == 0 )
                    Abc_Print( 1, "Abc_NtkDarSeqSim(): Counter-example verification has FAILED.\n" );
            }
            ABC_FREE( pNtk->pModel );
            ABC_FREE( pNtk->pSeqModel );
            pNtk->pSeqModel = pCex;
            RetValue = 0;
        }
        else
        {
            Abc_Print( 1, "Simulation of %d frames with %d words did not assert the outputs.    ", 
                nFrames, nWords );
        }
        Fra_SmlStop( pSml );
    }
    ABC_PRT( "Time", Abc_Clock() - clk );
    Aig_ManStop( pMan );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Performs random simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarSeqSim3( Abc_Ntk_t * pNtk, Ssw_RarPars_t * pPars )
{
    Aig_Man_t * pMan;
    int status, RetValue = -1;
//    abctime clk = Abc_Clock();
    if ( Abc_NtkGetChoiceNum(pNtk) )
    {
        Abc_Print( 1, "Removing %d choices from the AIG.\n", Abc_NtkGetChoiceNum(pNtk) );
        Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);
    }
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( Ssw_RarSimulate( pMan, pPars ) == 0 )
    { 
        if ( pMan->pSeqModel )
        {
//            Abc_Print( 1, "Simulation of %d frames with %d words asserted output %d in frame %d. ", 
//                nFrames, nWords, pMan->pSeqModel->iPo, pMan->pSeqModel->iFrame );
            status = Saig_ManVerifyCex( pMan, pMan->pSeqModel );
            if ( status == 0 )
                Abc_Print( 1, "Abc_NtkDarSeqSim(): Counter-example verification has FAILED.\n" );
        }
        ABC_FREE( pNtk->pModel );
        ABC_FREE( pNtk->pSeqModel );
        pNtk->pSeqModel = pMan->pSeqModel; pMan->pSeqModel = NULL;
        RetValue = 0;
    }
    else
    {
//        Abc_Print( 1, "Simulation of %d frames with %d words did not assert the outputs.    ", 
//            nFrames, nWords );
    }
    if ( pNtk->vSeqModelVec )
        Vec_PtrFreeFree( pNtk->vSeqModelVec );
    pNtk->vSeqModelVec = pMan->vSeqModelVec;  pMan->vSeqModelVec = NULL;
//    ABC_PRT( "Time", Abc_Clock() - clk );
    pNtk->pData = pMan->pData; pMan->pData = NULL;
    Aig_ManStop( pMan );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarClau( Abc_Ntk_t * pNtk, int nFrames, int nPref, int nClauses, int nLutSize, int nLevels, int nCutsMax, int nBatches, int fStepUp, int fBmc, int fRefs, int fTarget, int fVerbose, int fVeryVerbose )
{
    extern int Fra_Clau( Aig_Man_t * pMan, int nIters, int fVerbose, int fVeryVerbose );
    extern int Fra_Claus( Aig_Man_t * pAig, int nFrames, int nPref, int nClauses, int nLutSize, int nLevels, int nCutsMax, int nBatches, int fStepUp, int fBmc, int fRefs, int fTarget, int fVerbose, int fVeryVerbose );
    Aig_Man_t * pMan;
    if ( fTarget && Abc_NtkPoNum(pNtk) != 1 )
    {
        Abc_Print( 1, "The number of outputs should be 1.\n" );
        return 1;
    }
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return 1;
//    Aig_ManReduceLachesCount( pMan );
    if ( pMan->vFlopNums )
        Vec_IntFree( pMan->vFlopNums ); 
    pMan->vFlopNums = NULL;

//    Fra_Clau( pMan, nStepsMax, fVerbose, fVeryVerbose );
    Fra_Claus( pMan, nFrames, nPref, nClauses, nLutSize, nLevels, nCutsMax, nBatches, fStepUp, fBmc, fRefs, fTarget, fVerbose, fVeryVerbose );
    Aig_ManStop( pMan );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs targe enlargement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarEnlarge( Abc_Ntk_t * pNtk, int nFrames, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    pMan = Aig_ManFrames( pTemp = pMan, nFrames, 0, 1, 1, 1, NULL );
    Aig_ManStop( pTemp );
    pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs targe enlargement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarTempor( Abc_Ntk_t * pNtk, int nFrames, int TimeOut, int nConfLimit, int fUseBmc, int fUseTransSigs, int fVerbose, int fVeryVerbose )
{
    extern Aig_Man_t * Saig_ManTempor( Aig_Man_t * pAig, int nFrames, int TimeOut, int nConfLimit, int fUseBmc, int fUseTransSigs, int fVerbose, int fVeryVerbose );
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    pTemp = Saig_ManTempor( pMan, nFrames, TimeOut, nConfLimit, fUseBmc, fUseTransSigs, fVerbose, fVeryVerbose );
    Aig_ManStop( pMan );
    if ( pTemp == NULL )
        return Abc_NtkDup( pNtk );
    pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pTemp );
    Aig_ManStop( pTemp );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs induction for property only.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarInduction( Abc_Ntk_t * pNtk, int nTimeOut, int nFramesMax, int nConfMax, int fUnique, int fUniqueAll, int fGetCex, int fVerbose, int fVeryVerbose )
{ 
    Aig_Man_t * pMan;
    abctime clkTotal = Abc_Clock();
    int RetValue;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return -1;
    RetValue = Saig_ManInduction( pMan, nTimeOut, nFramesMax, nConfMax, fUnique, fUniqueAll, fGetCex, fVerbose, fVeryVerbose );
    if ( RetValue == 1 )
    {
        Abc_Print( 1, "Networks are equivalent.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    else if ( RetValue == 0 )
    {
        Abc_Print( 1, "Networks are NOT EQUIVALENT.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    else
    {
        Abc_Print( 1, "Networks are UNDECIDED.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    if ( fGetCex )
    {
        ABC_FREE( pNtk->pModel );
        ABC_FREE( pNtk->pSeqModel );
        pNtk->pSeqModel = pMan->pSeqModel; pMan->pSeqModel = NULL;
    }
    Aig_ManStop( pMan );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Interplates two networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkInterOne( Abc_Ntk_t * pNtkOn, Abc_Ntk_t * pNtkOff, int fRelation, int fVerbose )
{
    extern Aig_Man_t * Aig_ManInter( Aig_Man_t * pManOn, Aig_Man_t * pManOff, int fRelation, int fVerbose );
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pManOn, * pManOff, * pManAig;
    if ( Abc_NtkCoNum(pNtkOn) != 1 || Abc_NtkCoNum(pNtkOff) != 1 )
    {
        Abc_Print( 1, "Currently works only for single-output networks.\n" );
        return NULL;
    }
    if ( Abc_NtkCiNum(pNtkOn) != Abc_NtkCiNum(pNtkOff) )
    {
        Abc_Print( 1, "The number of PIs should be the same.\n" );
        return NULL;
    }
    // create internal AIGs
    pManOn = Abc_NtkToDar( pNtkOn, 0, 0 );
    if ( pManOn == NULL )
        return NULL;
    pManOff = Abc_NtkToDar( pNtkOff, 0, 0 );
    if ( pManOff == NULL )
        return NULL;
    // derive the interpolant
    pManAig = Aig_ManInter( pManOn, pManOff, fRelation, fVerbose );
    if ( pManAig == NULL )
    {
        Abc_Print( 1, "Interpolant computation failed.\n" );
        return NULL;
    }
    Aig_ManStop( pManOn );
    Aig_ManStop( pManOff );
    // for the relation, add an extra input
    if ( fRelation )
    {
        Abc_Obj_t * pObj;
        pObj = Abc_NtkCreatePi( pNtkOff );
        Abc_ObjAssignName( pObj, "New", NULL );
    }
    // create logic network
    pNtkAig = Abc_NtkFromDar( pNtkOff, pManAig );
    Aig_ManStop( pManAig );
    return pNtkAig;
}
Gia_Man_t * Gia_ManInterOne( Gia_Man_t * pNtkOn, Gia_Man_t * pNtkOff, int fVerbose )
{
    extern Aig_Man_t * Aig_ManInter( Aig_Man_t * pManOn, Aig_Man_t * pManOff, int fRelation, int fVerbose );
    Gia_Man_t * pNtkAig;
    Aig_Man_t * pManOn, * pManOff, * pManAig;
    assert( Gia_ManCiNum(pNtkOn)  == Gia_ManCiNum(pNtkOff) );
    assert( Gia_ManCoNum(pNtkOn)  == 1 );
    assert( Gia_ManCoNum(pNtkOff) == 1 );
    // create internal AIGs
    pManOn = Gia_ManToAigSimple( pNtkOn );
    if ( pManOn == NULL )
        return NULL;
    pManOff = Gia_ManToAigSimple( pNtkOff );
    if ( pManOff == NULL )
        return NULL;
    // derive the interpolant
    pManAig = Aig_ManInter( pManOn, pManOff, 0, fVerbose );
    if ( pManAig == NULL )
    {
        Abc_Print( 1, "Interpolant computation failed.\n" );
        return NULL;
    }
    Aig_ManStop( pManOn );
    Aig_ManStop( pManOff );
    // create logic network
    pNtkAig = Gia_ManFromAigSimple( pManAig );
    Aig_ManStop( pManAig );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Fast interpolation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkInterFast( Abc_Ntk_t * pNtkOn, Abc_Ntk_t * pNtkOff, int fVerbose )
{
    extern void Aig_ManInterFast( Aig_Man_t * pManOn, Aig_Man_t * pManOff, int fVerbose );
    Aig_Man_t * pManOn, * pManOff;
    // create internal AIGs
    pManOn = Abc_NtkToDar( pNtkOn, 0, 0 );
    if ( pManOn == NULL )
        return;
    pManOff = Abc_NtkToDar( pNtkOff, 0, 0 );
    if ( pManOff == NULL )
        return;
    Aig_ManInterFast( pManOn, pManOff, fVerbose );
    Aig_ManStop( pManOn );
    Aig_ManStop( pManOff );
}

abctime timeCnf;
abctime timeSat;
abctime timeInt;

/**Function*************************************************************

  Synopsis    [Interplates two networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkInter( Abc_Ntk_t * pNtkOn, Abc_Ntk_t * pNtkOff, int fRelation, int fVerbose )
{
    Abc_Ntk_t * pNtkOn1, * pNtkOff1, * pNtkInter1, * pNtkInter;
    Abc_Obj_t * pObj;
    int i; //, clk = Abc_Clock();
    if ( Abc_NtkCoNum(pNtkOn) != Abc_NtkCoNum(pNtkOff) )
    {
        Abc_Print( 1, "Currently works only for networks with equal number of POs.\n" );
        return NULL;
    }
    // compute the fast interpolation time
//    Abc_NtkInterFast( pNtkOn, pNtkOff, fVerbose );
    // consider the case of one output
    if ( Abc_NtkCoNum(pNtkOn) == 1 )
        return Abc_NtkInterOne( pNtkOn, pNtkOff, fRelation, fVerbose );
    // start the new network
    pNtkInter = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pNtkInter->pName = Extra_UtilStrsav(pNtkOn->pName);
    Abc_NtkForEachPi( pNtkOn, pObj, i )
        Abc_NtkDupObj( pNtkInter, pObj, 1 );
    // process each POs separately
timeCnf = 0;
timeSat = 0;
timeInt = 0;
    Abc_NtkForEachCo( pNtkOn, pObj, i )
    {
        pNtkOn1 = Abc_NtkCreateCone( pNtkOn, Abc_ObjFanin0(pObj), Abc_ObjName(pObj), 1 );
        if ( Abc_ObjFaninC0(pObj) )
            Abc_ObjXorFaninC( Abc_NtkPo(pNtkOn1, 0), 0 );

        pObj   = Abc_NtkCo(pNtkOff, i);
        pNtkOff1 = Abc_NtkCreateCone( pNtkOff, Abc_ObjFanin0(pObj), Abc_ObjName(pObj), 1 );
        if ( Abc_ObjFaninC0(pObj) )
            Abc_ObjXorFaninC( Abc_NtkPo(pNtkOff1, 0), 0 );

        if ( Abc_NtkNodeNum(pNtkOn1) == 0 )
            pNtkInter1 = Abc_NtkDup( pNtkOn1 );
        else if ( Abc_NtkNodeNum(pNtkOff1) == 0 )
        {
            pNtkInter1 = Abc_NtkDup( pNtkOff1 );
            Abc_ObjXorFaninC( Abc_NtkPo(pNtkInter1, 0), 0 );
        }
        else
            pNtkInter1 = Abc_NtkInterOne( pNtkOn1, pNtkOff1, 0, fVerbose );
        if ( pNtkInter1 )
        {
            Abc_NtkAppend( pNtkInter, pNtkInter1, 1 );
            Abc_NtkDelete( pNtkInter1 );
        }

        Abc_NtkDelete( pNtkOn1 );
        Abc_NtkDelete( pNtkOff1 );
    }
//    ABC_PRT( "CNF", timeCnf );
//    ABC_PRT( "SAT", timeSat );
//    ABC_PRT( "Int", timeInt );
//    ABC_PRT( "Slow interpolation time", Abc_Clock() - clk );

    // return the network
    if ( !Abc_NtkCheck( pNtkInter ) )
        Abc_Print( 1, "Abc_NtkAttachBottom(): Network check has failed.\n" );
    return pNtkInter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintSccs( Abc_Ntk_t * pNtk, int fVerbose )
{
//    extern Vec_Ptr_t * Aig_ManRegPartitionLinear( Aig_Man_t * pAig, int nPartSize );
    Aig_Man_t * pMan;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return;
    Aig_ManComputeSccs( pMan );
//    Aig_ManRegPartitionLinear( pMan, 1000 );
    Aig_ManStop( pMan );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarPrintCone( Abc_Ntk_t * pNtk )
{
    extern void Saig_ManPrintCones( Aig_Man_t * pAig );
    Aig_Man_t * pMan;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return 0;
    assert( Aig_ManRegNum(pMan) > 0 );
    Saig_ManPrintCones( pMan );
    Aig_ManStop( pMan );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
Abc_Ntk_t * Abc_NtkBalanceExor( Abc_Ntk_t * pNtk, int fUpdateLevel, int fVerbose )
{
    extern void Dar_BalancePrintStats( Aig_Man_t * p );
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;//, * pTemp2;
    assert( Abc_NtkIsStrash(pNtk) );
    // derive AIG with EXORs
    pMan = Abc_NtkToDar( pNtk, 1, 0 );
    if ( pMan == NULL )
        return NULL;
//    Aig_ManPrintStats( pMan );
    if ( fVerbose )
        Dar_BalancePrintStats( pMan );
    // perform balancing
    pTemp = Dar_ManBalance( pMan, fUpdateLevel );
//    Aig_ManPrintStats( pTemp );
    // create logic network
    pNtkAig = Abc_NtkFromDar( pNtk, pTemp );
    Aig_ManStop( pTemp );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs phase abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkPhaseAbstract( Abc_Ntk_t * pNtk, int nFrames, int nPref, int fIgnore, int fPrint, int fVerbose )
{
    extern Aig_Man_t * Saig_ManPhaseAbstract( Aig_Man_t * p, Vec_Int_t * vInits, int nFrames, int nPref, int fIgnore, int fPrint, int fVerbose );
    Vec_Int_t * vInits;
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    vInits = Abc_NtkGetLatchValues(pNtk);
    pMan = Saig_ManPhaseAbstract( pTemp = pMan, vInits, nFrames, nPref, fIgnore, fPrint, fVerbose );
    Vec_IntFree( vInits );
    Aig_ManStop( pTemp );
    if ( pMan == NULL )
        return NULL;
    pNtkAig = Abc_NtkFromAigPhase( pMan );
//    pNtkAig->pName = Extra_UtilStrsav(pNtk->pName);
//    pNtkAig->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs phase abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkPhaseFrameNum( Abc_Ntk_t * pNtk )
{
    extern int Saig_ManPhaseFrameNum( Aig_Man_t * p, Vec_Int_t * vInits );
    Vec_Int_t * vInits;
    Aig_Man_t * pMan;
    int nFrames;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return 1;
    vInits = Abc_NtkGetLatchValues(pNtk);
    nFrames = Saig_ManPhaseFrameNum( pMan, vInits );
    Vec_IntFree( vInits );
    Aig_ManStop( pMan );
    return nFrames;
}

/**Function*************************************************************

  Synopsis    [Performs phase abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarSynchOne( Abc_Ntk_t * pNtk, int nWords, int fVerbose )
{
    extern Aig_Man_t * Saig_SynchSequenceApply( Aig_Man_t * pAig, int nWords, int fVerbose );
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    pMan = Saig_SynchSequenceApply( pTemp = pMan, nWords, fVerbose );
    Aig_ManStop( pTemp );
    if ( pMan == NULL )
        return NULL;
    pNtkAig = Abc_NtkFromDar( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs phase abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarSynch( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nWords, int fVerbose )
{
    extern Aig_Man_t * Saig_Synchronize( Aig_Man_t * pAig1, Aig_Man_t * pAig2, int nWords, int fVerbose );
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan1, * pMan2, * pMan;
    pMan1 = Abc_NtkToDar( pNtk1, 0, 1 );
    if ( pMan1 == NULL )
        return NULL;
    pMan2 = Abc_NtkToDar( pNtk2, 0, 1 );
    if ( pMan2 == NULL )
    {
        Aig_ManStop( pMan1 );
        return NULL;
    }
    pMan = Saig_Synchronize( pMan1, pMan2, nWords, fVerbose );
    Aig_ManStop( pMan1 );
    Aig_ManStop( pMan2 );
    if ( pMan == NULL )
        return NULL;
    pNtkAig = Abc_NtkFromAigPhase( pMan );
//    pNtkAig->pName = Extra_UtilStrsav("miter");
//    pNtkAig->pSpec = NULL;
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs phase abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarClockGate( Abc_Ntk_t * pNtk, Abc_Ntk_t * pCare, Cgt_Par_t * pPars )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan1, * pMan2 = NULL, * pMan;
    pMan1 = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan1 == NULL )
        return NULL;
    if ( pCare )
    {
        pMan2 = Abc_NtkToDar( pCare, 0, 0 );
        if ( pMan2 == NULL )
        {
            Aig_ManStop( pMan1 );
            return NULL;
        }
    }
    pMan = Cgt_ClockGating( pMan1, pMan2, pPars );
    Aig_ManStop( pMan1 );
    if ( pMan2 )
        Aig_ManStop( pMan2 );
    if ( pMan == NULL )
        return NULL;
    pNtkAig = Abc_NtkFromDar( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs phase abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarExtWin( Abc_Ntk_t * pNtk, int nObjId, int nDist, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan1, * pMan;
    Aig_Obj_t * pObj;
    pMan1 = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan1 == NULL )
        return NULL;
    if ( nObjId == -1 )
    {
        pObj = Saig_ManFindPivot( pMan1 );
        Abc_Print( 1, "Selected object %d as a window pivot.\n", pObj->Id );
    }
    else
    {
        if ( nObjId >= Aig_ManObjNumMax(pMan1) )
        {
            Aig_ManStop( pMan1 );
            Abc_Print( 1, "The ID is too large.\n" );
            return NULL;
        }
        pObj = Aig_ManObj( pMan1, nObjId );
        if ( pObj == NULL )
        {
            Aig_ManStop( pMan1 );
            Abc_Print( 1, "Object with ID %d does not exist.\n", nObjId );
            return NULL;
        }
        if ( !Saig_ObjIsLo(pMan1, pObj) && !Aig_ObjIsNode(pObj) )
        {
            Aig_ManStop( pMan1 );
            Abc_Print( 1, "Object with ID %d is not a node or reg output.\n", nObjId );
            return NULL;
        }
    }
    pMan = Saig_ManWindowExtract( pMan1, pObj, nDist );
    Aig_ManStop( pMan1 );
    if ( pMan == NULL )
        return NULL;
    pNtkAig = Abc_NtkFromAigPhase( pMan );
    pNtkAig->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkAig->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs phase abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarInsWin( Abc_Ntk_t * pNtk, Abc_Ntk_t * pCare, int nObjId, int nDist, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan1, * pMan2 = NULL, * pMan;
    Aig_Obj_t * pObj;
    pMan1 = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan1 == NULL )
        return NULL;
    if ( nObjId == -1 )
    {
        pObj = Saig_ManFindPivot( pMan1 );
        Abc_Print( 1, "Selected object %d as a window pivot.\n", pObj->Id );
    }
    else
    {
        if ( nObjId >= Aig_ManObjNumMax(pMan1) )
        {
            Aig_ManStop( pMan1 );
            Abc_Print( 1, "The ID is too large.\n" );
            return NULL;
        }
        pObj = Aig_ManObj( pMan1, nObjId );
        if ( pObj == NULL )
        {
            Aig_ManStop( pMan1 );
            Abc_Print( 1, "Object with ID %d does not exist.\n", nObjId );
            return NULL;
        }
        if ( !Saig_ObjIsLo(pMan1, pObj) && !Aig_ObjIsNode(pObj) )
        {
            Aig_ManStop( pMan1 );
            Abc_Print( 1, "Object with ID %d is not a node or reg output.\n", nObjId );
            return NULL;
        }
    }
    if ( pCare )
    {
        pMan2 = Abc_NtkToDar( pCare, 0, 0 );
        if ( pMan2 == NULL )
        {
            Aig_ManStop( pMan1 );
            return NULL;
        }
    }
    pMan = Saig_ManWindowInsert( pMan1, pObj, nDist, pMan2 );
    Aig_ManStop( pMan1 );
    if ( pMan2 )
        Aig_ManStop( pMan2 );
    if ( pMan == NULL )
        return NULL;
    pNtkAig = Abc_NtkFromDarSeqSweep( pNtk, pMan );
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs phase abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarFrames( Abc_Ntk_t * pNtk, int nPrefix, int nFrames, int fInit, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    pMan = Saig_ManTimeframeSimplify( pTemp = pMan, nPrefix, nFrames, fInit, fVerbose );
    Aig_ManStop( pTemp );
    if ( pMan == NULL )
        return NULL;
    pNtkAig = Abc_NtkFromAigPhase( pMan );
    pNtkAig->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkAig->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs phase abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarCleanupAig( Abc_Ntk_t * pNtk, int fCleanupPis, int fCleanupPos, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    if ( fCleanupPis )
    {
        int Temp = Aig_ManCiCleanup( pMan );
        if ( fVerbose )
            Abc_Print( 1, "Cleanup removed %d primary inputs without fanout.\n", Temp );                                                                     
    }
    if ( fCleanupPos )
    {
        int Temp = Aig_ManCoCleanup( pMan );
        if ( fVerbose )
            Abc_Print( 1, "Cleanup removed %d primary outputs driven by const-0.\n", Temp );                                                                     
    }
    pNtkAig = Abc_NtkFromAigPhase( pMan );
    pNtkAig->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkAig->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs BDD-based reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarReach( Abc_Ntk_t * pNtk, Saig_ParBbr_t * pPars )
{
    Aig_Man_t * pMan;
    int RetValue;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return -1;
    RetValue = Aig_ManVerifyUsingBdds( pMan, pPars );
    ABC_FREE( pNtk->pModel );
    ABC_FREE( pNtk->pSeqModel );
    pNtk->pSeqModel = pMan->pSeqModel; pMan->pSeqModel = NULL;
    Aig_ManStop( pMan );
    return RetValue;
}

ABC_NAMESPACE_IMPL_END

#include "map/amap/amap.h"
#include "map/mio/mio.h"

ABC_NAMESPACE_IMPL_START


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Amap_ManProduceNetwork( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMapping )
{
//    extern void * Abc_FrameReadLibGen();
    Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
    Amap_Out_t * pRes;
    Vec_Ptr_t * vNodesNew;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pNodeNew, * pFaninNew;
    int i, k, iPis, iPos, nDupGates;
    // make sure gates exist in the current library
    Vec_PtrForEachEntry( Amap_Out_t *, vMapping, pRes, i )
        if ( pRes->pName && Mio_LibraryReadGateByName( pLib, pRes->pName, NULL ) == NULL )
        {
            Abc_Print( 1, "Current library does not contain gate \"%s\".\n", pRes->pName );
            return NULL;
        }
    // create the network
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_MAP );
    pNtkNew->pManFunc = pLib;
    iPis = iPos = 0;
    vNodesNew = Vec_PtrAlloc( Vec_PtrSize(vMapping) );
    Vec_PtrForEachEntry( Amap_Out_t *, vMapping, pRes, i )
    {
        if ( pRes->Type == -1 )
            pNodeNew = Abc_NtkCi( pNtkNew, iPis++ );
        else if ( pRes->Type == 1 )
            pNodeNew = Abc_NtkCo( pNtkNew, iPos++ );
        else
        {
            pNodeNew = Abc_NtkCreateNode( pNtkNew );
            pNodeNew->pData = Mio_LibraryReadGateByName( pLib, pRes->pName, NULL );
        }
        for ( k = 0; k < pRes->nFans; k++ )
        {
            pFaninNew = (Abc_Obj_t *)Vec_PtrEntry( vNodesNew, pRes->pFans[k] );
            Abc_ObjAddFanin( pNodeNew, pFaninNew );
        }
        Vec_PtrPush( vNodesNew, pNodeNew );
    }
    Vec_PtrFree( vNodesNew );
    assert( iPis == Abc_NtkCiNum(pNtkNew) );
    assert( iPos == Abc_NtkCoNum(pNtkNew) );
    // decouple the PO driver nodes to reduce the number of levels
    nDupGates = Abc_NtkLogicMakeSimpleCos( pNtkNew, 0 );
//    if ( nDupGates && Map_ManReadVerbose(pMan) )
//        Abc_Print( 1, "Duplicated %d gates to decouple the CO drivers.\n", nDupGates );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarAmap( Abc_Ntk_t * pNtk, Amap_Par_t * pPars )
{
    extern Vec_Ptr_t * Amap_ManTest( Aig_Man_t * pAig, Amap_Par_t * pPars );
    Vec_Ptr_t * vMapping;
    Abc_Ntk_t * pNtkAig = NULL;
    Aig_Man_t * pMan;
    Aig_MmFlex_t * pMem;

    assert( Abc_NtkIsStrash(pNtk) );
    // convert to the AIG manager
    pMan = Abc_NtkToDarChoices( pNtk );
    if ( pMan == NULL )
        return NULL;

    // perform computation
    vMapping = Amap_ManTest( pMan, pPars );
    Aig_ManStop( pMan );
    if ( vMapping == NULL )
        return NULL;
    pMem = (Aig_MmFlex_t *)Vec_PtrPop( vMapping );
    pNtkAig = Amap_ManProduceNetwork( pNtk, vMapping );
    Aig_MmFlexStop( pMem, 0 );
    Vec_PtrFree( vMapping );

    // make sure everything is okay
    if ( pNtkAig && !Abc_NtkCheck( pNtkAig ) )
    {
        Abc_Print( 1, "Abc_NtkDar: The network check has failed.\n" );
        Abc_NtkDelete( pNtkAig );
        return NULL;
    }
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs BDD-based reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDarConstr( Abc_Ntk_t * pNtk, int nFrames, int nConfs, int nProps, int fStruct, int fOldAlgo, int fVerbose )
{
    Aig_Man_t * pMan;//, * pMan2;//, * pTemp;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return;
    if ( fStruct )
        Saig_ManDetectConstrTest( pMan );
    else
        Saig_ManDetectConstrFuncTest( pMan, nFrames, nConfs, nProps, fOldAlgo, fVerbose );
    Aig_ManStop( pMan );
}

/**Function*************************************************************

  Synopsis    [Performs BDD-based reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarOutdec( Abc_Ntk_t * pNtk, int nLits, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    pMan = Saig_ManDecPropertyOutput( pTemp = pMan, nLits, fVerbose );
    Aig_ManStop( pTemp );
    if ( pMan == NULL )
        return NULL;
    pNtkAig = Abc_NtkFromAigPhase( pMan );
    pNtkAig->pName = Extra_UtilStrsav(pMan->pName);
    pNtkAig->pSpec = Extra_UtilStrsav(pMan->pSpec);
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs BDD-based reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarUnfold( Abc_Ntk_t * pNtk, int nFrames, int nConfs, int nProps, int fStruct, int fOldAlgo, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    if ( fStruct )
        pMan = Saig_ManDupUnfoldConstrs( pTemp = pMan );
    else
        pMan = Saig_ManDupUnfoldConstrsFunc( pTemp = pMan, nFrames, nConfs, nProps, fOldAlgo, fVerbose );
    Aig_ManStop( pTemp );
    if ( pMan == NULL )
        return NULL;
    pNtkAig = Abc_NtkFromAigPhase( pMan );
    pNtkAig->pName = Extra_UtilStrsav(pMan->pName);
    pNtkAig->pSpec = Extra_UtilStrsav(pMan->pSpec);
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs BDD-based reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarFold( Abc_Ntk_t * pNtk, int fCompl, int fVerbose )
{
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;
    pMan = Saig_ManDupFoldConstrsFunc( pTemp = pMan, fCompl, fVerbose );
    Aig_ManStop( pTemp );
    pNtkAig = Abc_NtkFromAigPhase( pMan );
    pNtkAig->pName = Extra_UtilStrsav(pMan->pName);
    pNtkAig->pSpec = Extra_UtilStrsav(pMan->pSpec);
    Aig_ManStop( pMan );
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Performs BDD-based reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDarConstrProfile( Abc_Ntk_t * pNtk, int fVerbose )
{
    extern int Ssw_ManProfileConstraints( Aig_Man_t * p, int nWords, int nFrames, int fVerbose );
    extern Vec_Int_t * Saig_ManComputeSwitchProbs( Aig_Man_t * p, int nFrames, int nPref, int fProbOne );
    Aig_Man_t * pMan;
//    Vec_Int_t * vProbOne;
//    Aig_Obj_t * pObj;
//    int i, Entry;
    assert( Abc_NtkIsStrash(pNtk) );
    assert( Abc_NtkConstrNum(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return;
    // value in the init state
//    Abc_AigSetNodePhases( pNtk );
/*
    // derive probabilities
    vProbOne = Saig_ManComputeSwitchProbs( pMan, 48, 16, 1 );
    // iterate over the constraint outputs
    Saig_ManForEachPo( pMan, pObj, i )
    {
        Entry = Vec_IntEntry( vProbOne, Aig_ObjId(pObj) );
        if ( i < Saig_ManPoNum(pMan) - Saig_ManConstrNum(pMan) )
            Abc_Print( 1, "Primary output :  ", i );
        else
            Abc_Print( 1, "Constraint %3d :  ", i-(Saig_ManPoNum(pMan) - Saig_ManConstrNum(pMan)) );
        Abc_Print( 1, "ProbOne = %f  ", Abc_Int2Float(Entry) );
        Abc_Print( 1, "AllZeroValue = %d ", Aig_ObjPhase(pObj) );
        Abc_Print( 1, "\n" );
    }
*/
    // double-check
    Ssw_ManProfileConstraints( pMan, 16, 64, 1 );
    Abc_Print( 1, "TwoFrameSatValue = %d.\n", Ssw_ManSetConstrPhases(pMan, 2, NULL) );
    // clean up
//    Vec_IntFree( vProbOne );
    Aig_ManStop( pMan );
}

/**Function*************************************************************

  Synopsis    [Performs BDD-based reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDarTest( Abc_Ntk_t * pNtk, int Num )
{
//    extern void Saig_ManDetectConstr( Aig_Man_t * p );
//    extern void Saig_ManDetectConstrFuncTest( Aig_Man_t * p );
//    extern void Saig_ManFoldConstrTest( Aig_Man_t * pAig );
    extern void Llb_ManComputeDomsTest( Aig_Man_t * pAig, int Num );



//    extern void Fsim_ManTest( Aig_Man_t * pAig );
    extern Vec_Int_t * Saig_StrSimPerformMatching( Aig_Man_t * p0, Aig_Man_t * p1, int nDist, int fVerbose, Aig_Man_t ** ppMiter );
//    Vec_Int_t * vPairs;
    Aig_Man_t * pMan;//, * pMan2;//, * pTemp;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return;
/*
Aig_ManSetRegNum( pMan, pMan->nRegs );
Aig_ManPrintStats( pMan );
Saig_ManDumpBlif( pMan, "_temp_.blif" );
Aig_ManStop( pMan );
pMan = Saig_ManReadBlif( "_temp_.blif" );
Aig_ManPrintStats( pMan );
*/
/*
    Aig_ManSetRegNum( pMan, pMan->nRegs );
    pTemp = Ssw_SignalCorrespondeceTestPairs( pMan );
    Aig_ManStop( pTemp );
*/

/*
//    Ssw_SecSpecialMiter( pMan, NULL, 2, 1 );
    pMan2 = Aig_ManDupSimple(pMan);
    vPairs = Saig_StrSimPerformMatching( pMan, pMan2, 0, 1, NULL );
    Vec_IntFree( vPairs );
    Aig_ManStop( pMan );
    Aig_ManStop( pMan2 );
*/
//    Ioa_WriteAigerBufferTest( pMan, "test.aig", 0, 0 );
//    Saig_ManFoldConstrTest( pMan );
    {
    extern void Saig_ManBmcSectionsTest( Aig_Man_t * p );
    extern void Saig_ManBmcTerSimTest( Aig_Man_t * p );
    extern void Saig_ManBmcSupergateTest( Aig_Man_t * p );
    extern void Saig_ManBmcMappingTest( Aig_Man_t * p );
//    Saig_ManBmcSectionsTest( pMan );
//    Saig_ManBmcTerSimTest( pMan );
//    Saig_ManBmcSupergateTest( pMan );
//    Saig_ManBmcMappingTest( pMan );
    }

    {
//        void Pdr_ManEquivClasses( Aig_Man_t * pMan );
//        Pdr_ManEquivClasses( pMan );
    }

//    Llb_ManComputeDomsTest( pMan, Num );
    {
        extern void Llb_ManMinCutTest( Aig_Man_t * pMan, int Num );
        extern void Llb_BddStructAnalysis( Aig_Man_t * pMan );
        extern void Llb_NonlinExperiment( Aig_Man_t * pAig, int Num );
//        Llb_BddStructAnalysis( pMan );
//        Llb_ManMinCutTest( pMan, Num );
//        Llb_NonlinExperiment( pMan, Num );
    }

//    Saig_MvManSimulate( pMan, 1 );
//    Saig_ManDetectConstr( pMan );
//    Saig_ManDetectConstrFuncTest( pMan );

//    Fsim_ManTest( pMan );
    Aig_ManStop( pMan );

}

/**Function*************************************************************

  Synopsis    [Performs BDD-based reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDarTestNtk( Abc_Ntk_t * pNtk )
{
//    extern Aig_Man_t * Saig_ManDualRail( Aig_Man_t * p, int fMiter );

/*
    extern Aig_Man_t * Ssw_SignalCorrespondeceTestPairs( Aig_Man_t * pAig );

    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan, * pTemp;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;

    Aig_ManSetRegNum( pMan, pMan->nRegs );
    pMan = Ssw_SignalCorrespondeceTestPairs( pTemp = pMan );
    Aig_ManStop( pTemp );
    if ( pMan == NULL )
        return NULL;

    pNtkAig = Abc_NtkFromAigPhase( pMan );
    pNtkAig->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkAig->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    Aig_ManStop( pMan );
    return pNtkAig;
*/
    Abc_Ntk_t * pNtkAig;
    Aig_Man_t * pMan;//, * pTemp;
    assert( Abc_NtkIsStrash(pNtk) );
    pMan = Abc_NtkToDar( pNtk, 0, 1 );
    if ( pMan == NULL )
        return NULL;

/*
    Aig_ManSetRegNum( pMan, pMan->nRegs );
    pMan = Saig_ManDualRail( pTemp = pMan, 1 );
    Aig_ManStop( pTemp );
    if ( pMan == NULL )
        return NULL;

    pNtkAig = Abc_NtkFromAigPhase( pMan );
    pNtkAig->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkAig->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    Aig_ManStop( pMan );
*/

    pNtkAig = Abc_NtkFromDar( pNtk, pMan );
    Aig_ManStop( pMan );

    return pNtkAig;

}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#include "abcDarUnfold2.c"
ABC_NAMESPACE_IMPL_END

