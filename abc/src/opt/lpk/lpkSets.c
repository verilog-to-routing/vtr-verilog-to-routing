/**CFile****************************************************************

  FileName    [lpkSets.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast Boolean matching for LUT structures.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: lpkSets.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "lpkInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Lpk_Set_t_ Lpk_Set_t;
struct Lpk_Set_t_
{
    char         iVar;        // the cofactoring variable
    char         Over;        // the overlap in supports
    char         SRed;        // the support reduction
    char         Size;        // the size of the boundset
    unsigned     uSubset0;    // the first subset (with removed)
    unsigned     uSubset1;    // the second subset (with removed)
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Recursively computes decomposable subsets.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Lpk_ComputeSets_rec( Kit_DsdNtk_t * p, int iLit, Vec_Int_t * vSets )
{
    unsigned i, iLitFanin, uSupport, uSuppCur;
    Kit_DsdObj_t * pObj;
    // consider the case of simple gate
    pObj = Kit_DsdNtkObj( p, Abc_Lit2Var(iLit) );
    if ( pObj == NULL )
        return (1 << Abc_Lit2Var(iLit));
    if ( pObj->Type == KIT_DSD_AND || pObj->Type == KIT_DSD_XOR )
    {
        unsigned uSupps[16], Limit, s;
        uSupport = 0;
        Kit_DsdObjForEachFanin( p, pObj, iLitFanin, i )
        {
            uSupps[i] = Lpk_ComputeSets_rec( p, iLitFanin, vSets );
            uSupport |= uSupps[i];
        }
        // create all subsets, except empty and full
        Limit = (1 << pObj->nFans) - 1;
        for ( s = 1; s < Limit; s++ )
        {
            uSuppCur = 0;
            for ( i = 0; i < pObj->nFans; i++ )
                if ( s & (1 << i) )
                    uSuppCur |= uSupps[i];
            Vec_IntPush( vSets, uSuppCur );
        }
        return uSupport;
    }
    assert( pObj->Type == KIT_DSD_PRIME );
    // get the cumulative support of all fanins
    uSupport = 0;
    Kit_DsdObjForEachFanin( p, pObj, iLitFanin, i )
    {
        uSuppCur  = Lpk_ComputeSets_rec( p, iLitFanin, vSets );
        uSupport |= uSuppCur;
        Vec_IntPush( vSets, uSuppCur );
    }
    return uSupport;
}

/**Function*************************************************************

  Synopsis    [Computes the set of subsets of decomposable variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Lpk_ComputeSets( Kit_DsdNtk_t * p, Vec_Int_t * vSets )
{
    unsigned uSupport, Entry;
    int Number, i;
    assert( p->nVars <= 16 );
    Vec_IntClear( vSets );
    Vec_IntPush( vSets, 0 );
    if ( Kit_DsdNtkRoot(p)->Type == KIT_DSD_CONST1 )
        return 0;
    if ( Kit_DsdNtkRoot(p)->Type == KIT_DSD_VAR )
    {
        uSupport = ( 1 << Abc_Lit2Var(Kit_DsdNtkRoot(p)->pFans[0]) );
        Vec_IntPush( vSets, uSupport );
        return uSupport;
    }
    uSupport = Lpk_ComputeSets_rec( p, p->Root, vSets );
    assert( (uSupport & 0xFFFF0000) == 0 );
    Vec_IntPush( vSets, uSupport );
    // set the remaining variables
    Vec_IntForEachEntry( vSets, Number, i )
    {
        Entry = Number;
        Vec_IntWriteEntry( vSets, i, Entry | ((uSupport & ~Entry) << 16) );
    }
    return uSupport;
}

/**Function*************************************************************

  Synopsis    [Prints the sets of subsets.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Lpk_PrintSetOne( int uSupport )
{
    unsigned k;
    for ( k = 0; k < 16; k++ )
        if ( uSupport & (1<<k) )
            printf( "%c", 'a'+k );
    printf( " " );
}
/**Function*************************************************************

  Synopsis    [Prints the sets of subsets.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Lpk_PrintSets( Vec_Int_t * vSets )
{
    unsigned uSupport;
    int Number, i;
    printf( "Subsets(%d): ", Vec_IntSize(vSets) );
    Vec_IntForEachEntry( vSets, Number, i )
    {
        uSupport = Number;
        Lpk_PrintSetOne( uSupport );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Computes maximal support reducing bound-sets.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lpk_ComposeSets( Vec_Int_t * vSets0, Vec_Int_t * vSets1, int nVars, int iCofVar,
    Lpk_Set_t * pStore, int * pSize, int nSizeLimit )
{
    static int nTravId = 0;            // the number of the times this is visited
    static int TravId[1<<16] = {0};    // last visited
    static char SRed[1<<16];           // best support reduction
    static char Over[1<<16];           // best overlaps
    static unsigned Parents[1<<16];    // best set of parents
    static unsigned short Used[1<<16]; // storage for used subsets
    int nSuppSize, nSuppOver, nSuppRed, nUsed, nMinOver, i, k, s;
    unsigned Entry, Entry0, Entry1;
    unsigned uSupp, uSupp0, uSupp1, uSuppTotal;
    Lpk_Set_t * pEntry;

    if ( nTravId == (1 << 30) )
        memset( TravId, 0, sizeof(int) * (1 << 16) );

    // collect support reducing subsets
    nUsed = 0;
    nTravId++;
    uSuppTotal = Kit_BitMask(nVars) & ~(1<<iCofVar);
    Vec_IntForEachEntry( vSets0, Entry0, i )
    Vec_IntForEachEntry( vSets1, Entry1, k )
    {
        uSupp0 = (Entry0 & 0xFFFF);
        uSupp1 = (Entry1 & 0xFFFF);
        // skip trivial
        if ( uSupp0 == 0 || uSupp1 == 0 || (uSupp0 | uSupp1) == uSuppTotal )
            continue;
        if ( Kit_WordHasOneBit(uSupp0) && Kit_WordHasOneBit(uSupp1) )
            continue;
        // get the entry
        Entry = Entry0 | Entry1;
        uSupp = Entry & 0xFFFF;
        // set the bound set size
        nSuppSize = Kit_WordCountOnes( uSupp );
        // get the number of overlapping vars
        nSuppOver = Kit_WordCountOnes( Entry & (Entry >> 16) );
        // get the support reduction
        nSuppRed  = nSuppSize - 1 - nSuppOver;
        // only consider support-reducing subsets
        if ( nSuppRed <= 0 )
            continue;
        // check if this support is already used
        if ( TravId[uSupp] < nTravId )
        {
            Used[nUsed++] = uSupp;

            TravId[uSupp] = nTravId;
            SRed[uSupp] = nSuppRed;
            Over[uSupp] = nSuppOver;
            Parents[uSupp] = (k << 16) | i;
        }
        else if ( TravId[uSupp] == nTravId && SRed[uSupp] < nSuppRed )
        {
            TravId[uSupp] = nTravId;
            SRed[uSupp] = nSuppRed;
            Over[uSupp] = nSuppOver;
            Parents[uSupp] = (k << 16) | i;
        }
    }

    // find the minimum overlap
    nMinOver = 1000;
    for ( s = 0; s < nUsed; s++ )
        if ( nMinOver > Over[Used[s]] )
            nMinOver = Over[Used[s]];


    // collect the accumulated ones
    for ( s = 0; s < nUsed; s++ )
        if ( Over[Used[s]] == nMinOver )
        {
            // save the entry
            if ( *pSize == nSizeLimit )
                return;
            pEntry = pStore + (*pSize)++;

            i = Parents[Used[s]] & 0xFFFF;
            k = Parents[Used[s]] >> 16;

            pEntry->uSubset0 = Vec_IntEntry(vSets0, i);
            pEntry->uSubset1 = Vec_IntEntry(vSets1, k);
            Entry  = pEntry->uSubset0 | pEntry->uSubset1;

            // record the cofactoring variable
            pEntry->iVar = iCofVar;
            // set the bound set size
            pEntry->Size = Kit_WordCountOnes( Entry & 0xFFFF );
            // get the number of overlapping vars
            pEntry->Over = Kit_WordCountOnes( Entry & (Entry >> 16) );
            // get the support reduction
            pEntry->SRed = pEntry->Size - 1 - pEntry->Over;
            assert( pEntry->SRed > 0 );
        }
}

/**Function*************************************************************

  Synopsis    [Prints one set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lpk_MapSuppPrintSet( Lpk_Set_t * pSet, int i )
{
    unsigned Entry;
    Entry = pSet->uSubset0 | pSet->uSubset1;
    printf( "%2d : ", i );
    printf( "Var = %c  ",  'a' + pSet->iVar );
    printf( "Size = %2d  ", pSet->Size );
    printf( "Over = %2d  ", pSet->Over );
    printf( "SRed = %2d  ", pSet->SRed );
    Lpk_PrintSetOne( Entry );
    printf( "              " );
    Lpk_PrintSetOne( Entry >> 16 );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Evaluates the cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Lpk_MapSuppRedDecSelect( Lpk_Man_t * p, unsigned * pTruth, int nVars, int * piVar, int * piVarReused )
{
    static int nStoreSize = 256;
    static Lpk_Set_t pStore[256], * pSet, * pSetBest;
    Kit_DsdNtk_t * ppNtks[2], * pTemp;
    Vec_Int_t * vSets0 = p->vSets[0];
    Vec_Int_t * vSets1 = p->vSets[1];
    unsigned * pCof0 = (unsigned *)Vec_PtrEntry( p->vTtNodes, 0 );
    unsigned * pCof1 = (unsigned *)Vec_PtrEntry( p->vTtNodes, 1 );
    int nSets, i, SizeMax;//, SRedMax;
    unsigned Entry;
    int fVerbose = p->pPars->fVeryVerbose;
//    int fVerbose = 0;

    // collect decomposable subsets for each pair of cofactors
    if ( fVerbose )
    {
        printf( "\nExploring support-reducing bound-sets of function:\n" );
        Kit_DsdPrintFromTruth( pTruth, nVars );
    }
    nSets = 0;
    for ( i = 0; i < nVars; i++ )
    {
        if ( fVerbose )
        printf( "Evaluating variable %c:\n", 'a'+i );
        // evaluate the cofactor pair
        Kit_TruthCofactor0New( pCof0, pTruth, nVars, i );
        Kit_TruthCofactor1New( pCof1, pTruth, nVars, i );
        // decompose and expand
        ppNtks[0] = Kit_DsdDecompose( pCof0, nVars );
        ppNtks[1] = Kit_DsdDecompose( pCof1, nVars );
        ppNtks[0] = Kit_DsdExpand( pTemp = ppNtks[0] );      Kit_DsdNtkFree( pTemp );
        ppNtks[1] = Kit_DsdExpand( pTemp = ppNtks[1] );      Kit_DsdNtkFree( pTemp );
        if ( fVerbose )
        Kit_DsdPrint( stdout, ppNtks[0] );
        if ( fVerbose )
        Kit_DsdPrint( stdout, ppNtks[1] );
        // compute subsets
        Lpk_ComputeSets( ppNtks[0], vSets0 );
        Lpk_ComputeSets( ppNtks[1], vSets1 );
        // print subsets
        if ( fVerbose )
        Lpk_PrintSets( vSets0 );
        if ( fVerbose )
        Lpk_PrintSets( vSets1 );
        // free the networks
        Kit_DsdNtkFree( ppNtks[0] );
        Kit_DsdNtkFree( ppNtks[1] );
        // evaluate the pair
        Lpk_ComposeSets( vSets0, vSets1, nVars, i, pStore, &nSets, nStoreSize );
    }

    // print the results
    if ( fVerbose )
    printf( "\n" );
    if ( fVerbose )
    for ( i = 0; i < nSets; i++ )
        Lpk_MapSuppPrintSet( pStore + i, i );

    // choose the best subset
    SizeMax = 0;
    pSetBest = NULL;
    for ( i = 0; i < nSets; i++ )
    {
        pSet = pStore + i;
        if ( pSet->Size > p->pPars->nLutSize - 1 )
            continue;
        if ( SizeMax < pSet->Size )
        {
            pSetBest = pSet;
            SizeMax = pSet->Size;
        }
    }
/*
    // if the best is not choosen, select the one with largest reduction
    SRedMax = 0;
    if ( pSetBest == NULL )
    {
        for ( i = 0; i < nSets; i++ )
        {
            pSet = pStore + i;
            if ( SRedMax < pSet->SRed )
            {
                pSetBest = pSet;
                SRedMax = pSet->SRed;
            }
        }
    }
*/
    if ( pSetBest == NULL )
    {
        if ( fVerbose )
        printf( "Could not select a subset.\n" );
        return 0;
    }
    else
    {
        if ( fVerbose )
        printf( "Selected the following subset:\n" );
        if ( fVerbose )
        Lpk_MapSuppPrintSet( pSetBest, pSetBest - pStore );
    }

    // prepare the return result
    // get the remaining variables
    Entry = ((pSetBest->uSubset0 >> 16) | (pSetBest->uSubset1 >> 16));
    // get the variables to be removed
    Entry = Kit_BitMask(nVars) & ~(1<<pSetBest->iVar) & ~Entry;
    // make sure there are some - otherwise it is not supp-red
    assert( Entry );
    // remember the first such variable
    *piVarReused = Kit_WordFindFirstBit( Entry );
    *piVar = pSetBest->iVar;
    return (pSetBest->uSubset1 << 16) | (pSetBest->uSubset0 & 0xFFFF);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

