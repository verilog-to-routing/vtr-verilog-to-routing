/**CFile****************************************************************

  FileName    [amapMerge.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    [Computing cuts for the node.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapMerge.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "amapInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates new cut and adds it to storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Cut_t * Amap_ManSetupPis( Amap_Man_t * p )
{
    Amap_Obj_t * pObj;
    Amap_Cut_t * pCut;
    int i, nBytes = sizeof(Amap_Cut_t) + sizeof(int);
    char * pBuffer = ABC_ALLOC( char, Amap_ManPiNum(p) * nBytes );
    Amap_ManForEachPi( p, pObj, i )
    {
        pCut = (Amap_Cut_t *)( pBuffer + i*nBytes );
        pCut->iMat = 0;
        pCut->fInv = 0;
        pCut->nFans = 1;
        pCut->Fans[0] = Abc_Var2Lit( pObj->Id, 0 );
        pObj->pData = pCut;
        pObj->nCuts = 1;
        pObj->EstRefs = (float)1.0;
    }
    return (Amap_Cut_t *)pBuffer;
}

/**Function*************************************************************

  Synopsis    [Creates new cut and adds it to storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Cut_t * Amap_ManCutStore( Amap_Man_t * p, Amap_Cut_t * pCut, int fCompl )
{
    Amap_Cut_t * pNew;
    int iFan, nBytes = sizeof(Amap_Cut_t) + sizeof(int) * pCut->nFans + sizeof(Amap_Cut_t *);
    pNew = (Amap_Cut_t *)Aig_MmFlexEntryFetch( p->pMemTemp, nBytes );
    pNew->iMat  = pCut->iMat;
    pNew->fInv  = pCut->fInv ^ fCompl;
    pNew->nFans = pCut->nFans;
    memcpy( pNew->Fans, pCut->Fans, sizeof(int) * pCut->nFans );
    // add it to storage
    iFan = Abc_Var2Lit( pNew->iMat, pNew->fInv );
    if ( p->ppCutsTemp[ iFan ] == NULL )
        Vec_IntPushOrder( p->vTemp, iFan );
    *Amap_ManCutNextP( pNew ) = p->ppCutsTemp[ iFan ];
    p->ppCutsTemp[ iFan ] = pNew;
    return pNew;
} 

/**Function*************************************************************

  Synopsis    [Creates new cut and adds it to storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Cut_t * Amap_ManCutCreate( Amap_Man_t * p, 
    Amap_Cut_t * pCut0, Amap_Cut_t * pCut1, int iMat )
{
    Amap_Cut_t * pCut;
    int i, nSize  = pCut0->nFans + pCut1->nFans;
    int nBytes = sizeof(Amap_Cut_t) + sizeof(int) * nSize + sizeof(Amap_Cut_t *);
    assert( pCut0->iMat >= pCut1->iMat );
    pCut = (Amap_Cut_t *)Aig_MmFlexEntryFetch( p->pMemTemp, nBytes );
    pCut->iMat  = iMat;
    pCut->fInv  = 0;
    pCut->nFans = nSize;
    for ( i = 0; i < (int)pCut0->nFans; i++ )
        pCut->Fans[i] = pCut0->Fans[i];
    for ( i = 0; i < (int)pCut1->nFans; i++ )
        pCut->Fans[pCut0->nFans+i] = pCut1->Fans[i];
    // add it to storage
    if ( p->ppCutsTemp[ pCut->iMat ] == NULL )
        Vec_IntPushOrder( p->vTemp, pCut->iMat );
    *Amap_ManCutNextP( pCut ) = p->ppCutsTemp[ pCut->iMat ];
    p->ppCutsTemp[ pCut->iMat ] = pCut;
    return pCut;
} 

/**Function*************************************************************

  Synopsis    [Creates new cut and adds it to storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Cut_t * Amap_ManCutCreate3( Amap_Man_t * p, 
    Amap_Cut_t * pCut0, Amap_Cut_t * pCut1, Amap_Cut_t * pCut2, int iMat )
{
    Amap_Cut_t * pCut;
    int i, nSize  = pCut0->nFans + pCut1->nFans + pCut2->nFans;
    int nBytes = sizeof(Amap_Cut_t) + sizeof(int) * nSize + sizeof(Amap_Cut_t *);
    pCut = (Amap_Cut_t *)Aig_MmFlexEntryFetch( p->pMemTemp, nBytes );
    pCut->iMat  = iMat;
    pCut->fInv  = 0;
    pCut->nFans = nSize;
    for ( i = 0; i < (int)pCut0->nFans; i++ )
        pCut->Fans[i] = pCut0->Fans[i];
    for ( i = 0; i < (int)pCut1->nFans; i++ )
        pCut->Fans[pCut0->nFans+i] = pCut1->Fans[i];
    for ( i = 0; i < (int)pCut2->nFans; i++ )
        pCut->Fans[pCut0->nFans+pCut1->nFans+i] = pCut2->Fans[i];
    // add it to storage
    if ( p->ppCutsTemp[ pCut->iMat ] == NULL )
        Vec_IntPushOrder( p->vTemp, pCut->iMat );
    *Amap_ManCutNextP( pCut ) = p->ppCutsTemp[ pCut->iMat ];
    p->ppCutsTemp[ pCut->iMat ] = pCut;
    return pCut;
} 

/**Function*************************************************************

  Synopsis    [Removes cuts from the temporary storage.]

  Description [] 
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
void Amap_ManCutSaveStored( Amap_Man_t * p, Amap_Obj_t * pNode )
{
    int nMaxCuts = p->pPars->nCutsMax;
    int * pBuffer;
    Amap_Cut_t * pNext, * pCut;
    int i, nWords, Entry, nCuts, nCuts2;
    assert( pNode->pData == NULL );
    // count memory needed
    nCuts = 1;
    nWords = 2;
    Vec_IntForEachEntry( p->vTemp, Entry, i )
    {
        for ( pCut = p->ppCutsTemp[Entry]; pCut; pCut = *Amap_ManCutNextP(pCut) )
        {
            nCuts++;
            if ( nCuts < nMaxCuts )
                nWords += pCut->nFans + 1;
        }
    }
    p->nBytesUsed += 4*nWords;
    // allocate memory
    pBuffer = (int *)Aig_MmFlexEntryFetch( p->pMemCuts, 4*nWords );
    pNext = (Amap_Cut_t *)pBuffer;
    // add the first cut
    pNext->iMat  = 0; 
    pNext->fInv  = 0;
    pNext->nFans = 1;
    pNext->Fans[0] = Abc_Var2Lit(pNode->Id, 0);
    pNext  = (Amap_Cut_t *)(pBuffer + 2);
    // add other cuts
    nCuts2 = 1;
    Vec_IntForEachEntry( p->vTemp, Entry, i )
    {
        for ( pCut = p->ppCutsTemp[Entry]; pCut; pCut = *Amap_ManCutNextP(pCut) )
        {
            nCuts2++;
            if ( nCuts2 < nMaxCuts )
            {
                memcpy( pNext, pCut, sizeof(int) * (pCut->nFans + 1) );
                pNext = (Amap_Cut_t *)((int *)pNext + pCut->nFans + 1);
            }
        }
        p->ppCutsTemp[Entry] = NULL;
    }
    assert( nCuts == nCuts2 );
    assert( (int *)pNext - pBuffer == nWords );
    // restore the storage
    Vec_IntClear( p->vTemp );
    Aig_MmFlexRestart( p->pMemTemp );
    for ( i = 0; i < 2*p->pLib->nNodes; i++ )
        if ( p->ppCutsTemp[i] != NULL )
            printf( "Amap_ManCutSaveStored(): Error!\n" );
    pNode->pData = (Amap_Cut_t *)pBuffer;
    pNode->nCuts = Abc_MinInt( nCuts, nMaxCuts-1 );
    assert( nCuts < (1<<20) );
//    printf("%d ", nCuts );
    // verify cuts
    pCut = NULL;
    Amap_NodeForEachCut( pNode, pNext, i )
//        for ( i = 0, pNext = (Amap_Cut_t *)pNode->pData; i < (int)pNode->nCuts; 
//        i++, pNext = Amap_ManCutNext(pNext) )
    {
        if ( i == nMaxCuts )
            break;
        assert( pCut == NULL || pCut->iMat <= pNext->iMat );
        pCut = pNext;
    }
}

/**Function*************************************************************

  Synopsis    [Returns the number of possible new cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_ManMergeCountCuts( Amap_Man_t * p, Amap_Obj_t * pNode )
{
    Amap_Obj_t * pFanin0 = Amap_ObjFanin0( p, pNode );
    Amap_Obj_t * pFanin1 = Amap_ObjFanin1( p, pNode );
    Amap_Cut_t * pCut0, * pCut1;
    int Entry, c0, c1, iCompl0, iCompl1, iFan0, iFan1;
    int Counter = 1;
    Amap_NodeForEachCut( pFanin0, pCut0, c0 )
    Amap_NodeForEachCut( pFanin1, pCut1, c1 )
    {
        iCompl0 = pCut0->fInv ^ Amap_ObjFaninC0(pNode);
        iCompl1 = pCut1->fInv ^ Amap_ObjFaninC1(pNode);
        iFan0   = !pCut0->iMat? 0: Abc_Var2Lit( pCut0->iMat, iCompl0 );
        iFan1   = !pCut1->iMat? 0: Abc_Var2Lit( pCut1->iMat, iCompl1 );
        Entry = Amap_LibFindNode( p->pLib, iFan0, iFan1, pNode->Type == AMAP_OBJ_XOR );
        Counter += ( Entry >=0 );
//        if ( Entry >=0 )
//            printf( "Full: %d + %d = %d\n", iFan0, iFan1, Entry );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Print cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManPrintCuts( Amap_Obj_t * pNode )
{
    Amap_Cut_t * pCut;
    int c, i;
    printf( "NODE %5d : Type = ", pNode->Id );
    if ( pNode->Type == AMAP_OBJ_AND )
        printf( "AND" );
    else if ( pNode->Type == AMAP_OBJ_XOR )
        printf( "XOR" );
    else if ( pNode->Type == AMAP_OBJ_MUX )
        printf( "MUX" );
    printf( "  Cuts = %d\n", pNode->nCuts );
    Amap_NodeForEachCut( pNode, pCut, c )
    {
        printf( "%3d :  Mat= %3d  Inv=%d  ", c, pCut->iMat, pCut->fInv );
        for ( i = 0; i < (int)pCut->nFans; i++ )
            printf( "%d%c ", Abc_Lit2Var(pCut->Fans[i]), Abc_LitIsCompl(pCut->Fans[i])?'-':'+' );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Derives cuts for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManMergeNodeChoice( Amap_Man_t * p, Amap_Obj_t * pNode )
{
    Amap_Obj_t * pTemp;
    Amap_Cut_t * pCut;
    int c;
    // go through the nodes of the choice node
    for ( pTemp = pNode; pTemp; pTemp = Amap_ObjChoice(p, pTemp) )
    {
        Amap_NodeForEachCut( pTemp, pCut, c )
        {
            if (!pCut) break;   // mikelee added; abort when pCut is NULL
            if ( pCut->iMat )
                Amap_ManCutStore( p, pCut, pNode->fPhase ^ pTemp->fPhase );
        }
        pTemp->pData = NULL;
    }
    Amap_ManCutSaveStored( p, pNode );

//    Amap_ManPrintCuts( pNode );
}

/**Function*************************************************************

  Synopsis    [Derives cuts for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_ManFindCut( Amap_Obj_t * pNode, Amap_Obj_t * pFanin, int fComplFanin, int Val, Vec_Ptr_t * vCuts )
{
    Amap_Cut_t * pCut;
    int c, iCompl, iFan;
    Vec_PtrClear( vCuts );
    Amap_NodeForEachCut( pFanin, pCut, c )
    {
        iCompl = pCut->fInv ^ fComplFanin;
        iFan   = !pCut->iMat? 0: Abc_Var2Lit( pCut->iMat, iCompl );
        if ( iFan == Val )
            Vec_PtrPush( vCuts, pCut );
    }
    return Vec_PtrSize(vCuts) == 0;
}

/**Function*************************************************************

  Synopsis    [Derives cuts for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManMergeNodeCutsMux( Amap_Man_t * p, Amap_Obj_t * pNode )
{
    Vec_Int_t * vRules = p->pLib->vRules3;
    Amap_Obj_t * pFanin0 = Amap_ObjFanin0( p, pNode );
    Amap_Obj_t * pFanin1 = Amap_ObjFanin1( p, pNode );
    Amap_Obj_t * pFanin2 = Amap_ObjFanin2( p, pNode );
    int fComplFanin0 = Amap_ObjFaninC0( pNode );
    int fComplFanin1 = Amap_ObjFaninC1( pNode );
    int fComplFanin2 = Amap_ObjFaninC2( pNode );
    Amap_Cut_t * pCut0, * pCut1, * pCut2;
    int x, c0, c1, c2;
    assert( pNode->pData == NULL );
    assert( pNode->Type == AMAP_OBJ_MUX );
    assert( pNode->fRepr == 0 );
    // go through the rules
    for ( x = 0; x < Vec_IntSize(vRules); x += 4 )
    {
        if ( Amap_ManFindCut( pNode, pFanin0, fComplFanin0, Vec_IntEntry(vRules, x), p->vCuts0 ) )
            continue;
        if ( Amap_ManFindCut( pNode, pFanin1, fComplFanin1, Vec_IntEntry(vRules, x+1), p->vCuts1 ) )
            continue;
        if ( Amap_ManFindCut( pNode, pFanin2, fComplFanin2, Vec_IntEntry(vRules, x+2), p->vCuts2 ) )
            continue;
        Vec_PtrForEachEntry( Amap_Cut_t *, p->vCuts0, pCut0, c0 )
        Vec_PtrForEachEntry( Amap_Cut_t *, p->vCuts1, pCut1, c1 )
        Vec_PtrForEachEntry( Amap_Cut_t *, p->vCuts2, pCut2, c2 )
        {
            Amap_Nod_t * pNod = Amap_LibNod( p->pLib, Vec_IntEntry(vRules, x+3) );
            if ( pNod->pSets == NULL )
                continue;
            // complement literals
            if ( pCut0->nFans == 1 && (pCut0->fInv ^ fComplFanin0) )
                pCut0->Fans[0] = Abc_LitNot(pCut0->Fans[0]);
            if ( pCut1->nFans == 1 && (pCut1->fInv ^ fComplFanin1) )
                pCut1->Fans[0] = Abc_LitNot(pCut1->Fans[0]);
            if ( pCut2->nFans == 1 && (pCut2->fInv ^ fComplFanin2) )
                pCut2->Fans[0] = Abc_LitNot(pCut2->Fans[0]);
            // create new cut
            Amap_ManCutCreate3( p, pCut0, pCut1, pCut2, Vec_IntEntry(vRules, x+3) );
            // uncomplement literals
            if ( pCut0->nFans == 1 && (pCut0->fInv ^ fComplFanin0) )
                pCut0->Fans[0] = Abc_LitNot(pCut0->Fans[0]);
            if ( pCut1->nFans == 1 && (pCut1->fInv ^ fComplFanin1) )
                pCut1->Fans[0] = Abc_LitNot(pCut1->Fans[0]);
            if ( pCut2->nFans == 1 && (pCut2->fInv ^ fComplFanin2) )
                pCut2->Fans[0] = Abc_LitNot(pCut2->Fans[0]);
        }
    }
    Amap_ManCutSaveStored( p, pNode );
    p->nCutsUsed += pNode->nCuts;
    p->nCutsTried3 += pFanin0->nCuts * pFanin1->nCuts * pFanin2->nCuts;

//    Amap_ManPrintCuts( pNode );
}

/**Function*************************************************************

  Synopsis    [Derives cuts for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManMergeNodeCuts( Amap_Man_t * p, Amap_Obj_t * pNode )
{
    Amap_Obj_t * pFanin0 = Amap_ObjFanin0( p, pNode );
    Amap_Obj_t * pFanin1 = Amap_ObjFanin1( p, pNode );
    Amap_Cut_t * pCut0, * pCut1;
    int ** pRules, Entry, i, k, c, iCompl0, iCompl1, iFan0, iFan1;
    assert( pNode->pData == NULL );
    if ( pNode->Type == AMAP_OBJ_MUX )
    {
        Amap_ManMergeNodeCutsMux( p, pNode );
        return;
    }
    assert( pNode->Type != AMAP_OBJ_MUX );
    pRules = (pNode->Type == AMAP_OBJ_AND)? p->pLib->pRules: p->pLib->pRulesX;
    Amap_NodeForEachCut( pFanin0, pCut0, c )
    {
        iCompl0 = pCut0->fInv ^ Amap_ObjFaninC0(pNode);
        iFan0   = !pCut0->iMat? 0: Abc_Var2Lit( pCut0->iMat, iCompl0 );
        // complement literals
        if ( pCut0->nFans == 1 && iCompl0 )
            pCut0->Fans[0] = Abc_LitNot(pCut0->Fans[0]);
        // label resulting sets
        for ( i = 0; (Entry = pRules[iFan0][i]); i++ )
            p->pMatsTemp[Entry & 0xffff] = (Entry >> 16);
        // iterate through the cuts
        Amap_NodeForEachCut( pFanin1, pCut1, k )
        {
            iCompl1 = pCut1->fInv ^ Amap_ObjFaninC1(pNode);
            iFan1   = !pCut1->iMat? 0: Abc_Var2Lit( pCut1->iMat, iCompl1 );
            if ( p->pMatsTemp[iFan1] == 0 )
                continue;
            // complement literals
            if ( pCut1->nFans == 1 && iCompl1 )
                pCut1->Fans[0] = Abc_LitNot(pCut1->Fans[0]);
            // create new cut
            if ( iFan0 >= iFan1 )
                Amap_ManCutCreate( p, pCut0, pCut1, p->pMatsTemp[iFan1] );
            else
                Amap_ManCutCreate( p, pCut1, pCut0, p->pMatsTemp[iFan1] );
            // uncomplement literals
            if ( pCut1->nFans == 1 && iCompl1 )
                pCut1->Fans[0] = Abc_LitNot(pCut1->Fans[0]);
        }
        // uncomplement literals
        if ( pCut0->nFans == 1 && iCompl0 )
            pCut0->Fans[0] = Abc_LitNot(pCut0->Fans[0]);
        // label resulting sets
        for ( i = 0; (Entry = pRules[iFan0][i]); i++ )
            p->pMatsTemp[Entry & 0xffff] = 0;
    }
    Amap_ManCutSaveStored( p, pNode );
    p->nCutsUsed += pNode->nCuts;
    p->nCutsTried += pFanin0->nCuts * pFanin1->nCuts;
//    assert( (int)pNode->nCuts == Amap_ManMergeCountCuts(p, pNode) );
    if ( pNode->fRepr )
        Amap_ManMergeNodeChoice( p, pNode );

//    Amap_ManPrintCuts( pNode );
}

/**Function*************************************************************

  Synopsis    [Derives cuts for all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManMerge( Amap_Man_t * p )
{
    Amap_Obj_t * pObj;
    int i;
    abctime clk = Abc_Clock();
    p->pCutsPi = Amap_ManSetupPis( p );
    Amap_ManForEachNode( p, pObj, i )
        Amap_ManMergeNodeCuts( p, pObj );
    if ( p->pPars->fVerbose )
    {
        printf( "AIG object is %d bytes.  ", (int)sizeof(Amap_Obj_t) );
        printf( "Internal AIG = %5.2f MB.  Cuts = %5.2f MB.  CutsMax = %d.\n", 
            1.0*Amap_ManObjNum(p)*sizeof(Amap_Obj_t)/(1<<20), 1.0*p->nBytesUsed/(1<<20), p->pPars->nCutsMax );
        printf( "Node =%6d. Try =%9d. Try3 =%10d. Used =%7d. R =%6.2f.  ", 
            Amap_ManNodeNum(p), p->nCutsTried, p->nCutsTried3, p->nCutsUsed, 
            1.0*p->nCutsUsed/Amap_ManNodeNum(p) );
ABC_PRT( "Time ", Abc_Clock() - clk );
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

