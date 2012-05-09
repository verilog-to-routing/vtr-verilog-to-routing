/**CFile****************************************************************

  FileName    [abcUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Various utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcUtil.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "main.h"
#include "mio.h"
#include "dec.h"
//#include "seq.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Frees one attribute manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abc_NtkAttrFree( Abc_Ntk_t * pNtk, int Attr, int fFreeMan ) 
{  
    void * pUserMan;
    Vec_Att_t * pAttrMan;
    pAttrMan = Vec_PtrEntry( pNtk->vAttrs, Attr );
    Vec_PtrWriteEntry( pNtk->vAttrs, Attr, NULL );
    pUserMan = Vec_AttFree( pAttrMan, fFreeMan );
    return pUserMan;
}

/**Function*************************************************************

  Synopsis    [Increments the current traversal ID of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkIncrementTravId( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    if ( pNtk->nTravIds >= (1<<30)-1 )
    {
        pNtk->nTravIds = 0;
        Abc_NtkForEachObj( pNtk, pObj, i )
            pObj->TravId = 0;
    }
    pNtk->nTravIds++;
}

/**Function*************************************************************

  Synopsis    [Order CI/COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkOrderCisCos( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pTerm;
    int i, k;
    Vec_PtrClear( pNtk->vCis );
    Vec_PtrClear( pNtk->vCos );
    Abc_NtkForEachPi( pNtk, pObj, i )
        Vec_PtrPush( pNtk->vCis, pObj );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Vec_PtrPush( pNtk->vCos, pObj );
    Abc_NtkForEachAssert( pNtk, pObj, i )
        Vec_PtrPush( pNtk->vCos, pObj );
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        if ( Abc_ObjIsLatch(pObj) )
            continue;
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Vec_PtrPush( pNtk->vCos, pTerm );
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Vec_PtrPush( pNtk->vCis, pTerm );
    }
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        if ( !Abc_ObjIsLatch(pObj) )
            continue;
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Vec_PtrPush( pNtk->vCos, pTerm );
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Vec_PtrPush( pNtk->vCis, pTerm );
    }
}

/**Function*************************************************************

  Synopsis    [Reads the number of cubes of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetCubeNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, nCubes = 0;
    assert( Abc_NtkHasSop(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( Abc_NodeIsConst(pNode) )
            continue;
        assert( pNode->pData );
        nCubes += Abc_SopGetCubeNum( pNode->pData );
    }
    return nCubes;
}

/**Function*************************************************************

  Synopsis    [Reads the number of literals in the SOPs of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetLitNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, nLits = 0;
    assert( Abc_NtkHasSop(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        assert( pNode->pData );
        nLits += Abc_SopGetLitNum( pNode->pData );
    }
    return nLits;
}

/**Function*************************************************************

  Synopsis    [Counts the number of literals in the factored forms.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetLitFactNum( Abc_Ntk_t * pNtk )
{
    Dec_Graph_t * pFactor;
    Abc_Obj_t * pNode;
    int nNodes, i;
    assert( Abc_NtkHasSop(pNtk) );
    nNodes = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( Abc_NodeIsConst(pNode) )
            continue;
        pFactor = Dec_Factor( pNode->pData );
        nNodes += 1 + Dec_GraphNodeNum(pFactor);
        Dec_GraphFree( pFactor );
    }
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Reads the number of BDD nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetBddNodeNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, nNodes = 0;
    assert( Abc_NtkIsBddLogic(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        assert( pNode->pData );
        if ( Abc_ObjFaninNum(pNode) < 2 )
            continue;
        nNodes += pNode->pData? -1 + Cudd_DagSize( pNode->pData ) : 0;
    }
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Reads the number of BDD nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetAigNodeNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, nNodes = 0;
    assert( Abc_NtkIsAigLogic(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        assert( pNode->pData );
        if ( Abc_ObjFaninNum(pNode) < 2 )
            continue;
//printf( "%d ", Hop_DagSize( pNode->pData ) );
        nNodes += pNode->pData? Hop_DagSize( pNode->pData ) : 0;
    }
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Reads the number of BDD nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetClauseNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    DdNode * bCover, * zCover, * bFunc;
    DdManager * dd = pNtk->pManFunc;
    int i, nClauses = 0;
    assert( Abc_NtkIsBddLogic(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        assert( pNode->pData );
        bFunc = pNode->pData;

        bCover = Cudd_zddIsop( dd, bFunc, bFunc, &zCover );  
        Cudd_Ref( bCover );
        Cudd_Ref( zCover );
        nClauses += Abc_CountZddCubes( dd, zCover );
        Cudd_RecursiveDeref( dd, bCover );
        Cudd_RecursiveDerefZdd( dd, zCover );

        bCover = Cudd_zddIsop( dd, Cudd_Not(bFunc), Cudd_Not(bFunc), &zCover );  
        Cudd_Ref( bCover );
        Cudd_Ref( zCover );
        nClauses += Abc_CountZddCubes( dd, zCover );
        Cudd_RecursiveDeref( dd, bCover );
        Cudd_RecursiveDerefZdd( dd, zCover );
    }
    return nClauses;
}

/**Function*************************************************************

  Synopsis    [Computes the area of the mapped circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Abc_NtkGetMappedArea( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    double TotalArea;
    int i;
    assert( Abc_NtkHasMapping(pNtk) );
    TotalArea = 0.0;
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
//        assert( pNode->pData );
        if ( pNode->pData == NULL )
        {
            printf( "Node without mapping is encountered.\n" );
            continue;
        }
        TotalArea += Mio_GateReadArea( pNode->pData );
    }
    return TotalArea;
}

/**Function*************************************************************

  Synopsis    [Counts the number of exors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetExorNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        Counter += pNode->fExor;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of exors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetMuxNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        Counter += Abc_NodeIsMuxType(pNode);
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if it is an AIG with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetChoiceNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, Counter;
    if ( !Abc_NtkIsStrash(pNtk) )
        return 0;
    Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        Counter += Abc_AigNodeIsChoice( pNode );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Reads the maximum number of fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetFaninMax( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, nFaninsMax = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( nFaninsMax < Abc_ObjFaninNum(pNode) )
            nFaninsMax = Abc_ObjFaninNum(pNode);
    }
    return nFaninsMax;
}

/**Function*************************************************************

  Synopsis    [Reads the total number of all fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetTotalFanins( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, nFanins = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        nFanins += Abc_ObjFaninNum(pNode);
    return nFanins;
}

/**Function*************************************************************

  Synopsis    [Cleans the copy field of all objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCleanCopy( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->pCopy = NULL;
}

/**Function*************************************************************

  Synopsis    [Cleans the copy field of all objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCleanData( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->pData = NULL;
}

/**Function*************************************************************

  Synopsis    [Cleans the copy field of all objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCleanEquiv( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->pEquiv = NULL;
}

/**Function*************************************************************

  Synopsis    [Counts the number of nodes having non-trivial copies.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCountCopy( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i, Counter = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( Abc_ObjIsNode(pObj) )
            Counter += (pObj->pCopy != NULL);
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Saves copy field of the objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkSaveCopy( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vCopies;
    Abc_Obj_t * pObj;
    int i;
    vCopies = Vec_PtrStart( Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachObj( pNtk, pObj, i )
        Vec_PtrWriteEntry( vCopies, i, pObj->pCopy );
    return vCopies;
}

/**Function*************************************************************

  Synopsis    [Loads copy field of the objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkLoadCopy( Abc_Ntk_t * pNtk, Vec_Ptr_t * vCopies )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->pCopy = Vec_PtrEntry( vCopies, i );
}

/**Function*************************************************************

  Synopsis    [Cleans the copy field of all objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCleanNext( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->pNext = NULL;
}

/**Function*************************************************************

  Synopsis    [Cleans the copy field of all objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCleanMarkA( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->fMarkA = 0;
}

/**Function*************************************************************

  Synopsis    [Checks if the internal node has CO fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeFindCoFanout( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanout;
    int i;
    Abc_ObjForEachFanout( pNode, pFanout, i )
        if ( Abc_ObjIsCo(pFanout) )
            return pFanout;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Checks if the internal node has CO fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeFindNonCoFanout( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanout;
    int i;
    Abc_ObjForEachFanout( pNode, pFanout, i )
        if ( !Abc_ObjIsCo(pFanout) )
            return pFanout;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Checks if the internal node has CO drivers with the same name.]

  Description [Checks if the internal node can borrow its name from CO fanouts. 
  This is possible if all COs with non-complemented fanin edge pointing to this 
  node have the same name.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeHasUniqueCoFanout( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanout, * pFanoutCo;
    int i;
    pFanoutCo = NULL;
    Abc_ObjForEachFanout( pNode, pFanout, i )
    {
        if ( !Abc_ObjIsCo(pFanout) )
            continue;
        if ( Abc_ObjFaninC0(pFanout) )
            continue;
        if ( pFanoutCo == NULL )
        {
            assert( Abc_ObjFaninNum(pFanout) == 1 );
            assert( Abc_ObjFanin0(pFanout) == pNode );
            pFanoutCo = pFanout;
            continue;
        }
        if ( strcmp( Abc_ObjName(pFanoutCo), Abc_ObjName(pFanout) ) ) // they have diff names
            return NULL;
    }
    return pFanoutCo;
}

/**Function*************************************************************

  Synopsis    [Fixes the CO driver problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFixCoDriverProblem( Abc_Obj_t * pDriver, Abc_Obj_t * pNodeCo, int fDuplicate )
{
    Abc_Ntk_t * pNtk = pDriver->pNtk;
    Abc_Obj_t * pDriverNew, * pFanin;
    int k;
    if ( fDuplicate && !Abc_ObjIsCi(pDriver) )
    {
        pDriverNew = Abc_NtkDupObj( pNtk, pDriver, 0 ); 
        Abc_ObjForEachFanin( pDriver, pFanin, k )
            Abc_ObjAddFanin( pDriverNew, pFanin );
        if ( Abc_ObjFaninC0(pNodeCo) )
        {
            // change polarity of the duplicated driver
            Abc_NodeComplement( pDriverNew );
            Abc_ObjXorFaninC( pNodeCo, 0 );
        }
    }
    else
    {
        // add inverters and buffers when necessary
        if ( Abc_ObjFaninC0(pNodeCo) )
        {
            pDriverNew = Abc_NtkCreateNodeInv( pNtk, pDriver );
            Abc_ObjXorFaninC( pNodeCo, 0 );
        }
        else
            pDriverNew = Abc_NtkCreateNodeBuf( pNtk, pDriver );        
    }
    // update the fanin of the PO node
    Abc_ObjPatchFanin( pNodeCo, pDriver, pDriverNew );
    assert( Abc_ObjFanoutNum(pDriverNew) == 1 );
    // remove the old driver if it dangles
    // (this happens when the duplicated driver had only one complemented fanout)
    if ( Abc_ObjFanoutNum(pDriver) == 0 )
        Abc_NtkDeleteObj( pDriver );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if COs of a logic network are simple.]

  Description [The COs of a logic network are simple under three conditions:
  (1) The edge from CO to its driver is not complemented.
  (2) If CI is a driver of a CO, they have the same name.]
  (3) If two COs share the same driver, they have the same name.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NtkLogicHasSimpleCos( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode, * pDriver;
    int i;
    assert( Abc_NtkIsLogic(pNtk) );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachCo( pNtk, pNode, i ) 
    {
        // if the driver is complemented, this is an error
        pDriver = Abc_ObjFanin0(pNode);
        if ( Abc_ObjFaninC0(pNode) )
            return 0;
        // if the driver is a CI and has different name, this is an error
        if ( Abc_ObjIsCi(pDriver) && strcmp(Abc_ObjName(pDriver), Abc_ObjName(pNode)) )
            return 0;
        // if the driver is visited for the first time, remember the CO name
        if ( !Abc_NodeIsTravIdCurrent(pDriver) )
        {
            pDriver->pNext = (Abc_Obj_t *)Abc_ObjName(pNode);
            Abc_NodeSetTravIdCurrent(pDriver);
            continue;
        }
        // the driver has second CO - if they have different name, this is an error
        if ( strcmp((char *)pDriver->pNext, Abc_ObjName(pNode)) ) // diff names
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Transforms the network to have simple COs.]

  Description [The COs of a logic network are simple under three conditions:
  (1) The edge from CO to its driver is not complemented.
  (2) If CI is a driver of a CO, they have the same name.]
  (3) If two COs share the same driver, they have the same name.
  In some cases, such as FPGA mapping, we prevent the increase in delay
  by duplicating the driver nodes, rather than adding invs/bufs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkLogicMakeSimpleCos( Abc_Ntk_t * pNtk, bool fDuplicate )
{
    Abc_Obj_t * pNode, * pDriver;
    int i, nDupGates = 0;
    assert( Abc_NtkIsLogic(pNtk) );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachCo( pNtk, pNode, i ) 
    {
        // if the driver is complemented, this is an error
        pDriver = Abc_ObjFanin0(pNode);
        if ( Abc_ObjFaninC0(pNode) )
        {
            Abc_NtkFixCoDriverProblem( pDriver, pNode, fDuplicate );
            nDupGates++;
            continue;
        }
        // if the driver is a CI and has different name, this is an error
        if ( Abc_ObjIsCi(pDriver) && strcmp(Abc_ObjName(pDriver), Abc_ObjName(pNode)) )
        {
            Abc_NtkFixCoDriverProblem( pDriver, pNode, fDuplicate );
            nDupGates++;
            continue;
        }
        // if the driver is visited for the first time, remember the CO name
        if ( !Abc_NodeIsTravIdCurrent(pDriver) )
        {
            pDriver->pNext = (Abc_Obj_t *)Abc_ObjName(pNode);
            Abc_NodeSetTravIdCurrent(pDriver);
            continue;
        }
        // the driver has second CO - if they have different name, this is an error
        if ( strcmp((char *)pDriver->pNext, Abc_ObjName(pNode)) ) // diff names
        {
            Abc_NtkFixCoDriverProblem( pDriver, pNode, fDuplicate );
            nDupGates++;
            continue;
        }
    }
    assert( Abc_NtkLogicHasSimpleCos(pNtk) );
    return nDupGates;
}

/**Function*************************************************************

  Synopsis    [Inserts a new node in the order by levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_VecObjPushUniqueOrderByLevel( Vec_Ptr_t * p, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNode1, * pNode2;
    int i;
    if ( Vec_PtrPushUnique(p, pNode) )
        return;
    // find the p of the node
    for ( i = p->nSize-1; i > 0; i-- )
    {
        pNode1 = p->pArray[i  ];
        pNode2 = p->pArray[i-1];
        if ( Abc_ObjRegular(pNode1)->Level <= Abc_ObjRegular(pNode2)->Level )
            break;
        p->pArray[i  ] = pNode2;
        p->pArray[i-1] = pNode1;
    }
}



/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the root of EXOR/NEXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NodeIsExorType( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNode0, * pNode1;
    // check that the node is regular
    assert( !Abc_ObjIsComplement(pNode) );
    // if the node is not AND, this is not EXOR
    if ( !Abc_AigNodeIsAnd(pNode) )
        return 0;
    // if the children are not complemented, this is not EXOR
    if ( !Abc_ObjFaninC0(pNode) || !Abc_ObjFaninC1(pNode) )
        return 0;
    // get children
    pNode0 = Abc_ObjFanin0(pNode);
    pNode1 = Abc_ObjFanin1(pNode);
    // if the children are not ANDs, this is not EXOR
    if ( Abc_ObjFaninNum(pNode0) != 2 || Abc_ObjFaninNum(pNode1) != 2 )
        return 0;
    // otherwise, the node is EXOR iff its grand-children are the same
    return (Abc_ObjFaninId0(pNode0) == Abc_ObjFaninId0(pNode1) || Abc_ObjFaninId0(pNode0) == Abc_ObjFaninId1(pNode1)) &&
           (Abc_ObjFaninId1(pNode0) == Abc_ObjFaninId0(pNode1) || Abc_ObjFaninId1(pNode0) == Abc_ObjFaninId1(pNode1));
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the root of MUX or EXOR/NEXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NodeIsMuxType( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNode0, * pNode1;
    // check that the node is regular
    assert( !Abc_ObjIsComplement(pNode) );
    // if the node is not AND, this is not MUX
    if ( !Abc_AigNodeIsAnd(pNode) )
        return 0;
    // if the children are not complemented, this is not MUX
    if ( !Abc_ObjFaninC0(pNode) || !Abc_ObjFaninC1(pNode) )
        return 0;
    // get children
    pNode0 = Abc_ObjFanin0(pNode);
    pNode1 = Abc_ObjFanin1(pNode);
    // if the children are not ANDs, this is not MUX
    if ( Abc_ObjFaninNum(pNode0) != 2 || Abc_ObjFaninNum(pNode1) != 2 )
        return 0;
    // otherwise the node is MUX iff it has a pair of equal grandchildren
    return (Abc_ObjFaninId0(pNode0) == Abc_ObjFaninId0(pNode1) && (Abc_ObjFaninC0(pNode0) ^ Abc_ObjFaninC0(pNode1))) || 
           (Abc_ObjFaninId0(pNode0) == Abc_ObjFaninId1(pNode1) && (Abc_ObjFaninC0(pNode0) ^ Abc_ObjFaninC1(pNode1))) ||
           (Abc_ObjFaninId1(pNode0) == Abc_ObjFaninId0(pNode1) && (Abc_ObjFaninC1(pNode0) ^ Abc_ObjFaninC0(pNode1))) ||
           (Abc_ObjFaninId1(pNode0) == Abc_ObjFaninId1(pNode1) && (Abc_ObjFaninC1(pNode0) ^ Abc_ObjFaninC1(pNode1)));
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the control type of the MUX.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Abc_NodeIsMuxControlType( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pNode0, * pNode1;
    // check that the node is regular
    assert( !Abc_ObjIsComplement(pNode) );
    // skip the node that do not have two fanouts
    if ( Abc_ObjFanoutNum(pNode) != 2 )
        return 0;
    // get the fanouts
    pNode0 = Abc_ObjFanout( pNode, 0 );
    pNode1 = Abc_ObjFanout( pNode, 1 );
    // if they have more than one fanout, we are not interested
    if ( Abc_ObjFanoutNum(pNode0) != 1 ||  Abc_ObjFanoutNum(pNode1) != 1 )
        return 0;
    // if the fanouts have the same fanout, this is MUX or EXOR (or a redundant gate (CA)(CB))
    return Abc_ObjFanout0(pNode0) == Abc_ObjFanout0(pNode1);
}

/**Function*************************************************************

  Synopsis    [Recognizes what nodes are control and data inputs of a MUX.]

  Description [If the node is a MUX, returns the control variable C.
  Assigns nodes T and E to be the then and else variables of the MUX. 
  Node C is never complemented. Nodes T and E can be complemented.
  This function also recognizes EXOR/NEXOR gates as MUXes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeRecognizeMux( Abc_Obj_t * pNode, Abc_Obj_t ** ppNodeT, Abc_Obj_t ** ppNodeE )
{
    Abc_Obj_t * pNode0, * pNode1;
    assert( !Abc_ObjIsComplement(pNode) );
    assert( Abc_NodeIsMuxType(pNode) );
    // get children
    pNode0 = Abc_ObjFanin0(pNode);
    pNode1 = Abc_ObjFanin1(pNode);
    // find the control variable
//    if ( pNode1->p1 == Fraig_Not(pNode2->p1) )
    if ( Abc_ObjFaninId0(pNode0) == Abc_ObjFaninId0(pNode1) && (Abc_ObjFaninC0(pNode0) ^ Abc_ObjFaninC0(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p1) )
        if ( Abc_ObjFaninC0(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Abc_ObjNot(Abc_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Abc_ObjNot(Abc_ObjChild1(pNode0));//pNode1->p2);
            return Abc_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Abc_ObjNot(Abc_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Abc_ObjNot(Abc_ObjChild1(pNode1));//pNode2->p2);
            return Abc_ObjChild0(pNode0);//pNode1->p1;
        }
    }
//    else if ( pNode1->p1 == Fraig_Not(pNode2->p2) )
    else if ( Abc_ObjFaninId0(pNode0) == Abc_ObjFaninId1(pNode1) && (Abc_ObjFaninC0(pNode0) ^ Abc_ObjFaninC1(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p1) )
        if ( Abc_ObjFaninC0(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Abc_ObjNot(Abc_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Abc_ObjNot(Abc_ObjChild1(pNode0));//pNode1->p2);
            return Abc_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Abc_ObjNot(Abc_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Abc_ObjNot(Abc_ObjChild0(pNode1));//pNode2->p1);
            return Abc_ObjChild0(pNode0);//pNode1->p1;
        }
    }
//    else if ( pNode1->p2 == Fraig_Not(pNode2->p1) )
    else if ( Abc_ObjFaninId1(pNode0) == Abc_ObjFaninId0(pNode1) && (Abc_ObjFaninC1(pNode0) ^ Abc_ObjFaninC0(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p2) )
        if ( Abc_ObjFaninC1(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Abc_ObjNot(Abc_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Abc_ObjNot(Abc_ObjChild0(pNode0));//pNode1->p1);
            return Abc_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Abc_ObjNot(Abc_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Abc_ObjNot(Abc_ObjChild1(pNode1));//pNode2->p2);
            return Abc_ObjChild1(pNode0);//pNode1->p2;
        }
    }
//    else if ( pNode1->p2 == Fraig_Not(pNode2->p2) )
    else if ( Abc_ObjFaninId1(pNode0) == Abc_ObjFaninId1(pNode1) && (Abc_ObjFaninC1(pNode0) ^ Abc_ObjFaninC1(pNode1)) )
    {
//        if ( Fraig_IsComplement(pNode1->p2) )
        if ( Abc_ObjFaninC1(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Abc_ObjNot(Abc_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Abc_ObjNot(Abc_ObjChild0(pNode0));//pNode1->p1);
            return Abc_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Abc_ObjNot(Abc_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Abc_ObjNot(Abc_ObjChild0(pNode1));//pNode2->p1);
            return Abc_ObjChild1(pNode0);//pNode1->p2;
        }
    }
    assert( 0 ); // this is not MUX
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Prepares two network for a two-argument command similar to "verify".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkPrepareTwoNtks( FILE * pErr, Abc_Ntk_t * pNtk, char ** argv, int argc, 
    Abc_Ntk_t ** ppNtk1, Abc_Ntk_t ** ppNtk2, int * pfDelete1, int * pfDelete2 )
{
    int fCheck = 1;
    FILE * pFile;
    Abc_Ntk_t * pNtk1, * pNtk2;
    int util_optind = 0;

    *pfDelete1 = 0;
    *pfDelete2 = 0;
    if ( argc == util_optind ) 
    { // use the spec
        if ( pNtk == NULL )
        {
            fprintf( pErr, "Empty current network.\n" );
            return 0;
        }
        if ( pNtk->pSpec == NULL )
        {
            fprintf( pErr, "The external spec is not given.\n" );
            return 0;
        }
        pFile = fopen( pNtk->pSpec, "r" );
        if ( pFile == NULL )
        {
            fprintf( pErr, "Cannot open the external spec file \"%s\".\n", pNtk->pSpec );
            return 0;
        }
        else
            fclose( pFile );
        pNtk1 = pNtk;
        pNtk2 = Io_Read( pNtk->pSpec, Io_ReadFileType(pNtk->pSpec), fCheck );
        if ( pNtk2 == NULL )
            return 0;
        *pfDelete2 = 1;
    }
    else if ( argc == util_optind + 1 ) 
    {
        if ( pNtk == NULL )
        {
            fprintf( pErr, "Empty current network.\n" );
            return 0;
        }
        pNtk1 = pNtk;
        pNtk2 = Io_Read( argv[util_optind], Io_ReadFileType(argv[util_optind]), fCheck );
        if ( pNtk2 == NULL )
            return 0;
        *pfDelete2 = 1;
    }
    else if ( argc == util_optind + 2 ) 
    {
        pNtk1 = Io_Read( argv[util_optind], Io_ReadFileType(argv[util_optind]), fCheck );
        if ( pNtk1 == NULL )
            return 0;
        pNtk2 = Io_Read( argv[util_optind+1], Io_ReadFileType(argv[util_optind+1]), fCheck );
        if ( pNtk2 == NULL )
        {
            Abc_NtkDelete( pNtk1 );
            return 0;
        }
        *pfDelete1 = 1;
        *pfDelete2 = 1;
    }
    else
    {
        fprintf( pErr, "Wrong number of arguments.\n" );
        return 0;
    }
    *ppNtk1 = pNtk1;
    *ppNtk2 = pNtk2;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if it is an AIG with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeCollectFanins( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanin;
    int i;
    Vec_PtrClear(vNodes);
    Abc_ObjForEachFanin( pNode, pFanin, i )
        Vec_PtrPush( vNodes, pFanin );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if it is an AIG with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeCollectFanouts( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanout;
    int i;
    Vec_PtrClear(vNodes);
    Abc_ObjForEachFanout( pNode, pFanout, i )
        Vec_PtrPush( vNodes, pFanout );
}

/**Function*************************************************************

  Synopsis    [Collects all latches in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkCollectLatches( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vLatches;
    Abc_Obj_t * pObj;
    int i;
    vLatches = Vec_PtrAlloc( 10 );
    Abc_NtkForEachObj( pNtk, pObj, i )
        Vec_PtrPush( vLatches, pObj );
    return vLatches;
}

/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in increasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCompareLevelsIncrease( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 )
{
    int Diff = Abc_ObjRegular(*pp1)->Level - Abc_ObjRegular(*pp2)->Level;
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in decreasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCompareLevelsDecrease( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 )
{
    int Diff = Abc_ObjRegular(*pp1)->Level - Abc_ObjRegular(*pp2)->Level;
    if ( Diff > 0 )
        return -1;
    if ( Diff < 0 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Creates the array of fanout counters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkFanoutCounts( Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vFanNums;
    Abc_Obj_t * pObj;
    int i;
    vFanNums = Vec_IntAlloc( 0 );
    Vec_IntFill( vFanNums, Abc_NtkObjNumMax(pNtk), -1 );
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( Abc_ObjIsCi(pObj) || Abc_ObjIsNode(pObj) )
            Vec_IntWriteEntry( vFanNums, i, Abc_ObjFanoutNum(pObj) );
    return vFanNums;
}

/**Function*************************************************************

  Synopsis    [Collects all objects into one array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkCollectObjects( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNode;
    int i;
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NtkForEachObj( pNtk, pNode, i )
        Vec_PtrPush( vNodes, pNode );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Returns the array of CI IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkGetCiIds( Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vCiIds;
    Abc_Obj_t * pObj;
    int i;
    vCiIds = Vec_IntAlloc( Abc_NtkCiNum(pNtk) );
    Abc_NtkForEachCi( pNtk, pObj, i )
        Vec_IntPush( vCiIds, pObj->Id );
    return vCiIds;
}

/**Function*************************************************************

  Synopsis    [Puts the nodes into the DFS order and reassign their IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkReassignIds( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Vec_Ptr_t * vObjsNew;
    Abc_Obj_t * pNode, * pTemp, * pConst1;
    int i, k;
    assert( Abc_NtkIsStrash(pNtk) );
//printf( "Total = %d. Current = %d.\n", Abc_NtkObjNumMax(pNtk), Abc_NtkObjNum(pNtk) );
    // start the array of objects with new IDs
    vObjsNew = Vec_PtrAlloc( pNtk->nObjs );
    // put constant node first
    pConst1 = Abc_AigConst1(pNtk);
    assert( pConst1->Id == 0 );
    Vec_PtrPush( vObjsNew, pConst1 );
    // put PI nodes next
    Abc_NtkForEachPi( pNtk, pNode, i )
    {
        pNode->Id = Vec_PtrSize( vObjsNew );
        Vec_PtrPush( vObjsNew, pNode );
    }
    // put PO nodes next
    Abc_NtkForEachPo( pNtk, pNode, i )
    {
        pNode->Id = Vec_PtrSize( vObjsNew );
        Vec_PtrPush( vObjsNew, pNode );
    }
    // put assert nodes next
    Abc_NtkForEachAssert( pNtk, pNode, i )
    {
        pNode->Id = Vec_PtrSize( vObjsNew );
        Vec_PtrPush( vObjsNew, pNode );
    }
    // put latches and their inputs/outputs next
    Abc_NtkForEachBox( pNtk, pNode, i )
    {
        pNode->Id = Vec_PtrSize( vObjsNew );
        Vec_PtrPush( vObjsNew, pNode );
        Abc_ObjForEachFanin( pNode, pTemp, k )
        {
            pTemp->Id = Vec_PtrSize( vObjsNew );
            Vec_PtrPush( vObjsNew, pTemp );
        }
        Abc_ObjForEachFanout( pNode, pTemp, k )
        {
            pTemp->Id = Vec_PtrSize( vObjsNew );
            Vec_PtrPush( vObjsNew, pTemp );
        }
    }
    // finally, internal nodes in the DFS order
    vNodes = Abc_AigDfs( pNtk, 1, 0 );
    Vec_PtrForEachEntry( vNodes, pNode, i )
    {
        if ( pNode == pConst1 )
            continue;
        pNode->Id = Vec_PtrSize( vObjsNew );
        Vec_PtrPush( vObjsNew, pNode );
    }
    Vec_PtrFree( vNodes );
    assert( Vec_PtrSize(vObjsNew) == pNtk->nObjs );

    // update the fanin/fanout arrays
    Abc_NtkForEachObj( pNtk, pNode, i )
    {
        Abc_ObjForEachFanin( pNode, pTemp, k )
            pNode->vFanins.pArray[k] = pTemp->Id;
        Abc_ObjForEachFanout( pNode, pTemp, k )
            pNode->vFanouts.pArray[k] = pTemp->Id;
    }

    // replace the array of objs
    Vec_PtrFree( pNtk->vObjs );
    pNtk->vObjs = vObjsNew;

    // rehash the AIG
    Abc_AigRehash( pNtk->pManFunc );

    // update the name manager!!!
}

/**Function*************************************************************

  Synopsis    [Detect cases when non-trivial FF matching is possible.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDetectMatching( Abc_Ntk_t * pNtk )
{
/*
    Abc_Obj_t * pLatch, * pFanin;
    int i, nTFFs, nJKFFs;
    nTFFs = nJKFFs = 0;
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        pFanin = Abc_ObjFanin0(pLatch);
        if ( Abc_ObjFaninNum(pFanin) != 2 )
            continue;
        if ( Abc_NodeIsExorType(pLatch) )
        {
            if ( Abc_ObjFanin0(Abc_ObjFanin0(pFanin)) == pLatch ||
                 Abc_ObjFanin1(Abc_ObjFanin0(pFanin)) == pLatch )
                 nTFFs++;
        }
        if ( Abc_ObjFaninNum( Abc_ObjFanin0(pFanin) ) != 2 || 
             Abc_ObjFaninNum( Abc_ObjFanin1(pFanin) ) != 2 )
            continue;

        if ( (Abc_ObjFanin0(Abc_ObjFanin0(pFanin)) == pLatch ||
              Abc_ObjFanin1(Abc_ObjFanin0(pFanin)) == pLatch) && 
             (Abc_ObjFanin0(Abc_ObjFanin1(pFanin)) == pLatch ||
              Abc_ObjFanin1(Abc_ObjFanin1(pFanin)) == pLatch) )
        {
            nJKFFs++;
        }
    }
    printf( "D = %6d.   T = %6d.   JK = %6d.  (%6.2f %%)\n", 
        Abc_NtkLatchNum(pNtk), nTFFs, nJKFFs, 100.0 * nJKFFs / Abc_NtkLatchNum(pNtk) );
*/
}


/**Function*************************************************************

  Synopsis    [Compares the pointers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjPointerCompare( void ** pp1, void ** pp2 )
{
    if ( *pp1 < *pp2 )
        return -1;
    if ( *pp1 > *pp2 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Adjusts the copy pointers.]

  Description [This procedure assumes that the network was transformed
  into another network, which was in turn transformed into yet another
  network. It makes the pCopy pointers of the original network point to
  the objects of the yet another network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTransferCopy( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsNet(pObj) )
            pObj->pCopy = pObj->pCopy? Abc_ObjCopyCond(pObj->pCopy) : NULL;
}


/**Function*************************************************************

  Synopsis    [Increaments the cut counter.]

  Description [Returns 1 if it becomes equal to the ref counter.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_ObjCrossCutInc( Abc_Obj_t * pObj )
{
//    pObj->pCopy = (void *)(((int)pObj->pCopy)++);
    int Value = (int)pObj->pCopy;
    pObj->pCopy = (void *)(Value + 1);
    return (int)pObj->pCopy == Abc_ObjFanoutNum(pObj);
}

/**Function*************************************************************

  Synopsis    [Computes cross-cut of the circuit.]

  Description [Returns 1 if it is the last visit to the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCrossCut_rec( Abc_Obj_t * pObj, int * pnCutSize, int * pnCutSizeMax )
{
    Abc_Obj_t * pFanin;
    int i, nDecrem = 0;
    int fReverse = 0;
    if ( Abc_ObjIsCi(pObj) )
        return 0;
    // if visited, increment visit counter 
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return Abc_ObjCrossCutInc( pObj );
    Abc_NodeSetTravIdCurrent( pObj );
    // visit the fanins
    if ( !Abc_ObjIsCi(pObj) )
    {
        if ( fReverse )
        {
            Abc_ObjForEachFanin( pObj, pFanin, i )
            {
                pFanin = Abc_ObjFanin( pObj, Abc_ObjFaninNum(pObj) - 1 - i );
                nDecrem += Abc_NtkCrossCut_rec( pFanin, pnCutSize, pnCutSizeMax );
            }
        }
        else
        {
            Abc_ObjForEachFanin( pObj, pFanin, i )
                nDecrem += Abc_NtkCrossCut_rec( pFanin, pnCutSize, pnCutSizeMax );
        }
    }
    // count the node
    (*pnCutSize)++;
    if ( *pnCutSizeMax < *pnCutSize )
        *pnCutSizeMax = *pnCutSize;
    (*pnCutSize) -= nDecrem;
    return Abc_ObjCrossCutInc( pObj );
}

/**Function*************************************************************

  Synopsis    [Computes cross-cut of the circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCrossCut( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int nCutSize = 0, nCutSizeMax = 0;
    int i;
    Abc_NtkCleanCopy( pNtk );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        Abc_NtkCrossCut_rec( pObj, &nCutSize, &nCutSizeMax );
        nCutSize--;
    }
    assert( nCutSize == 0 );
    printf( "Max cross cut size = %6d.  Ratio = %6.2f %%\n", nCutSizeMax, 100.0 * nCutSizeMax/Abc_NtkObjNum(pNtk) );
    return nCutSizeMax;
}


/**Function*************************************************************

  Synopsis    [Prints all 3-var functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrint256()
{
    FILE * pFile;
    unsigned i;
    pFile = fopen( "4varfs.txt", "w" );
    for ( i = 1; i < (1<<16)-1; i++ )
    {
        fprintf( pFile, "read_truth " );
        Extra_PrintBinary( pFile, &i, 16 );
        fprintf( pFile, "; clp; st; w 1.blif; map; cec 1.blif\n" );
    }
    fclose( pFile );
}


static     int * pSupps;

/**Function*************************************************************

  Synopsis    [Compares the supergates by their level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCompareConesCompare( int * pNum1, int * pNum2 )
{
    if ( pSupps[*pNum1] > pSupps[*pNum2] )
        return -1;
    if ( pSupps[*pNum1] < pSupps[*pNum2] )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Analyze choice node support.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCompareCones( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vSupp, * vNodes, * vReverse;
    Abc_Obj_t * pObj, * pTemp;
    int Iter, i, k, Counter, CounterCos, CounterCosNew;
    int * pPerms;

    // sort COs by support size
    pPerms = ALLOC( int, Abc_NtkCoNum(pNtk) );
    pSupps = ALLOC( int, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pPerms[i] = i;
        vSupp = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        pSupps[i] = Vec_PtrSize(vSupp);
        Vec_PtrFree( vSupp );
    }
    qsort( (void *)pPerms, Abc_NtkCoNum(pNtk), sizeof(int), (int (*)(const void *, const void *)) Abc_NtkCompareConesCompare );

    // consider COs in this order
    Iter = 0;
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pObj = Abc_NtkCo( pNtk, pPerms[i] );
        if ( pObj->fMarkA )
            continue;
        Iter++;

        vSupp = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        vNodes = Abc_NtkDfsNodes( pNtk, &pObj, 1 );
        vReverse = Abc_NtkDfsReverseNodesContained( pNtk, (Abc_Obj_t **)Vec_PtrArray(vSupp), Vec_PtrSize(vSupp) );
        // count the number of nodes in the reverse cone
        Counter = 0;
        for ( k = 1; k < Vec_PtrSize(vReverse) - 1; k++ )
            for ( pTemp = Vec_PtrEntry(vReverse, k); pTemp; pTemp = pTemp->pCopy )
                Counter++;
        CounterCos = CounterCosNew = 0;
        for ( pTemp = Vec_PtrEntryLast(vReverse); pTemp; pTemp = pTemp->pCopy )
        {
            assert( Abc_ObjIsCo(pTemp) );
            CounterCos++;
            if ( pTemp->fMarkA == 0 )
                CounterCosNew++;
            pTemp->fMarkA = 1;
        }
        // print statistics
        printf( "%4d CO %5d :  Supp = %5d.  Lev = %3d.  Cone = %5d.  Rev = %5d.  COs = %3d (%3d).\n",
            Iter, pPerms[i], Vec_PtrSize(vSupp), Abc_ObjLevel(Abc_ObjFanin0(pObj)), Vec_PtrSize(vNodes), Counter, CounterCos, CounterCosNew );

        // free arrays
        Vec_PtrFree( vSupp );
        Vec_PtrFree( vNodes );
        Vec_PtrFree( vReverse );

        if ( Vec_PtrSize(vSupp) < 10 )
            break;
    }
    Abc_NtkForEachCo( pNtk, pObj, i )
        pObj->fMarkA = 0;

    free( pPerms );
    free( pSupps );
}

/**Function*************************************************************

  Synopsis    [Analyze choice node support.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCompareSupports( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vSupp;
    Abc_Obj_t * pObj, * pTemp;
    int i, nNodesOld;
    assert( Abc_NtkIsStrash(pNtk) );
    Abc_AigForEachAnd( pNtk, pObj, i )
    {
        if ( !Abc_AigNodeIsChoice(pObj) )
            continue;

        vSupp = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        nNodesOld = Vec_PtrSize(vSupp);
        Vec_PtrFree( vSupp );

        for ( pTemp = pObj->pData; pTemp; pTemp = pTemp->pData )
        {
            vSupp = Abc_NtkNodeSupport( pNtk, &pTemp, 1 );
            if ( nNodesOld != Vec_PtrSize(vSupp) )
                printf( "Choice orig = %3d  Choice new = %3d\n", nNodesOld, Vec_PtrSize(vSupp) );
            Vec_PtrFree( vSupp );
        }
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


