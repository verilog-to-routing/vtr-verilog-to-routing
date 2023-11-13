/**CFile****************************************************************

  FileName    [aigCanon.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Processing the library of semi-canonical AIGs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigCanon.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"
#include "bool/kit/kit.h"
#include "bool/bdc/bdc.h"
#include "aig/ioa/ioa.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define RMAN_MAXVARS  12
#define RMAX_MAXWORD  (RMAN_MAXVARS <= 5 ? 1 : (1 << (RMAN_MAXVARS - 5)))

typedef struct Aig_VSig_t_ Aig_VSig_t;
struct Aig_VSig_t_
{
    int           nOnes;
    int           nCofOnes[RMAN_MAXVARS];
};

typedef struct Aig_Tru_t_ Aig_Tru_t;
struct Aig_Tru_t_
{
    Aig_Tru_t *   pNext;
    int           Id;    
    unsigned      nVisits : 27;
    unsigned      nVars   :  5;
    unsigned      pTruth[0];
};

typedef struct Aig_RMan_t_ Aig_RMan_t;
struct Aig_RMan_t_
{
    int           nVars;       // the largest variable number
    Aig_Man_t *   pAig;        // recorded subgraphs
    // hash table
    int           nBins;
    Aig_Tru_t **  pBins;
    int           nEntries;
    Aig_MmFlex_t* pMemTrus;
    // bidecomposion
    Bdc_Man_t *   pBidec;
    // temporaries
    unsigned      pTruthInit[RMAX_MAXWORD]; // canonical truth table
    unsigned      pTruth[RMAX_MAXWORD];     // current truth table
    unsigned      pTruthC[RMAX_MAXWORD];    // canonical truth table
    unsigned      pTruthTemp[RMAX_MAXWORD]; // temporary truth table
    Aig_VSig_t    pMints[2*RMAN_MAXVARS];   // minterm count
    char          pPerm[RMAN_MAXVARS];      // permutation
    char          pPermR[RMAN_MAXVARS];     // reverse permutation
    // statistics
    int           nVarFuncs[RMAN_MAXVARS+1];
    int           nTotal;
    int           nTtDsd;
    int           nTtDsdPart;
    int           nTtDsdNot;
    int           nUniqueVars;
};

static Aig_RMan_t * s_pRMan = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates recording manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_RMan_t * Aig_RManStart()
{
    static Bdc_Par_t Pars = {0}, * pPars = &Pars;
    Aig_RMan_t * p;
    p = ABC_ALLOC( Aig_RMan_t, 1 );
    memset( p, 0, sizeof(Aig_RMan_t) );
    p->nVars = RMAN_MAXVARS;
    p->pAig  = Aig_ManStart( 1000000 );
    Aig_IthVar( p->pAig, p->nVars-1 );
    // create hash table
    p->nBins = Abc_PrimeCudd(5000);
    p->pBins = ABC_CALLOC( Aig_Tru_t *, p->nBins );
    p->pMemTrus = Aig_MmFlexStart();
    // bi-decomposition manager
    pPars->nVarsMax = p->nVars;
    pPars->fVerbose = 0;
    p->pBidec = Bdc_ManAlloc( pPars );
    return p;
}

/**Function*************************************************************

  Synopsis    [Returns the hash key.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_RManTableHash( unsigned * pTruth, int nVars, int nBins, int * pPrimes )
{
    int i, nWords = Kit_TruthWordNum( nVars );
    unsigned uHash = 0;
    for ( i = 0; i < nWords; i++ )
        uHash ^= pTruth[i] * pPrimes[i & 0xf];
    return (int)(uHash % nBins);
}

/**Function*************************************************************

  Synopsis    [Returns the given record.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Tru_t ** Aig_RManTableLookup( Aig_RMan_t * p, unsigned * pTruth, int nVars )
{
    static int s_Primes[16] = { 
        1291, 1699, 1999, 2357, 2953, 3313, 3907, 4177, 
        4831, 5147, 5647, 6343, 6899, 7103, 7873, 8147 };
    Aig_Tru_t ** ppSpot, * pEntry;
    ppSpot = p->pBins + Aig_RManTableHash( pTruth, nVars, p->nBins, s_Primes );
    for ( pEntry = *ppSpot; pEntry; ppSpot = &pEntry->pNext, pEntry = pEntry->pNext )
        if ( Kit_TruthIsEqual( pEntry->pTruth, pTruth, nVars ) )
            return ppSpot;
    return ppSpot;
}

/**Function*************************************************************

  Synopsis    [Find or add new entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_RManTableResize( Aig_RMan_t * p )
{
    Aig_Tru_t * pEntry, * pNext;
    Aig_Tru_t ** pBinsOld, ** ppPlace;
    int nBinsOld, Counter, i;
    abctime clk;
    assert( p->pBins != NULL );
clk = Abc_Clock();
    // save the old Bins
    pBinsOld = p->pBins;
    nBinsOld = p->nBins;
    // get the new Bins
    p->nBins = Abc_PrimeCudd( 3 * nBinsOld ); 
    p->pBins = ABC_CALLOC( Aig_Tru_t *, p->nBins );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < nBinsOld; i++ )
    for ( pEntry = pBinsOld[i], pNext = pEntry? pEntry->pNext : NULL; 
          pEntry; pEntry = pNext, pNext = pEntry? pEntry->pNext : NULL )
    {
        // get the place where this entry goes in the Bins 
        ppPlace = Aig_RManTableLookup( p, pEntry->pTruth, pEntry->nVars );
        assert( *ppPlace == NULL ); // should not be there
        // add the entry to the list
        *ppPlace = pEntry;
        pEntry->pNext = NULL;
        Counter++;
    }
    assert( Counter == p->nEntries );
//    ABC_PRT( "Time", Abc_Clock() - clk );
    ABC_FREE( pBinsOld );
}

/**Function*************************************************************

  Synopsis    [Find or add new entry.]

  Description [Returns 1 if this is a new entry.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_RManTableFindOrAdd( Aig_RMan_t * p, unsigned * pTruth, int nVars )
{
    Aig_Tru_t ** ppSpot, * pEntry;
    int nBytes;
    ppSpot = Aig_RManTableLookup( p, pTruth, nVars );
    if ( *ppSpot )
    {
        (*ppSpot)->nVisits++;
        return 0;
    }
    nBytes = sizeof(Aig_Tru_t) + sizeof(unsigned) * Kit_TruthWordNum(nVars);
    if ( p->nEntries == 3*p->nBins )
        Aig_RManTableResize( p );
    pEntry = (Aig_Tru_t *)Aig_MmFlexEntryFetch( p->pMemTrus, nBytes );
    pEntry->Id = p->nEntries++;
    pEntry->nVars = nVars;
    pEntry->nVisits = 1;
    pEntry->pNext = NULL;
    memcpy( pEntry->pTruth, pTruth, sizeof(unsigned) * Kit_TruthWordNum(nVars) );
    *ppSpot = pEntry;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Deallocates recording manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_RManStop( Aig_RMan_t * p )
{
    int i;
    printf( "Total funcs    = %10d\n", p->nTotal );
    printf( "Full DSD funcs = %10d\n", p->nTtDsd );
    printf( "Part DSD funcs = %10d\n", p->nTtDsdPart );
    printf( "Non- DSD funcs = %10d\n", p->nTtDsdNot );
    printf( "Uniq-var funcs = %10d\n", p->nUniqueVars );
    printf( "Unique   funcs = %10d\n", p->nEntries );
    printf( "Distribution of functions:\n" );
    for ( i = 5; i <= p->nVars; i++ )
        printf( "%2d = %8d\n", i, p->nVarFuncs[i] );
    Aig_MmFlexStop( p->pMemTrus, 0 );
    Aig_ManStop( p->pAig );
    Bdc_ManFree( p->pBidec );
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Stops recording.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_RManQuit()
{
//    extern void Ioa_WriteAiger( Aig_Man_t * pMan, char * pFileName, int fWriteSymbols, int fCompact );
    char Buffer[20];
    if ( s_pRMan == NULL )
        return;
    // dump the library file
    sprintf( Buffer, "aiglib%02d.aig", s_pRMan->nVars );
    Ioa_WriteAiger( s_pRMan->pAig, Buffer, 0, 1 );
    // quit the manager
    Aig_RManStop( s_pRMan );
    s_pRMan = NULL;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if all variables are unique.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_RManPrintVarProfile( unsigned * pTruth, int nVars,  unsigned * pTruthAux )
{
    int pStore2[32];
    int i;
    Kit_TruthCountOnesInCofsSlow( pTruth, nVars, pStore2, pTruthAux );
    for ( i = 0; i < nVars; i++ )
        printf( "%2d/%2d ", pStore2[2*i], pStore2[2*i+1] );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Sorts numbers in the increasing order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_RManSortNums( int * pArray, int nVars )
{
    int i, j, best_i, tmp;
    for ( i = 0; i < nVars-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nVars; j++ )
            if ( pArray[j] > pArray[best_i] )
                best_i = j;
        tmp = pArray[i]; pArray[i] = pArray[best_i]; pArray[best_i] = tmp;
    }
}

/**Function*************************************************************

  Synopsis    [Prints signatures for all variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_RManPrintSigs( Aig_VSig_t * pSigs, int nVars )
{
    int v, i, k;
    for ( v = 0; v < nVars; v++ )
    {
        printf( "%2d : ", v );
        for ( k = 0; k < 2; k++ )
        {
            printf( "%5d  ", pSigs[2*v+k].nOnes );
            printf( "(" );
            for ( i = 0; i < nVars; i++ )
                printf( "%4d ", pSigs[2*v+k].nCofOnes[i] );
            printf( ")  " );
        }
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Computes signatures for all variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_RManComputeVSigs( unsigned * pTruth, int nVars, Aig_VSig_t * pSigs, unsigned * pAux )
{
    int v;
    for ( v = 0; v < nVars; v++ )
    {
        Kit_TruthCofactor0New( pAux, pTruth, nVars, v );
        pSigs[2*v+0].nOnes = Kit_TruthCountOnes( pAux, nVars );
        Kit_TruthCountOnesInCofs0( pAux, nVars, pSigs[2*v+0].nCofOnes );
        Aig_RManSortNums( pSigs[2*v+0].nCofOnes, nVars );

        Kit_TruthCofactor1New( pAux, pTruth, nVars, v );
        pSigs[2*v+1].nOnes = Kit_TruthCountOnes( pAux, nVars );
        Kit_TruthCountOnesInCofs0( pAux, nVars, pSigs[2*v+1].nCofOnes );
        Aig_RManSortNums( pSigs[2*v+1].nCofOnes, nVars );
    }
}

/**Function*************************************************************

  Synopsis    [Computs signatures for all variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_RManCompareSigs( Aig_VSig_t * p0, Aig_VSig_t * p1, int nVars )
{
//    return memcmp( p0, p1, sizeof(int) + sizeof(int) * nVars );
    return memcmp( p0, p1, sizeof(int) );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if all variables are unique.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_RManVarsAreUnique( Aig_VSig_t * pMints, int nVars )
{
    int i;
    for ( i = 0; i < nVars - 1; i++ )
        if ( Aig_RManCompareSigs( &pMints[2*i], &pMints[2*(i+1)], nVars ) == 0 )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if all variables are unique.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_RManPrintUniqueVars( Aig_VSig_t * pMints, int nVars )
{
    int i;
    for ( i = 0; i < nVars; i++ )
        if ( Aig_RManCompareSigs( &pMints[2*i], &pMints[2*i+1], nVars ) == 0 )
            printf( "=" );
        else 
            printf( "x" );
    printf( "\n" );

    printf( "0" );
    for ( i = 1; i < nVars; i++ )
        if ( Aig_RManCompareSigs( &pMints[2*(i-1)], &pMints[2*i], nVars ) == 0 )
            printf( "-" );
        else if ( i < 10 )
            printf( "%c", '0' + i );
        else 
            printf( "%c", 'A' + i-10 );
    printf( "\n" );
}
 
/**Function*************************************************************

  Synopsis    [Canonicize the truth table.]

  Description [Returns the phase. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Aig_RManSemiCanonicize( unsigned * pOut, unsigned * pIn, int nVars, char * pCanonPerm, Aig_VSig_t * pSigs, int fReturnIn )
{
    Aig_VSig_t TempSig;
    int i, Temp, fChange, Counter;
    unsigned * pTemp, uCanonPhase = 0;
    // collect signatures 
    Aig_RManComputeVSigs( pIn, nVars, pSigs, pOut );
    // canonicize phase
    for ( i = 0; i < nVars; i++ )
    {
//        if ( pStore[2*i+0] <= pStore[2*i+1] )
        if ( Aig_RManCompareSigs( &pSigs[2*i+0], &pSigs[2*i+1], nVars ) <= 0 )
            continue;
        uCanonPhase |= (1 << i);
        TempSig = pSigs[2*i+0];
        pSigs[2*i+0] = pSigs[2*i+1];
        pSigs[2*i+1] = TempSig;
        Kit_TruthChangePhase( pIn, nVars, i );
    }
    // permute
    Counter = 0;
    do {
        fChange = 0;
        for ( i = 0; i < nVars-1; i++ )
        {
//            if ( pStore[2*i] <= pStore[2*(i+1)] )
            if ( Aig_RManCompareSigs( &pSigs[2*i], &pSigs[2*(i+1)], nVars ) <= 0 )
                continue;
            Counter++;
            fChange = 1;

            Temp = pCanonPerm[i];
            pCanonPerm[i] = pCanonPerm[i+1];
            pCanonPerm[i+1] = Temp;

            TempSig = pSigs[2*i];
            pSigs[2*i] = pSigs[2*(i+1)];
            pSigs[2*(i+1)] = TempSig;

            TempSig = pSigs[2*i+1];
            pSigs[2*i+1] = pSigs[2*(i+1)+1];
            pSigs[2*(i+1)+1] = TempSig;

            Kit_TruthSwapAdjacentVars( pOut, pIn, nVars, i );
            pTemp = pIn; pIn = pOut; pOut = pTemp;
        }
    } while ( fChange );

    // swap if it was moved an even number of times
    if ( fReturnIn ^ !(Counter & 1) )
        Kit_TruthCopy( pOut, pIn, nVars );
    return uCanonPhase;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Aig_Obj_t * Bdc_FunCopyHop( Bdc_Fun_t * pObj )  
{ return Aig_NotCond( (Aig_Obj_t *)Bdc_FuncCopy(Bdc_Regular(pObj)), Bdc_IsComplement(pObj) );  }

/**Function*************************************************************

  Synopsis    [Records one function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_RManSaveOne( Aig_RMan_t * p, unsigned * pTruth, int nVars )
{
    int i, nNodes, RetValue;
    Bdc_Fun_t * pFunc;
    Aig_Obj_t * pTerm;
    // perform decomposition
    RetValue = Bdc_ManDecompose( p->pBidec, pTruth, NULL, nVars, NULL, 1000 );
    if ( RetValue < 0 )
    {
        printf( "Decomposition failed.\n" );
        return;
    }
    // convert back into HOP
    Bdc_FuncSetCopy( Bdc_ManFunc( p->pBidec, 0 ), Aig_ManConst1(p->pAig) );
    for ( i = 0; i < nVars; i++ )
        Bdc_FuncSetCopy( Bdc_ManFunc( p->pBidec, i+1 ), Aig_IthVar(p->pAig, i) );
    nNodes = Bdc_ManNodeNum(p->pBidec);
    for ( i = nVars + 1; i < nNodes; i++ )
    {
        pFunc = Bdc_ManFunc( p->pBidec, i );
        Bdc_FuncSetCopy( pFunc, Aig_And( p->pAig, 
            Bdc_FunCopyHop(Bdc_FuncFanin0(pFunc)), 
            Bdc_FunCopyHop(Bdc_FuncFanin1(pFunc)) ) );
    }
    pTerm = Bdc_FunCopyHop( Bdc_ManRoot(p->pBidec) );
    pTerm = Aig_ObjCreateCo( p->pAig, pTerm );
//    assert( pTerm->fPhase == 0 );
}

/**Function*************************************************************

  Synopsis    [Records one function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_RManRecord( unsigned * pTruth, int nVarsInit )
{
    int fVerify = 1;
    Kit_DsdNtk_t * pNtk;
    Kit_DsdObj_t * pObj;
    unsigned uPhaseC;
    int i, nVars, nWords;
    int fUniqueVars;

    if ( nVarsInit > RMAN_MAXVARS )
    {
        printf( "The number of variables in too large.\n" );
        return;
    }

    if ( s_pRMan == NULL )
        s_pRMan = Aig_RManStart();
    s_pRMan->nTotal++;
    // canonicize the function
    pNtk = Kit_DsdDecompose( pTruth, nVarsInit );
    pObj = Kit_DsdNonDsdPrimeMax( pNtk );
    if ( pObj == NULL || pObj->nFans == 3 )
    {
        s_pRMan->nTtDsd++;
        Kit_DsdNtkFree( pNtk );
        return;
    }
    nVars = pObj->nFans;
    s_pRMan->nVarFuncs[nVars]++;
    if ( nVars < nVarsInit )
        s_pRMan->nTtDsdPart++;
    else
        s_pRMan->nTtDsdNot++;
    // compute the number of words
    nWords = Abc_TruthWordNum( nVars );
    // copy the function
    memcpy( s_pRMan->pTruthInit, Kit_DsdObjTruth(pObj), (size_t)(4*nWords) );
    Kit_DsdNtkFree( pNtk );
    // canonicize the output
    if ( s_pRMan->pTruthInit[0] & 1 )
        Kit_TruthNot( s_pRMan->pTruthInit, s_pRMan->pTruthInit, nVars );
    memcpy( s_pRMan->pTruth, s_pRMan->pTruthInit, 4*nWords );

    // canonize the function
    for ( i = 0; i < nVars; i++ )
        s_pRMan->pPerm[i] = i;
    uPhaseC = Aig_RManSemiCanonicize( s_pRMan->pTruthTemp, s_pRMan->pTruth, nVars, s_pRMan->pPerm, s_pRMan->pMints, 1 );
    // check unique variables
    fUniqueVars = Aig_RManVarsAreUnique( s_pRMan->pMints, nVars );
    s_pRMan->nUniqueVars += fUniqueVars;

/*
    printf( "%4d : ", s_pRMan->nTotal );
    printf( "%2d %2d  ", nVarsInit, nVars );
    Extra_PrintBinary( stdout, &uPhaseC, nVars );
    printf( "  " );
    for ( i = 0; i < nVars; i++ )
        printf( "%2d/%2d ", s_pRMan->pMints[2*i], s_pRMan->pMints[2*i+1] );
    printf( "\n" );
    Aig_RManPrintUniqueVars( s_pRMan->pMints, nVars );
Extra_PrintBinary( stdout, s_pRMan->pTruth, 1<<nVars ); printf( "\n\n" );
*/
/*
    printf( "\n" );
    printf( "%4d : ", s_pRMan->nTotal );
    printf( "%2d %2d  ", nVarsInit, nVars );
    printf( "   " );
    printf( "\n" );
    Aig_RManPrintUniqueVars( s_pRMan->pMints, nVars );
//    Aig_RManPrintSigs( s_pRMan->pMints, nVars );
*/

//Extra_PrintBinary( stdout, s_pRMan->pTruth, 1<<nVars ); printf( "\n\n" );

    if ( Aig_RManTableFindOrAdd( s_pRMan, s_pRMan->pTruth, nVars ) )
        Aig_RManSaveOne( s_pRMan, s_pRMan->pTruth, nVars );

    if ( fVerify )
    {
        // derive reverse permutation
        for ( i = 0; i < nVars; i++ )
            s_pRMan->pPermR[i] = s_pRMan->pPerm[i];
        // implement permutation
        Kit_TruthPermute( s_pRMan->pTruthTemp, s_pRMan->pTruth, nVars, s_pRMan->pPermR, 1 );
        // implement polarity
        for ( i = 0; i < nVars; i++ )
            if ( uPhaseC & (1 << i) )
                Kit_TruthChangePhase( s_pRMan->pTruth, nVars, i );

        // perform verification
        if ( fUniqueVars && !Kit_TruthIsEqual( s_pRMan->pTruth, s_pRMan->pTruthInit, nVars ) )
            printf( "Verification failed.\n" );
    }
//Aig_RManPrintVarProfile( s_pRMan->pTruth, nVars, s_pRMan->pTruthTemp );
//Extra_PrintBinary( stdout, s_pRMan->pTruth, 1<<nVars ); printf( "\n" );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

