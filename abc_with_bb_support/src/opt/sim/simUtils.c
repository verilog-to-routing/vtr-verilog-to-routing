/**CFile****************************************************************

  FileName    [simUtils.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Various simulation utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: simUtils.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "sim.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
static int bit_count[256] = {
  0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates simulation information for all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Sim_UtilInfoAlloc( int nSize, int nWords, int fClean )
{
    Vec_Ptr_t * vInfo;
    int i;
    assert( nSize > 0 && nWords > 0 );
    vInfo = Vec_PtrAlloc( nSize );
    vInfo->pArray[0] = ABC_ALLOC( unsigned, nSize * nWords );
    if ( fClean )
        memset( vInfo->pArray[0], 0, sizeof(unsigned) * nSize * nWords );
    for ( i = 1; i < nSize; i++ )
        vInfo->pArray[i] = ((unsigned *)vInfo->pArray[i-1]) + nWords;
    vInfo->nSize = nSize;
    return vInfo;
}

/**Function*************************************************************

  Synopsis    [Allocates simulation information for all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilInfoFree( Vec_Ptr_t * p )
{
    ABC_FREE( p->pArray[0] );
    Vec_PtrFree( p );
}

/**Function*************************************************************

  Synopsis    [Adds the second supp-info the first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilInfoAdd( unsigned * pInfo1, unsigned * pInfo2, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        pInfo1[w] |= pInfo2[w];
}

/**Function*************************************************************

  Synopsis    [Returns the positions where pInfo2 is 1 while pInfo1 is 0.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilInfoDetectDiffs( unsigned * pInfo1, unsigned * pInfo2, int nWords, Vec_Int_t * vDiffs )
{
    int w, b;
    unsigned uMask;
    vDiffs->nSize = 0;
    for ( w = 0; w < nWords; w++ )
        if ( (uMask = (pInfo2[w] ^ pInfo1[w])) )
            for ( b = 0; b < 32; b++ )
                if ( uMask & (1 << b) )
                    Vec_IntPush( vDiffs, 32*w + b );
}

/**Function*************************************************************

  Synopsis    [Returns the positions where pInfo2 is 1 while pInfo1 is 0.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilInfoDetectNews( unsigned * pInfo1, unsigned * pInfo2, int nWords, Vec_Int_t * vDiffs )
{
    int w, b;
    unsigned uMask;
    vDiffs->nSize = 0;
    for ( w = 0; w < nWords; w++ )
        if ( (uMask = (pInfo2[w] & ~pInfo1[w])) )
            for ( b = 0; b < 32; b++ )
                if ( uMask & (1 << b) )
                    Vec_IntPush( vDiffs, 32*w + b );
}

/**Function*************************************************************

  Synopsis    [Flips the simulation info of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilInfoFlip( Sim_Man_t * p, Abc_Obj_t * pNode )
{
    unsigned * pSimInfo1, * pSimInfo2;
    int k;
    pSimInfo1 = (unsigned *)p->vSim0->pArray[pNode->Id];
    pSimInfo2 = (unsigned *)p->vSim1->pArray[pNode->Id];
    for ( k = 0; k < p->nSimWords; k++ )
        pSimInfo2[k] = ~pSimInfo1[k];
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the simulation infos are equal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_UtilInfoCompare( Sim_Man_t * p, Abc_Obj_t * pNode )
{
    unsigned * pSimInfo1, * pSimInfo2;
    int k;
    pSimInfo1 = (unsigned *)p->vSim0->pArray[pNode->Id];
    pSimInfo2 = (unsigned *)p->vSim1->pArray[pNode->Id];
    for ( k = 0; k < p->nSimWords; k++ )
        if ( pSimInfo2[k] != pSimInfo1[k] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Simulates the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilSimulate( Sim_Man_t * p, int fType )
{
    Abc_Obj_t * pNode;
    int i;
    // simulate the internal nodes
    Abc_NtkForEachNode( p->pNtk, pNode, i )
        Sim_UtilSimulateNode( p, pNode, fType, fType, fType );
    // assign simulation info of the CO nodes
    Abc_NtkForEachCo( p->pNtk, pNode, i )
        Sim_UtilSimulateNode( p, pNode, fType, fType, fType );
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilSimulateNode( Sim_Man_t * p, Abc_Obj_t * pNode, int fType, int fType1, int fType2 )
{
    unsigned * pSimmNode, * pSimmNode1, * pSimmNode2;
    int k, fComp1, fComp2;
    // simulate the internal nodes
    if ( Abc_ObjIsNode(pNode) )
    {
        if ( fType )
            pSimmNode  = (unsigned *)p->vSim1->pArray[ pNode->Id ];
        else
            pSimmNode  = (unsigned *)p->vSim0->pArray[ pNode->Id ];

        if ( fType1 )
            pSimmNode1 = (unsigned *)p->vSim1->pArray[ Abc_ObjFaninId0(pNode) ];
        else
            pSimmNode1 = (unsigned *)p->vSim0->pArray[ Abc_ObjFaninId0(pNode) ];

        if ( fType2 )
            pSimmNode2 = (unsigned *)p->vSim1->pArray[ Abc_ObjFaninId1(pNode) ];
        else
            pSimmNode2 = (unsigned *)p->vSim0->pArray[ Abc_ObjFaninId1(pNode) ];

        fComp1 = Abc_ObjFaninC0(pNode);
        fComp2 = Abc_ObjFaninC1(pNode);
        if ( fComp1 && fComp2 )
            for ( k = 0; k < p->nSimWords; k++ )
                pSimmNode[k] = ~pSimmNode1[k] & ~pSimmNode2[k];
        else if ( fComp1 && !fComp2 )
            for ( k = 0; k < p->nSimWords; k++ )
                pSimmNode[k] = ~pSimmNode1[k] &  pSimmNode2[k];
        else if ( !fComp1 && fComp2 )
            for ( k = 0; k < p->nSimWords; k++ )
                pSimmNode[k] =  pSimmNode1[k] & ~pSimmNode2[k];
        else // if ( fComp1 && fComp2 )
            for ( k = 0; k < p->nSimWords; k++ )
                pSimmNode[k] =  pSimmNode1[k] &  pSimmNode2[k];
    }
    else 
    {
        assert( Abc_ObjFaninNum(pNode) == 1 );
        if ( fType )
            pSimmNode  = (unsigned *)p->vSim1->pArray[ pNode->Id ];
        else
            pSimmNode  = (unsigned *)p->vSim0->pArray[ pNode->Id ];

        if ( fType1 )
            pSimmNode1 = (unsigned *)p->vSim1->pArray[ Abc_ObjFaninId0(pNode) ];
        else
            pSimmNode1 = (unsigned *)p->vSim0->pArray[ Abc_ObjFaninId0(pNode) ];

        fComp1 = Abc_ObjFaninC0(pNode);
        if ( fComp1 )
            for ( k = 0; k < p->nSimWords; k++ )
                pSimmNode[k] = ~pSimmNode1[k];
        else 
            for ( k = 0; k < p->nSimWords; k++ )
                pSimmNode[k] =  pSimmNode1[k];
    }
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilSimulateNodeOne( Abc_Obj_t * pNode, Vec_Ptr_t * vSimInfo, int nSimWords, int nOffset )
{
    unsigned * pSimmNode, * pSimmNode1, * pSimmNode2;
    int k, fComp1, fComp2;
    // simulate the internal nodes
    assert( Abc_ObjIsNode(pNode) );
    pSimmNode  = (unsigned *)Vec_PtrEntry(vSimInfo, pNode->Id);
    pSimmNode1 = (unsigned *)Vec_PtrEntry(vSimInfo, Abc_ObjFaninId0(pNode));
    pSimmNode2 = (unsigned *)Vec_PtrEntry(vSimInfo, Abc_ObjFaninId1(pNode));
    pSimmNode  += nOffset;
    pSimmNode1 += nOffset;
    pSimmNode2 += nOffset;
    fComp1 = Abc_ObjFaninC0(pNode);
    fComp2 = Abc_ObjFaninC1(pNode);
    if ( fComp1 && fComp2 )
        for ( k = 0; k < nSimWords; k++ )
            pSimmNode[k] = ~pSimmNode1[k] & ~pSimmNode2[k];
    else if ( fComp1 && !fComp2 )
        for ( k = 0; k < nSimWords; k++ )
            pSimmNode[k] = ~pSimmNode1[k] &  pSimmNode2[k];
    else if ( !fComp1 && fComp2 )
        for ( k = 0; k < nSimWords; k++ )
            pSimmNode[k] =  pSimmNode1[k] & ~pSimmNode2[k];
    else // if ( fComp1 && fComp2 )
        for ( k = 0; k < nSimWords; k++ )
            pSimmNode[k] =  pSimmNode1[k] &  pSimmNode2[k];
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilTransferNodeOne( Abc_Obj_t * pNode, Vec_Ptr_t * vSimInfo, int nSimWords, int nOffset, int fShift )
{
    unsigned * pSimmNode, * pSimmNode1;
    int k, fComp1;
    // simulate the internal nodes
    assert( Abc_ObjIsCo(pNode) );
    pSimmNode  = (unsigned *)Vec_PtrEntry(vSimInfo, pNode->Id);
    pSimmNode1 = (unsigned *)Vec_PtrEntry(vSimInfo, Abc_ObjFaninId0(pNode));
    pSimmNode  += nOffset + (fShift > 0)*nSimWords;
    pSimmNode1 += nOffset;
    fComp1 = Abc_ObjFaninC0(pNode);
    if ( fComp1 )
        for ( k = 0; k < nSimWords; k++ )
            pSimmNode[k] = ~pSimmNode1[k];
    else 
        for ( k = 0; k < nSimWords; k++ )
            pSimmNode[k] =  pSimmNode1[k];
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the simulation infos are equal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_UtilCountSuppSizes( Sim_Man_t * p, int fStruct )
{
    Abc_Obj_t * pNode, * pNodeCi;
    int i, v, Counter;
    Counter = 0;
    if ( fStruct )
    {
        Abc_NtkForEachCo( p->pNtk, pNode, i )
            Abc_NtkForEachCi( p->pNtk, pNodeCi, v )
                Counter += Sim_SuppStrHasVar( p->vSuppStr, pNode, v );
    }
    else
    {
        Abc_NtkForEachCo( p->pNtk, pNode, i )
            Abc_NtkForEachCi( p->pNtk, pNodeCi, v )
                Counter += Sim_SuppFunHasVar( p->vSuppFun, i, v );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of 1's in the bitstring.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_UtilCountOnes( unsigned * pSimInfo, int nSimWords )
{
    unsigned char * pBytes;
    int nOnes, nBytes, i;
    pBytes = (unsigned char *)pSimInfo;
    nBytes = 4 * nSimWords;
    nOnes  = 0;
    for ( i = 0; i < nBytes; i++ )
        nOnes += bit_count[ pBytes[i] ];
    return nOnes;    
}

/**Function*************************************************************

  Synopsis    [Counts the number of 1's in the bitstring.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Sim_UtilCountOnesArray( Vec_Ptr_t * vInfo, int nSimWords )
{
    Vec_Int_t * vCounters;
    unsigned * pSimInfo;
    int i;
    vCounters = Vec_IntStart( Vec_PtrSize(vInfo) );
    Vec_PtrForEachEntry( unsigned *, vInfo, pSimInfo, i )
        Vec_IntWriteEntry( vCounters, i, Sim_UtilCountOnes(pSimInfo, nSimWords) );
    return vCounters;
}

/**Function*************************************************************

  Synopsis    [Returns random patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilSetRandom( unsigned * pPatRand, int nSimWords )
{
    int k;
    for ( k = 0; k < nSimWords; k++ )
        pPatRand[k] = SIM_RANDOM_UNSIGNED;
}

/**Function*************************************************************

  Synopsis    [Returns complemented patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilSetCompl( unsigned * pPatRand, int nSimWords )
{
    int k;
    for ( k = 0; k < nSimWords; k++ )
        pPatRand[k] = ~pPatRand[k];
}

/**Function*************************************************************

  Synopsis    [Returns constant patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilSetConst( unsigned * pPatRand, int nSimWords, int fConst1 )
{
    int k;
    for ( k = 0; k < nSimWords; k++ )
        pPatRand[k] = 0;
    if ( fConst1 )
        Sim_UtilSetCompl( pPatRand, nSimWords );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if equal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_UtilInfoIsEqual( unsigned * pPats1, unsigned * pPats2, int nSimWords )
{
    int k;
    for ( k = 0; k < nSimWords; k++ )
        if ( pPats1[k] != pPats2[k] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if Node1 implies Node2.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_UtilInfoIsImp( unsigned * pPats1, unsigned * pPats2, int nSimWords )
{
    int k;
    for ( k = 0; k < nSimWords; k++ )
        if ( pPats1[k] & ~pPats2[k] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if Node1 v Node2 is always true.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_UtilInfoIsClause( unsigned * pPats1, unsigned * pPats2, int nSimWords )
{
    int k;
    for ( k = 0; k < nSimWords; k++ )
        if ( ~pPats1[k] & ~pPats2[k] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Counts the total number of pairs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_UtilCountAllPairs( Vec_Ptr_t * vSuppFun, int nSimWords, Vec_Int_t * vCounters )
{
    unsigned * pSupp;
    int Counter, nOnes, nPairs, i;
    Counter = 0;
    Vec_PtrForEachEntry( unsigned *, vSuppFun, pSupp, i )
    {
        nOnes  = Sim_UtilCountOnes( pSupp, nSimWords );
        nPairs = nOnes * (nOnes - 1) / 2;
        Vec_IntWriteEntry( vCounters, i, nPairs );
        Counter += nPairs;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of entries in the array of matrices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_UtilCountPairsOne( Extra_BitMat_t * pMat, Vec_Int_t * vSupport )
{
    int i, k, Index1, Index2;
    int Counter = 0;
//    int Counter2;
    Vec_IntForEachEntry( vSupport, i, Index1 )
    Vec_IntForEachEntryStart( vSupport, k, Index2, Index1+1 )
        Counter += Extra_BitMatrixLookup1( pMat, i, k );
//    Counter2 = Extra_BitMatrixCountOnesUpper(pMat);
//    assert( Counter == Counter2 );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of entries in the array of matrices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_UtilCountPairsOnePrint( Extra_BitMat_t * pMat, Vec_Int_t * vSupport )
{
    int i, k, Index1, Index2;
    Vec_IntForEachEntry( vSupport, i, Index1 )
    Vec_IntForEachEntryStart( vSupport, k, Index2, Index1+1 )
        if ( Extra_BitMatrixLookup1( pMat, i, k ) )
            printf( "(%d,%d) ", i, k );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Counts the number of entries in the array of matrices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilCountPairsAllPrint( Sym_Man_t * p )
{
    int i;
    abctime clk;
clk = Abc_Clock();
    for ( i = 0; i < p->nOutputs; i++ )
    {
        printf( "Output %2d :", i );
        Sim_UtilCountPairsOnePrint( (Extra_BitMat_t *)Vec_PtrEntry(p->vMatrSymms, i), Vec_VecEntryInt(p->vSupports, i) );
        printf( "\n" );
    }
p->timeCount += Abc_Clock() - clk;
}

/**Function*************************************************************

  Synopsis    [Counts the number of entries in the array of matrices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_UtilCountPairsAll( Sym_Man_t * p )
{
    int nPairsTotal, nPairsSym, nPairsNonSym, i;
    abctime clk;
clk = Abc_Clock();
    p->nPairsSymm    = 0;
    p->nPairsNonSymm = 0;
    for ( i = 0; i < p->nOutputs; i++ )
    {
        nPairsTotal  = Vec_IntEntry(p->vPairsTotal, i);
        nPairsSym    = Vec_IntEntry(p->vPairsSym,   i);
        nPairsNonSym = Vec_IntEntry(p->vPairsNonSym,i);
        assert( nPairsTotal >= nPairsSym + nPairsNonSym ); 
        if ( nPairsTotal == nPairsSym + nPairsNonSym )
        {
            p->nPairsSymm    += nPairsSym;
            p->nPairsNonSymm += nPairsNonSym;
            continue;
        }
        nPairsSym    = Sim_UtilCountPairsOne( (Extra_BitMat_t *)Vec_PtrEntry(p->vMatrSymms,   i), Vec_VecEntryInt(p->vSupports, i) );
        nPairsNonSym = Sim_UtilCountPairsOne( (Extra_BitMat_t *)Vec_PtrEntry(p->vMatrNonSymms,i), Vec_VecEntryInt(p->vSupports, i) );
        assert( nPairsTotal >= nPairsSym + nPairsNonSym ); 
        Vec_IntWriteEntry( p->vPairsSym,    i, nPairsSym );
        Vec_IntWriteEntry( p->vPairsNonSym, i, nPairsNonSym );
        p->nPairsSymm    += nPairsSym;
        p->nPairsNonSymm += nPairsNonSym;
//        printf( "%d ", nPairsTotal - nPairsSym - nPairsNonSym );
    }
//printf( "\n" );
    p->nPairsRem = p->nPairsTotal-p->nPairsSymm-p->nPairsNonSymm;
p->timeCount += Abc_Clock() - clk;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_UtilMatrsAreDisjoint( Sym_Man_t * p )
{
    int i;
    for ( i = 0; i < p->nOutputs; i++ )
        if ( !Extra_BitMatrixIsDisjoint( (Extra_BitMat_t *)Vec_PtrEntry(p->vMatrSymms,i), (Extra_BitMat_t *)Vec_PtrEntry(p->vMatrNonSymms,i) ) )
            return 0;
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

