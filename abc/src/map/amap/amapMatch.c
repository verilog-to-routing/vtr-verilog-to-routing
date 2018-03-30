/**CFile****************************************************************

  FileName    [amapMatch.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapMatch.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Duplicates the cut using new memory manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Cut_t * Amap_ManDupCut( Amap_Man_t * p, Amap_Cut_t * pCut )
{
    Amap_Cut_t * pNew;
    int nBytes = sizeof(Amap_Cut_t) + sizeof(int) * pCut->nFans;
    pNew = (Amap_Cut_t *)Aig_MmFlexEntryFetch( p->pMemCutBest, nBytes );
    memcpy( pNew, pCut, nBytes );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Starts the match with cut and set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Amap_ManMatchStart( Amap_Mat_t * p, Amap_Cut_t * pCut, Amap_Set_t * pSet )
{
    memset( p, 0, sizeof(Amap_Mat_t) );
    p->pCut = pCut;
    p->pSet = pSet;
}

/**Function*************************************************************

  Synopsis    [Cleans reference counters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManCleanRefs( Amap_Man_t * p )
{
    Amap_Obj_t * pObj;
    int i;
    Amap_ManForEachObj( p, pObj, i )
        pObj->nFouts[0] = pObj->nFouts[1] = 0;
}

/**Function*************************************************************

  Synopsis    [Computes delay.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Amap_ManMaxDelay( Amap_Man_t * p )
{
    Amap_Obj_t * pObj;
    float Delay = 0.0;
    int i;
    Amap_ManForEachPo( p, pObj, i )
        Delay = Abc_MaxInt( Delay, Amap_ObjFanin0(p,pObj)->Best.Delay );
    return Delay;
}

/**Function*************************************************************

  Synopsis    [Cleans reference counters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManCleanData( Amap_Man_t * p )
{
    Amap_Obj_t * pObj;
    int i;
//    Amap_ManForEachNode( p, pObj, i )
//        ABC_FREE( pObj->pData );
    Amap_ManForEachObj( p, pObj, i )
        pObj->pData = NULL;
}

/**Function*************************************************************

  Synopsis    [Compute nodes used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Amap_ManComputeMapping_rec( Amap_Man_t * p, Amap_Obj_t * pObj, int fCompl )
{
    Amap_Mat_t * pM = &pObj->Best;
    Amap_Obj_t * pFanin;
    Amap_Gat_t * pGate;
    int i, iFanin, fComplFanin;
    float Area;
    if ( pObj->nFouts[fCompl]++ + pObj->nFouts[!fCompl] > 0 )
        return 0.0;
    if ( Amap_ObjIsPi(pObj) || Amap_ObjIsConst1(pObj) )
        return 0.0;
    pGate = Amap_LibGate( p->pLib, pM->pSet->iGate );
    assert( pGate->nPins == pM->pCut->nFans );
    Area = pGate->dArea;
    for ( i = 0; i < (int)pGate->nPins; i++ )
    {
        iFanin = Abc_Lit2Var( pM->pSet->Ins[i] );
        pFanin = Amap_ManObj( p, Abc_Lit2Var(pM->pCut->Fans[iFanin]) );
        fComplFanin = Abc_LitIsCompl( pM->pSet->Ins[i] ) ^ Abc_LitIsCompl( pM->pCut->Fans[iFanin] );
        Area += Amap_ManComputeMapping_rec( p, pFanin, fComplFanin );
    }
    return Area;
}

/**Function*************************************************************

  Synopsis    [Compute nodes used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Amap_ManComputeMapping( Amap_Man_t * p )
{
    Amap_Obj_t * pObj;
    float Area = 0.0;
    int i;
    Amap_ManCleanRefs( p );
    Amap_ManForEachPo( p, pObj, i )
        Area += Amap_ManComputeMapping_rec( p, Amap_ObjFanin0(p, pObj), Amap_ObjFaninC0(pObj) );
    return Area;
}

/**Function*************************************************************

  Synopsis    [Counts the number of inverters to be added.]

  Description [Should be called after mapping has been set.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_ManCountInverters( Amap_Man_t * p )
{
    Amap_Obj_t * pObj;
    int i, Counter = 0;
    Amap_ManForEachObj( p, pObj, i )
        Counter += (int)(pObj->nFouts[!pObj->fPolar] > 0);
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Compare two matches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Amap_CutCompareDelay( Amap_Man_t * p, Amap_Mat_t * pM0, Amap_Mat_t * pM1 )
{
    // compare delay
    if ( pM0->Delay < pM1->Delay - p->pPars->fEpsilon )
        return -1;
    if ( pM0->Delay > pM1->Delay + p->pPars->fEpsilon )
        return 1;

    // compare area flows
    if ( pM0->Area < pM1->Area - p->pPars->fEpsilon )
        return -1;
    if ( pM0->Area > pM1->Area + p->pPars->fEpsilon )
        return 1;

    // compare average fanouts
    if ( pM0->AveFan > pM1->AveFan - p->pPars->fEpsilon )
        return -1;
    if ( pM0->AveFan < pM1->AveFan + p->pPars->fEpsilon )
        return 1;
    return 1;
}
static inline int Amap_CutCompareArea( Amap_Man_t * p, Amap_Mat_t * pM0, Amap_Mat_t * pM1 )
{
    // compare area flows
    if ( pM0->Area < pM1->Area - p->pPars->fEpsilon )
        return -1;
    if ( pM0->Area > pM1->Area + p->pPars->fEpsilon )
        return 1;

    // compare average fanouts
    if ( pM0->AveFan > pM1->AveFan - p->pPars->fEpsilon )
        return -1;
    if ( pM0->AveFan < pM1->AveFan + p->pPars->fEpsilon )
        return 1;

    // compare delay
    if ( pM0->Delay < pM1->Delay - p->pPars->fEpsilon )
        return -1;
    if ( pM0->Delay > pM1->Delay + p->pPars->fEpsilon )
        return 1;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Counts area while dereferencing the match.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float Amap_CutAreaDeref( Amap_Man_t * p, Amap_Mat_t * pM )
{
    Amap_Obj_t * pFanin;
    int i, fCompl;
    float Area = Amap_LibGate( p->pLib, pM->pSet->iGate )->dArea;
    Amap_MatchForEachFaninCompl( p, pM, pFanin, fCompl, i )
    {
        assert( Amap_ObjRefsTotal(pFanin) > 0 );
        if ( (int)pFanin->fPolar != fCompl && pFanin->nFouts[fCompl] == 1 )
            Area += p->fAreaInv;
        if ( --pFanin->nFouts[fCompl] + pFanin->nFouts[!fCompl] == 0 && Amap_ObjIsNode(pFanin) )
            Area += Amap_CutAreaDeref( p, &pFanin->Best );
    }
    return Area;
}

/**Function*************************************************************

  Synopsis    [Counts area while referencing the match.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float Amap_CutAreaRef( Amap_Man_t * p, Amap_Mat_t * pM )
{
    Amap_Obj_t * pFanin;
    int i, fCompl;
    float Area = Amap_LibGate( p->pLib, pM->pSet->iGate )->dArea;
    Amap_MatchForEachFaninCompl( p, pM, pFanin, fCompl, i )
    {
        assert( Amap_ObjRefsTotal(pFanin) >= 0 );
        if ( (int)pFanin->fPolar != fCompl && pFanin->nFouts[fCompl] == 0 )
            Area += p->fAreaInv;
        if ( pFanin->nFouts[fCompl]++ + pFanin->nFouts[!fCompl] == 0 && Amap_ObjIsNode(pFanin) )
            Area += Amap_CutAreaRef( p, &pFanin->Best );
    }
    return Area;
}

/**Function*************************************************************

  Synopsis    [Derives area of the match for a non-referenced node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline float Amap_CutAreaDerefed( Amap_Man_t * p, Amap_Obj_t * pNode, Amap_Mat_t * pM )
{
    float aResult, aResult2;
    int fComplNew;
    aResult2 = Amap_CutAreaRef( p, pM );
    aResult  = Amap_CutAreaDeref( p, pM );
    assert( aResult > aResult2 - p->fEpsilonInternal );
    assert( aResult < aResult2 + p->fEpsilonInternal );
    // if node is needed in another polarity, add inverter
    fComplNew = pM->pCut->fInv ^ pM->pSet->fInv;
    if ( pNode->nFouts[fComplNew] == 0 && pNode->nFouts[!fComplNew] > 0 )
        aResult += p->fAreaInv;
    return aResult;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Amap_CutAreaTest( Amap_Man_t * p, Amap_Obj_t * pNode )
{
    float aResult, aResult2;
    if ( Amap_ObjRefsTotal(pNode) == 0 )
    {
        aResult2 = Amap_CutAreaRef( p, &pNode->Best );
        aResult  = Amap_CutAreaDeref( p, &pNode->Best );
        assert( aResult > aResult2 - p->fEpsilonInternal );
        assert( aResult < aResult2 + p->fEpsilonInternal );
    }
    else
    {
        aResult  = Amap_CutAreaDeref( p, &pNode->Best );
        aResult2 = Amap_CutAreaRef( p, &pNode->Best );
        assert( aResult > aResult2 - p->fEpsilonInternal );
        assert( aResult < aResult2 + p->fEpsilonInternal );
    }
}

/**Function*************************************************************

  Synopsis    [Derives parameters for the match.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Amap_ManMatchGetFlows( Amap_Man_t * p, Amap_Mat_t * pM )
{
    Amap_Mat_t * pMFanin;
    Amap_Obj_t * pFanin;
    Amap_Gat_t * pGate;
    float AddOn; 
    int i;
    pGate = Amap_LibGate( p->pLib, pM->pSet->iGate );
    assert( pGate->nPins == pM->pCut->nFans );
    assert( pM->Area == 0.0 );
    pM->Area = pGate->dArea;
    pM->AveFan = 0.0;
    pM->Delay = 0.0;
    Amap_MatchForEachFanin( p, pM, pFanin, i )
    {
        pMFanin = &pFanin->Best;
        pM->Delay = Abc_MaxInt( pM->Delay, pMFanin->Delay );
        pM->AveFan += Amap_ObjRefsTotal(pFanin);
//        if ( Amap_ObjRefsTotal(pFanin) == 0 )
//            pM->Area += pMFanin->Area;
//        else
//            pM->Area += pMFanin->Area / pFanin->EstRefs;
        AddOn = Amap_ObjRefsTotal(pFanin) == 0 ? pMFanin->Area : pMFanin->Area / pFanin->EstRefs; 
        if ( pM->Area >= (float)1e32 || AddOn >= (float)1e32 )
            pM->Area = (float)1e32;
        else 
            pM->Area += AddOn;
    }
    pM->AveFan /= pGate->nPins;
    pM->Delay += 1.0;
}

/**Function*************************************************************

  Synopsis    [Derives parameters for the match.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Amap_ManMatchGetExacts( Amap_Man_t * p, Amap_Obj_t * pNode, Amap_Mat_t * pM )
{
    Amap_Mat_t * pMFanin;
    Amap_Obj_t * pFanin;
    Amap_Gat_t * pGate;
    int i;
    pGate = Amap_LibGate( p->pLib, pM->pSet->iGate );
    assert( pGate->nPins == pM->pCut->nFans );
    assert( pM->Area == 0.0 );
    pM->AveFan = 0.0;
    pM->Delay = 0.0;
    Amap_MatchForEachFanin( p, pM, pFanin, i )
    {
        pMFanin = &pFanin->Best;
        pM->Delay = Abc_MaxInt( pM->Delay, pMFanin->Delay );
        pM->AveFan += Amap_ObjRefsTotal(pFanin);
    }
    pM->AveFan /= pGate->nPins;
    pM->Delay += 1.0;
    pM->Area = Amap_CutAreaDerefed( p, pNode, pM );
}

/**Function*************************************************************

  Synopsis    [Computes the best match at each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManMatchNode( Amap_Man_t * p, Amap_Obj_t * pNode, int fFlow, int fRefs )
{
    int fVerbose = 0; //(pNode->Level == 2 || pNode->Level == 4);
    int fVeryVerbose = fVerbose;

    Amap_Mat_t MA = {0}, MD = {0}, M = {0};
    Amap_Mat_t * pMBestA = &MA, * pMBestD = &MD, * pMThis = &M, * pMBest;
    Amap_Cut_t * pCut;
    Amap_Set_t * pSet;
    Amap_Nod_t * pNod;
    int i;

    if ( fRefs )
        pNode->EstRefs = (float)((2.0 * pNode->EstRefs + Amap_ObjRefsTotal(pNode)) / 3.0);
    else
        pNode->EstRefs = (float)pNode->nRefs;
    if ( fRefs && Amap_ObjRefsTotal(pNode) > 0 )
        Amap_CutAreaDeref( p, &pNode->Best );

    if ( fVerbose )
        printf( "\nNode %d (%d)\n", pNode->Id, pNode->Level );

    pMBestA->pCut = pMBestD->pCut = NULL;
    Amap_NodeForEachCut( pNode, pCut, i )
    {
        if ( pCut->iMat == 0 )
            continue;
        pNod = Amap_LibNod( p->pLib, pCut->iMat );
        Amap_LibNodeForEachSet( pNod, pSet )
        {
            Amap_ManMatchStart( pMThis, pCut, pSet );
            if ( fFlow )
                Amap_ManMatchGetFlows( p, pMThis );
            else
                Amap_ManMatchGetExacts( p, pNode, pMThis );
            if ( pMBestD->pCut == NULL || Amap_CutCompareDelay(p, pMBestD, pMThis) == 1 )
                *pMBestD = *pMThis;
            if ( pMBestA->pCut == NULL || Amap_CutCompareArea(p, pMBestA, pMThis) == 1 )
                *pMBestA = *pMThis;

            if ( fVeryVerbose ) 
            {
                printf( "Cut %2d (%d) :  ", i, pCut->nFans );
                printf( "Gate %10s  ",      Amap_LibGate(p->pLib, pMThis->pSet->iGate)->pName );
                printf( "%s  ",             pMThis->pSet->fInv ? "inv" : "   " );
                printf( "Delay %5.2f  ",    pMThis->Delay );
                printf( "Area %5.2f  ",     pMThis->Area );
                printf( "\n" );
            }
        }
    }

    if ( Abc_AbsFloat(pMBestA->Area - pMBestD->Area) / pMBestD->Area >= p->pPars->fADratio * Abc_AbsFloat(pMBestA->Delay - pMBestD->Delay) / pMBestA->Delay )
        pMBest = pMBestA;
    else
        pMBest = pMBestD;

    if ( fVerbose )
    {
        printf( "BEST MATCHA:  " );
        printf( "Gate %10s  ",    Amap_LibGate(p->pLib, pMBestA->pSet->iGate)->pName );
        printf( "%s  ",           pMBestA->pSet->fInv ? "inv" : "   " );
        printf( "Delay %5.2f  ",  pMBestA->Delay );
        printf( "Area %5.2f  ",   pMBestA->Area );
        printf( "\n" ); 

        printf( "BEST MATCHD:  " );
        printf( "Gate %10s  ",    Amap_LibGate(p->pLib, pMBestD->pSet->iGate)->pName );
        printf( "%s  ",           pMBestD->pSet->fInv ? "inv" : "   " );
        printf( "Delay %5.2f  ",  pMBestD->Delay );
        printf( "Area %5.2f  ",   pMBestD->Area );
        printf( "\n" ); 

        printf( "BEST MATCH :  " );
        printf( "Gate %10s  ",    Amap_LibGate(p->pLib, pMBest->pSet->iGate)->pName );
        printf( "%s  ",           pMBest->pSet->fInv ? "inv" : "   " );
        printf( "Delay %5.2f  ",  pMBest->Delay );
        printf( "Area %5.2f  ",   pMBest->Area );
        printf( "\n" ); 
    }

    pNode->fPolar = pMBest->pCut->fInv ^ pMBest->pSet->fInv;
    pNode->Best = *pMBest;
    pNode->Best.pCut = Amap_ManDupCut( p, pNode->Best.pCut );
    if ( fRefs && Amap_ObjRefsTotal(pNode) > 0 )
        Amap_CutAreaRef( p, &pNode->Best );
}

/**Function*************************************************************

  Synopsis    [Performs one round of mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManMatch( Amap_Man_t * p, int fFlow, int fRefs )
{
    Aig_MmFlex_t * pMemOld;
    Amap_Obj_t * pObj;
    float Area;
    int i, nInvs;
    abctime clk = Abc_Clock();
    pMemOld = p->pMemCutBest;
    p->pMemCutBest = Aig_MmFlexStart();
    Amap_ManForEachNode( p, pObj, i )
        if ( pObj->pData )
            Amap_ManMatchNode( p, pObj, fFlow, fRefs );
    Aig_MmFlexStop( pMemOld, 0 );
    Area = Amap_ManComputeMapping( p );
    nInvs = Amap_ManCountInverters( p );
if ( p->pPars->fVerbose )
{
    printf( "Area =%9.2f. Gate =%9.2f. Inv =%9.2f. (%6d.) Delay =%6.2f. ", 
        Area + nInvs * p->fAreaInv, 
        Area, nInvs * p->fAreaInv, nInvs,
        Amap_ManMaxDelay(p) );
ABC_PRT( "Time ", Abc_Clock() - clk );
}
    // test procedures
//    Amap_ManForEachNode( p, pObj, i )
//        Amap_CutAreaTest( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Performs mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManMap( Amap_Man_t * p )
{
    int i;
    Amap_ManMerge( p );
    for ( i = 0; i < p->pPars->nIterFlow; i++ )
        Amap_ManMatch( p, 1, i>0 );
    for ( i = 0; i < p->pPars->nIterArea; i++ )
        Amap_ManMatch( p, 0, p->pPars->nIterFlow>0||i>0 );
/*
    for ( i = 0; i < p->pPars->nIterFlow; i++ )
        Amap_ManMatch( p, 1, 1 );
    for ( i = 0; i < p->pPars->nIterArea; i++ )
        Amap_ManMatch( p, 0, 1 );
*/
    Amap_ManCleanData( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

