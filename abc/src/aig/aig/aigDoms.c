/**CFile****************************************************************

  FileName    [aigDoms.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Computing multi-output dominators.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigDoms.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"
#include "aig/saig/saig.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Aig_Sto_t_ Aig_Sto_t;
typedef struct Aig_Dom_t_ Aig_Dom_t;

struct Aig_Dom_t_
{
    int             uSign;         // signature
    int             nNodes;        // the number of nodes
    int             pNodes[0];     // the nodes
};

struct Aig_Sto_t_
{
    int             Limit;
    Aig_Man_t *     pAig;          // user's AIG
    Aig_MmFixed_t * pMem;          // memory manager for dominators
    Vec_Ptr_t *     vDoms;         // dominators
    Vec_Int_t *     vFans;         // temporary fanouts
    Vec_Int_t *     vTimes;        // the number of times each appears
    int             nDomNodes;     // nodes with dominators
    int             nDomsTotal;    // total dominators
    int             nDomsFilter1;  // filtered dominators
    int             nDomsFilter2;  // filtered dominators
};

#define Aig_DomForEachNode( pAig, pDom, pNode, i )                         \
    for ( i = 0; (i < pDom->nNodes) && ((pNode) = Aig_ManObj(pAig, (pDom)->pNodes[i])); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates dominator manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Sto_t * Aig_ManDomStart( Aig_Man_t * pAig, int Limit )
{
    Aig_Sto_t * pSto;
    pSto = ABC_CALLOC( Aig_Sto_t, 1 );
    pSto->pAig     = pAig;
    pSto->Limit    = Limit;
    pSto->pMem     = Aig_MmFixedStart( sizeof(Aig_Dom_t) + sizeof(int) * Limit, 10000 );
    pSto->vDoms    = Vec_PtrStart( Aig_ManObjNumMax(pAig) );
    pSto->vFans    = Vec_IntAlloc( 100 );
    pSto->vTimes   = Vec_IntStart( Aig_ManObjNumMax(pAig) );
    return pSto;
}

/**Function*************************************************************

  Synopsis    [Adds trivial dominator.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjAddTriv( Aig_Sto_t * pSto, int Id, Vec_Ptr_t * vDoms )
{
    Aig_Dom_t * pDom;
    pDom = (Aig_Dom_t *)Aig_MmFixedEntryFetch( pSto->pMem );
    pDom->uSign     = (1 << (Id % 63));
    pDom->nNodes    = 1;
    pDom->pNodes[0] = Id;
    Vec_PtrPushFirst( vDoms, pDom );
    assert( Vec_PtrEntry( pSto->vDoms, Id ) == NULL );
    Vec_PtrWriteEntry( pSto->vDoms, Id, vDoms );
}

/**Function*************************************************************

  Synopsis    [Duplicates vector of doms.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ObjDomVecDup( Aig_Sto_t * pSto, Vec_Ptr_t * vDoms, int fSkip1 )
{
    Vec_Ptr_t * vDoms2;
    Aig_Dom_t * pDom, * pDom2;
    int i;
    vDoms2 = Vec_PtrAlloc( 0 );
    Vec_PtrForEachEntryStart( Aig_Dom_t *, vDoms, pDom, i, fSkip1 )
    {
        pDom2 = (Aig_Dom_t *)Aig_MmFixedEntryFetch( pSto->pMem );
        memcpy( pDom2, pDom, sizeof(Aig_Dom_t) + sizeof(int) * pSto->Limit );
        Vec_PtrPush( vDoms2, pDom2 );
    }
    return vDoms2;
}

/**Function*************************************************************

  Synopsis    [Recycles vector of doms.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjDomVecRecycle( Aig_Sto_t * pSto, Vec_Ptr_t * vDoms )
{
    Aig_Dom_t * pDom;
    int i;
    Vec_PtrForEachEntry( Aig_Dom_t *, vDoms, pDom, i )
        Aig_MmFixedEntryRecycle( pSto->pMem, (char *)pDom );
    Vec_PtrFree( vDoms );
}

/**Function*************************************************************

  Synopsis    [Duplicates the vector of doms.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjDomPrint( Aig_Sto_t * pSto, Aig_Dom_t * pDom, int Num )
{
    int k;
    printf( "%4d : {", Num );
    for ( k = 0; k < pDom->nNodes; k++ )
        printf( " %4d", pDom->pNodes[k] );
    for ( ; k < pSto->Limit; k++ )
        printf( "     " );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    [Duplicates the vector of doms.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjDomVecPrint( Aig_Sto_t * pSto, Vec_Ptr_t * vDoms )
{
    Aig_Dom_t * pDom;
    int i;
    Vec_PtrForEachEntry( Aig_Dom_t *, vDoms, pDom, i )
        Aig_ObjDomPrint( pSto, pDom, i );
}

/**Function*************************************************************

  Synopsis    [Computes multi-node dominators.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDomPrint( Aig_Sto_t * pSto )
{
    Aig_Obj_t * pObj;
    int i;
    Saig_ManForEachPi( pSto->pAig, pObj, i )
    {
        printf( "*** PI %4d %4d :\n", i, pObj->Id );
        Aig_ObjDomVecPrint( pSto, (Vec_Ptr_t *)Vec_PtrEntry(pSto->vDoms, pObj->Id) );
    }
}

/**Function*************************************************************

  Synopsis    [Divides the circuit into well-balanced parts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDomStop( Aig_Sto_t * pSto )
{
    Vec_Ptr_t * vDoms;
    int i;
    Vec_PtrForEachEntry( Vec_Ptr_t *, pSto->vDoms, vDoms, i )
        if ( vDoms )
            Aig_ObjDomVecRecycle( pSto, vDoms );
    Vec_PtrFree( pSto->vDoms );
    Vec_IntFree( pSto->vFans );
    Vec_IntFree( pSto->vTimes );
    Aig_MmFixedStop( pSto->pMem, 0 );
    ABC_FREE( pSto );
}


/**Function*************************************************************

  Synopsis    [Checks correctness of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjDomCheck( Aig_Dom_t * pDom )
{
    int i;
    for ( i = 1; i < pDom->nNodes; i++ )
    {
        if ( pDom->pNodes[i-1] >= pDom->pNodes[i] )
        {
            Abc_Print( -1, "Aig_ObjDomCheck(): Cut has wrong ordering of inputs.\n" );
            return 0;
        }
        assert( pDom->pNodes[i-1] < pDom->pNodes[i] );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pDom is contained in pCut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_ObjDomCheckDominance( Aig_Dom_t * pDom, Aig_Dom_t * pCut )
{
    int i, k;
    for ( i = 0; i < pDom->nNodes; i++ )
    {
        for ( k = 0; k < (int)pCut->nNodes; k++ )
            if ( pDom->pNodes[i] == pCut->pNodes[k] )
                break;
        if ( k == (int)pCut->nNodes ) // node i in pDom is not contained in pCut
            return 0;
    }
    // every node in pDom is contained in pCut
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the cut is contained.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjDomFilter( Aig_Sto_t * pSto, Vec_Ptr_t * vDoms, Aig_Dom_t * pDom )
{ 
    Aig_Dom_t * pTemp;
    int i;
    Vec_PtrForEachEntry( Aig_Dom_t *, vDoms, pTemp, i )
    {
        if ( pTemp->nNodes > pDom->nNodes )
        {
            // do not fiter the first cut
            if ( i == 0 )
                continue;
            // skip the non-contained cuts
            if ( (pTemp->uSign & pDom->uSign) != pDom->uSign )
                continue;
            // check containment seriously
            if ( Aig_ObjDomCheckDominance( pDom, pTemp ) )
            {
                Vec_PtrRemove( vDoms, pTemp );
                Aig_MmFixedEntryRecycle( pSto->pMem, (char *)pTemp );
                i--;
                pSto->nDomsFilter1++;
            }
         }
        else
        {
            // skip the non-contained cuts
            if ( (pTemp->uSign & pDom->uSign) != pTemp->uSign )
                continue;
            // check containment seriously
            if ( Aig_ObjDomCheckDominance( pTemp, pDom ) )
            {
                pSto->nDomsFilter2++;
                return 1;
            }
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Merges two cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_ObjDomMergeOrdered( Aig_Dom_t * pD0, Aig_Dom_t * pD1, Aig_Dom_t * pD, int Limit )
{ 
    int i, k, c;
    assert( pD0->nNodes >= pD1->nNodes );
    // the case of the largest cut sizes
    if ( pD0->nNodes == Limit && pD1->nNodes == Limit )
    {
        for ( i = 0; i < pD0->nNodes; i++ )
            if ( pD0->pNodes[i] != pD1->pNodes[i] )
                return 0;
        for ( i = 0; i < pD0->nNodes; i++ )
            pD->pNodes[i] = pD0->pNodes[i];
        pD->nNodes = pD0->nNodes;
        return 1;
    }
    // the case when one of the cuts is the largest
    if ( pD0->nNodes == Limit )
    {
        for ( i = 0; i < pD1->nNodes; i++ )
        {
            for ( k = pD0->nNodes - 1; k >= 0; k-- )
                if ( pD0->pNodes[k] == pD1->pNodes[i] )
                    break;
            if ( k == -1 ) // did not find
                return 0;
        }
        for ( i = 0; i < pD0->nNodes; i++ )
            pD->pNodes[i] = pD0->pNodes[i];
        pD->nNodes = pD0->nNodes;
        return 1;
    }

    // compare two cuts with different numbers
    i = k = 0;
    for ( c = 0; c < (int)Limit; c++ )
    {
        if ( k == pD1->nNodes )
        {
            if ( i == pD0->nNodes )
            {
                pD->nNodes = c;
                return 1;
            }
            pD->pNodes[c] = pD0->pNodes[i++];
            continue;
        }
        if ( i == pD0->nNodes )
        {
            if ( k == pD1->nNodes )
            {
                pD->nNodes = c;
                return 1;
            }
            pD->pNodes[c] = pD1->pNodes[k++];
            continue;
        }
        if ( pD0->pNodes[i] < pD1->pNodes[k] )
        {
            pD->pNodes[c] = pD0->pNodes[i++];
            continue;
        }
        if ( pD0->pNodes[i] > pD1->pNodes[k] )
        {
            pD->pNodes[c] = pD1->pNodes[k++];
            continue;
        }
        pD->pNodes[c] = pD0->pNodes[i++]; 
        k++;
    }
    if ( i < pD0->nNodes || k < pD1->nNodes )
        return 0;
    pD->nNodes = c;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prepares the object for FPGA mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjDomMergeTwo( Aig_Dom_t * pDom0, Aig_Dom_t * pDom1, Aig_Dom_t * pDom, int Limit )
{ 
    assert( Limit > 0 );
    if ( pDom0->nNodes < pDom1->nNodes )
    {
        if ( !Aig_ObjDomMergeOrdered( pDom1, pDom0, pDom, Limit ) )
            return 0;
    }
    else
    {
        if ( !Aig_ObjDomMergeOrdered( pDom0, pDom1, pDom, Limit ) )
            return 0;
    }
    pDom->uSign = pDom0->uSign | pDom1->uSign;
    assert( pDom->nNodes <= Limit );
    assert( Aig_ObjDomCheck( pDom ) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Merge two arrays of dominators.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ObjDomMerge( Aig_Sto_t * pSto, Vec_Ptr_t * vDoms0, Vec_Ptr_t * vDoms1 )
{
    Vec_Ptr_t * vDoms;
    Aig_Dom_t * pDom0, * pDom1, * pDom;
    int i, k;
    vDoms = Vec_PtrAlloc( 16 );
    Vec_PtrForEachEntry( Aig_Dom_t *, vDoms0, pDom0, i )
    Vec_PtrForEachEntry( Aig_Dom_t *, vDoms1, pDom1, k )
    {
        if ( Aig_WordCountOnes( pDom0->uSign | pDom1->uSign ) > pSto->Limit )
            continue;
        // check if the cut exists
        pDom = (Aig_Dom_t *)Aig_MmFixedEntryFetch( pSto->pMem );
        if ( !Aig_ObjDomMergeTwo( pDom0, pDom1, pDom, pSto->Limit ) )
        {
            Aig_MmFixedEntryRecycle( pSto->pMem, (char *)pDom );
            continue;
        }
        // check if this cut is contained in any of the available cuts
        if ( Aig_ObjDomFilter( pSto, vDoms, pDom ) )
        {
            Aig_MmFixedEntryRecycle( pSto->pMem, (char *)pDom );
            continue;
        }
        Vec_PtrPush( vDoms, pDom );
    }
    return vDoms;
}

/**Function*************************************************************

  Synopsis    [Union two arrays of dominators.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjDomUnion( Aig_Sto_t * pSto, Vec_Ptr_t * vDoms2, Vec_Ptr_t * vDoms1 )
{
    Aig_Dom_t * pDom1, * pDom2;
    int i;
    Vec_PtrForEachEntry( Aig_Dom_t *, vDoms1, pDom1, i )
    {
        if ( i == 0 )
            continue;
        if ( Aig_ObjDomFilter( pSto, vDoms2, pDom1 ) )
            continue;
        pDom2 = (Aig_Dom_t *)Aig_MmFixedEntryFetch( pSto->pMem );
        memcpy( pDom2, pDom1, sizeof(Aig_Dom_t) + sizeof(int) * pSto->Limit );
        Vec_PtrPush( vDoms2, pDom2 );
    }
}


/**Function*************************************************************

  Synopsis    [Computes multi-node dominators.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjDomCompute( Aig_Sto_t * pSto, Aig_Obj_t * pObj )
{
    Vec_Ptr_t * vDoms0, * vDoms1, * vDoms2, * vDomsT;
    Aig_Obj_t * pFanout;
    int i, iFanout;
    pSto->nDomNodes += Aig_ObjIsNode(pObj);
    Vec_IntClear( pSto->vFans );
    Aig_ObjForEachFanout( pSto->pAig, pObj, pFanout, iFanout, i )
        if ( Aig_ObjIsTravIdCurrent(pSto->pAig, pFanout) )
            Vec_IntPush( pSto->vFans, iFanout>>1 );
    if ( Vec_IntSize(pSto->vFans) == 0 )
        return;
    vDoms0 = (Vec_Ptr_t *)Vec_PtrEntry( pSto->vDoms, Vec_IntEntry(pSto->vFans, 0) );
    vDoms2 = Aig_ObjDomVecDup( pSto, vDoms0, 0 );
    Vec_IntForEachEntryStart( pSto->vFans, iFanout, i, 1 )
    {
        vDoms1 = (Vec_Ptr_t *)Vec_PtrEntry( pSto->vDoms, iFanout );
        vDoms2 = Aig_ObjDomMerge( pSto, vDomsT = vDoms2, vDoms1 );
        Aig_ObjDomVecRecycle( pSto, vDomsT );
    }
    Aig_ObjAddTriv( pSto, pObj->Id, vDoms2 );
    pSto->nDomsTotal += Vec_PtrSize(vDoms2);
}

/**Function*************************************************************

  Synopsis    [Marks the flop TFI with the current traversal ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManMarkFlopTfi_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    int Count;
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) )
        return 1;
    Count = Aig_ManMarkFlopTfi_rec( p, Aig_ObjFanin0(pObj) );
    if ( Aig_ObjIsNode(pObj) )
        Count += Aig_ManMarkFlopTfi_rec( p, Aig_ObjFanin1(pObj) );
    return Count;
}

/**Function*************************************************************

  Synopsis    [Marks the flop TFI with the current traversal ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManMarkFlopTfi( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManIncrementTravId( p );
    Saig_ManForEachLi( p, pObj, i )
        Aig_ManMarkFlopTfi_rec( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Computes multi-node dominators.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Sto_t * Aig_ManComputeDomsFlops( Aig_Man_t * pAig, int Limit )
{
    Aig_Sto_t * pSto;
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    abctime clk = Abc_Clock();
    pSto = Aig_ManDomStart( pAig, Limit );
    // initialize flop inputs
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjAddTriv( pSto, pObj->Id, Vec_PtrAlloc(1) );
    // compute internal nodes
    vNodes = Aig_ManDfsReverse( pAig );
    Aig_ManMarkFlopTfi( pAig );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        if ( Aig_ObjIsTravIdCurrent(pSto->pAig, pObj) )
            Aig_ObjDomCompute( pSto, pObj );
    Vec_PtrFree( vNodes );
    // compute combinational inputs
    Aig_ManForEachCi( pAig, pObj, i )
        Aig_ObjDomCompute( pSto, pObj );
    // print statistics
    printf( "Nodes =%4d. Flops =%4d. Doms =%9d. Ave =%8.2f.   ", 
        pSto->nDomNodes, Aig_ManRegNum(pSto->pAig), pSto->nDomsTotal, 
//        pSto->nDomsFilter1, pSto->nDomsFilter2,
        1.0 * pSto->nDomsTotal / (pSto->nDomNodes + Aig_ManRegNum(pSto->pAig)) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return pSto;
}

/**Function*************************************************************

  Synopsis    [Computes multi-node dominators.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Sto_t * Aig_ManComputeDomsPis( Aig_Man_t * pAig, int Limit )
{
    Aig_Sto_t * pSto;
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    abctime clk = Abc_Clock();
    pSto = Aig_ManDomStart( pAig, Limit );
    // initialize flop inputs
    Aig_ManForEachCo( pAig, pObj, i )
        Aig_ObjAddTriv( pSto, pObj->Id, Vec_PtrAlloc(1) );
    // compute internal nodes
    vNodes = Aig_ManDfsReverse( pAig );
    Aig_ManMarkFlopTfi( pAig );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        if ( Aig_ObjIsTravIdCurrent(pSto->pAig, pObj) )
            Aig_ObjDomCompute( pSto, pObj );
    Vec_PtrFree( vNodes );
    // compute combinational inputs
    Saig_ManForEachPi( pAig, pObj, i )
        Aig_ObjDomCompute( pSto, pObj );
    // print statistics
    printf( "Nodes =%4d. PIs =%4d. Doms =%9d. Ave =%8.2f.   ", 
        pSto->nDomNodes, Saig_ManPiNum(pSto->pAig), pSto->nDomsTotal, 
//        pSto->nDomsFilter1, pSto->nDomsFilter2,
        1.0 * pSto->nDomsTotal / (pSto->nDomNodes + Saig_ManPiNum(pSto->pAig)) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return pSto;
}

/**Function*************************************************************

  Synopsis    [Computes multi-node dominators.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Sto_t * Aig_ManComputeDomsNodes( Aig_Man_t * pAig, int Limit )
{
    Aig_Sto_t * pSto;
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    abctime clk = Abc_Clock();
    pSto = Aig_ManDomStart( pAig, Limit );
    // initialize flop inputs
    Aig_ManForEachCo( pAig, pObj, i )
        Aig_ObjAddTriv( pSto, pObj->Id, Vec_PtrAlloc(1) );
    // compute internal nodes
    vNodes = Aig_ManDfsReverse( pAig );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        Aig_ObjDomCompute( pSto, pObj );
    Vec_PtrFree( vNodes );
    // compute combinational inputs
    Aig_ManForEachCi( pAig, pObj, i )
        Aig_ObjDomCompute( pSto, pObj );
    // print statistics
    printf( "Nodes =%6d. Doms =%9d. Ave =%8.2f.   ", 
        pSto->nDomNodes, pSto->nDomsTotal, 
//        pSto->nDomsFilter1, pSto->nDomsFilter2,
        1.0 * pSto->nDomsTotal / pSto->nDomNodes );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return pSto;
}

/**Function*************************************************************

  Synopsis    [Collects dominators from the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ObjDomCollect( Aig_Sto_t * pSto, Vec_Int_t * vCut )
{
    Vec_Ptr_t * vDoms0, * vDoms1, * vDoms2;
    int i, ObjId;
    vDoms0 = (Vec_Ptr_t *)Vec_PtrEntry( pSto->vDoms, Vec_IntEntry(vCut, 0) );
    vDoms2 = Aig_ObjDomVecDup( pSto, vDoms0, 1 );
    Vec_IntForEachEntryStart( vCut, ObjId, i, 1 )
    {
        vDoms1 = (Vec_Ptr_t *)Vec_PtrEntry( pSto->vDoms, ObjId );
        if ( vDoms1 == NULL )
            continue;
        Aig_ObjDomUnion( pSto, vDoms2, vDoms1 );
    }
    return vDoms2;
}


/**Function*************************************************************

  Synopsis    [Marks the flop TFI with the current traversal ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjDomVolume_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    int Count;
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( pObj->fMarkA )
        return 1;
//    assert( !Aig_ObjIsCi(pObj) && !Aig_ObjIsConst1(pObj) );
    if ( Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) )
        return 1;
    Count = Aig_ObjDomVolume_rec( p, Aig_ObjFanin0(pObj) );
    if ( Aig_ObjIsNode(pObj) )
        Count += Aig_ObjDomVolume_rec( p, Aig_ObjFanin1(pObj) );
    return Count;
}

/**Function*************************************************************

  Synopsis    [Count the number of nodes in the dominator.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjDomVolume( Aig_Sto_t * pSto, Aig_Dom_t * pDom )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    Aig_ManIncrementTravId( pSto->pAig );
    Aig_DomForEachNode( pSto->pAig, pDom, pObj, i )
        Counter += Aig_ObjDomVolume_rec( pSto->pAig, pObj );
    return Counter;
}




/**Function*************************************************************

  Synopsis    [Dereferences the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjDomDeref_rec( Aig_Obj_t * pNode )
{
    int Counter = 0;
    assert( pNode->nRefs > 0 );
    if ( --pNode->nRefs > 0 )
        return 0;
    assert( pNode->nRefs == 0 );
    if ( pNode->fMarkA )
        return 1;
    if ( Aig_ObjIsCi(pNode) )
        return 0;
    Counter += Aig_ObjDomDeref_rec( Aig_ObjFanin0(pNode) );
    if ( Aig_ObjIsNode(pNode) )
    Counter += Aig_ObjDomDeref_rec( Aig_ObjFanin1(pNode) );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [References the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjDomRef_rec( Aig_Obj_t * pNode )
{
    int Counter = 0;
    assert( pNode->nRefs >= 0 );
    if ( pNode->nRefs++ > 0 )
        return 0;
    assert( pNode->nRefs == 1 );
    if ( pNode->fMarkA )
        return 1;
    if ( Aig_ObjIsCi(pNode) )
        return 0;
    Counter += Aig_ObjDomRef_rec( Aig_ObjFanin0(pNode) );
    if ( Aig_ObjIsNode(pNode) )
    Counter += Aig_ObjDomRef_rec( Aig_ObjFanin1(pNode) );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Count the number of nodes in the dominator.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ObjDomDomed( Aig_Sto_t * pSto, Aig_Dom_t * pDom )
{
    Aig_Obj_t * pObj;
    int i, Counter0, Counter1;
    Counter0 = 0;
    Aig_DomForEachNode( pSto->pAig, pDom, pObj, i )
    {
        assert( !Aig_ObjIsCi(pObj) );
        Counter0 += Aig_ObjDomDeref_rec( Aig_ObjFanin0(pObj) );
        if ( Aig_ObjIsNode(pObj) )
        Counter0 += Aig_ObjDomDeref_rec( Aig_ObjFanin1(pObj) );
    }
    Counter1 = 0;
    Aig_DomForEachNode( pSto->pAig, pDom, pObj, i )
    {
        assert( !Aig_ObjIsCi(pObj) );
        Counter1 += Aig_ObjDomRef_rec( Aig_ObjFanin0(pObj) );
        if ( Aig_ObjIsNode(pObj) )
        Counter1 += Aig_ObjDomRef_rec( Aig_ObjFanin1(pObj) );
    }
    assert( Counter0 == Counter1 );
    return Counter0;
}


/**Function*************************************************************

  Synopsis    [Collects dominators from the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Aig_ObjDomCollectLos( Aig_Sto_t * pSto )
{
    Vec_Int_t * vCut;
    Aig_Obj_t * pObj;
    int i;
    vCut = Vec_IntAlloc( Aig_ManRegNum(pSto->pAig) );
    Saig_ManForEachLo( pSto->pAig, pObj, i )
    {
        Vec_IntPush( vCut, pObj->Id );
        pObj->fMarkA = 1;
    }
    return vCut;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjPoLogicDeref( Aig_Sto_t * pSto )
{
    Aig_Obj_t * pObj;
    int i;
    Saig_ManForEachPo( pSto->pAig, pObj, i )
        Aig_ObjDomDeref_rec( Aig_ObjFanin0(pObj) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjPoLogicRef( Aig_Sto_t * pSto )
{
    Aig_Obj_t * pObj;
    int i;
    Saig_ManForEachPo( pSto->pAig, pObj, i )
        Aig_ObjDomRef_rec( Aig_ObjFanin0(pObj) );
}

/**Function*************************************************************

  Synopsis    [Collects dominators from the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjDomFindGood( Aig_Sto_t * pSto )
{
    Aig_Dom_t * pDom; 
    Vec_Int_t * vCut;
    Vec_Ptr_t * vDoms;
    int i;
    vCut  = Aig_ObjDomCollectLos( pSto );
    vDoms = Aig_ObjDomCollect( pSto, vCut );
    Vec_IntFree( vCut );
    printf( "The cut has %d non-trivial %d-dominators.\n", Vec_PtrSize(vDoms), pSto->Limit );

    Aig_ObjPoLogicDeref( pSto );
    Vec_PtrForEachEntry( Aig_Dom_t *, vDoms, pDom, i )
    {
//        if ( Aig_ObjDomDomed(pSto, pDom) <= 1 )
//            continue;
        printf( "Vol =%3d.  ", Aig_ObjDomVolume(pSto, pDom) );
        printf( "Dom =%3d.  ", Aig_ObjDomDomed(pSto, pDom) );
        Aig_ObjDomPrint( pSto, pDom, i );
    }
    Aig_ObjPoLogicRef( pSto );

    Aig_ObjDomVecRecycle( pSto, vDoms );
    Aig_ManCleanMarkA( pSto->pAig );
}


/**Function*************************************************************

  Synopsis    [Computes multi-node dominators.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManComputeDomsTest2( Aig_Man_t * pAig, int Num )
{
    Aig_Sto_t * pSto;
//    int i;
//Aig_ManShow( pAig, 0, NULL );
    Aig_ManFanoutStart( pAig );
//    for ( i = 1; i < 9; i++ )
    {
        printf( "ITERATION %d:\n", Num );
        pSto = Aig_ManComputeDomsFlops( pAig, Num );
        Aig_ObjDomFindGood( pSto );
//        Aig_ManDomPrint( pSto );
        Aig_ManDomStop( pSto );
    }
    Aig_ManFanoutStop( pAig );
}

/**Function*************************************************************

  Synopsis    [Computes multi-node dominators.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManComputeDomsTest( Aig_Man_t * pAig )
{
    Aig_Sto_t * pSto;
    Aig_ManFanoutStart( pAig );
    pSto = Aig_ManComputeDomsPis( pAig, 1 );
    Aig_ManDomPrint( pSto );
    Aig_ManDomStop( pSto );
    Aig_ManFanoutStop( pAig );
}





/**Function*************************************************************

  Synopsis    [Collects dominators from the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjDomCount( Aig_Sto_t * pSto, Aig_Obj_t * pObj )
{
    Aig_Dom_t * pDom;
    Aig_Obj_t * pFanout;
    Vec_Int_t * vSingles;
    Vec_Ptr_t * vDoms;
    int i, k, Entry, iFanout, fPrint = 0;
    vSingles = Vec_IntAlloc( 100 );
    // for each dominator of a fanout, count how many fanouts have it as a dominator
    Aig_ObjForEachFanout( pSto->pAig, pObj, pFanout, iFanout, i )
    {
        vDoms = (Vec_Ptr_t *)Vec_PtrEntry( pSto->vDoms, Aig_ObjId(pFanout) );
        Vec_PtrForEachEntryStart( Aig_Dom_t *, vDoms, pDom, k, 1 )
        {
//            printf( "Fanout %d  Dominator %d\n", Aig_ObjId(pFanout), pDom->pNodes[0] );
            Vec_IntAddToEntry( pSto->vTimes, pDom->pNodes[0], 1 );
            Vec_IntPushUnique( vSingles, pDom->pNodes[0] );
        }
    }
    // clear storage
    Vec_IntForEachEntry( vSingles, Entry, i )
    {
        if ( Vec_IntEntry(pSto->vTimes, Entry) > 5 )
        {
            if ( fPrint == 0 )
            {
                printf( "%6d : Level =%4d. Fanout =%6d.\n", 
                    Aig_ObjId(pObj), Aig_ObjLevel(pObj), Aig_ObjRefs(pObj) );
            }
            fPrint = 1;
            printf( "%d(%d) ", Entry, Vec_IntEntry(pSto->vTimes, Entry) );
        }
        Vec_IntWriteEntry( pSto->vTimes, Entry, 0);
    }
    if ( fPrint )
        printf( "\n" );
    Vec_IntFree( vSingles );
}


/**Function*************************************************************

  Synopsis    [Computes multi-node dominators.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManComputeDomsForCofactoring( Aig_Man_t * pAig )
{
    Vec_Ptr_t * vDoms;
    Aig_Sto_t * pSto;
    Aig_Obj_t * pObj;
    int i;
    Aig_ManFanoutStart( pAig );
    pSto = Aig_ManComputeDomsNodes( pAig, 1 );
    Aig_ManForEachObj( pAig, pObj, i )
    {
        if ( !Aig_ObjIsCi(pObj) && !Aig_ObjIsNode(pObj) )
            continue;
        if ( Aig_ObjRefs(pObj) < 10 )
            continue;
        vDoms = (Vec_Ptr_t *)Vec_PtrEntry( pSto->vDoms, Aig_ObjId(pObj) );
//        printf( "%6d : Level =%4d. Fanout =%6d.\n", 
//            Aig_ObjId(pObj), Aig_ObjLevel(pObj), Aig_ObjRefs(pObj) );

        Aig_ObjDomCount( pSto, pObj );
    }
    Aig_ManDomStop( pSto );
    Aig_ManFanoutStop( pAig );
}





////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

