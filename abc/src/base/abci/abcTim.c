/**CFile****************************************************************

  FileName    [abcTim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Testing hierarchy/timing manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcTim.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "aig/gia/giaAig.h"
#include "misc/tim/tim.h"
#include "opt/dar/dar.h"
#include "proof/dch/dch.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define TIM_TEST_BOX_RATIO 200

// assume that every TIM_TEST_BOX_RATIO'th object is a white box
static inline int Abc_NodeIsWhiteBox( Abc_Obj_t * pObj )  { assert( Abc_ObjIsNode(pObj) ); return Abc_ObjId(pObj) % TIM_TEST_BOX_RATIO == 0 && Abc_ObjFaninNum(pObj) > 0 && Abc_ObjFaninNum(pObj) < 10; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives GIA for the output of the local function of one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTestTimNodeStrash_rec( Gia_Man_t * pGia, Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return;
    Abc_NtkTestTimNodeStrash_rec( pGia, Hop_ObjFanin0(pObj) ); 
    Abc_NtkTestTimNodeStrash_rec( pGia, Hop_ObjFanin1(pObj) );
    pObj->iData = Gia_ManHashAnd( pGia, Hop_ObjChild0CopyI(pObj), Hop_ObjChild1CopyI(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}
int Abc_NtkTestTimNodeStrash( Gia_Man_t * pGia, Abc_Obj_t * pNode )
{
    Hop_Man_t * pMan;
    Hop_Obj_t * pRoot;
    Abc_Obj_t * pFanin;
    int i;
    assert( Abc_ObjIsNode(pNode) );
    assert( Abc_NtkIsAigLogic(pNode->pNtk) );
    // get the local AIG manager and the local root node
    pMan = (Hop_Man_t *)pNode->pNtk->pManFunc;
    pRoot = (Hop_Obj_t *)pNode->pData;
    // check the constant case
    if ( Abc_NodeIsConst(pNode) || Hop_Regular(pRoot) == Hop_ManConst1(pMan) )
        return !Hop_IsComplement(pRoot);
    // set elementary variables
    Abc_ObjForEachFanin( pNode, pFanin, i )
        Hop_IthVar(pMan, i)->iData = pFanin->iTemp;
    // strash the AIG of this node
    Abc_NtkTestTimNodeStrash_rec( pGia, Hop_Regular(pRoot) );
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );
    // return the final node with complement if needed
    return Abc_LitNotCond( Hop_Regular(pRoot)->iData, Hop_IsComplement(pRoot) );
}


#if 0 

/**Function*************************************************************

  Synopsis    [Derives GIA manager using special pins to denote box boundaries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Abc_NtkTestPinDeriveGia( Abc_Ntk_t * pNtk, int fWhiteBoxOnly, int fVerbose )
{
    Gia_Man_t * pTemp;
    Gia_Man_t * pGia = NULL;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pFanin;
    int i, k, iPinLit = 0;
    // prepare logic network
    assert( Abc_NtkIsLogic(pNtk) );
    Abc_NtkToAig( pNtk );
    // construct GIA
    Abc_NtkFillTemp( pNtk );
    pGia = Gia_ManStart( Abc_NtkObjNumMax(pNtk) );
    Gia_ManHashAlloc( pGia );
    // create primary inputs
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->iTemp = Gia_ManAppendCi(pGia);
    // create internal nodes in a topologic order from white boxes
    vNodes = Abc_NtkDfs( pNtk, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        // input side
        if ( !fWhiteBoxOnly || Abc_NodeIsWhiteBox(pObj) )
        {
            // create special pintype for this node
            iPinLit = Gia_ManAppendPinType( pGia, 1 );
            // create input pins
            Abc_ObjForEachFanin( pObj, pFanin, k )
                pFanin->iTemp = Gia_ManAppendAnd( pGia, pFanin->iTemp, iPinLit );
        }
        // perform GIA construction
        pObj->iTemp = Abc_NtkTestTimNodeStrash( pGia, pObj );
        // output side
        if ( !fWhiteBoxOnly || Abc_NodeIsWhiteBox(pObj) )
        {
            // create special pintype for this node
            iPinLit = Gia_ManAppendPinType( pGia, 1 );
            // create output pins
            pObj->iTemp = Gia_ManAppendAnd( pGia, pObj->iTemp, iPinLit );
        }
    }
    Vec_PtrFree( vNodes );
    // create primary outputs
    Abc_NtkForEachCo( pNtk, pObj, i )
        pObj->iTemp = Gia_ManAppendCo( pGia, Abc_ObjFanin0(pObj)->iTemp );
    // finalize GIA
    Gia_ManHashStop( pGia );
    Gia_ManSetRegNum( pGia, 0 );
    // clean up GIA
    pGia = Gia_ManCleanup( pTemp = pGia );
    Gia_ManStop( pTemp );
    return pGia;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTestPinGia( Abc_Ntk_t * pNtk, int fWhiteBoxOnly, int fVerbose )
{
    Gia_Man_t * pGia;
    char * pFileName = "testpin.aig";
    pGia = Abc_NtkTestPinDeriveGia( pNtk, fWhiteBoxOnly, fVerbose );
    Gia_AigerWrite( pGia, pFileName, 0, 0 );
    Gia_ManStop( pGia );
    printf( "AIG with pins derived from mapped network \"%s\" was written into file \"%s\".\n", 
        Abc_NtkName(pNtk), pFileName );
}

#endif

/**Function*************************************************************

  Synopsis    [Collect nodes reachable from this box.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTestTimCollectCone_rec( Abc_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanin;
    int i;
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    if ( Abc_ObjIsCi(pObj) )
        return;
    assert( Abc_ObjIsNode( pObj ) );
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Abc_NtkTestTimCollectCone_rec( pFanin, vNodes );
    Vec_PtrPush( vNodes, pObj );
}
Vec_Ptr_t * Abc_NtkTestTimCollectCone( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj )
{
    Vec_Ptr_t * vCone = Vec_PtrAlloc( 1000 );
    if ( pObj != NULL )
    {
        // collect for one node
        assert( Abc_ObjIsNode(pObj) );
        assert( !Abc_NodeIsTravIdCurrent( pObj ) );
        Abc_NtkTestTimCollectCone_rec( pObj, vCone );
        // remove the node because it is a white box
        Vec_PtrPop( vCone );
    }
    else
    {
        // collect for all COs
        Abc_Obj_t * pObj;
        int i;
        Abc_NtkForEachCo( pNtk, pObj, i )
            Abc_NtkTestTimCollectCone_rec( Abc_ObjFanin0(pObj), vCone );
    }
    return vCone;

}

/**Function*************************************************************

  Synopsis    [Create arrival times]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Flt_t * Abc_NtkTestCreateArrivals( int nInputs )
{
    Vec_Flt_t * p;
    int i;
    p = Vec_FltAlloc( nInputs );
    for ( i = 0; i < nInputs; i++ )
        Vec_FltPush( p, 1.0*(i % 10) );
    return p;
}
Vec_Flt_t * Abc_NtkTestCreateRequired( int nOutputs )
{
    Vec_Flt_t * p;
    int i;
    p = Vec_FltAlloc( nOutputs );
    for ( i = 0; i < nOutputs; i++ )
        Vec_FltPush( p, 100.0 + 1.0*i );
    return p;
}

/**Function*************************************************************

  Synopsis    [Derives GIA manager together with the hierachy manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Abc_NtkTestTimDeriveGia( Abc_Ntk_t * pNtk, int fVerbose )
{
    Gia_Man_t * pTemp;
    Gia_Man_t * pGia = NULL;
    Gia_Man_t * pHoles = NULL;
    Tim_Man_t * pTim = NULL;
    Vec_Int_t * vGiaCoLits, * vGiaCoLits2;
    Vec_Flt_t * vArrTimes, * vReqTimes;
    Abc_Obj_t * pObj, * pFanin;
    int i, k, Entry, curPi, curPo, BoxUniqueId;
    int nBoxFaninMax = 0;
    assert( Abc_NtkIsTopo(pNtk) );
    Abc_NtkFillTemp( pNtk );

    // create white boxes
    curPi = Abc_NtkCiNum(pNtk);
    curPo = Abc_NtkCoNum(pNtk);
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        pObj->fMarkA = Abc_NodeIsWhiteBox( pObj );
        if ( !pObj->fMarkA )
            continue;
        nBoxFaninMax  = Abc_MaxInt( nBoxFaninMax, Abc_ObjFaninNum(pObj) );
        curPi++;
        curPo += Abc_ObjFaninNum(pObj);
        if ( fVerbose )
            printf( "Selecting node %6d as white boxes with %d inputs and %d output.\n", i, Abc_ObjFaninNum(pObj), 1 );
    }

    // construct GIA
    pGia = Gia_ManStart( Abc_NtkObjNumMax(pNtk) );
    pHoles = Gia_ManStart( 1000 );
    for ( i = 0; i < curPi; i++ )
        Gia_ManAppendCi(pGia);
    for ( i = 0; i < nBoxFaninMax; i++ )
        Gia_ManAppendCi(pHoles);
    Gia_ManHashAlloc( pGia );
    Gia_ManHashAlloc( pHoles );

    // construct the timing manager
    pTim = Tim_ManStart( curPi, curPo );

    // assign primary inputs
    curPi = 0;
    curPo = 0;
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->iTemp = Abc_Var2Lit( Gia_ObjId(pGia, Gia_ManCi(pGia, curPi++)), 0 );
    // create internal nodes in a topologic order from white boxes
    vGiaCoLits = Vec_IntAlloc( 1000 );
    vGiaCoLits2 = Vec_IntAlloc( 1000 );
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( !pObj->fMarkA ) // not a white box
        {
            pObj->iTemp = Abc_NtkTestTimNodeStrash( pGia, pObj );
            continue;
        }
        // create box
        BoxUniqueId = Abc_ObjFaninNum(pObj); // in this case, the node size is the ID of its delay table
        Tim_ManCreateBox( pTim, curPo, Abc_ObjFaninNum(pObj), curPi, 1, BoxUniqueId, 0 );
        curPo += Abc_ObjFaninNum(pObj);

        // handle box inputs
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            // save CO drivers for the AIG
            Vec_IntPush( vGiaCoLits, pFanin->iTemp );
            // load CI nodes for the Holes
            pFanin->iTemp = Abc_Var2Lit( Gia_ObjId(pHoles, Gia_ManCi(pHoles, k)), 0 );
        }

        // handle logic of the box
        pObj->iTemp = Abc_NtkTestTimNodeStrash( pHoles, pObj );

        // handle box outputs
        // save CO drivers for the Holes
        Vec_IntPush( vGiaCoLits2, pObj->iTemp );
//        Gia_ManAppendCo( pHoles, pObj->iTemp );
        // load CO drivers for the AIG
        pObj->iTemp = Abc_Var2Lit( Gia_ObjId(pGia, Gia_ManCi(pGia, curPi++)), 0 );
    }
    Abc_NtkCleanMarkA( pNtk );
    // create COs of the AIG
    Abc_NtkForEachCo( pNtk, pObj, i )
        Gia_ManAppendCo( pGia, Abc_ObjFanin0(pObj)->iTemp );
    Vec_IntForEachEntry( vGiaCoLits, Entry, i )
        Gia_ManAppendCo( pGia, Entry );
    Vec_IntFree( vGiaCoLits );
    // second AIG
    Vec_IntForEachEntry( vGiaCoLits2, Entry, i )
        Gia_ManAppendCo( pHoles, Entry );
    Vec_IntFree( vGiaCoLits2 );
    // check parameters
    curPo += Abc_NtkPoNum( pNtk );
    assert( curPi == Gia_ManPiNum(pGia) );
    assert( curPo == Gia_ManPoNum(pGia) );
    // finalize GIA
    Gia_ManHashStop( pGia );
    Gia_ManSetRegNum( pGia, 0 );
    Gia_ManHashStop( pHoles );
    Gia_ManSetRegNum( pHoles, 0 );

    // clean up GIA
    pGia = Gia_ManCleanup( pTemp = pGia );
    Gia_ManStop( pTemp );
    pHoles = Gia_ManCleanup( pTemp = pHoles );
    Gia_ManStop( pTemp );

    // attach the timing manager
    assert( pGia->pManTime == NULL );
    pGia->pManTime = pTim;

    // derive hierarchy manager from box info and input/output arrival/required info
    vArrTimes = Abc_NtkTestCreateArrivals( Abc_NtkPiNum(pNtk) );
    vReqTimes = Abc_NtkTestCreateRequired( Abc_NtkPoNum(pNtk) );

    Tim_ManPrint( (Tim_Man_t *)pGia->pManTime );
    Tim_ManCreate( (Tim_Man_t *)pGia->pManTime, Abc_FrameReadLibBox(), vArrTimes, vReqTimes );
    Tim_ManPrint( (Tim_Man_t *)pGia->pManTime );

    Vec_FltFree( vArrTimes );
    Vec_FltFree( vReqTimes );

Gia_AigerWrite( pHoles, "holes00.aig", 0, 0 );

    // return 
    pGia->pAigExtra = pHoles;
    return pGia;
}


/**Function*************************************************************

  Synopsis    [Performs synthesis with or without structural choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Abc_NtkTestTimPerformSynthesis( Gia_Man_t * p, int fChoices )
{
    Gia_Man_t * pGia;
    Aig_Man_t * pNew, * pTemp;
    Dch_Pars_t Pars, * pPars = &Pars;
    Dch_ManSetDefaultParams( pPars );
    pNew = Gia_ManToAig( p, 0 );
    if ( fChoices )
        pNew = Dar_ManChoiceNew( pNew, pPars );
    else
    {
        pNew = Dar_ManCompress2( pTemp = pNew, 1, 1, 1, 0, 0 );
        Aig_ManStop( pTemp );
    }
    pGia = Gia_ManFromAig( pNew );
    Aig_ManStop( pNew );
    return pGia;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManVerifyChoices( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, iRepr, iNode, fProb = 0;
    assert( p->pReprs );

    // mark nodes 
    Gia_ManCleanMark0(p);
    Gia_ManForEachClass( p, iRepr )
        Gia_ClassForEachObj1( p, iRepr, iNode )
        {
            if ( Gia_ObjIsHead(p, iNode) )
                printf( "Member %d of choice class %d is a representative.\n", iNode, iRepr ), fProb = 1;
            if ( Gia_ManObj( p, iNode )->fMark0 == 1 )
                printf( "Node %d participates in more than one choice node.\n", iNode ), fProb = 1;
            Gia_ManObj( p, iNode )->fMark0 = 1;
        }
    Gia_ManCleanMark0(p);

    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
        {
            if ( Gia_ObjHasRepr(p, Gia_ObjFaninId0(pObj, i)) )
                printf( "Fanin 0 of AND node %d has a repr.\n", i ), fProb = 1;
            if ( Gia_ObjHasRepr(p, Gia_ObjFaninId1(pObj, i)) )
                printf( "Fanin 1 of AND node %d has a repr.\n", i ), fProb = 1;
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            if ( Gia_ObjHasRepr(p, Gia_ObjFaninId0(pObj, i)) )
                printf( "Fanin 0 of CO node %d has a repr.\n", i ), fProb = 1;
        }
    }
//    if ( !fProb )
//        printf( "GIA with choices is correct.\n" );
}

/**Function*************************************************************

  Synopsis    [Reverse the order of nodes in equiv classes.]

  Description [If the flag is 1, assumed current increasing order ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManReverseClasses( Gia_Man_t * p, int fNowIncreasing )
{
    Vec_Int_t * vCollected;
    Vec_Int_t * vClass;
    int i, k, iRepr, iNode, iPrev;
    // collect classes
    vCollected = Vec_IntAlloc( 100 );
    Gia_ManForEachClass( p, iRepr )
        Vec_IntPush( vCollected, iRepr );
    // correct each class
    vClass = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vCollected, iRepr, i )
    {
        Vec_IntClear( vClass );
        Vec_IntPush( vClass, iRepr );
        Gia_ClassForEachObj1( p, iRepr, iNode )
        {
            if ( fNowIncreasing )
                assert( iRepr < iNode );
            else
                assert( iRepr > iNode );
            Vec_IntPush( vClass, iNode );
        }
//        if ( !fNowIncreasing )
//            Vec_IntSort( vClass, 1 );
        // reverse the class
        iPrev = 0;
        iRepr = Vec_IntEntryLast( vClass );
        Vec_IntForEachEntry( vClass, iNode, k )
        {
            if ( fNowIncreasing )
                Gia_ObjSetReprRev( p, iNode, iNode == iRepr ? GIA_VOID : iRepr );
            else
                Gia_ObjSetRepr( p, iNode, iNode == iRepr ? GIA_VOID : iRepr );
            Gia_ObjSetNext( p, iNode, iPrev );
            iPrev = iNode;
        }
    }
    Vec_IntFree( vCollected );
    Vec_IntFree( vClass );
    // verify
    Gia_ManForEachClass( p, iRepr )
        Gia_ClassForEachObj1( p, iRepr, iNode )
            if ( fNowIncreasing )
                assert( Gia_ObjRepr(p, iNode) == iRepr && iRepr > iNode );
            else
                assert( Gia_ObjRepr(p, iNode) == iRepr && iRepr < iNode );
}

/**Function*************************************************************

  Synopsis    [Tests the hierarchy-timing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTestTimByWritingFile( Gia_Man_t * pGia, char * pFileName )
{
    Gia_Man_t * pGia2;

    // normalize choices
    if ( Gia_ManHasChoices(pGia) )
    {
        Gia_ManVerifyChoices( pGia );
        Gia_ManReverseClasses( pGia, 0 );
    }
    // write file
    Gia_AigerWrite( pGia, pFileName, 0, 0 );
    // unnormalize choices
    if ( Gia_ManHasChoices(pGia) )
        Gia_ManReverseClasses( pGia, 1 );

    // read file
    pGia2 = Gia_AigerRead( pFileName, 0, 1, 1 );

    // normalize choices
    if ( Gia_ManHasChoices(pGia2) )
    {
        Gia_ManVerifyChoices( pGia2 );
        Gia_ManReverseClasses( pGia2, 1 );
    }

    // compare resulting managers
    if ( Gia_ManCompare( pGia, pGia2 ) )
        printf( "Verification suceessful.\n" );

    Gia_ManStop( pGia2 );
}

/**Function*************************************************************

  Synopsis    [Tests construction and serialization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTestTim( Abc_Ntk_t * pNtk, int fVerbose )
{
    int fUseChoices = 0;
    Gia_Man_t * pGia, * pTemp;

    // this test only works for a logic network (for example, network with LUT mapping)
    assert( Abc_NtkIsLogic(pNtk) );
    // make sure local functions of the nodes are in the AIG form
    Abc_NtkToAig( pNtk );

    // create GIA manager (pGia) with hierarhy/timing manager attached (pGia->pManTime)
    // while assuming that some nodes are white boxes (see Abc_NodeIsWhiteBox)
    pGia = Abc_NtkTestTimDeriveGia( pNtk, fVerbose );
    printf( "Created GIA manager for network with %d white boxes.\n", Tim_ManBoxNum((Tim_Man_t *)pGia->pManTime) );

    // print the timing manager
    if ( fVerbose )
        Tim_ManPrint( (Tim_Man_t *)pGia->pManTime );

    // test writing both managers into a file and reading them back
    Abc_NtkTestTimByWritingFile( pGia, "test1.aig" );

    // perform synthesis
    pGia = Abc_NtkTestTimPerformSynthesis( pTemp = pGia, fUseChoices );
    Gia_ManStop( pTemp );

    // test writing both managers into a file and reading them back
    Abc_NtkTestTimByWritingFile( pGia, "test2.aig" );

    Gia_ManStop( pGia );
}




////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

