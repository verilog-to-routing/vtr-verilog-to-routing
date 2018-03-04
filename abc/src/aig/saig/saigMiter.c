/**CFile****************************************************************

  FileName    [saigMiter.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Computes sequential miter of two sequential AIGs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigMiter.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"
#include "proof/fra/fra.h"

ABC_NAMESPACE_IMPL_START

 
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks the status of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sec_MtrStatus_t Sec_MiterStatus( Aig_Man_t * p )
{
    Sec_MtrStatus_t Status;
    Aig_Obj_t * pObj, * pChild;
    int i;
    memset( &Status, 0, sizeof(Sec_MtrStatus_t) );
    Status.iOut = -1;
    Status.nInputs  = Saig_ManPiNum( p );
    Status.nNodes   = Aig_ManNodeNum( p );
    Status.nOutputs = Saig_ManPoNum(p);
    Saig_ManForEachPo( p, pObj, i )
    {
        pChild = Aig_ObjChild0(pObj);
        // check if the output is constant 0
        if ( pChild == Aig_ManConst0(p) )
            Status.nUnsat++;
        // check if the output is constant 1
        else if ( pChild == Aig_ManConst1(p) )
        {
            Status.nSat++;
            if ( Status.iOut == -1 )
                Status.iOut = i;
        }
        // check if the output is a primary input
        else if ( Saig_ObjIsPi(p, Aig_Regular(pChild)) )
        {
            Status.nSat++;
            if ( Status.iOut == -1 )
                Status.iOut = i;
        }
    // check if the output is 1 for the 0000 pattern
        else if ( Aig_Regular(pChild)->fPhase != (unsigned)Aig_IsComplement(pChild) )
        {
            Status.nSat++;
            if ( Status.iOut == -1 )
                Status.iOut = i;
        }
        else
            Status.nUndec++;
    }
    return Status;
}

/**Function*************************************************************

  Synopsis    [Creates sequential miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManCreateMiter( Aig_Man_t * p0, Aig_Man_t * p1, int Oper )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    int i;
    assert( Saig_ManRegNum(p0) > 0 || Saig_ManRegNum(p1) > 0 );
    assert( Saig_ManPiNum(p0) == Saig_ManPiNum(p1) );
    assert( Saig_ManPoNum(p0) == Saig_ManPoNum(p1) );
    pNew = Aig_ManStart( Aig_ManObjNumMax(p0) + Aig_ManObjNumMax(p1) );
    pNew->pName = Abc_UtilStrsav( "miter" );
    Aig_ManCleanData( p0 );
    Aig_ManCleanData( p1 );
    // map constant nodes
    Aig_ManConst1(p0)->pData = Aig_ManConst1(pNew);
    Aig_ManConst1(p1)->pData = Aig_ManConst1(pNew);
    // map primary inputs
    Saig_ManForEachPi( p0, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    Saig_ManForEachPi( p1, pObj, i )
        pObj->pData = Aig_ManCi( pNew, i );
    // map register outputs
    Saig_ManForEachLo( p0, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    Saig_ManForEachLo( p1, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    // map internal nodes
    Aig_ManForEachNode( p0, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    Aig_ManForEachNode( p1, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // create primary outputs
    Saig_ManForEachPo( p0, pObj, i )
    {
        if ( Oper == 0 ) // XOR
            pObj = Aig_Exor( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild0Copy(Aig_ManCo(p1,i)) );
        else if ( Oper == 1 ) // implication is PO(p0) -> PO(p1)  ...  complement is PO(p0) & !PO(p1) 
            pObj = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_Not(Aig_ObjChild0Copy(Aig_ManCo(p1,i))) );
        else
            assert( 0 );
        Aig_ObjCreateCo( pNew, pObj );
    }
    // create register inputs
    Saig_ManForEachLi( p0, pObj, i )
        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    Saig_ManForEachLi( p1, pObj, i )
        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    // cleanup
    Aig_ManSetRegNum( pNew, Saig_ManRegNum(p0) + Saig_ManRegNum(p1) );
//    Aig_ManCleanup( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Creates combinational miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManCreateMiterComb( Aig_Man_t * p0, Aig_Man_t * p1, int Oper )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    int i;
    assert( Aig_ManCiNum(p0) == Aig_ManCiNum(p1) );
    assert( Aig_ManCoNum(p0) == Aig_ManCoNum(p1) );
    pNew = Aig_ManStart( Aig_ManObjNumMax(p0) + Aig_ManObjNumMax(p1) );
    pNew->pName = Abc_UtilStrsav( "miter" );
    // map constant nodes
    Aig_ManConst1(p0)->pData = Aig_ManConst1(pNew);
    Aig_ManConst1(p1)->pData = Aig_ManConst1(pNew);
    // map primary inputs and register outputs
    Aig_ManForEachCi( p0, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    Aig_ManForEachCi( p1, pObj, i )
        pObj->pData = Aig_ManCi( pNew, i );
    // map internal nodes
    Aig_ManForEachNode( p0, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    Aig_ManForEachNode( p1, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // create primary outputs
    Aig_ManForEachCo( p0, pObj, i )
    {
        if ( Oper == 0 ) // XOR
            pObj = Aig_Exor( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild0Copy(Aig_ManCo(p1,i)) );
        else if ( Oper == 1 ) // implication is PO(p0) -> PO(p1)  ...  complement is PO(p0) & !PO(p1) 
            pObj = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_Not(Aig_ObjChild0Copy(Aig_ManCo(p1,i))) );
        else
            assert( 0 );
        Aig_ObjCreateCo( pNew, pObj );
    }
    // cleanup
    Aig_ManSetRegNum( pNew, 0 );
    Aig_ManCleanup( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Derives dual-rail AIG.]

  Description [Orders nodes as follows: PIs, ANDs, POs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_AndDualRail( Aig_Man_t * pNew, Aig_Obj_t * pObj, Aig_Obj_t ** ppData, Aig_Obj_t ** ppNext )
{
    Aig_Obj_t * pFanin0 = Aig_ObjFanin0(pObj);
    Aig_Obj_t * pFanin1 = Aig_ObjFanin1(pObj);
    Aig_Obj_t * p0Data = Aig_ObjFaninC0(pObj)? pFanin0->pNext              : (Aig_Obj_t *)pFanin0->pData;
    Aig_Obj_t * p0Next = Aig_ObjFaninC0(pObj)? (Aig_Obj_t *)pFanin0->pData : pFanin0->pNext;
    Aig_Obj_t * p1Data = Aig_ObjFaninC1(pObj)? pFanin1->pNext              : (Aig_Obj_t *)pFanin1->pData;
    Aig_Obj_t * p1Next = Aig_ObjFaninC1(pObj)? (Aig_Obj_t *)pFanin1->pData : pFanin1->pNext;
    *ppData = Aig_Or( pNew, 
        Aig_And(pNew, p0Data, Aig_Not(p0Next)),
        Aig_And(pNew, p1Data, Aig_Not(p1Next)) );
    *ppNext = Aig_And( pNew, 
        Aig_And(pNew, Aig_Not(p0Data), p0Next),
        Aig_And(pNew, Aig_Not(p1Data), p1Next) );
}

/**Function*************************************************************

  Synopsis    [Derives dual-rail AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManDualRail( Aig_Man_t * p, int fMiter )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj, * pMiter;
    int i;
    Aig_ManCleanData( p );
    Aig_ManCleanNext( p );
    // create the new manager
    pNew = Aig_ManStart( 4*Aig_ManObjNumMax(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // create the PIs 
    Aig_ManConst1(p)->pData = Aig_ManConst0(pNew);
    Aig_ManConst1(p)->pNext = Aig_ManConst1(pNew);
    Aig_ManForEachCi( p, pObj, i )
    {
        pObj->pData = Aig_ObjCreateCi( pNew );
        pObj->pNext = Aig_ObjCreateCi( pNew );
    }
    // duplicate internal nodes
    Aig_ManForEachNode( p, pObj, i )
        Saig_AndDualRail( pNew, pObj, (Aig_Obj_t **)&pObj->pData, &pObj->pNext );
    // add the POs
    if ( fMiter )
    {
        pMiter = Aig_ManConst1(pNew);
        Saig_ManForEachLo( p, pObj, i )
        {
            pMiter = Aig_And( pNew, pMiter, 
                Aig_Or(pNew, (Aig_Obj_t *)pObj->pData, pObj->pNext) );
        } 
        Aig_ObjCreateCo( pNew, pMiter );
        Saig_ManForEachLi( p, pObj, i )
        {
            if ( !Aig_ObjFaninC0(pObj) )
            {
                Aig_ObjCreateCo( pNew, (Aig_Obj_t *)Aig_ObjFanin0(pObj)->pData );
                Aig_ObjCreateCo( pNew, Aig_ObjFanin0(pObj)->pNext );
            }
            else
            {
                Aig_ObjCreateCo( pNew, Aig_ObjFanin0(pObj)->pNext );
                Aig_ObjCreateCo( pNew, (Aig_Obj_t *)Aig_ObjFanin0(pObj)->pData );
            }
        }
    }
    else
    {
        Aig_ManForEachCo( p, pObj, i )
        {
            if ( !Aig_ObjFaninC0(pObj) )
            {
                Aig_ObjCreateCo( pNew, (Aig_Obj_t *)Aig_ObjFanin0(pObj)->pData );
                Aig_ObjCreateCo( pNew, Aig_ObjFanin0(pObj)->pNext );
            }
            else
            {
                Aig_ObjCreateCo( pNew, Aig_ObjFanin0(pObj)->pNext );
                Aig_ObjCreateCo( pNew, (Aig_Obj_t *)Aig_ObjFanin0(pObj)->pData );
            }
        }
    }
    Aig_ManSetRegNum( pNew, 2*Aig_ManRegNum(p) );
    Aig_ManCleanData( p );
    Aig_ManCleanNext( p );
    Aig_ManCleanup( pNew );
    // check the resulting network
    if ( !Aig_ManCheck(pNew) )
        printf( "Aig_ManDupSimple(): The check has failed.\n" );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Create combinational timeframes by unrolling sequential circuits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManUnrollTwo( Aig_Man_t * pBot, Aig_Man_t * pTop, int nFrames )
{
    Aig_Man_t * p, * pAig;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i, f;
//    assert( nFrames > 1 );
    assert( Saig_ManPiNum(pBot) == Saig_ManPiNum(pTop) );
    assert( Saig_ManPoNum(pBot) == Saig_ManPoNum(pTop) );
    assert( Saig_ManRegNum(pBot) == Saig_ManRegNum(pTop) );
    assert( Saig_ManRegNum(pBot) > 0 || Saig_ManRegNum(pTop) > 0 );
    // start timeframes
    p = Aig_ManStart( nFrames * Abc_MaxInt(Aig_ManObjNumMax(pBot), Aig_ManObjNumMax(pTop)) );
    p->pName = Abc_UtilStrsav( "frames" );
    // create variables for register outputs
    pAig = pBot;
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( p );
    // add timeframes
    for ( f = 0; f < nFrames; f++ )
    {
        // create PI nodes for this frame
        Aig_ManConst1(pAig)->pData = Aig_ManConst1( p );
        Saig_ManForEachPi( pAig, pObj, i )
            pObj->pData = Aig_ObjCreateCi( p );
        // add internal nodes of this frame
        Aig_ManForEachNode( pAig, pObj, i )
            pObj->pData = Aig_And( p, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
        if ( f == nFrames - 1 )
        {
            // create POs for this frame
            Aig_ManForEachCo( pAig, pObj, i )
                Aig_ObjCreateCo( p, Aig_ObjChild0Copy(pObj) );
            break;
        }
        // create POs for this frame
        Saig_ManForEachPo( pAig, pObj, i )
            Aig_ObjCreateCo( p, Aig_ObjChild0Copy(pObj) );
        // save register inputs
        Saig_ManForEachLi( pAig, pObj, i )
            pObj->pData = Aig_ObjChild0Copy(pObj);
        // transfer to register outputs
        Saig_ManForEachLiLo(  pAig, pObjLi, pObjLo, i )
            pObjLo->pData = pObjLi->pData;
        if ( f == 0 )
        {
            // transfer from pOld to pNew
            Saig_ManForEachLo( pAig, pObj, i )
                Saig_ManLo(pTop, i)->pData = pObj->pData;
            pAig = pTop;
        }
    }
    Aig_ManCleanup( p );
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG while creating POs from the set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManDupNodesAll( Aig_Man_t * p, Vec_Ptr_t * vSet )
{
    Aig_Man_t * pNew, * pCopy;
    Aig_Obj_t * pObj;
    int i;
    pNew = Aig_ManStart( Aig_ManObjNumMax(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    Aig_ManForEachCi( p, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    Aig_ManForEachNode( p, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
//    Saig_ManForEachPo( p, pObj, i )
//        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    Vec_PtrForEachEntry( Aig_Obj_t *, vSet, pObj, i )
        Aig_ObjCreateCo( pNew, Aig_NotCond((Aig_Obj_t *)Aig_Regular(pObj)->pData, Aig_IsComplement(pObj)) );
    Saig_ManForEachLi( p, pObj, i )
        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    Aig_ManSetRegNum( pNew, Saig_ManRegNum(p) );
    // cleanup and return a copy
    Aig_ManSeqCleanup( pNew );
    pCopy = Aig_ManDupSimpleDfs( pNew );
    Aig_ManStop( pNew );
    return pCopy;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG while creating POs from the set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManDupNodesHalf( Aig_Man_t * p, Vec_Ptr_t * vSet, int iPart )
{
    Aig_Man_t * pNew, * pCopy;
    Aig_Obj_t * pObj;
    int i;
    Aig_ManCleanData( p );
    pNew = Aig_ManStart( Aig_ManObjNumMax(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    Saig_ManForEachPi( p, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    if ( iPart == 0 )
    {
        Saig_ManForEachLo( p, pObj, i )
            if ( i < Saig_ManRegNum(p)/2 )
                pObj->pData = Aig_ObjCreateCi( pNew );
    }
    else
    {
        Saig_ManForEachLo( p, pObj, i )
            if ( i >= Saig_ManRegNum(p)/2 )
                pObj->pData = Aig_ObjCreateCi( pNew );
    }
    Aig_ManForEachNode( p, pObj, i )
        if ( Aig_ObjFanin0(pObj)->pData && Aig_ObjFanin1(pObj)->pData )
            pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
//    Saig_ManForEachPo( p, pObj, i )
//        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    Vec_PtrForEachEntry( Aig_Obj_t *, vSet, pObj, i )
    {
        assert( Aig_Regular(pObj)->pData != NULL );
        Aig_ObjCreateCo( pNew, Aig_NotCond((Aig_Obj_t *)Aig_Regular(pObj)->pData, Aig_IsComplement(pObj)) );
    }
    if ( iPart == 0 )
    {
        Saig_ManForEachLi( p, pObj, i )
            if ( i < Saig_ManRegNum(p)/2 )
                pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    }
    else
    {
        Saig_ManForEachLi( p, pObj, i )
            if ( i >= Saig_ManRegNum(p)/2 )
                pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    }
    Aig_ManSetRegNum( pNew, Saig_ManRegNum(p)/2 );
    // cleanup and return a copy
//    Aig_ManSeqCleanup( pNew );
    Aig_ManCleanup( pNew );
    pCopy = Aig_ManDupSimpleDfs( pNew );
    Aig_ManStop( pNew );
    return pCopy;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG to have constant-0 initial state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManDemiterSimple( Aig_Man_t * p, Aig_Man_t ** ppAig0, Aig_Man_t ** ppAig1 )
{
    Vec_Ptr_t * vSet0, * vSet1;
    Aig_Obj_t * pObj, * pFanin, * pObj0, * pObj1;
    int i, Counter = 0;
    assert( Saig_ManRegNum(p) % 2 == 0 );
    vSet0 = Vec_PtrAlloc( Saig_ManPoNum(p) );
    vSet1 = Vec_PtrAlloc( Saig_ManPoNum(p) );
    Saig_ManForEachPo( p, pObj, i )
    {
        pFanin = Aig_ObjFanin0(pObj);
        if ( Aig_ObjIsConst1( pFanin ) )
        {
            if ( !Aig_ObjFaninC0(pObj) )
                printf( "The output number %d of the miter is constant 1.\n", i );
            Counter++;
            continue;
        }
        if ( !Aig_ObjIsNode(pFanin) || !Aig_ObjRecognizeExor( pFanin, &pObj0, &pObj1 ) )
        {
            printf( "The miter cannot be demitered.\n" );
            Vec_PtrFree( vSet0 );
            Vec_PtrFree( vSet1 );
            return 0;
        }
        if ( Aig_ObjFaninC0(pObj) )
            pObj0 = Aig_Not(pObj0);

//        printf( "%d %d  ", Aig_Regular(pObj0)->Id, Aig_Regular(pObj1)->Id );
        if ( Aig_Regular(pObj0)->Id < Aig_Regular(pObj1)->Id )
        {
            Vec_PtrPush( vSet0, pObj0 );
            Vec_PtrPush( vSet1, pObj1 );
        }
        else
        {
            Vec_PtrPush( vSet0, pObj1 );
            Vec_PtrPush( vSet1, pObj0 );
        }
    }
//    printf( "Miter has %d constant outputs.\n", Counter );
//    printf( "\n" );
    if ( ppAig0 )
    {
        *ppAig0 = Aig_ManDupNodesHalf( p, vSet0, 0 );
        ABC_FREE( (*ppAig0)->pName );
        (*ppAig0)->pName = Abc_UtilStrsav( "part0" );
    }
    if ( ppAig1 )
    {
        *ppAig1 = Aig_ManDupNodesHalf( p, vSet1, 1 );
        ABC_FREE( (*ppAig1)->pName );
        (*ppAig1)->pName = Abc_UtilStrsav( "part1" );
    }
    Vec_PtrFree( vSet0 );
    Vec_PtrFree( vSet1 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if PO can be demitered.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManDemiterMarkPos( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManCleanMarkAB( p );
    Saig_ManForEachLo( p, pObj, i )
        if ( i < Saig_ManRegNum(p)/2 )
            pObj->fMarkA = 1;
        else
            pObj->fMarkB = 1;
    Aig_ManForEachNode( p, pObj, i )
    {
        pObj->fMarkA = Aig_ObjFanin0(pObj)->fMarkA | Aig_ObjFanin1(pObj)->fMarkA;
        pObj->fMarkB = Aig_ObjFanin0(pObj)->fMarkB | Aig_ObjFanin1(pObj)->fMarkB;
    }
}

/**Function*************************************************************

  Synopsis    [Returns 1 if PO can be demitered.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManDemiterCheckPo( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t ** ppPo0, Aig_Obj_t ** ppPo1 )
{
    Aig_Obj_t * pFanin, * pObj0, * pObj1, * pObjR0, * pObjR1;
    assert( Saig_ObjIsPo(p, pObj) );
    pFanin = Aig_ObjFanin0( pObj );
    if ( Aig_ObjIsConst1(pFanin) )
    {
        if ( !Aig_ObjFaninC0(pObj) )
            return 0;
        *ppPo0 = Aig_ManConst0(p);
        *ppPo1 = Aig_ManConst0(p);
        return 1;
    }
    if ( !Aig_ObjIsNode(pFanin) )
        return 0;
    if ( !Aig_ObjRecognizeExor( pFanin, &pObj0, &pObj1 ) )
        return 0;
    if ( Aig_ObjFaninC0(pObj) )
        pObj0 = Aig_Not(pObj0);
    // make sure they can reach only one
    pObjR0 = Aig_Regular(pObj0);
    pObjR1 = Aig_Regular(pObj1);
    if ( (pObjR0->fMarkA && pObjR0->fMarkB) || (pObjR1->fMarkA && pObjR1->fMarkB) || 
         (pObjR0->fMarkA && pObjR1->fMarkA) || (pObjR0->fMarkB && pObjR1->fMarkB) )
         return 0;

    if ( pObjR1->fMarkA && !pObjR0->fMarkA )
    {
        *ppPo0 = pObj1;
        *ppPo1 = pObj0;
    }
    else if ( pObjR0->fMarkA && !pObjR1->fMarkA )
    {
        *ppPo0 = pObj0;
        *ppPo1 = pObj1;
    }
    else
    {
/*
printf( "%d", pObjR0->fMarkA );
printf( "%d", pObjR0->fMarkB );
printf( ":" );
printf( "%d", pObjR1->fMarkA );
printf( "%d", pObjR1->fMarkB );
printf( "   " );
*/
        if ( Aig_Regular(pObj0)->Id < Aig_Regular(pObj1)->Id )
        {
            *ppPo0 = pObj0;
            *ppPo1 = pObj1;
        }
        else
        {
            *ppPo0 = pObj1;
            *ppPo1 = pObj0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if AIG can be demitered.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManDemiterSimpleDiff( Aig_Man_t * p, Aig_Man_t ** ppAig0, Aig_Man_t ** ppAig1 )
{
    Vec_Ptr_t * vSet0, * vSet1;
    Aig_Obj_t * pObj, * pObj0, * pObj1;
    int i;
    if ( Aig_ManRegNum(p) == 0 || (Aig_ManRegNum(p) & 1) )
        return 0;
    Saig_ManDemiterMarkPos( p );
    vSet0 = Vec_PtrAlloc( Saig_ManPoNum(p) );
    vSet1 = Vec_PtrAlloc( Saig_ManPoNum(p) );
    Saig_ManForEachPo( p, pObj, i )
    {
        if ( !Saig_ManDemiterCheckPo( p, pObj, &pObj0, &pObj1 ) )
        {
            Vec_PtrFree( vSet0 );
            Vec_PtrFree( vSet1 );
            Aig_ManCleanMarkAB( p );
            return 0;
        }
        Vec_PtrPush( vSet0, pObj0 );
        Vec_PtrPush( vSet1, pObj1 );
    }
    // create new AIG
    *ppAig0 = Aig_ManDupNodesHalf( p, vSet0, 0 );
    ABC_FREE( (*ppAig0)->pName );
    (*ppAig0)->pName = Abc_UtilStrsav( "part0" );
    // create new AIGs
    *ppAig1 = Aig_ManDupNodesHalf( p, vSet1, 1 );
    ABC_FREE( (*ppAig1)->pName );
    (*ppAig1)->pName = Abc_UtilStrsav( "part1" );
    // cleanup
    Vec_PtrFree( vSet0 );
    Vec_PtrFree( vSet1 );
    Aig_ManCleanMarkAB( p );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if AIG can be demitered.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManDemiterDual( Aig_Man_t * p, Aig_Man_t ** ppAig0, Aig_Man_t ** ppAig1 )
{
    Aig_Man_t * pTemp;
    Aig_Obj_t * pObj;
    int i, k;

    if ( p->pFanData )
        Aig_ManFanoutStop( p );

    k = 0;
    pTemp = Aig_ManDupSimple( p );
    Saig_ManForEachPo( pTemp, pObj, i )
    {
        if ( i & 1 )
            Aig_ObjDeletePo( pTemp, pObj );
        else
            Vec_PtrWriteEntry( pTemp->vCos, k++, pObj );
    }
    Saig_ManForEachLi( pTemp, pObj, i )
        Vec_PtrWriteEntry( pTemp->vCos, k++, pObj );
    Vec_PtrShrink( pTemp->vCos, k );
    pTemp->nTruePos = k - Saig_ManRegNum(pTemp);
    Aig_ManSeqCleanup( pTemp );
    *ppAig0 = Aig_ManDupSimple( pTemp );
    Aig_ManStop( pTemp );

    k = 0;
    pTemp = Aig_ManDupSimple( p );
    Saig_ManForEachPo( pTemp, pObj, i )
    {
        if ( i & 1 )
            Vec_PtrWriteEntry( pTemp->vCos, k++, pObj );
        else
            Aig_ObjDeletePo( pTemp, pObj );
    }
    Saig_ManForEachLi( pTemp, pObj, i )
        Vec_PtrWriteEntry( pTemp->vCos, k++, pObj );
    Vec_PtrShrink( pTemp->vCos, k );
    pTemp->nTruePos = k - Saig_ManRegNum(pTemp);
    Aig_ManSeqCleanup( pTemp );
    *ppAig1 = Aig_ManDupSimple( pTemp );
    Aig_ManStop( pTemp );

    return 1;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG to have constant-0 initial state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManDemiterSimpleDiff_old( Aig_Man_t * p, Aig_Man_t ** ppAig0, Aig_Man_t ** ppAig1 )
{
    Vec_Ptr_t * vSet0, * vSet1;
    Aig_Obj_t * pObj, * pFanin, * pObj0, * pObj1;
    int i, Counter = 0;
//    assert( Saig_ManRegNum(p) % 2 == 0 );
    vSet0 = Vec_PtrAlloc( Saig_ManPoNum(p) );
    vSet1 = Vec_PtrAlloc( Saig_ManPoNum(p) );
    Saig_ManForEachPo( p, pObj, i )
    {
        pFanin = Aig_ObjFanin0(pObj);
        if ( Aig_ObjIsConst1( pFanin ) )
        {
            if ( !Aig_ObjFaninC0(pObj) )
                printf( "The output number %d of the miter is constant 1.\n", i );
            Counter++;
            continue;
        } 
        if ( !Aig_ObjIsNode(pFanin) || !Aig_ObjRecognizeExor( pFanin, &pObj0, &pObj1 ) )
        {
/*
            printf( "The miter cannot be demitered.\n" );
            Vec_PtrFree( vSet0 );
            Vec_PtrFree( vSet1 );
            return 0;
*/
            printf( "The output number %d cannot be demitered.\n", i );
            continue;
        }
        if ( Aig_ObjFaninC0(pObj) )
            pObj0 = Aig_Not(pObj0);

//        printf( "%d %d  ", Aig_Regular(pObj0)->Id, Aig_Regular(pObj1)->Id );
        if ( Aig_Regular(pObj0)->Id < Aig_Regular(pObj1)->Id )
        {
            Vec_PtrPush( vSet0, pObj0 );
            Vec_PtrPush( vSet1, pObj1 );
        }
        else
        {
            Vec_PtrPush( vSet0, pObj1 );
            Vec_PtrPush( vSet1, pObj0 );
        }
    }
//    printf( "Miter has %d constant outputs.\n", Counter );
//    printf( "\n" );
    if ( ppAig0 )
    {
        *ppAig0 = Aig_ManDupNodesAll( p, vSet0 );
        ABC_FREE( (*ppAig0)->pName );
        (*ppAig0)->pName = Abc_UtilStrsav( "part0" );
    }
    if ( ppAig1 )
    {
        *ppAig1 = Aig_ManDupNodesAll( p, vSet1 );
        ABC_FREE( (*ppAig1)->pName );
        (*ppAig1)->pName = Abc_UtilStrsav( "part1" );
    }
    Vec_PtrFree( vSet0 );
    Vec_PtrFree( vSet1 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Labels logic reachable from the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManDemiterLabel_rec( Aig_Man_t * p, Aig_Obj_t * pObj, int Value )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Value ) 
        pObj->fMarkB = 1;
    else
        pObj->fMarkA = 1;
    if ( Saig_ObjIsPi(p, pObj) )
        return;
    if ( Saig_ObjIsLo(p, pObj) )
    {
        Saig_ManDemiterLabel_rec( p, Aig_ObjFanin0( Saig_ObjLoToLi(p, pObj) ), Value );
        return;
    }
    assert( Aig_ObjIsNode(pObj) );
    Saig_ManDemiterLabel_rec( p, Aig_ObjFanin0(pObj), Value );
    Saig_ManDemiterLabel_rec( p, Aig_ObjFanin1(pObj), Value );
}

/**Function*************************************************************

  Synopsis    [Returns the first labeled register encountered during traversal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Saig_ManGetLabeledRegister_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pResult;
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return NULL;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Saig_ObjIsPi(p, pObj) )
        return NULL;
    if ( Saig_ObjIsLo(p, pObj) )
    {
        if ( pObj->fMarkA || pObj->fMarkB )
            return pObj;
        return Saig_ManGetLabeledRegister_rec( p, Aig_ObjFanin0( Saig_ObjLoToLi(p, pObj) ) );
    }
    assert( Aig_ObjIsNode(pObj) );
    pResult = Saig_ManGetLabeledRegister_rec( p, Aig_ObjFanin0(pObj) );
    if ( pResult )
        return pResult;
    return Saig_ManGetLabeledRegister_rec( p, Aig_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG to have constant-0 initial state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManDemiter( Aig_Man_t * p, Aig_Man_t ** ppAig0, Aig_Man_t ** ppAig1 )
{
    Vec_Ptr_t * vPairs, * vSet0, * vSet1;
    Aig_Obj_t * pObj, * pObj0, * pObj1, * pFlop0, * pFlop1;
    int i, Counter;
    assert( Saig_ManRegNum(p) > 0 );
    Aig_ManSetCioIds( p );
    // check if demitering is possible
    vPairs = Vec_PtrAlloc( 2 * Saig_ManPoNum(p) );
    Saig_ManForEachPo( p, pObj, i )
    {
        if ( !Aig_ObjRecognizeExor( Aig_ObjFanin0(pObj), &pObj0, &pObj1 ) )
        {
            Vec_PtrFree( vPairs );
            return 0;
        }
        Vec_PtrPush( vPairs, pObj0 );
        Vec_PtrPush( vPairs, pObj1 );
    }
    // start array of outputs
    vSet0 = Vec_PtrAlloc( Saig_ManPoNum(p) );
    vSet1 = Vec_PtrAlloc( Saig_ManPoNum(p) );
    // get the first pair of outputs
    pObj0 = (Aig_Obj_t *)Vec_PtrEntry( vPairs, 0 );
    pObj1 = (Aig_Obj_t *)Vec_PtrEntry( vPairs, 1 );
    // label registers reachable from the outputs
    Aig_ManIncrementTravId( p );
    Saig_ManDemiterLabel_rec( p, Aig_Regular(pObj0), 0 );
    Vec_PtrPush( vSet0, pObj0 );
    Aig_ManIncrementTravId( p );
    Saig_ManDemiterLabel_rec( p, Aig_Regular(pObj1), 1 );
    Vec_PtrPush( vSet1, pObj1 );
    // find where each output belongs
    for ( i = 2; i < Vec_PtrSize(vPairs); i += 2 )
    {
        pObj0 = (Aig_Obj_t *)Vec_PtrEntry( vPairs, i   );
        pObj1 = (Aig_Obj_t *)Vec_PtrEntry( vPairs, i+1 );

        Aig_ManIncrementTravId( p );
        pFlop0 = Saig_ManGetLabeledRegister_rec( p, Aig_Regular(pObj0) );

        Aig_ManIncrementTravId( p );
        pFlop1 = Saig_ManGetLabeledRegister_rec( p, Aig_Regular(pObj1) );

        if ( (pFlop0->fMarkA && pFlop0->fMarkB) || (pFlop1->fMarkA && pFlop1->fMarkB) || 
             (pFlop0->fMarkA && pFlop1->fMarkA) || (pFlop0->fMarkB && pFlop1->fMarkB)  )
            printf( "Ouput pair %4d: Difficult case...\n", i/2 );

        if ( pFlop0->fMarkB )
        {
            Saig_ManDemiterLabel_rec( p, pObj0, 1 );
            Vec_PtrPush( vSet0, pObj0 );
        }
        else // if ( pFlop0->fMarkA ) or none
        {
            Saig_ManDemiterLabel_rec( p, pObj0, 0 );
            Vec_PtrPush( vSet1, pObj0 );
        }

        if ( pFlop1->fMarkB )
        {
            Saig_ManDemiterLabel_rec( p, pObj1, 1 );
            Vec_PtrPush( vSet0, pObj1 );
        }
        else // if ( pFlop1->fMarkA ) or none
        {
            Saig_ManDemiterLabel_rec( p, pObj1, 0 );
            Vec_PtrPush( vSet1, pObj1 );
        }
    }
    // check if there are any flops in common
    Counter = 0;
    Saig_ManForEachLo( p, pObj, i )
        if ( pObj->fMarkA && pObj->fMarkB )
            Counter++;
    if ( Counter > 0 )
        printf( "The miters contains %d flops reachable from both AIGs.\n", Counter );

    // produce two miters
    if ( ppAig0 )
    {
        assert( 0 );
        *ppAig0 = Aig_ManDupNodesHalf( p, vSet0, 0 ); // not ready
        ABC_FREE( (*ppAig0)->pName );
        (*ppAig0)->pName = Abc_UtilStrsav( "part0" );
    }
    if ( ppAig1 )
    {
        assert( 0 );
        *ppAig1 = Aig_ManDupNodesHalf( p, vSet1, 1 ); // not ready
        ABC_FREE( (*ppAig1)->pName );
        (*ppAig1)->pName = Abc_UtilStrsav( "part1" );
    }
    Vec_PtrFree( vSet0 );
    Vec_PtrFree( vSet1 );
    Vec_PtrFree( vPairs );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Create specialized miter by unrolling two circuits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManCreateMiterTwo( Aig_Man_t * pOld, Aig_Man_t * pNew, int nFrames )
{
    Aig_Man_t * pFrames0, * pFrames1, * pMiter;
//    assert( Aig_ManNodeNum(pOld) <= Aig_ManNodeNum(pNew) );
    pFrames0 = Saig_ManUnrollTwo( pOld, pOld, nFrames );
    pFrames1 = Saig_ManUnrollTwo( pNew, pOld, nFrames );
    pMiter = Saig_ManCreateMiterComb( pFrames0, pFrames1, 0 );
    Aig_ManStop( pFrames0 );
    Aig_ManStop( pFrames1 );
    return pMiter;
}


/**Function*************************************************************

  Synopsis    [Resimulates counter-example and returns the failed output number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SecCexResimulate( Aig_Man_t * p, int * pModel, int * pnOutputs )
{
    Aig_Obj_t * pObj;
    int i, RetValue = -1;
    *pnOutputs = 0;
    Aig_ManConst1(p)->fMarkA = 1;
    Aig_ManForEachCi( p, pObj, i )
        pObj->fMarkA = pModel[i];
    Aig_ManForEachNode( p, pObj, i )
        pObj->fMarkA = ( Aig_ObjFanin0(pObj)->fMarkA ^ Aig_ObjFaninC0(pObj) ) & 
                      ( Aig_ObjFanin1(pObj)->fMarkA ^ Aig_ObjFaninC1(pObj) );
    Aig_ManForEachCo( p, pObj, i )
        pObj->fMarkA = Aig_ObjFanin0(pObj)->fMarkA ^ Aig_ObjFaninC0(pObj);
    Aig_ManForEachCo( p, pObj, i )
        if ( pObj->fMarkA )
        {
            if ( RetValue == -1 )
                RetValue = i;
            (*pnOutputs)++;
        }
    Aig_ManCleanMarkA(p);
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Reduces SEC to CEC for the special case of seq synthesis.]

  Description [The first circuit (pPart0) should be circuit before synthesis.
  The second circuit (pPart1) should be circuit after synthesis.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SecSpecial( Aig_Man_t * pPart0, Aig_Man_t * pPart1, int nFrames, int fVerbose )
{
//    extern int Fra_FraigCec( Aig_Man_t ** ppAig, int nConfLimit, int fVerbose );
    int iOut, nOuts;
    Aig_Man_t * pMiterCec;
    int RetValue;
    abctime clkTotal = Abc_Clock();
    if ( fVerbose )
    {
        Aig_ManPrintStats( pPart0 );
        Aig_ManPrintStats( pPart1 );
    }
//    Aig_ManDumpBlif( pPart0, "file0.blif", NULL, NULL );
//    Aig_ManDumpBlif( pPart1, "file1.blif", NULL, NULL );
//    assert( Aig_ManNodeNum(pPart0) <= Aig_ManNodeNum(pPart1) );
/*
    if ( Aig_ManNodeNum(pPart0) >= Aig_ManNodeNum(pPart1) )
    {
        printf( "Warning: The design after synthesis is smaller!\n" );
        printf( "This warning may indicate that the order of designs is changed.\n" );
        printf( "The solver expects the original design as first argument and\n" );
        printf( "the modified design as the second argument in \"absec\".\n" );
    }
*/
    // create two-level miter
    pMiterCec = Saig_ManCreateMiterTwo( pPart0, pPart1, nFrames );
    if ( fVerbose )
    {
        Aig_ManPrintStats( pMiterCec );
//        Aig_ManDumpBlif( pMiterCec, "miter01.blif", NULL, NULL );
//        printf( "The new miter is written into file \"%s\".\n", "miter01.blif" );
    }
    // run CEC on this miter
    RetValue = Fra_FraigCec( &pMiterCec, 100000, fVerbose );
    // transfer model if given
//    if ( pNtk2 == NULL )
//        pNtk1->pModel = pMiterCec->pData, pMiterCec->pData = NULL;
    // report the miter
    if ( RetValue == 1 )
    {
        printf( "Networks are equivalent.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    else if ( RetValue == 0 )
    {
        printf( "Networks are NOT EQUIVALENT.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
        if ( pMiterCec->pData == NULL )
            printf( "Counter-example is not available.\n" );
        else
        {
            iOut = Ssw_SecCexResimulate( pMiterCec, (int *)pMiterCec->pData, &nOuts );
            if ( iOut == -1 )
                printf( "Counter-example verification has failed.\n" );
            else 
            {
                if ( iOut < Saig_ManPoNum(pPart0) * nFrames )
                    printf( "Primary output %d has failed in frame %d.\n", 
                        iOut%Saig_ManPoNum(pPart0), iOut/Saig_ManPoNum(pPart0) );
                else
                    printf( "Flop input %d has failed in the last frame.\n", 
                        iOut - Saig_ManPoNum(pPart0) * nFrames );
                printf( "The counter-example detected %d incorrect POs or flop inputs.\n", nOuts );
            }
        }
    }
    else
    {
        printf( "Networks are UNDECIDED.   " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    fflush( stdout );
    Aig_ManStop( pMiterCec );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Reduces SEC to CEC for the special case of seq synthesis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssw_SecSpecialMiter( Aig_Man_t * p0, Aig_Man_t * p1, int nFrames, int fVerbose )
{
    Aig_Man_t * pPart0, * pPart1;
    int RetValue;
    if ( fVerbose )
        printf( "Performing sequential verification using combinational A/B miter.\n" );
    // consider the case when a miter is given
    if ( p1 == NULL )
    {
        if ( fVerbose )
        {
            Aig_ManPrintStats( p0 );
        }
        // demiter the miter
        if ( !Saig_ManDemiterSimpleDiff( p0, &pPart0, &pPart1 ) )
        {
            printf( "Demitering has failed.\n" );
            return -1;
        }
        if ( Aig_ManRegNum(pPart0) != Aig_ManRegNum(pPart1) )
        {
            Aig_ManStop( pPart0 );
            Aig_ManStop( pPart1 );
            printf( "After demitering AIGs have different number of flops. Quitting.\n" );
            return -1;
        }
    }
    else
    {
        pPart0 = Aig_ManDupSimple( p0 );
        pPart1 = Aig_ManDupSimple( p1 );
    }
    if ( fVerbose )
    {
//        Aig_ManPrintStats( pPart0 );
//        Aig_ManPrintStats( pPart1 );
        if ( p1 == NULL )
        {
//        Aig_ManDumpBlif( pPart0, "part0.blif", NULL, NULL );
//        Aig_ManDumpBlif( pPart1, "part1.blif", NULL, NULL );
//        printf( "The result of demitering is written into files \"%s\" and \"%s\".\n", "part0.blif", "part1.blif" );
        }
    }
    assert( Aig_ManRegNum(pPart0) > 0 );
    assert( Aig_ManRegNum(pPart1) > 0 );
    assert( Saig_ManPiNum(pPart0) == Saig_ManPiNum(pPart1) );
    assert( Saig_ManPoNum(pPart0) == Saig_ManPoNum(pPart1) );
    assert( Aig_ManRegNum(pPart0) == Aig_ManRegNum(pPart1) );
    RetValue = Ssw_SecSpecial( pPart0, pPart1, nFrames, fVerbose );
    if ( RetValue != 1 && Aig_ManNodeNum(pPart0) >= Aig_ManNodeNum(pPart1) )
        RetValue = Ssw_SecSpecial( pPart1, pPart0, nFrames, fVerbose );
    Aig_ManStop( pPart0 );
    Aig_ManStop( pPart1 );
    return RetValue;
}



/**Function*************************************************************

  Synopsis    [Performs demitering of the network.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManDemiterNew( Aig_Man_t * pMan )
{
    Vec_Ptr_t * vSuper, * vSupp0, * vSupp1;
    Aig_Obj_t * pObj, * pTemp, * pFan0, * pFan1;
    int i, k;
    vSuper = Vec_PtrAlloc( 100 );
    Saig_ManForEachPo( pMan, pObj, i )
    {
        if ( pMan->nConstrs && i >= Saig_ManPoNum(pMan) - pMan->nConstrs )
            break;
        printf( "Output %3d : ", i );
        if ( Aig_ObjIsConst1(Aig_ObjFanin0(pObj)) )
        {
            if ( !Aig_ObjFaninC0(pObj) )
                printf( "Const1\n" );
            else
                printf( "Const0\n" );
            continue;
        }
        if ( !Aig_ObjIsNode(Aig_ObjFanin0(pObj)) )
        {
            printf( "Terminal\n" );
            continue;
        }
        // check AND
        if ( !Aig_ObjFaninC0(pObj) )
        { 
            printf( "AND  " );
            if ( Aig_ObjRecognizeExor(Aig_ObjFanin0(pObj), &pFan0, &pFan1) )
                printf( " Yes" );
            else
                printf( " No" );
            printf( "\n" );
            continue;
        }
        // check OR
        Aig_ObjCollectSuper( Aig_ObjFanin0(pObj), vSuper );
        printf( "OR with %d inputs    ", Vec_PtrSize(vSuper) );
        if ( Vec_PtrSize(vSuper) == 2 )
        {
            if ( Aig_ObjRecognizeExor(Aig_ObjFanin0(pObj), &pFan0, &pFan1) )
            {
                printf( " Yes" );
                printf( "\n" );

                vSupp0 = Aig_Support( pMan, Aig_Regular(pFan0) );
                Vec_PtrForEachEntry( Aig_Obj_t *, vSupp0, pTemp, k )
                    if ( Saig_ObjIsLo(pMan, pTemp) )
                        printf( " %d", Aig_ObjCioId(pTemp) );
                printf( "\n" );
                Vec_PtrFree( vSupp0 );

                vSupp1 = Aig_Support( pMan, Aig_Regular(pFan1) );
                Vec_PtrForEachEntry( Aig_Obj_t *, vSupp1, pTemp, k )
                    if ( Saig_ObjIsLo(pMan, pTemp) )
                        printf( " %d", Aig_ObjCioId(pTemp) );
                printf( "\n" );
                Vec_PtrFree( vSupp1 );
            }
            else
                printf( " No" );
            printf( "\n" );
            continue;
        }
/*
        Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pTemp, k )
            if ( Aig_ObjRecognizeExor(Aig_Regular(pTemp), &pFan0, &pFan1) )
            {
                printf( " Yes" );
                if ( Aig_IsComplement(pTemp) )
                    pFan0 = Aig_Not(pFan0);
            }
            else
                printf( " No" );
*/
        printf( "\n" );
    }
    Vec_PtrFree( vSuper );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

