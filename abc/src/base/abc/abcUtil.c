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
#include "base/main/main.h"
#include "map/mio/mio.h"
#include "bool/dec/dec.h"
#include "opt/fxu/fxu.h"
#include "aig/miniaig/ndr.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


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
    pAttrMan = (Vec_Att_t *)Vec_PtrEntry( pNtk->vAttrs, Attr );
    Vec_PtrWriteEntry( pNtk->vAttrs, Attr, NULL );
    pUserMan = Vec_AttFree( pAttrMan, fFreeMan );
    return pUserMan;
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
        nCubes += Abc_SopGetCubeNum( (char *)pNode->pData );
    }
    return nCubes;
}

/**Function*************************************************************

  Synopsis    [Reads the number of cubes of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetCubePairNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i;
    word nCubes, nCubePairs = 0;
    assert( Abc_NtkHasSop(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( Abc_NodeIsConst(pNode) )
            continue;
        assert( pNode->pData );
        nCubes = (word)Abc_SopGetCubeNum( (char *)pNode->pData );
        if ( nCubes > 1 )
            nCubePairs += nCubes * (nCubes - 1) / 2;
    }
    return (int)(nCubePairs > (1<<30) ? (1<<30) : nCubePairs);
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
        nLits += Abc_SopGetLitNum( (char *)pNode->pData );
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
        pFactor = Dec_Factor( (char *)pNode->pData );
        nNodes += 1 + Dec_GraphNodeNum(pFactor);
        Dec_GraphFree( pFactor );
    }
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Counts the number of nodes with more than 1 reference.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetMultiRefNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int nNodes, i;
    assert( Abc_NtkIsStrash(pNtk) );
    nNodes = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        nNodes += (int)(Abc_ObjFanoutNum(pNode) > 1);
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
    int nNodes = 0;
#ifdef ABC_USE_CUDD
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsBddLogic(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        assert( pNode->pData );
        if ( Abc_ObjFaninNum(pNode) < 2 )
            continue;
        nNodes += pNode->pData? -1 + Cudd_DagSize( (DdNode *)pNode->pData ) : 0;
    }
#endif
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
        nNodes += pNode->pData? Hop_DagSize( (Hop_Obj_t *)pNode->pData ) : 0;
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
    int nClauses = 0;
#ifdef ABC_USE_CUDD
    extern int Abc_CountZddCubes( DdManager * dd, DdNode * zCover );
    Abc_Obj_t * pNode;
    DdNode * bCover, * zCover, * bFunc;
    DdManager * dd = (DdManager *)pNtk->pManFunc;
    int i;
    assert( Abc_NtkIsBddLogic(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        assert( pNode->pData );
        bFunc = (DdNode *)pNode->pData;

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
#endif
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
    Abc_Obj_t * pObj;
    double TotalArea;
    int i;
    assert( Abc_NtkHasMapping(pNtk) );
    TotalArea = 0.0;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( Abc_ObjIsBarBuf(pObj) )
            continue;
//        assert( pObj->pData );
        if ( pObj->pData == NULL )
        {
            printf( "Node without mapping is encountered.\n" );
            continue;
        }
        TotalArea += Mio_GateReadArea( (Mio_Gate_t *)pObj->pData );
        // assuming that twin gates follow each other
        if ( Abc_NtkFetchTwinNode(pObj) )
            i++;
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

  Synopsis    [Counts the number of exors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetBufNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        Counter += (Abc_ObjFaninNum(pNode) == 1);
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of exors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkGetLargeNodeNum( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        Counter += (Abc_ObjFaninNum(pNode) > 1);
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
int Abc_NtkGetFanoutMax( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, nFaninsMax = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( nFaninsMax < Abc_ObjFanoutNum(pNode) )
            nFaninsMax = Abc_ObjFanoutNum(pNode);
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
void Abc_NtkCleanCopy_rec( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj; 
    int i;
    Abc_NtkCleanCopy( pNtk );
    Abc_NtkForEachBox( pNtk, pObj, i )
        Abc_NtkCleanCopy_rec( Abc_ObjModel(pObj) );
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
void Abc_NtkFillTemp( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->iTemp = -1;
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
        pObj->pCopy = (Abc_Obj_t *)Vec_PtrEntry( vCopies, i );
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
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->pNext = NULL;
}
void Abc_NtkCleanNext_rec( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj; 
    int i;
    Abc_NtkCleanNext( pNtk );
    Abc_NtkForEachBox( pNtk, pObj, i )
        Abc_NtkCleanNext_rec( Abc_ObjModel(pObj) );
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
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->fMarkA = 0;
}

/**Function*************************************************************

  Synopsis    [Cleans the copy field of all objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCleanMarkB( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->fMarkB = 0;
}

/**Function*************************************************************

  Synopsis    [Cleans the copy field of all objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCleanMarkC( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->fMarkC = 0;
}

/**Function*************************************************************

  Synopsis    [Cleans the copy field of all objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCleanMarkAB( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->fMarkA = pObj->fMarkB = 0;
}

/**Function*************************************************************

  Synopsis    [Cleans the copy field of all objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCleanMarkABC( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->fMarkA = pObj->fMarkB = pObj->fMarkC = 0;
}

/**Function*************************************************************

  Synopsis    [Returns the index of the given fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeFindFanin( Abc_Obj_t * pNode, Abc_Obj_t * pFanin )
{
    Abc_Obj_t * pThis;
    int i;
    Abc_ObjForEachFanin( pNode, pThis, i )
        if ( pThis == pFanin )
            return i;
    return -1;
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
int Abc_NtkLogicHasSimpleCos( Abc_Ntk_t * pNtk )
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
int Abc_NtkLogicMakeSimpleCos2( Abc_Ntk_t * pNtk, int fDuplicate )
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

  Synopsis    [Transforms the network to have simple COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkLogicMakeSimpleCosTest( Abc_Ntk_t * pNtk, int fDuplicate )
{
    int nObjs = Abc_NtkObjNumMax(pNtk);
    unsigned * pType = ABC_CALLOC( unsigned, nObjs );
    Abc_Obj_t * pNode;
    int i, Counts[4] = {0}, Consts[2] = {0}, Inputs[2] = {0};
    // collect info
    Abc_NtkForEachCo( pNtk, pNode, i ) 
    {
        if ( Abc_ObjFaninId0(pNode) == 0 )
            Consts[Abc_ObjFaninC0(pNode)]++;
        if ( Abc_ObjIsCi(Abc_ObjFanin0(pNode)) )
            Inputs[Abc_ObjFaninC0(pNode)]++;
        pType[Abc_ObjFaninId0(pNode)] |= (1 << Abc_ObjFaninC0(pNode));
    }
    // count the numbers
    for ( i = 0; i < nObjs; i++ )
        Counts[pType[i]]++;
    for ( i = 0; i < 4; i++ )
        printf( "%d = %d     ", i, Counts[i] );
    for ( i = 0; i < 2; i++ )
        printf( "c%d = %d     ", i, Consts[i] );
    for ( i = 0; i < 2; i++ )
        printf( "i%d = %d    ", i, Inputs[i] );
    printf( "\n" );
    ABC_FREE( pType );
}

/**Function*************************************************************

  Synopsis    [Transforms the network to have simple COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkLogicMakeSimpleCos( Abc_Ntk_t * pNtk, int fDuplicate )
{
    int fAddBuffers = 1;
    Vec_Ptr_t * vDrivers, * vCoTerms;
    Abc_Obj_t * pNode, * pDriver, * pDriverNew, * pFanin;
    int i, k, LevelMax, nTotal = 0;
    assert( Abc_NtkIsLogic(pNtk) );
    LevelMax = Abc_NtkLevel(pNtk);
//    Abc_NtkLogicMakeSimpleCosTest( pNtk, fDuplicate );

    // fix constant drivers
    Abc_NtkForEachCo( pNtk, pNode, i ) 
    {
        pDriver = Abc_ObjFanin0(pNode);
        if ( !Abc_NodeIsConst(pDriver) )
            continue;
        pDriverNew = (Abc_ObjFaninC0(pNode) == Abc_NodeIsConst0(pDriver)) ? Abc_NtkCreateNodeConst1(pNtk) : Abc_NtkCreateNodeConst0(pNtk);
        if ( Abc_ObjFaninC0(pNode) )
            Abc_ObjXorFaninC( pNode, 0 );
        Abc_ObjPatchFanin( pNode, pDriver, pDriverNew );
        if ( Abc_ObjFanoutNum(pDriver) == 0 )
            Abc_NtkDeleteObj( pDriver );
    }

    // collect drivers pointed by complemented edges
    vDrivers = Vec_PtrAlloc( 100 );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachCo( pNtk, pNode, i ) 
    {
        if ( !Abc_ObjFaninC0(pNode) )
            continue;
        pDriver = Abc_ObjFanin0(pNode);
        if ( Abc_NodeIsTravIdCurrent(pDriver) )
            continue;
        Abc_NodeSetTravIdCurrent(pDriver);
        Vec_PtrPush( vDrivers, pDriver );
    }
    // fix complemented drivers
    if ( Vec_PtrSize(vDrivers) > 0 )
    {
        int nDupGates = 0, nDupInvs = 0, nDupChange = 0;
        Vec_Ptr_t * vFanouts = Vec_PtrAlloc( 100 );
        Vec_PtrForEachEntry( Abc_Obj_t *, vDrivers, pDriver, i )
        {
            int fHasDir = 0, fHasInv = 0, fHasOther = 0;
            Abc_ObjForEachFanout( pDriver, pNode, k )
            {
                if ( !Abc_ObjIsCo(pNode) )
                {
                    assert( !Abc_ObjFaninC0(pNode) );
                    fHasOther = 1;
                    continue;
                }
                if ( Abc_ObjFaninC0(pNode) )
                    fHasInv = 1;
                else //if ( Abc_ObjFaninC0(pNode) )
                    fHasDir = 1;
            }
            assert( fHasInv );
            if ( Abc_ObjIsCi(pDriver) || fHasDir || (fHasOther && Abc_NtkHasMapping(pNtk)) ) // cannot change
            {
                // duplicate if critical
                if ( fDuplicate && Abc_ObjIsNode(pDriver) && Abc_ObjLevel(pDriver) == LevelMax )
                {
                    pDriverNew = Abc_NtkDupObj( pNtk, pDriver, 0 ); 
                    Abc_ObjForEachFanin( pDriver, pFanin, k )
                        Abc_ObjAddFanin( pDriverNew, pFanin );
                    Abc_NodeComplement( pDriverNew );
                    nDupGates++;
                }
                else // add inverter
                {
                    pDriverNew = Abc_NtkCreateNodeInv( pNtk, pDriver );
                    nDupInvs++;
                }
                // collect CO fanouts to be redirected to the new node
                Vec_PtrClear( vFanouts );
                Abc_ObjForEachFanout( pDriver, pNode, k )
                    if ( Abc_ObjIsCo(pNode) && Abc_ObjFaninC0(pNode) )
                        Vec_PtrPush( vFanouts, pNode );
                assert( Vec_PtrSize(vFanouts) > 0 );
                Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pNode, k )
                {
                    Abc_ObjXorFaninC( pNode, 0 );
                    Abc_ObjPatchFanin( pNode, pDriver, pDriverNew );
                    assert( Abc_ObjIsCi(pDriver) || Abc_ObjFanoutNum(pDriver) > 0 );
                }
            }
            else // can change
            {
                // change polarity of the driver
                assert( Abc_ObjIsNode(pDriver) );
                Abc_NodeComplement( pDriver );
                Abc_ObjForEachFanout( pDriver, pNode, k )
                {
                    if ( Abc_ObjIsCo(pNode) )
                    {
                        assert( Abc_ObjFaninC0(pNode) );
                        Abc_ObjXorFaninC( pNode, 0 );
                    }
                    else if ( Abc_ObjIsNode(pNode) )
                        Abc_NodeComplementInput( pNode, pDriver );
                    else assert( 0 );
                }
                nDupChange++;
            }
        }
        Vec_PtrFree( vFanouts );
//        printf( "Resolving inverted CO drivers: Invs = %d. Dups = %d. Changes = %d.\n",
//            nDupInvs, nDupGates, nDupChange );
        nTotal += nDupInvs + nDupGates;
    }
    Vec_PtrFree( vDrivers );

    // collect COs that needs fixing by adding buffers or duplicating
    vCoTerms = Vec_PtrAlloc( 100 );
    Abc_NtkIncrementTravId( pNtk );
    
    // The following cases should be addressed only if the network is written 
    // into a BLIF file. Otherwise, it is possible to skip them:
    // (1) if a CO points to a CI with a different name
    // (2) if an internal node drives more than one CO
    if ( fAddBuffers )
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        // if the driver is a CI and has different name, this is an error
        pDriver = Abc_ObjFanin0(pNode);
        if ( Abc_ObjIsCi(pDriver) && strcmp(Abc_ObjName(pDriver), Abc_ObjName(pNode)) )
        {
            Vec_PtrPush( vCoTerms, pNode );
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
            Vec_PtrPush( vCoTerms, pNode );
            continue;
        }
    }
    // fix duplication problem
    if ( Vec_PtrSize(vCoTerms) > 0 )
    {
        int nDupBufs = 0, nDupGates = 0;
        Vec_PtrForEachEntry( Abc_Obj_t *, vCoTerms, pNode, i )
        {
            pDriver = Abc_ObjFanin0(pNode);
            // duplicate if critical
            if ( fDuplicate && Abc_ObjIsNode(pDriver) && (Abc_NtkHasMapping(pNtk) || Abc_ObjLevel(pDriver) == LevelMax) )
            {
                pDriverNew = Abc_NtkDupObj( pNtk, pDriver, 0 ); 
                Abc_ObjForEachFanin( pDriver, pFanin, k )
                    Abc_ObjAddFanin( pDriverNew, pFanin );
                nDupGates++;
            }
            else // add buffer
            {
                pDriverNew = Abc_NtkCreateNodeBuf( pNtk, pDriver );
                Abc_ObjAssignName( pDriverNew, Abc_ObjName(pDriver), "_buf" );
                nDupBufs++;
            }
            // swing the PO
            Abc_ObjPatchFanin( pNode, pDriver, pDriverNew );
            assert( Abc_ObjIsCi(pDriver) || Abc_ObjFanoutNum(pDriver) > 0 );
        }
//        printf( "Resolving shared CO drivers: Bufs = %d. Dups = %d.\n", nDupBufs, nDupGates );
        nTotal += nDupBufs + nDupGates;
    }
    Vec_PtrFree( vCoTerms );
    return nTotal;
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
        pNode1 = (Abc_Obj_t *)p->pArray[i  ];
        pNode2 = (Abc_Obj_t *)p->pArray[i-1];
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
int Abc_NodeIsExorType( Abc_Obj_t * pNode )
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
    // this is AIG, which means the fanins should be ordered
    assert( Abc_ObjFaninId0(pNode0) != Abc_ObjFaninId1(pNode1) || 
            Abc_ObjFaninId0(pNode1) != Abc_ObjFaninId1(pNode0) );
    // if grand children are not the same, this is not EXOR
    if ( Abc_ObjFaninId0(pNode0) != Abc_ObjFaninId0(pNode1) ||
         Abc_ObjFaninId1(pNode0) != Abc_ObjFaninId1(pNode1) )
         return 0;
    // finally, if the complemented edges are matched, this is not EXOR
    if ( Abc_ObjFaninC0(pNode0) == Abc_ObjFaninC0(pNode1) || 
         Abc_ObjFaninC1(pNode0) == Abc_ObjFaninC1(pNode1) )
         return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the root of MUX or EXOR/NEXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsMuxType( Abc_Obj_t * pNode )
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
    if ( !Abc_AigNodeIsAnd(pNode0) || !Abc_AigNodeIsAnd(pNode1) )
        return 0;
    // otherwise the node is MUX iff it has a pair of equal grandchildren with opposite polarity
    return (Abc_ObjFaninId0(pNode0) == Abc_ObjFaninId0(pNode1) && (Abc_ObjFaninC0(pNode0) ^ Abc_ObjFaninC0(pNode1))) || 
           (Abc_ObjFaninId0(pNode0) == Abc_ObjFaninId1(pNode1) && (Abc_ObjFaninC0(pNode0) ^ Abc_ObjFaninC1(pNode1))) ||
           (Abc_ObjFaninId1(pNode0) == Abc_ObjFaninId0(pNode1) && (Abc_ObjFaninC1(pNode0) ^ Abc_ObjFaninC0(pNode1))) ||
           (Abc_ObjFaninId1(pNode0) == Abc_ObjFaninId1(pNode1) && (Abc_ObjFaninC1(pNode0) ^ Abc_ObjFaninC1(pNode1)));
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the root of MUX or EXOR/NEXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCountMuxes( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i;
    int Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        Counter += Abc_NodeIsMuxType( pNode );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is the control type of the MUX.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeIsMuxControlType( Abc_Obj_t * pNode )
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
    Abc_Ntk_t ** ppNtk1, Abc_Ntk_t ** ppNtk2, int * pfDelete1, int * pfDelete2, int fCheck )
{
    FILE * pFile;
    Abc_Ntk_t * pNtk1, * pNtk2, * pNtkTemp;
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
        pNtk1 = Abc_NtkDup(pNtk);
        pNtk2 = Io_Read( pNtk->pSpec, Io_ReadFileType(pNtk->pSpec), fCheck, 0 );
        if ( pNtk2 == NULL )
            return 0;
        *pfDelete1 = 1;
        *pfDelete2 = 1;
    }
    else if ( argc == util_optind + 1 ) 
    {
        if ( pNtk == NULL )
        {
            fprintf( pErr, "Empty current network.\n" );
            return 0;
        }
        pNtk1 = Abc_NtkDup(pNtk);
        pNtk2 = Io_Read( argv[util_optind], Io_ReadFileType(argv[util_optind]), fCheck, 0 );
        if ( pNtk2 == NULL )
            return 0;
        *pfDelete1 = 1;
        *pfDelete2 = 1;
    }
    else if ( argc == util_optind + 2 ) 
    {
        pNtk1 = Io_Read( argv[util_optind], Io_ReadFileType(argv[util_optind]), fCheck, 0 );
        if ( pNtk1 == NULL )
            return 0;
        pNtk2 = Io_Read( argv[util_optind+1], Io_ReadFileType(argv[util_optind+1]), fCheck, 0 );
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

    // make sure the networks are strashed
    if ( !Abc_NtkIsStrash(pNtk1) )
    {
        pNtkTemp = Abc_NtkStrash( pNtk1, 0, 1, 0 );
        if ( *pfDelete1 )
            Abc_NtkDelete( pNtk1 );
        pNtk1 = pNtkTemp;
        *pfDelete1 = 1;
    }
    if ( !Abc_NtkIsStrash(pNtk2) )
    {
        pNtkTemp = Abc_NtkStrash( pNtk2, 0, 1, 0 );
        if ( *pfDelete2 )
            Abc_NtkDelete( pNtk2 );
        pNtk2 = pNtkTemp;
        *pfDelete2 = 1;
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
    Diff = Abc_ObjRegular(*pp1)->Id - Abc_ObjRegular(*pp2)->Id;
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
    Diff = Abc_ObjRegular(*pp1)->Id - Abc_ObjRegular(*pp2)->Id;
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
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
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
    Abc_AigRehash( (Abc_Aig_t *)pNtk->pManFunc );

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
    int Value = (int)(ABC_PTRINT_T)pObj->pCopy;
    pObj->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)(Value + 1);
    return (int)(ABC_PTRINT_T)pObj->pCopy == Abc_ObjFanoutNum(pObj);
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
    pPerms = ABC_ALLOC( int, Abc_NtkCoNum(pNtk) );
    pSupps = ABC_ALLOC( int, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pPerms[i] = i;
        vSupp = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        pSupps[i] = Vec_PtrSize(vSupp);
        Vec_PtrFree( vSupp );
    }
    qsort( (void *)pPerms, (size_t)Abc_NtkCoNum(pNtk), sizeof(int), (int (*)(const void *, const void *)) Abc_NtkCompareConesCompare );

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
            for ( pTemp = (Abc_Obj_t *)Vec_PtrEntry(vReverse, k); pTemp; pTemp = (Abc_Obj_t *)pTemp->pCopy )
                Counter++;
        CounterCos = CounterCosNew = 0;
        for ( pTemp = (Abc_Obj_t *)Vec_PtrEntryLast(vReverse); pTemp; pTemp = (Abc_Obj_t *)pTemp->pCopy )
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

        if ( Vec_PtrSize(vSupp) < 10 )
        {
            // free arrays
            Vec_PtrFree( vSupp );
            Vec_PtrFree( vNodes );
            Vec_PtrFree( vReverse );
            break;
        }

        // free arrays
        Vec_PtrFree( vSupp );
        Vec_PtrFree( vNodes );
        Vec_PtrFree( vReverse );

    }
    Abc_NtkForEachCo( pNtk, pObj, i )
        pObj->fMarkA = 0;

    ABC_FREE( pPerms );
    ABC_FREE( pSupps );
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

        for ( pTemp = (Abc_Obj_t *)pObj->pData; pTemp; pTemp = (Abc_Obj_t *)pTemp->pData )
        {
            vSupp = Abc_NtkNodeSupport( pNtk, &pTemp, 1 );
            if ( nNodesOld != Vec_PtrSize(vSupp) )
                printf( "Choice orig = %3d  Choice new = %3d\n", nNodesOld, Vec_PtrSize(vSupp) );
            Vec_PtrFree( vSupp );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Complements the constraint outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkInvertConstraints( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    if ( Abc_NtkConstrNum(pNtk) == 0 )
        return;
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        if ( i >= Abc_NtkPoNum(pNtk) - Abc_NtkConstrNum(pNtk) )
            Abc_ObjXorFaninC( pObj, 0 );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintCiLevels( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachCi( pNtk, pObj, i )
        printf( "%c=%d ", 'a'+i, pObj->Level );
    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    [Returns 1 if all other fanouts of pFanin are below pNode.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkAddBuffsEval( Abc_Obj_t * pNode, Abc_Obj_t * pFanin )
{
    Abc_Obj_t * pFanout;
    int i;
    Abc_ObjForEachFanout( pFanin, pFanout, i )
        if ( pFanout != pNode && pFanout->Level >= pNode->Level )
            return 0;
    return 1;
}
/**Function*************************************************************

  Synopsis    [Returns 1 if there exist a fanout of pFanin higher than pNode.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkAddBuffsEval2( Abc_Obj_t * pNode, Abc_Obj_t * pFanin )
{
    Abc_Obj_t * pFanout;
    int i;
    Abc_ObjForEachFanout( pFanin, pFanout, i )
        if ( pFanout != pNode && pFanout->Level > pNode->Level )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkAddBuffsOne( Vec_Ptr_t * vBuffs, Abc_Obj_t * pFanin, int Level, int nLevelMax )
{
    Abc_Obj_t * pBuffer;
    assert( Level - 1 >= Abc_ObjLevel(pFanin) );
    pBuffer = (Abc_Obj_t *)Vec_PtrEntry( vBuffs, Abc_ObjId(pFanin) * nLevelMax + Level );
    if ( pBuffer == NULL )
    {
        if ( Level - 1 == Abc_ObjLevel(pFanin) )
            pBuffer = pFanin;
        else
            pBuffer = Abc_NtkAddBuffsOne( vBuffs, pFanin, Level - 1, nLevelMax );
        pBuffer = Abc_NtkCreateNodeBuf( Abc_ObjNtk(pFanin), pBuffer ); 
        Vec_PtrWriteEntry( vBuffs, Abc_ObjId(pFanin) * nLevelMax + Level, pBuffer );
    }
    return pBuffer;
}
Abc_Ntk_t * Abc_NtkAddBuffsInt( Abc_Ntk_t * pNtkInit, int fReverse, int nImprove, int fVerbose )
{
    Vec_Ptr_t * vBuffs;
    Abc_Ntk_t * pNtk = Abc_NtkDup( pNtkInit );
    Abc_Obj_t * pObj, * pFanin, * pBuffer;
    int i, k, Iter, nLevelMax = Abc_NtkLevel( pNtk );
    Abc_NtkForEachCo( pNtk, pObj, i )
        pObj->Level = nLevelMax + 1;
    if ( fReverse )
    {
        Vec_Ptr_t * vNodes = Abc_NtkDfs( pNtk, 1 );
        assert( nLevelMax < (1<<18) );
        Vec_PtrForEachEntryReverse( Abc_Obj_t *, vNodes, pObj, i )
        {
            pObj->Level = (1<<18);
            Abc_ObjForEachFanout( pObj, pFanin, k )
                pObj->Level = Abc_MinInt( pFanin->Level - 1, pObj->Level );
            assert( pObj->Level > 0 );
        }
        Abc_NtkForEachCi( pNtk, pObj, i )
            pObj->Level = 0;

        // move the nodes down one step at a time
        for ( Iter = 0; Iter < nImprove; Iter++ )
        {
            int Counter = 0, TotalGain = 0;
            Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
            {
                int CountGain = -1;
                assert( pObj->Level > 0 );
                Abc_ObjForEachFanin( pObj, pFanin, k )
                {
                    assert( pFanin->Level < pObj->Level );
                    if ( pFanin->Level + 1 == pObj->Level )
                        break;
                }
                if ( k < Abc_ObjFaninNum(pObj) ) // cannot move
                    continue;
                Abc_ObjForEachFanin( pObj, pFanin, k )
                    CountGain += Abc_NtkAddBuffsEval( pObj, pFanin );
                if ( CountGain >= 0 ) // can move
                {
                    pObj->Level--;
                    Counter++;
                    TotalGain += CountGain;
                }
            }
            if ( fVerbose )
                printf( "Shifted %5d nodes down with total gain %5d.\n", Counter, TotalGain );
            if ( Counter == 0 )
                break;
        }
        Vec_PtrFree( vNodes );
    }
    else
    {
        // move the nodes up one step at a time
        Vec_Ptr_t * vNodes = Abc_NtkDfs( pNtk, 1 );
        for ( Iter = 0; Iter < nImprove; Iter++ )
        {
            int Counter = 0, TotalGain = 0;
            Vec_PtrForEachEntryReverse( Abc_Obj_t *, vNodes, pObj, i )
            {
                int CountGain = 1;
                assert( pObj->Level <= (unsigned)nLevelMax );
                Abc_ObjForEachFanout( pObj, pFanin, k )
                {
                    assert( pFanin->Level > pObj->Level );
                    if ( pFanin->Level == pObj->Level + 1 )
                        break;
                }
                if ( k < Abc_ObjFanoutNum(pObj) ) // cannot move
                    continue;
                Abc_ObjForEachFanin( pObj, pFanin, k )
                    CountGain -= !Abc_NtkAddBuffsEval2( pObj, pFanin );
                if ( CountGain >= 0 ) // can move
                {
                    pObj->Level++;
                    Counter++;
                    TotalGain += CountGain;
                }
            } 
            if ( fVerbose )
                printf( "Shifted %5d nodes up with total gain %5d.\n", Counter, TotalGain );
            if ( Counter == 0 )
                break;
        }
        Vec_PtrFree( vNodes );
    }
    vBuffs = Vec_PtrStart( Abc_NtkObjNumMax(pNtk) * (nLevelMax + 1) );
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( i == Vec_PtrSize(vBuffs) / (nLevelMax + 1) )
            break;
        if ( !Abc_ObjIsNode(pObj) && !Abc_ObjIsCo(pObj) )
            continue;
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            assert( Abc_ObjLevel(pObj) - 1 >= Abc_ObjLevel(pFanin) );
            if ( Abc_ObjLevel(pObj) - 1 == Abc_ObjLevel(pFanin) )
                continue;
            pBuffer = Abc_NtkAddBuffsOne( vBuffs, pFanin, Abc_ObjLevel(pObj) - 1, nLevelMax );
            Abc_ObjPatchFanin( pObj, pFanin, pBuffer );
        }
    }
    Vec_PtrFree( vBuffs );
    Abc_NtkForEachCo( pNtk, pObj, i )
        pObj->Level = 0;
    return pNtk;
}
Abc_Ntk_t * Abc_NtkAddBuffs( Abc_Ntk_t * pNtkInit, int fDirect, int fReverse, int nImprove, int fVerbose )
{
    Abc_Ntk_t * pNtkD, * pNtkR;
    if ( fDirect )
        return Abc_NtkAddBuffsInt( pNtkInit, 0, nImprove, fVerbose );
    if ( fReverse )
        return Abc_NtkAddBuffsInt( pNtkInit, 1, nImprove, fVerbose );
    pNtkD = Abc_NtkAddBuffsInt( pNtkInit, 0, nImprove, fVerbose );
    pNtkR = Abc_NtkAddBuffsInt( pNtkInit, 1, nImprove, fVerbose );
    if ( Abc_NtkNodeNum(pNtkD) < Abc_NtkNodeNum(pNtkR) )
    {
        Abc_NtkDelete( pNtkR );
        return pNtkD;
    }
    else
    {
        Abc_NtkDelete( pNtkD );
        return pNtkR;
    }
}

/**Function*************************************************************

  Synopsis    [Computes max delay using log(n) delay model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_NtkComputeDelay( Abc_Ntk_t * pNtk )
{
    static double GateDelays[20] = { 1.00, 1.00, 2.00, 2.58, 3.00, 3.32, 3.58, 3.81, 4.00, 4.17, 4.32, 4.46, 4.58, 4.70, 4.81, 4.91, 5.00, 5.09, 5.17, 5.25 };
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pFanin;
    float DelayMax, Delays[15] = {0};
    int nFaninMax, i, k;
    // calculate relative gate delays
    nFaninMax = Abc_NtkGetFaninMax( pNtk );
    assert( nFaninMax > 1 && nFaninMax < 15 );
    for ( i = 0; i <= nFaninMax; i++ )
        Delays[i] = GateDelays[i]/GateDelays[nFaninMax];
    // set max CI delay
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->dTemp = 0.0;
    // compute delays for each node
    vNodes = Abc_NtkDfs( pNtk, 1 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        pObj->dTemp = 0.0;
        Abc_ObjForEachFanin( pObj, pFanin, k )
            pObj->dTemp = Abc_MaxFloat( pObj->dTemp, pFanin->dTemp );
        pObj->dTemp += Delays[Abc_ObjFaninNum(pObj)];
    }
    Vec_PtrFree( vNodes );
    DelayMax = 0.0;
    // find max CO delay
    Abc_NtkForEachCo( pNtk, pObj, i )
        DelayMax = Abc_MaxFloat( DelayMax, Abc_ObjFanin0(pObj)->dTemp );
    return DelayMax;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeSopToCubes( Abc_Obj_t * pNodeOld, Abc_Ntk_t * pNtkNew, int fXor )
{
    Abc_Obj_t * pNodeOr, * pNodeNew, * pFanin;
    char * pCube, * pSop = (char *)pNodeOld->pData;
    int v, Value, nVars = Abc_ObjFaninNum(pNodeOld), nFanins;
    // create the root node
    if ( Abc_SopGetCubeNum(pSop) < 2 )
    {
        pNodeNew = Abc_NtkDupObj( pNtkNew, pNodeOld, 0 );
        Abc_ObjForEachFanin( pNodeOld, pFanin, v )
            Abc_ObjAddFanin( pNodeNew, pFanin->pCopy );
        assert( pNodeOld->pCopy == pNodeNew );
        return;
    }
    // add the OR gate
    pNodeOr = Abc_NtkCreateNode( pNtkNew );
    if ( fXor )
        pNodeOr->pData = Abc_SopCreateXorSpecial( (Mem_Flex_t *)pNtkNew->pManFunc, Abc_SopGetCubeNum(pSop) );
    else 
        pNodeOr->pData = Abc_SopCreateOr( (Mem_Flex_t *)pNtkNew->pManFunc, Abc_SopGetCubeNum(pSop), NULL );
    // check the logic function of the node
    Abc_SopForEachCube( pSop, nVars, pCube )
    {
        nFanins = 0;
        Abc_CubeForEachVar( pCube, Value, v )
            if ( Value == '0' || Value == '1' )
                nFanins++;
        if ( nFanins == 0 ) // const1 cube in ESOP
        {
            pNodeNew = Abc_NtkCreateNodeConst1( pNtkNew );
            Abc_ObjAddFanin( pNodeOr, pNodeNew );
            continue;
        }
        assert( nFanins > 0 );
        // create node
        pNodeNew = Abc_NtkCreateNode( pNtkNew );
        pNodeNew->pData = Abc_SopCreateAnd( (Mem_Flex_t *)pNtkNew->pManFunc, nFanins, NULL );
        nFanins = 0;
        Abc_CubeForEachVar( pCube, Value, v )
        {
            if ( Value != '0' && Value != '1' )
                continue;
            Abc_ObjAddFanin( pNodeNew, Abc_ObjFanin(pNodeOld, v)->pCopy );
            if ( Value == '0' )
                Abc_SopComplementVar( (char *)pNodeNew->pData, nFanins );
            nFanins++;
        }
        Abc_ObjAddFanin( pNodeOr, pNodeNew );
    }
    // check the complement
    if ( Abc_SopIsComplement(pSop) )
        Abc_SopComplement( (char *)pNodeOr->pData );
    // mark the old node with the new one
    assert( pNodeOld->pCopy == NULL );
    pNodeOld->pCopy = pNodeOr;
}
Abc_Ntk_t * Abc_NtkSopToCubes( Abc_Ntk_t * pNtk, int fXor )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pNode;
    Vec_Ptr_t * vNodes;
    int i;
    assert( Abc_NtkIsSopLogic(pNtk) );
    Abc_NtkCleanCopy( pNtk );
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );
    // perform conversion in the topological order
    vNodes = Abc_NtkDfs( pNtk, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
        Abc_NodeSopToCubes( pNode, pNtkNew, fXor );
    Vec_PtrFree( vNodes );
    // make sure everything is okay
    Abc_NtkFinalize( pNtk, pNtkNew );
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkSopToCubes: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Creates precomputed reverse topological order for each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int  Abc_NtkTopoHasBeg( Abc_Obj_t * p )  { return Vec_IntEntry(p->pNtk->vTopo, 2*Abc_ObjId(p)  );       }
static inline int  Abc_NtkTopoHasEnd( Abc_Obj_t * p )  { return Vec_IntEntry(p->pNtk->vTopo, 2*Abc_ObjId(p)+1);       }

static inline void Abc_NtkTopoSetBeg( Abc_Obj_t * p )  { Vec_IntWriteEntry(p->pNtk->vTopo, 2*Abc_ObjId(p)  , Vec_IntSize(p->pNtk->vTopo));  }
static inline void Abc_NtkTopoSetEnd( Abc_Obj_t * p )  { Vec_IntWriteEntry(p->pNtk->vTopo, 2*Abc_ObjId(p)+1, Vec_IntSize(p->pNtk->vTopo));  }

void Abc_NtkReverseTopoOrder_rec( Abc_Obj_t * pObj, int fThisIsPivot )
{
    Abc_Obj_t * pNext, * pPivot = NULL;
    int i;
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    if ( Abc_ObjIsPo(pObj) )
    {
        Vec_IntPush( pObj->pNtk->vTopo, Abc_ObjId(pObj) );
        return;
    }
    assert( Abc_ObjIsNode(pObj) );
    // mark begining
    if ( fThisIsPivot )
        Abc_NtkTopoSetBeg( pObj );        
    // find fanout without topo
    Abc_ObjForEachFanout( pObj, pNext, i )
        if ( !Abc_NtkTopoHasBeg(pNext) )
        { 
            assert( !Abc_NtkTopoHasEnd(pNext) );
            Abc_NtkReverseTopoOrder_rec( pNext, 1 );
            pPivot = pNext;
            break;
        }
    Abc_ObjForEachFanout( pObj, pNext, i )
        if ( pNext != pPivot )
            Abc_NtkReverseTopoOrder_rec( pNext, 0 );
    // mark end
    if ( fThisIsPivot )
        Abc_NtkTopoSetEnd( pObj );
    // save current node        
    Vec_IntPush( pObj->pNtk->vTopo, Abc_ObjId(pObj) );
}
void Abc_NtkReverseTopoOrder( Abc_Ntk_t * p )
{
    Abc_Obj_t * pObj;
    int i;
    assert( p->vTopo == NULL );
    p->vTopo = Vec_IntAlloc( 10 * Abc_NtkObjNumMax(p) );
    Vec_IntFill( p->vTopo, 2 * Abc_NtkObjNumMax(p), 0 );
    Abc_NtkForEachNode( p, pObj, i )
    {
        if ( Abc_NtkTopoHasBeg(pObj) )
            continue;
        Abc_NtkIncrementTravId( p );        
        Abc_NtkReverseTopoOrder_rec( pObj, 1 );
    }
    printf( "Nodes = %d.   Size = %d.  Ratio = %f.\n", 
        Abc_NtkNodeNum(p), Vec_IntSize(p->vTopo), 1.0*Vec_IntSize(p->vTopo)/Abc_NtkNodeNum(p) );
}

void Abc_NtkReverse_rec( Abc_Obj_t * pObj, Vec_Int_t * vVisited )
{
    Abc_Obj_t * pNext;
    int i;
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    Abc_ObjForEachFanout( pObj, pNext, i )
        Abc_NtkReverse_rec( pNext, vVisited );
    Vec_IntPush( vVisited, Abc_ObjId(pObj) );
}
void Abc_NtkReverseTopoOrderTest( Abc_Ntk_t * p )
{
    Vec_Int_t * vVisited;
    Abc_Obj_t * pObj;
    int i;//, k, iBeg, iEnd;
    abctime clk = Abc_Clock();
    Abc_NtkReverseTopoOrder( p );
/*
    printf( "Reverse topological order for nodes:\n" );
    Abc_NtkForEachNode( p, pObj, i )
    {
        iBeg = Abc_NtkTopoHasBeg( pObj );
        iEnd = Abc_NtkTopoHasEnd( pObj );
        printf( "Node %4d : ", Abc_ObjId(pObj) );
        for ( k = iEnd - 1; k >= iBeg; k-- )
            printf( "%d ", Vec_IntEntry(p->vTopo, k) );
        printf( "\n" );
    }
*/
    Vec_IntFreeP( &p->vTopo );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    // compute regular fanout orders
    clk = Abc_Clock();
    vVisited = Vec_IntAlloc( 1000 );
    Abc_NtkForEachNode( p, pObj, i )
    {
        Vec_IntClear( vVisited );
        Abc_NtkIncrementTravId( p ); 
        Abc_NtkReverse_rec( pObj, vVisited );
    }
    Vec_IntFree( vVisited );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Converts multi-output PLA into an AIG with logic sharing.]

  Description [The first argument is an array of char*-strings representing
  individual output of a multi-output PLA. The number of inputs (nInputs) 
  and the number of outputs (nOutputs) are the second and third arguments. 
  This procedure returns the AIG manager with the given number of inputs 
  and outputs representing the PLA as a logic network with sharing.
  
  For example, if the original PLA is 
    1000 10
    0110 01
    0011 01
  the individual PLA for each the two outputs should be
    1000 1
  and
    0110 1
    0011 1

  Reprsentation in terms of two char*-strings will be:
    char * pPlas[2] = { "1000 1\n", "0110 1\n0011 1\n" };              
  The call to the procedure may look as follows:
    Abc_Ntk_t * pNtkAig = Abc_NtkFromPla( pPlas, 4, 2 );]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromPla( char ** pPlas, int nInputs, int nOutputs )
{
    Fxu_Data_t Params, * p = &Params;
    Abc_Ntk_t * pNtkSop, * pNtkAig; 
    Abc_Obj_t * pNode, * pFanin;
    int i, k;
    // allocate logic network with SOP local functions
    pNtkSop = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    pNtkSop->pName = Extra_FileNameGeneric("pla");
    // create primary inputs/outputs
    for ( i = 0; i < nInputs; i++ )
        Abc_NtkCreatePi( pNtkSop );
    for ( i = 0; i < nOutputs; i++ )
        Abc_NtkCreatePo( pNtkSop );
    Abc_NtkAddDummyPiNames( pNtkSop );
    Abc_NtkAddDummyPoNames( pNtkSop );
    // create internal nodes
    for ( i = 0; i < nOutputs; i++ )
    {
        pNode = Abc_NtkCreateNode( pNtkSop );
        Abc_NtkForEachPi( pNtkSop, pFanin, k )
            Abc_ObjAddFanin( pNode, pFanin );
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtkSop->pManFunc, pPlas[i] );
        Abc_ObjAddFanin( Abc_NtkPo(pNtkSop, i), pNode );
        // check that the number of inputs is the same
        assert( Abc_SopGetVarNum((char*)pNode->pData) == nInputs );
    }
    if ( !Abc_NtkCheck( pNtkSop ) )
        fprintf( stdout, "Abc_NtkFromPla(): Network check has failed.\n" );
    // perform fast_extract
    Abc_NtkSetDefaultFxParams( p );
    Abc_NtkFastExtract( pNtkSop, p );
    Abc_NtkFxuFreeInfo( p );
    // convert to an AIG
    pNtkAig = Abc_NtkStrash( pNtkSop, 0, 1, 0 );
    Abc_NtkDelete( pNtkSop );
    return pNtkAig;
}
void Abc_NtkFromPlaTest()
{
    char * pPlas[2] = { "1000 1\n", "0110 1\n0011 1\n" };
    Abc_Ntk_t * pNtkAig = Abc_NtkFromPla( pPlas, 4, 2 );
    Io_WriteBlifLogic( pNtkAig, "temp.blif", 0 );
    Abc_NtkDelete( pNtkAig );
}

/**Function*************************************************************

  Synopsis    [Checks if the logic network is in the topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkSplitSop( Abc_Ntk_t * pNtk, int nCubesMax, int fVerbose )
{
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pFanin, * pObjNew, * pObjNewRoot;
    int i, k, j, nCubes, nCubesThis, nSplits;
    char * pSopStr, * pSopStr2, * pTempSop, Symb;
    if ( pNtk == NULL )
        return NULL;
    assert( !Abc_NtkIsStrash(pNtk) && !Abc_NtkIsNetlist(pNtk) );
    // start the network
    pNtkNew = Abc_NtkStartFrom( pNtk, pNtk->ntkType, pNtk->ntkFunc );
    // copy the internal nodes
    vNodes = Abc_NtkDfs( pNtk, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        assert( Abc_ObjIsNode(pObj) );
        pObjNewRoot = Abc_NtkDupObj( pNtkNew, pObj, 0 );
        nCubes = Abc_SopGetCubeNum( (char *)pObj->pData );
        if ( nCubes <= nCubesMax )
        {            
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
            continue;
        }
        nSplits = (nCubes / nCubesMax) + (int)(nCubes % nCubesMax > 0);
        pSopStr = (char *)pObjNewRoot->pData;
        pObjNewRoot->pData = Abc_SopCreateOr((Mem_Flex_t *)pNtkNew->pManFunc, nSplits, NULL);
        if ( Abc_SopIsComplement(pSopStr) )
        {
            Abc_SopComplement( pSopStr );
            Abc_SopComplement( (char *)pObjNewRoot->pData );
        }
        pTempSop = (char *)pObj->pData; pObj->pData = (char *)"?";
        for ( j = 0; j < nSplits; j++ )
        {
            // clone the node
            pObjNew = Abc_NtkDupObj( pNtkNew, pObj, 0 );
            Abc_ObjAddFanin( pObjNewRoot, pObjNew );
            // get its cubes
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
            // create SOP for this node
            nCubesThis = (j < nCubes / nCubesMax) ? nCubesMax : nCubes % nCubesMax;
            pSopStr2 = pSopStr + (Abc_ObjFaninNum(pObj) + 3) * nCubesThis;
            Symb = *pSopStr2; *pSopStr2 = 0;
            pObjNew->pData = Abc_SopRegister( (Mem_Flex_t *)pNtkNew->pManFunc, pSopStr );
            *pSopStr2 = Symb;
            pSopStr = pSopStr2;
        }
        // update 
        pObj->pData = pTempSop;
        pObj->pCopy = pObjNewRoot;
    }
    Vec_PtrFree( vNodes );
    Abc_NtkFinalize( pNtk, pNtkNew );
    // check correctness
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkDup(): Network check has failed.\n" );
    pNtk->pCopy = pNtkNew;
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Checks if the logic network is in the topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkIsTopo( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pFanin;
    int i, k, Counter = 0;
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachCi( pNtk, pObj, i )
        Abc_NodeSetTravIdCurrent(pObj);
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        // check if fanins are in the topo order
        Abc_ObjForEachFanin( pObj, pFanin, k )
            if ( !Abc_NodeIsTravIdCurrent(pFanin) )
                break;
        if ( k != Abc_ObjFaninNum(pObj) )
        {
            if ( Counter++ == 0 )
                printf( "Node %d is out of topo order.\n", Abc_ObjId(pObj) );
        }
        Abc_NodeSetTravIdCurrent(pObj);
    }
    if ( Counter )
        printf( "Topological order does not hold for %d internal nodes.\n", Counter );
    return (int)(Counter == 0);
}

/**Function*************************************************************

  Synopsis    [Transfers phase information to the new network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTransferPhases( Abc_Ntk_t * pNtkNew, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    assert( pNtk->vPhases != NULL );
    assert( Vec_IntSize(pNtk->vPhases) == Abc_NtkObjNumMax(pNtk) );
    assert( pNtkNew->vPhases == NULL );
    pNtkNew->vPhases = Vec_IntStart( Abc_NtkObjNumMax(pNtkNew) );
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( pObj->pCopy && !Abc_ObjIsNone( (Abc_Obj_t *)pObj->pCopy ) )
            Vec_IntWriteEntry( pNtkNew->vPhases, Abc_ObjId( (Abc_Obj_t *)pObj->pCopy ), Vec_IntEntry(pNtk->vPhases, i) );
}

/**Function*************************************************************

  Synopsis    [Starts a new network using existing network as a model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDeriveWithOnePo( Abc_Ntk_t * pNtk, Vec_Int_t * vNodeIds, Vec_Int_t * vNodeValues )
{
    int fCopyNames = 1;
    Abc_Ntk_t * pNtkNew; 
    Abc_Obj_t * pObj, * pFanin, * pObjNew, * pOutputNew;
    Vec_Ptr_t * vFanins = Vec_PtrAlloc( 100 );
    int i, k, Id, Value;
    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    // clean the node copy fields
    Abc_NtkCleanCopy( pNtk );
    // map the constant nodes
    if ( Abc_NtkIsStrash(pNtk) && Abc_NtkIsStrash(pNtkNew) )
        Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    // clone CIs/CIs/boxes
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, fCopyNames );
    //Abc_NtkForEachPo( pNtk, pObj, i )
    //    Abc_NtkDupObj( pNtkNew, pObj, fCopyNames );
    // create one PO
    pObjNew = Abc_NtkCreateObj( pNtkNew, ABC_OBJ_PO );
    Abc_ObjAssignName( pObjNew, "monitor", NULL );
    // create boxes
    Abc_NtkForEachBox( pNtk, pObj, i )
        Abc_NtkDupBox( pNtkNew, pObj, fCopyNames );

    // duplicate nodes (CIs/COs/latches are already duplicated)
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( pObj->pCopy == NULL && !Abc_ObjIsPo(pObj) )
            Abc_NtkDupObj(pNtkNew, pObj, 0);
    // reconnect all objects except boxes (they are already connected) and POs (they will be connected later)
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsPo(pObj) && !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );

    // AND nodes (with interters if needed)
    pOutputNew = NULL;
    Vec_IntForEachEntryTwo( vNodeIds, vNodeValues, Id, Value, i )
    {
        pObjNew = Abc_NtkObj( pNtk, Id )->pCopy;
        if ( Value == 0 ) // negative polarity - add inverter
            pObjNew = Abc_NtkCreateNodeInv( pNtkNew, pObjNew );
        if ( pOutputNew == NULL )
            pOutputNew = pObjNew;
        else
        {
            Vec_PtrFillTwo( vFanins, 2, pOutputNew, pObjNew );
            pOutputNew = Abc_NtkCreateNodeAnd( pNtkNew, vFanins );
        }
    }
    Vec_PtrFree( vFanins );
    // create the only POs, which is the AND of the corresponding nodes
    Abc_ObjAddFanin( Abc_NtkPo(pNtkNew, 0), pOutputNew );

    // check that the CI/CO/latches are copied correctly
    assert( Abc_NtkPoNum(pNtkNew)    == 1 );
    assert( Abc_NtkCiNum(pNtkNew)    == Abc_NtkCiNum(pNtk) );
    assert( Abc_NtkLatchNum(pNtkNew) == Abc_NtkLatchNum(pNtk) );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Derives the AIG representing a property.]

  Description [Given is a sequential logic network (Abc_Ntk_t) and 
  an array of nodes (vector of object IDs) and their values (vector of 0s or 1s).
  This procedure creates a sequential AIG (Abc_Ntk_t), which can be given to a 
  sequential model checker (in particular "pdr") to prove that the given 
  combination of values never appears at the intenal nodes of the network, 
  or produce a counter-example showing that it can appear.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCreatePropertyMonitor( Abc_Ntk_t * p, Vec_Int_t * vNodeIds, Vec_Int_t * vNodeValues )
{
    Abc_Ntk_t * pMonitor, * pStrashed, * pResult;
    // sequential cleanup parameters
    int fLatchConst  =   1;
    int fLatchEqual  =   1;
    int fSaveNames   =   1;
    int fUseMvSweep  =   0;
    int nFramesSymb  =   1;
    int nFramesSatur = 512;
    int fVerbose     =   0;
    int fVeryVerbose =   0;
    // expecting sequential logic network
    assert( Abc_NtkIsLogic(p) );
    assert( Abc_NtkLatchNum(p) > 0 );
    assert( Vec_IntSize(vNodeIds) > 0 );
    assert( Vec_IntSize(vNodeIds) == Vec_IntSize(vNodeValues) );
    // derive ABC network whose only output is 1 iff the given nodes have the given values
    pMonitor = Abc_NtkDeriveWithOnePo( p, vNodeIds, vNodeValues );
    // perform structural hashing
    pStrashed = Abc_NtkStrash( pMonitor, 0, 1, 0 );
    Abc_NtkDelete( pMonitor );
    // it is a good idea to run sequential cleanup before giving the network to PDR
    pResult = Abc_NtkDarLatchSweep( pStrashed, fLatchConst, fLatchEqual, fSaveNames, fUseMvSweep, nFramesSymb, nFramesSatur, fVerbose, fVeryVerbose );
    Abc_NtkDelete( pStrashed );
    return pResult;
}

/**Function*************************************************************

  Synopsis    [Testbench.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCreatePropertyMonitorTest( Abc_Ntk_t * p )
{
    Abc_Ntk_t * pNtk;
    Vec_Int_t * vNodeIds = Vec_IntAlloc( 100 );
    Vec_Int_t * vNodeValues = Vec_IntAlloc( 100 );

    // this test will only work for the network, which has nodes with internal IDs such as these
    Vec_IntPush( vNodeIds, 90 );
    Vec_IntPush( vNodeIds, 80 );
    Vec_IntPush( vNodeIds, 100 );

    Vec_IntPush( vNodeValues, 1 );
    Vec_IntPush( vNodeValues, 0 );
    Vec_IntPush( vNodeValues, 1 );

    pNtk = Abc_NtkCreatePropertyMonitor( p, vNodeIds, vNodeValues );

    Vec_IntFree( vNodeIds );
    Vec_IntFree( vNodeValues );

    return pNtk;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_GateToType( Abc_Obj_t * pObj )
{
    char * pGateName = Mio_GateReadName((Mio_Gate_t *)pObj->pData);
    if ( !strncmp(pGateName, "buf",  3) )  return ABC_OPER_BIT_BUF;
    if ( !strncmp(pGateName, "inv",  3) )  return ABC_OPER_BIT_INV;
    if ( !strncmp(pGateName, "and",  3) )  return ABC_OPER_BIT_AND;
    if ( !strncmp(pGateName, "nand", 4) )  return ABC_OPER_BIT_NAND;
    if ( !strncmp(pGateName, "or",   2) )  return ABC_OPER_BIT_OR;
    if ( !strncmp(pGateName, "nor",  3) )  return ABC_OPER_BIT_NOR;
    if ( !strncmp(pGateName, "xor",  3) )  return ABC_OPER_BIT_XOR;
    if ( !strncmp(pGateName, "xnor", 4) )  return ABC_OPER_BIT_NXOR;
    if ( !strncmp(pGateName, "zero", 4) )  return ABC_OPER_CONST_F;
    if ( !strncmp(pGateName, "one",  3) )  return ABC_OPER_CONST_T;
    assert( 0 );
    return -1;
}
Vec_Wec_t * Abc_SopSynthesize( Vec_Ptr_t * vSops )
{
    Vec_Wec_t * vRes = NULL;
    Abc_Ntk_t * pNtk = Abc_NtkCreateFromSops( "top", vSops );
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pFanin;
    int i, k, iNode = 0;
    Abc_FrameReplaceCurrentNetwork( Abc_FrameReadGlobalFrame(), pNtk );
    //Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "fx; strash; balance; dc2; map -a" );
    Abc_FrameSetBatchMode( 1 );
    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "st; collapse; sop; fx; strash; &get; &ps; &deepsyn -I 4 -J 50 -T 5 -S 111 -t; &ps; &put; map -a" );
    Abc_FrameSetBatchMode( 0 );
    pNtkNew = Abc_FrameReadNtk( Abc_FrameReadGlobalFrame() );
    vRes = Vec_WecStart( Abc_NtkPiNum(pNtkNew) + Abc_NtkNodeNum(pNtkNew) + Abc_NtkPoNum(pNtkNew) );
    Abc_NtkForEachPi( pNtkNew, pObj, i )
        pObj->iTemp = iNode++;
    Abc_NtkForEachNode( pNtkNew, pObj, i )
    {
        Vec_Int_t * vNode = Vec_WecEntry(vRes, iNode);
        Vec_IntPush( vNode, Abc_GateToType(pObj) );
        Vec_IntPush( vNode, iNode );
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Vec_IntPush( vNode, pFanin->iTemp );
        pObj->iTemp = iNode++;
    }
    Abc_NtkForEachPo( pNtkNew, pObj, i )
        Vec_IntPushTwo( Vec_WecEntry(vRes, iNode++), ABC_OPER_BIT_BUF, Abc_ObjFanin0(pObj)->iTemp );
    assert( Vec_WecSize(vRes) == iNode );
    return vRes;
}
Vec_Wec_t * Abc_GiaSynthesize( Vec_Ptr_t * vGias, Gia_Man_t * pMulti )
{
    Vec_Wec_t * vRes = NULL;
    Abc_Ntk_t * pNtk = Abc_NtkCreateFromGias( "top", vGias, pMulti );
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pFanin;
    int i, k, iNode = 0;
    Abc_FrameReplaceCurrentNetwork( Abc_FrameReadGlobalFrame(), pNtk );
    Abc_FrameSetBatchMode( 1 );
    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "clp; sop; fx; strash; compress2rs; dch; map -a;  strash; compress2rs; dch; map -a" );
    Abc_FrameSetBatchMode( 0 );
    pNtkNew = Abc_FrameReadNtk( Abc_FrameReadGlobalFrame() );
    vRes = Vec_WecStart( Abc_NtkPiNum(pNtkNew) + Abc_NtkNodeNum(pNtkNew) + Abc_NtkPoNum(pNtkNew) );
    Abc_NtkForEachPi( pNtkNew, pObj, i )
        pObj->iTemp = iNode++;
    Abc_NtkForEachNode( pNtkNew, pObj, i )
    {
        Vec_Int_t * vNode = Vec_WecEntry(vRes, iNode);
        Vec_IntPush( vNode, Abc_GateToType(pObj) );
        Vec_IntPush( vNode, iNode );
        Abc_ObjForEachFanin( pObj, pFanin, k )
            Vec_IntPush( vNode, pFanin->iTemp );
        pObj->iTemp = iNode++;
    }
    Abc_NtkForEachPo( pNtkNew, pObj, i )
        Vec_IntPushTwo( Vec_WecEntry(vRes, iNode++), ABC_OPER_BIT_BUF, Abc_ObjFanin0(pObj)->iTemp );
    assert( Vec_WecSize(vRes) == iNode );
    return vRes;
}
Gia_Man_t * Abc_GiaSynthesizeInter( Gia_Man_t * p )
{
    Abc_Ntk_t * pNtkNew, * pNtk;
    Vec_Ptr_t * vGias = Vec_PtrAlloc( 1 );
    Vec_PtrPush( vGias, p );
    pNtk = Abc_NtkCreateFromGias( "top", vGias, NULL );
    Vec_PtrFree( vGias );
    Abc_FrameReplaceCurrentNetwork( Abc_FrameReadGlobalFrame(), pNtk );
    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "balance; collapse; muxes; strash; dc2" );
    pNtkNew = Abc_FrameReadNtk( Abc_FrameReadGlobalFrame() );
    return Abc_NtkClpGia( pNtkNew );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkClpOneGia_rec( Gia_Man_t * pNew, Abc_Obj_t * pNode )
{
    int iLit0, iLit1;
    if ( Abc_NodeIsTravIdCurrent(pNode) || Abc_ObjFaninNum(pNode) == 0 || Abc_ObjIsCi(pNode) )
        return pNode->iTemp;
    assert( Abc_ObjIsNode( pNode ) );
    Abc_NodeSetTravIdCurrent( pNode );
    iLit0 = Abc_NtkClpOneGia_rec( pNew, Abc_ObjFanin0(pNode) );
    iLit1 = Abc_NtkClpOneGia_rec( pNew, Abc_ObjFanin1(pNode) );
    iLit0 = Abc_LitNotCond( iLit0, Abc_ObjFaninC0(pNode) );
    iLit1 = Abc_LitNotCond( iLit1, Abc_ObjFaninC1(pNode) );
    return (pNode->iTemp = Gia_ManHashAnd(pNew, iLit0, iLit1));
}
Gia_Man_t * Abc_NtkStrashToGia( Abc_Ntk_t * pNtk )
{
    int i, iLit;
    Abc_Obj_t * pNode;
    Gia_Man_t * pNew, * pTemp;
    assert( Abc_NtkIsStrash(pNtk) );
    Abc_NtkForEachObj( pNtk, pNode, i )
        pNode->iTemp = -1;
    // start new manager
    pNew = Gia_ManStart( Abc_NtkObjNum(pNtk) );
    pNew->pName = Abc_UtilStrsav( pNtk->pName );
    pNew->pSpec = Abc_UtilStrsav( pNtk->pSpec );
    Gia_ManHashStart( pNew );
    // primary inputs
    Abc_AigConst1(pNtk)->iTemp = 1;
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->iTemp = Gia_ManAppendCi(pNew);
    // create the first cone
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        iLit = Abc_NtkClpOneGia_rec( pNew, Abc_ObjFanin0(pNode) );
        iLit = Abc_LitNotCond( iLit, Abc_ObjFaninC0(pNode) );
        Gia_ManAppendCo( pNew, iLit );
    }
    // perform cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
Gia_Man_t * Abc_SopSynthesizeOne( char * pSop, int fClp )
{
    Abc_Ntk_t * pNtkNew, * pNtk;
    Vec_Ptr_t * vSops;
    if ( strlen(pSop) == 3 )
    {
        Gia_Man_t * pNew = Gia_ManStart( 1 );
        pNew->pName = Abc_UtilStrsav( "top" );
        //Gia_ManAppendCi( pNew );
        assert( pSop[1] == '0' || pSop[1] == '1' );
        Gia_ManAppendCo( pNew, pSop[1] == '1' );
        return pNew;
    }
    vSops = Vec_PtrAlloc( 1 );
    Vec_PtrPush( vSops, pSop );
    pNtk = Abc_NtkCreateFromSops( "top", vSops );
    Vec_PtrFree( vSops );
    Abc_FrameReplaceCurrentNetwork( Abc_FrameReadGlobalFrame(), pNtk );
    Abc_FrameSetBatchMode( 1 );
    if ( fClp ) 
    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "clp; sop" );
    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), "fx; strash; balance; dc2" );
    Abc_FrameSetBatchMode( 0 );
    pNtkNew = Abc_FrameReadNtk( Abc_FrameReadGlobalFrame() );
    return Abc_NtkStrashToGia( pNtkNew );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int s_ArraySize = 145;
static int s_ArrayData[290] = {
    0, 0,
    0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
    10, 6,  14, 12,  10, 2,  22, 20,  2, 24,  16, 4,  28, 18,  16, 10,  8, 4,  34, 32,  30, 36,  38, 26,  16, 6,  36, 20,  44, 42,  46, 40,  42, 44,  14, 6,  52, 34,  32, 54,  56, 50,  58, 48,  32, 24,  20, 2,  12, 6,  66, 34,  68, 64,  62, 70,  28, 68,  74, 72,  76, 58,  70, 62,  80, 78,  68, 28,  84, 74,  4, 2,  88, 20,  64, 90,  92, 86,  66, 32,  18, 96,  98, 56,  100, 94,  52, 36,  104, 38,  90, 42,  36, 2,  108, 110,  112, 106,  114, 100,  102, 116,  118, 82,  116, 60,  120, 122,  124, 60,  118, 60,  102, 82,  128, 130,  132, 82,  134, 126,  82, 116,  122, 138,  122, 118,  142, 140,  60, 102,  130, 146,  130, 118,  150, 148,  152, 144,  154, 136,  18, 156,  144, 126,  68, 160,  32, 136,  164, 162,  166, 158,  28, 160,  70, 126,  90, 144,  174, 172,  176, 170,  152, 134,  36, 180,  2, 134,  184, 182,  186, 178,  188, 168,  64, 144,  164, 158,  194, 192,  96, 156,  44, 154,  200, 170,  202, 198,  204, 176,  206, 196,  204, 168,  62, 126,  212, 186,  24, 134,  108, 152,  218, 192,  220, 216,  222, 214,  224, 210,  220, 194,  110, 152,  30, 180,  232, 230,  184, 172,  236, 234,  238, 228,  234, 182,  242, 220,  244, 168,  42, 154,  248, 202,  54, 136,  252, 164,  254, 214,  256, 250,  218, 194,  252, 198,  262, 242,  264, 260,  232, 220,  268, 262,  270, 168,
    191, 191,  209, 209,  226, 226,  240, 240,  246, 246,  259, 259,  267, 267,  272, 272,
};
int Abc_NtkHasConstNode()
{
    int i;
    for ( i = 1; i < s_ArraySize; i++ )
        if ( s_ArrayData[2*i] || s_ArrayData[2*i+1] )
            break;
    for ( ; i < s_ArraySize; i++ )
        if ( s_ArrayData[2*i] < 2 && s_ArrayData[2*i+1] < 2 )
            return 1;
    return 0;
}
Abc_Ntk_t * Abc_NtkFromArray()
{
    Vec_Ptr_t * vNodes  = Vec_PtrAlloc( s_ArraySize ); int i, nPos = 0; 
    Abc_Ntk_t * pNtkNew = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    Abc_Obj_t * pObjNew = Abc_NtkHasConstNode() ? Abc_NtkCreateNode(pNtkNew) : NULL; 
    if ( pObjNew ) pObjNew->pData = Abc_SopCreateConst0((Mem_Flex_t *)pNtkNew->pManFunc);
    Vec_PtrPush( vNodes, pObjNew );
    for ( i = 1; i < s_ArraySize; i++ )
        if ( !s_ArrayData[2*i] && !s_ArrayData[2*i+1] )
            Vec_PtrPush( vNodes, Abc_NtkCreatePi(pNtkNew) );
        else
            break;
    for ( ; i < s_ArraySize; i++ )
    {
        char * pSop = NULL;
        if ( s_ArrayData[2*i] > s_ArrayData[2*i+1] )
            pSop = Abc_SopCreateXor( (Mem_Flex_t *)pNtkNew->pManFunc, 2 );
        else if ( s_ArrayData[2*i] < s_ArrayData[2*i+1] )
            pSop = Abc_SopCreateAnd( (Mem_Flex_t *)pNtkNew->pManFunc, 2, NULL );
        else
            break;
        pObjNew = Abc_NtkCreateNode( pNtkNew );
        Abc_ObjAddFanin( pObjNew, (Abc_Obj_t *)Vec_PtrEntry(vNodes, Abc_Lit2Var(s_ArrayData[2*i]))   );
        Abc_ObjAddFanin( pObjNew, (Abc_Obj_t *)Vec_PtrEntry(vNodes, Abc_Lit2Var(s_ArrayData[2*i+1])) );
        if ( Abc_LitIsCompl(s_ArrayData[2*i])   )  Abc_SopComplementVar( pSop, 0 );
        if ( Abc_LitIsCompl(s_ArrayData[2*i+1]) )  Abc_SopComplementVar( pSop, 1 );
        pObjNew->pData = pSop;
        Vec_PtrPush( vNodes, pObjNew );
    }
    for ( ; i < s_ArraySize; i++ )
    {
        char * pSop = NULL;
        assert( s_ArrayData[2*i] == s_ArrayData[2*i+1] );
        pObjNew = Abc_NtkCreateNode( pNtkNew );
        Abc_ObjAddFanin( pObjNew, (Abc_Obj_t *)Vec_PtrEntry(vNodes, Abc_Lit2Var(s_ArrayData[2*i]))   );
        if ( Abc_LitIsCompl(s_ArrayData[2*i]) )
            pSop = Abc_SopCreateInv( (Mem_Flex_t *)pNtkNew->pManFunc );
        else
            pSop = Abc_SopCreateBuf( (Mem_Flex_t *)pNtkNew->pManFunc );
        pObjNew->pData = pSop;
        Vec_PtrPush( vNodes, pObjNew );
        nPos++;
    }
    for ( i = 0; i < nPos; i++ )
        Abc_ObjAddFanin( Abc_NtkCreatePo(pNtkNew), (Abc_Obj_t *)Vec_PtrEntry(vNodes, s_ArraySize-nPos+i) );
    Vec_PtrFree( vNodes );
    pNtkNew->pName = Extra_UtilStrsav("test");
    Abc_NtkAddDummyPiNames( pNtkNew );
    Abc_NtkAddDummyPoNames( pNtkNew );
    Abc_NtkAddDummyBoxNames( pNtkNew );
    if ( !Abc_NtkCheck( pNtkNew ) )
        Abc_Print( 1, "Abc_NtkFromArray(): Network check has failed.\n" );
    return pNtkNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

