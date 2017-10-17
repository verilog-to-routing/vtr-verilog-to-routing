/**CFile****************************************************************

  FileName    [superGate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Pre-computation of supergates.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: superGate.c,v 1.7 2004/08/03 00:11:40 satrajit Exp $]

***********************************************************************/

#include <math.h>
#include "superInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// the bit masks
#define SUPER_MASK(n)     ((~((unsigned)0)) >> (32-(n)))
#define SUPER_FULL         (~((unsigned)0))
#define SUPER_NO_VAR         (-9999.0)
#define SUPER_EPSILON        (0.001)

// data structure for supergate precomputation
typedef struct Super_ManStruct_t_     Super_Man_t;   // manager
typedef struct Super_GateStruct_t_    Super_Gate_t;  // supergate

struct Super_ManStruct_t_
{
    // parameters
    char *              pName;        // the original genlib file name
    int                 nVarsMax;     // the number of inputs
    int                 nMints;       // the number of minterms
    int                 nLevels;      // the number of logic levels
    int                 nGatesMax;    // the number of gates computed
    float               tDelayMax;    // the max delay of the supergates in the library
    float               tAreaMax;     // the max area of the supergates in the library
    int                 fSkipInv;     // the flag says about skipping inverters
    int                 fWriteOldFormat; // in addition, writes the file in the old format
    int                 fVerbose;

    // supergates
    Super_Gate_t *      pInputs[10];  // the input supergates
    int                 nGates;       // the number of gates in the library
    Super_Gate_t **     pGates;       // the gates themselves
    stmm_table *        tTable;       // mapping of truth tables into gates

    // memory managers
    Extra_MmFixed_t *   pMem;         // memory manager for the supergates
    Extra_MmFlex_t *    pMemFlex;     // memory manager for the fanin arrays

    // statistics
    int                 nTried;       // the total number of tried
    int                 nAdded;       // the number of entries added
    int                 nRemoved;     // the number of entries removed
    int                 nClasses;     // the number of gate classes
    int                 nUnique;      // the number of unique gates
    int                 nLookups;     // the number of hash table lookups
    int                 nAliases;     // the number of hash table lookups thrown away due to aliasing

    // runtime
    abctime             Time;         // the runtime of the generation procedure
    int                 TimeLimit;    // the runtime limit (in seconds)
    int                 TimeSec;      // the time passed (in seconds)
    abctime             TimeStop;     // the time to stop computation (in miliseconds)
    abctime             TimePrint;    // the time to print message
};

struct Super_GateStruct_t_
{
    Mio_Gate_t *        pRoot;        // the root gate for this supergate
    unsigned            fVar :     1; // the flag signaling the elementary variable
    unsigned            fSuper :   1; // the flag signaling the elementary variable
    unsigned            nFanins :  6; // the number of fanin gates
    unsigned            Number :  24; // the number assigned in the process
    unsigned            uTruth[2];    // the truth table of this supergate
    Super_Gate_t *      pFanins[6];   // the fanins of the gate
    float               Area;         // the area of this gate
    float               ptDelays[6];  // the pin-to-pin delays for all inputs
    float               tDelayMax;    // the maximum delay
    Super_Gate_t *      pNext;        // the next gate in the table
};


// iterating through the gates in the library
#define Super_ManForEachGate( GateArray, Limit, Index, Gate )    \
    for ( Index = 0;                                             \
          Index < Limit && (Gate = GateArray[Index]);            \
          Index++ )

// static functions
static Super_Man_t *  Super_ManStart();
static void           Super_ManStop( Super_Man_t * pMan );

static void           Super_AddGateToTable( Super_Man_t * pMan, Super_Gate_t * pGate );
static void           Super_First( Super_Man_t * pMan, int nVarsMax );
static Super_Man_t *  Super_Compute( Super_Man_t * pMan, Mio_Gate_t ** ppGates, int nGates, int nGatesMax, int fSkipInv );
static Super_Gate_t * Super_CreateGateNew( Super_Man_t * pMan, Mio_Gate_t * pRoot, Super_Gate_t ** pSupers, int nSupers, unsigned uTruth[], float Area, float tPinDelaysRes[], float tDelayMax, int nPins );
static int            Super_CompareGates( Super_Man_t * pMan, unsigned uTruth[], float Area, float tPinDelaysRes[], int nPins );
static int            Super_DelayCompare( Super_Gate_t ** ppG1, Super_Gate_t ** ppG2 );
static int            Super_AreaCompare( Super_Gate_t ** ppG1, Super_Gate_t ** ppG2 );
static void           Super_TranferGatesToArray( Super_Man_t * pMan );
static int            Super_CheckTimeout( ProgressBar * pPro, Super_Man_t * pMan );
 
static Vec_Str_t *    Super_Write( Super_Man_t * pMan );
static int            Super_WriteCompare( Super_Gate_t ** ppG1, Super_Gate_t ** ppG2 );
static void           Super_WriteFileHeader( Super_Man_t * pMan, FILE * pFile );

static void           Super_WriteLibrary( Super_Man_t * pMan );

static void           Super_WriteLibraryTreeFile( Super_Man_t * pMan );
static Vec_Str_t *    Super_WriteLibraryTreeStr( Super_Man_t * pMan );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Precomputes the library of supergates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super_Precompute( Mio_Library_t * pLibGen, int nVarsMax, int nLevels, int nGatesMax, float tDelayMax, float tAreaMax, int TimeLimit, int fSkipInv, int fVerbose, char * pFileName )
{
    Vec_Str_t * vStr;
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {     
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }
    vStr = Super_PrecomputeStr( pLibGen, nVarsMax, nLevels, nGatesMax, tDelayMax, tAreaMax, TimeLimit, fSkipInv, fVerbose );
    fwrite( Vec_StrArray(vStr), 1, Vec_StrSize(vStr), pFile );
    fclose( pFile );
    Vec_StrFree( vStr );
    // report the result of writing
    if ( fVerbose )
    {
        printf( "The supergates are written using new format \"%s\" ", pFileName );
        printf( "(%0.3f MB).\n", ((double)Extra_FileSize(pFileName))/(1<<20) );
    }
}

/**Function*************************************************************

  Synopsis    [Precomputes the library of supergates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Super_PrecomputeStr( Mio_Library_t * pLibGen, int nVarsMax, int nLevels, int nGatesMax, float tDelayMax, float tAreaMax, int TimeLimit, int fSkipInv, int fVerbose )
{
    Vec_Str_t * vStr;
    Super_Man_t * pMan;
    Mio_Gate_t ** ppGates;
    int nGates, Level;
    abctime clk, clockStart;

    assert( nVarsMax < 7 );
    if ( nGatesMax && nGatesMax < nVarsMax )
    {
        fprintf( stderr, "Erro! The number of supergates requested (%d) in less than the number of variables (%d).\n", nGatesMax, nVarsMax );
        fprintf( stderr, "The library cannot be computed.\n" );
        return NULL;
    }

    // get the root gates
    ppGates = Mio_CollectRoots( pLibGen, nVarsMax, tDelayMax, 0, &nGates, fVerbose );
    if ( nGatesMax && nGates >= nGatesMax )
    {
        fprintf( stdout, "Warning! Genlib library contains more gates than can be computed.\n");
        fprintf( stdout, "Only one-gate supergates are included in the supergate library.\n" );
    }

    // start the manager
    pMan = Super_ManStart();
    pMan->pName     = Mio_LibraryReadName(pLibGen);
    pMan->nGatesMax = nGatesMax;
    pMan->fSkipInv  = fSkipInv;
    pMan->tDelayMax = tDelayMax;
    pMan->tAreaMax  = tAreaMax;
    pMan->TimeLimit = TimeLimit; // in seconds
    pMan->TimeStop  = TimeLimit ? TimeLimit * CLOCKS_PER_SEC + Abc_Clock() : 0; // in CPU ticks
    pMan->fVerbose  = fVerbose;

    if ( nGates == 0 )
    {
        fprintf( stderr, "Error: No genlib gates satisfy the limits criteria. Stop.\n");
        fprintf( stderr, "Limits: max delay =  %.2f, max area =  %.2f, time limit = %d sec.\n", 
            pMan->tDelayMax, pMan->tAreaMax, pMan->TimeLimit );

        // stop the manager
        Super_ManStop( pMan );
        ABC_FREE( ppGates );

        return NULL;
    }

    // get the starting supergates
    Super_First( pMan, nVarsMax );

    // perform the computation of supergates
    clockStart = Abc_Clock();
if ( fVerbose )
{
    printf( "Computing supergates with %d inputs, %d levels, and %d max gates.\n", 
        pMan->nVarsMax, nLevels, nGatesMax );
    printf( "Limits: max delay =  %.2f, max area =  %.2f, time limit = %d sec.\n", 
        pMan->tDelayMax, pMan->tAreaMax, pMan->TimeLimit );
}

    for ( Level = 1; Level <= nLevels; Level++ )
    {
        if ( pMan->TimeStop && Abc_Clock() > pMan->TimeStop )
            break;
clk = Abc_Clock();
        Super_Compute( pMan, ppGates, nGates, nGatesMax, fSkipInv );
        pMan->nLevels = Level;
if ( fVerbose )
{
        printf( "Lev %d: Try =%12d. Add =%6d. Rem =%5d. Save =%6d. Lookups =%12d. Aliases =%12d. ",
           Level, pMan->nTried, pMan->nAdded, pMan->nRemoved, pMan->nAdded - pMan->nRemoved, pMan->nLookups, pMan->nAliases );
ABC_PRT( "Time", Abc_Clock() - clk );
fflush( stdout );
}
    }
    pMan->Time = Abc_Clock() - clockStart;

if ( fVerbose )
{
printf( "Writing the output file...\n" );
fflush( stdout );
}
    // write them into a file
    vStr = Super_Write( pMan );

    // stop the manager
    Super_ManStop( pMan );
    ABC_FREE( ppGates );
    return vStr;
}


/**Function*************************************************************

  Synopsis    [Derives the starting supergates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super_First( Super_Man_t * pMan, int nVarsMax )
{
    Super_Gate_t * pSuper;
    int nMintLimit, nVarLimit;
    int v, m;
    // set the parameters
    pMan->nVarsMax  = nVarsMax;
    pMan->nMints    = (1 << nVarsMax);
    pMan->nLevels   = 0;
    // allocate room for the gates
    pMan->nGates    = nVarsMax;  
    pMan->pGates    = ABC_ALLOC( Super_Gate_t *, nVarsMax + 2 ); 
    // create the gates corresponding to the elementary variables
    for ( v = 0; v < nVarsMax; v++ )
    {
        // get a new gate
        pSuper = (Super_Gate_t *)Extra_MmFixedEntryFetch( pMan->pMem );
        memset( pSuper, 0, sizeof(Super_Gate_t) );
        // assign the elementary variable, the truth table, and the delays
        pSuper->fVar = 1;
        pSuper->Number = v;
        for ( m = 0; m < nVarsMax; m++ )
            pSuper->ptDelays[m] = SUPER_NO_VAR;
        pSuper->ptDelays[v] = 0.0;
        // set the gate
        pMan->pGates[v] = pSuper;
        Super_AddGateToTable( pMan, pSuper );
        pMan->pInputs[v] = pSuper;
    }
    // set up their truth tables
    nVarLimit  = (nVarsMax >= 5)? 5 : nVarsMax;
    nMintLimit = (1 << nVarLimit);
    for ( m = 0; m < nMintLimit; m++ )
        for ( v = 0; v < nVarLimit; v++ )
            if ( m & (1 << v) )
                pMan->pGates[v]->uTruth[0] |= (1 << m);
    // make adjustments for the case of 6 variables
    if ( nVarsMax == 6 )
    {
        for ( v = 0; v < 5; v++ )
            pMan->pGates[v]->uTruth[1] = pMan->pGates[v]->uTruth[0];
        pMan->pGates[5]->uTruth[0] = 0;
        pMan->pGates[5]->uTruth[1] = ~((unsigned)0);
    }
    else
    {
        for ( v = 0; v < nVarsMax; v++ )
            pMan->pGates[v]->uTruth[1] = 0;
    }
}

/**Function*************************************************************

  Synopsis    [Precomputes one level of supergates.]

  Description [This procedure computes the set of supergates that can be
  derived from the given set of root gates (from genlib library) by composing
  the root gates with the currently available supergates. This procedure is
  smart in the sense that it tries to avoid useless emuration by imposing
  tight bounds by area and delay. Only the supergates and are guaranteed to 
  have smaller area and delay are enumereated. See comments below for details.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Super_Man_t * Super_Compute( Super_Man_t * pMan, Mio_Gate_t ** ppGates, int nGates, int nGatesMax, int fSkipInv )
{
    Super_Gate_t * pSupers[6], * pGate0, * pGate1, * pGate2, * pGate3, * pGate4, * pGate5, * pGateNew;
    float tPinDelaysRes[6], * ptPinDelays[6], tPinDelayMax, tDelayMio;
    float Area = 0.0; // Suppress "might be used uninitialized"
    float Area0, Area1, Area2, Area3, Area4, AreaMio;
    unsigned uTruth[2], uTruths[6][2];
    int i0, i1, i2, i3, i4, i5; 
    Super_Gate_t ** ppGatesLimit;
    int nFanins, nGatesLimit, k, s, t;
    ProgressBar * pProgress;
    int fTimeOut;
    int fPrune = 1;                     // Shall we prune?
    int iPruneLimit = 3;                // Each of the gates plugged into the root gate will have 
                                        // less than these many fanins
    int iPruneLimitRoot = 4;            // The root gate may have only less than these many fanins

    // put the gates from the unique table into the array
    // the gates from the array will be used to compose other gates
    // the gates in tbe table are used to check uniqueness of collected gates
    Super_TranferGatesToArray( pMan );

    // sort the gates in the increasing order of maximum delay
    if ( pMan->nGates > 10000 )
    {
        printf( "Sorting array of %d supergates...\r", pMan->nGates );
        fflush( stdout );
    }
    qsort( (void *)pMan->pGates, pMan->nGates, sizeof(Super_Gate_t *), 
            (int (*)(const void *, const void *)) Super_DelayCompare );
    assert( Super_DelayCompare( pMan->pGates, pMan->pGates + pMan->nGates - 1 ) <= 0 );
    if ( pMan->nGates > 10000 )
    {
        printf( "                                       \r" );
    }

    pProgress = Extra_ProgressBarStart( stdout, pMan->TimeLimit );
    pMan->TimePrint = Abc_Clock() + CLOCKS_PER_SEC;
    ppGatesLimit = ABC_ALLOC( Super_Gate_t *, pMan->nGates );
    // go through the root gates
    // the root gates are sorted in the increasing gelay
    fTimeOut = 0;
    for ( k = 0; k < nGates; k++ )
    {
        if ( fTimeOut ) break;

        if ( fPrune )
        {
            if ( pMan->nLevels >= 1 )  // First level gates have been computed
            {
                if ( Mio_GateReadPinNum(ppGates[k]) >= iPruneLimitRoot )
                    continue;
            }
        }
/*
        if ( strcmp(Mio_GateReadName(ppGates[k]), "MUX2IX0") == 0 )
        {
            int s = 0;
        }
*/
        // select the subset of gates to be considered with this root gate
        // all the gates past this point will lead to delay larger than the limit
        tDelayMio = (float)Mio_GateReadDelayMax(ppGates[k]);
        for ( s = 0, t = 0; s < pMan->nGates; s++ )
        {
            if ( fPrune && ( pMan->nLevels >= 1 ) && ( ((int)pMan->pGates[s]->nFanins) >= iPruneLimit ))
                continue;
            
            ppGatesLimit[t] = pMan->pGates[s];
            if ( ppGatesLimit[t++]->tDelayMax + tDelayMio > pMan->tDelayMax && pMan->tDelayMax > 0.0 )
                break;
        }
        nGatesLimit = t;

        if ( pMan->fVerbose )
        {
            printf ("Trying %d choices for %d inputs\r", t, Mio_GateReadPinNum(ppGates[k]) );
        }

        // resort part of this range by area
        // now we can prune the search by going up in the list until we reach the limit on area
        // all the gates beyond this point can be skipped because their area can be only larger
        if ( nGatesLimit > 10000 )
            printf( "Sorting array of %d supergates...\r", nGatesLimit );
        qsort( (void *)ppGatesLimit, nGatesLimit, sizeof(Super_Gate_t *), 
                (int (*)(const void *, const void *)) Super_AreaCompare );
        assert( Super_AreaCompare( ppGatesLimit, ppGatesLimit + nGatesLimit - 1 ) <= 0 );
        if ( nGatesLimit > 10000 )
            printf( "                                       \r" );

        // consider the combinations of gates with the root gate on top
        AreaMio = (float)Mio_GateReadArea(ppGates[k]);
        nFanins = Mio_GateReadPinNum(ppGates[k]);
        switch ( nFanins )
        {
        case 0: // should not happen
            assert( 0 ); 
            break;
        case 1: // interter root
            Super_ManForEachGate( ppGatesLimit, nGatesLimit, i0, pGate0 )
            {
              if ( fTimeOut ) break;
              fTimeOut = Super_CheckTimeout( pProgress, pMan );
              // skip the inverter as the root gate before the elementary variable
              // as a result, the supergates will not have inverters on the input side
              // but inverters still may occur at the output of or inside complex supergates
              if ( fSkipInv && pGate0->tDelayMax == 0 )
                  continue;
              // compute area
              Area = AreaMio + pGate0->Area;
              if ( pMan->tAreaMax > 0.0 && Area > pMan->tAreaMax )
                  break;

              pSupers[0] = pGate0;  uTruths[0][0] = pGate0->uTruth[0];  uTruths[0][1] = pGate0->uTruth[1];  ptPinDelays[0] = pGate0->ptDelays; 
              Mio_DeriveGateDelays( ppGates[k], ptPinDelays, nFanins, pMan->nVarsMax, SUPER_NO_VAR, tPinDelaysRes, &tPinDelayMax );
              Mio_DeriveTruthTable( ppGates[k], uTruths, nFanins, pMan->nVarsMax, uTruth );
              if ( !Super_CompareGates( pMan, uTruth, Area, tPinDelaysRes, pMan->nVarsMax ) )
                  continue;
              // create a new gate
              pGateNew = Super_CreateGateNew( pMan, ppGates[k], pSupers, nFanins, uTruth, Area, tPinDelaysRes, tPinDelayMax, pMan->nVarsMax );
              Super_AddGateToTable( pMan, pGateNew );
              if ( nGatesMax && pMan->nClasses > nGatesMax )
                  goto done;
            }
            break;
        case 2: // two-input root gate
            Super_ManForEachGate( ppGatesLimit, nGatesLimit, i0, pGate0 )
            {
              Area0 = AreaMio + pGate0->Area;
              if ( pMan->tAreaMax > 0.0 && Area0 > pMan->tAreaMax )
                  break;
              pSupers[0] = pGate0;  uTruths[0][0] = pGate0->uTruth[0];  uTruths[0][1] = pGate0->uTruth[1];  ptPinDelays[0] = pGate0->ptDelays; 
              Super_ManForEachGate( ppGatesLimit, nGatesLimit, i1, pGate1 )
              if ( i1 != i0 )
              {
                if ( fTimeOut ) goto done;
                fTimeOut = Super_CheckTimeout( pProgress, pMan );
                // compute area
                Area = Area0 + pGate1->Area;
                if ( pMan->tAreaMax > 0.0 && Area > pMan->tAreaMax )
                    break;

                pSupers[1] = pGate1;  uTruths[1][0] = pGate1->uTruth[0];  uTruths[1][1] = pGate1->uTruth[1];  ptPinDelays[1] = pGate1->ptDelays;
                Mio_DeriveGateDelays( ppGates[k], ptPinDelays, nFanins, pMan->nVarsMax, SUPER_NO_VAR, tPinDelaysRes, &tPinDelayMax );
                Mio_DeriveTruthTable( ppGates[k], uTruths, nFanins, pMan->nVarsMax, uTruth );
                if ( !Super_CompareGates( pMan, uTruth, Area, tPinDelaysRes, pMan->nVarsMax ) )
                    continue;
                // create a new gate
                pGateNew = Super_CreateGateNew( pMan, ppGates[k], pSupers, nFanins, uTruth, Area, tPinDelaysRes, tPinDelayMax, pMan->nVarsMax );
                Super_AddGateToTable( pMan, pGateNew );
                if ( nGatesMax && pMan->nClasses > nGatesMax )
                    goto done;
              }
            }
            break;
        case 3: // three-input root gate
            Super_ManForEachGate( ppGatesLimit, nGatesLimit, i0, pGate0 )
            {
              Area0 = AreaMio + pGate0->Area;
              if ( pMan->tAreaMax > 0.0 && Area0 > pMan->tAreaMax )
                  break;
              pSupers[0] = pGate0;  uTruths[0][0] = pGate0->uTruth[0];  uTruths[0][1] = pGate0->uTruth[1];  ptPinDelays[0] = pGate0->ptDelays; 

              Super_ManForEachGate( ppGatesLimit, nGatesLimit, i1, pGate1 )
              if ( i1 != i0 )
              {
                Area1 = Area0 + pGate1->Area;
                if ( pMan->tAreaMax > 0.0 && Area1 > pMan->tAreaMax )
                    break;
                pSupers[1] = pGate1;  uTruths[1][0] = pGate1->uTruth[0];  uTruths[1][1] = pGate1->uTruth[1];  ptPinDelays[1] = pGate1->ptDelays;

                Super_ManForEachGate( ppGatesLimit, nGatesLimit, i2, pGate2 )
                if ( i2 != i0 && i2 != i1 )
                {
                  if ( fTimeOut ) goto done;
                  fTimeOut = Super_CheckTimeout( pProgress, pMan );
                  // compute area
                  Area = Area1 + pGate2->Area;
                  if ( pMan->tAreaMax > 0.0 && Area > pMan->tAreaMax )
                      break;
                  pSupers[2] = pGate2;  uTruths[2][0] = pGate2->uTruth[0];  uTruths[2][1] = pGate2->uTruth[1];   ptPinDelays[2] = pGate2->ptDelays;

                  Mio_DeriveGateDelays( ppGates[k], ptPinDelays, nFanins, pMan->nVarsMax, SUPER_NO_VAR, tPinDelaysRes, &tPinDelayMax );
                  Mio_DeriveTruthTable( ppGates[k], uTruths, nFanins, pMan->nVarsMax, uTruth );
                  if ( !Super_CompareGates( pMan, uTruth, Area, tPinDelaysRes, pMan->nVarsMax ) )
                      continue;
                  // create a new gate
                  pGateNew = Super_CreateGateNew( pMan, ppGates[k], pSupers, nFanins, uTruth, Area, tPinDelaysRes, tPinDelayMax, pMan->nVarsMax );
                  Super_AddGateToTable( pMan, pGateNew );
                  if ( nGatesMax && pMan->nClasses > nGatesMax )
                      goto done;
                }
              }
            }
            break;
        case 4: // four-input root gate
            Super_ManForEachGate( ppGatesLimit, nGatesLimit, i0, pGate0 )
            {
              Area0 = AreaMio + pGate0->Area;
              if ( pMan->tAreaMax > 0.0 && Area0 > pMan->tAreaMax )
                  break;
              pSupers[0] = pGate0;  uTruths[0][0] = pGate0->uTruth[0];  uTruths[0][1] = pGate0->uTruth[1];  ptPinDelays[0] = pGate0->ptDelays; 

              Super_ManForEachGate( ppGatesLimit, nGatesLimit, i1, pGate1 )
              if ( i1 != i0 )
              {
                Area1 = Area0 + pGate1->Area;
                if ( pMan->tAreaMax > 0.0 && Area1 > pMan->tAreaMax )
                    break;
                pSupers[1] = pGate1;  uTruths[1][0] = pGate1->uTruth[0];  uTruths[1][1] = pGate1->uTruth[1];  ptPinDelays[1] = pGate1->ptDelays;

                Super_ManForEachGate( ppGatesLimit, nGatesLimit, i2, pGate2 )
                if ( i2 != i0 && i2 != i1 )
                {
                  Area2 = Area1 + pGate2->Area;
                  if ( pMan->tAreaMax > 0.0 && Area2 > pMan->tAreaMax )
                      break;
                  pSupers[2] = pGate2;  uTruths[2][0] = pGate2->uTruth[0];  uTruths[2][1] = pGate2->uTruth[1];   ptPinDelays[2] = pGate2->ptDelays;

                  Super_ManForEachGate( ppGatesLimit, nGatesLimit, i3, pGate3 )
                  if ( i3 != i0 && i3 != i1 && i3 != i2 )
                  {
                    if ( fTimeOut ) goto done;
                    fTimeOut = Super_CheckTimeout( pProgress, pMan );
                    // compute area
                    Area = Area2 + pGate3->Area;
                    if ( pMan->tAreaMax > 0.0 && Area > pMan->tAreaMax )
                        break;
                    pSupers[3] = pGate3;   uTruths[3][0] = pGate3->uTruth[0];  uTruths[3][1] = pGate3->uTruth[1];   ptPinDelays[3] = pGate3->ptDelays;

                    Mio_DeriveGateDelays( ppGates[k], ptPinDelays, nFanins, pMan->nVarsMax, SUPER_NO_VAR, tPinDelaysRes, &tPinDelayMax );
                    Mio_DeriveTruthTable( ppGates[k], uTruths, nFanins, pMan->nVarsMax, uTruth );
                    if ( !Super_CompareGates( pMan, uTruth, Area, tPinDelaysRes, pMan->nVarsMax ) )
                        continue;
                    // create a new gate
                    pGateNew = Super_CreateGateNew( pMan, ppGates[k], pSupers, nFanins, uTruth, Area, tPinDelaysRes, tPinDelayMax, pMan->nVarsMax );
                    Super_AddGateToTable( pMan, pGateNew );
                    if ( nGatesMax && pMan->nClasses > nGatesMax )
                        goto done;
                  }
                }
              }
            }
            break;
        case 5: // five-input root gate
            Super_ManForEachGate( ppGatesLimit, nGatesLimit, i0, pGate0 )
            {
              Area0 = AreaMio + pGate0->Area;
              if ( pMan->tAreaMax > 0.0 && Area0 > pMan->tAreaMax )
                  break;
              pSupers[0] = pGate0;  uTruths[0][0] = pGate0->uTruth[0];  uTruths[0][1] = pGate0->uTruth[1];  ptPinDelays[0] = pGate0->ptDelays; 

              Super_ManForEachGate( ppGatesLimit, nGatesLimit, i1, pGate1 )
              if ( i1 != i0 )
              {
                Area1 = Area0 + pGate1->Area;
                if ( pMan->tAreaMax > 0.0 && Area1 > pMan->tAreaMax )
                    break;
                pSupers[1] = pGate1;  uTruths[1][0] = pGate1->uTruth[0];  uTruths[1][1] = pGate1->uTruth[1];  ptPinDelays[1] = pGate1->ptDelays;

                Super_ManForEachGate( ppGatesLimit, nGatesLimit, i2, pGate2 )
                if ( i2 != i0 && i2 != i1 )
                {
                  Area2 = Area1 + pGate2->Area;
                  if ( pMan->tAreaMax > 0.0 && Area2 > pMan->tAreaMax )
                      break;
                  pSupers[2] = pGate2;  uTruths[2][0] = pGate2->uTruth[0];  uTruths[2][1] = pGate2->uTruth[1];   ptPinDelays[2] = pGate2->ptDelays;

                  Super_ManForEachGate( ppGatesLimit, nGatesLimit, i3, pGate3 )
                  if ( i3 != i0 && i3 != i1 && i3 != i2 )
                  {
                    Area3 = Area2 + pGate3->Area;
                    if ( pMan->tAreaMax > 0.0 && Area3 > pMan->tAreaMax )
                        break;
                    pSupers[3] = pGate3;   uTruths[3][0] = pGate3->uTruth[0];  uTruths[3][1] = pGate3->uTruth[1];   ptPinDelays[3] = pGate3->ptDelays;

                    Super_ManForEachGate( ppGatesLimit, nGatesLimit, i4, pGate4 )
                    if ( i4 != i0 && i4 != i1 && i4 != i2 && i4 != i3 )
                    {
                      if ( fTimeOut ) goto done;
                      fTimeOut = Super_CheckTimeout( pProgress, pMan );
                      // compute area
                      Area = Area3 + pGate4->Area;
                      if ( pMan->tAreaMax > 0.0 && Area > pMan->tAreaMax )
                          break;
                      pSupers[4] = pGate4;   uTruths[4][0] = pGate4->uTruth[0];  uTruths[4][1] = pGate4->uTruth[1];  ptPinDelays[4] = pGate4->ptDelays;

                      Mio_DeriveGateDelays( ppGates[k], ptPinDelays, nFanins, pMan->nVarsMax, SUPER_NO_VAR, tPinDelaysRes, &tPinDelayMax );
                      Mio_DeriveTruthTable( ppGates[k], uTruths, nFanins, pMan->nVarsMax, uTruth );
                      if ( !Super_CompareGates( pMan, uTruth, Area, tPinDelaysRes, pMan->nVarsMax ) )
                          continue;
                      // create a new gate
                      pGateNew = Super_CreateGateNew( pMan, ppGates[k], pSupers, nFanins, uTruth, Area, tPinDelaysRes, tPinDelayMax, pMan->nVarsMax );
                      Super_AddGateToTable( pMan, pGateNew );
                      if ( nGatesMax && pMan->nClasses > nGatesMax )
                          goto done;
                    }
                  }
                }
              }
            }
            break;
        case 6: // six-input root gate
            Super_ManForEachGate( ppGatesLimit, nGatesLimit, i0, pGate0 )
            {
              Area0 = AreaMio + pGate0->Area;
              if ( pMan->tAreaMax > 0.0 && Area0 > pMan->tAreaMax )
                  break;
              pSupers[0] = pGate0;  uTruths[0][0] = pGate0->uTruth[0];  uTruths[0][1] = pGate0->uTruth[1];  ptPinDelays[0] = pGate0->ptDelays; 

              Super_ManForEachGate( ppGatesLimit, nGatesLimit, i1, pGate1 )
              if ( i1 != i0 )
              {
                Area1 = Area0 + pGate1->Area;
                if ( pMan->tAreaMax > 0.0 && Area1 > pMan->tAreaMax )
                    break;
                pSupers[1] = pGate1;  uTruths[1][0] = pGate1->uTruth[0];  uTruths[1][1] = pGate1->uTruth[1];  ptPinDelays[1] = pGate1->ptDelays;

                Super_ManForEachGate( ppGatesLimit, nGatesLimit, i2, pGate2 )
                if ( i2 != i0 && i2 != i1 )
                {
                  Area2 = Area1 + pGate2->Area;
                  if ( pMan->tAreaMax > 0.0 && Area2 > pMan->tAreaMax )
                      break;
                  pSupers[2] = pGate2;  uTruths[2][0] = pGate2->uTruth[0];  uTruths[2][1] = pGate2->uTruth[1];   ptPinDelays[2] = pGate2->ptDelays;

                  Super_ManForEachGate( ppGatesLimit, nGatesLimit, i3, pGate3 )
                  if ( i3 != i0 && i3 != i1 && i3 != i2 )
                  {
                    Area3 = Area2 + pGate3->Area;
                    if ( pMan->tAreaMax > 0.0 && Area3 > pMan->tAreaMax )
                        break;
                    pSupers[3] = pGate3;   uTruths[3][0] = pGate3->uTruth[0];  uTruths[3][1] = pGate3->uTruth[1];   ptPinDelays[3] = pGate3->ptDelays;

                    Super_ManForEachGate( ppGatesLimit, nGatesLimit, i4, pGate4 )
                    if ( i4 != i0 && i4 != i1 && i4 != i2 && i4 != i3 )
                    {
                      if ( fTimeOut ) break;
                      fTimeOut = Super_CheckTimeout( pProgress, pMan );
                      // compute area
                      Area4 = Area3 + pGate4->Area;
                      if ( pMan->tAreaMax > 0.0 && Area > pMan->tAreaMax )
                          break;
                      pSupers[4] = pGate4;   uTruths[4][0] = pGate4->uTruth[0];  uTruths[4][1] = pGate4->uTruth[1];  ptPinDelays[4] = pGate4->ptDelays;

                      Super_ManForEachGate( ppGatesLimit, nGatesLimit, i5, pGate5 )
                      if ( i5 != i0 && i5 != i1 && i5 != i2 && i5 != i3 && i5 != i4 )
                      {
                        if ( fTimeOut ) goto done;
                        fTimeOut = Super_CheckTimeout( pProgress, pMan );
                        // compute area
                        Area = Area4 + pGate5->Area;
                        if ( pMan->tAreaMax > 0.0 && Area > pMan->tAreaMax )
                            break;
                        pSupers[5] = pGate5;   uTruths[5][0] = pGate5->uTruth[0];  uTruths[5][1] = pGate5->uTruth[1];  ptPinDelays[5] = pGate5->ptDelays;

                        Mio_DeriveGateDelays( ppGates[k], ptPinDelays, nFanins, pMan->nVarsMax, SUPER_NO_VAR, tPinDelaysRes, &tPinDelayMax );
                        Mio_DeriveTruthTable( ppGates[k], uTruths, nFanins, pMan->nVarsMax, uTruth );
                        if ( !Super_CompareGates( pMan, uTruth, Area, tPinDelaysRes, pMan->nVarsMax ) )
                            continue;
                        // create a new gate
                        pGateNew = Super_CreateGateNew( pMan, ppGates[k], pSupers, nFanins, uTruth, Area, tPinDelaysRes, tPinDelayMax, pMan->nVarsMax );
                        Super_AddGateToTable( pMan, pGateNew );
                        if ( nGatesMax && pMan->nClasses > nGatesMax )
                            goto done;
                      }
                    }
                  }
                }
              }
            }
            break;
        default :
            assert( 0 );
            break;
        }
    }
done: 
    Extra_ProgressBarStop( pProgress );
    ABC_FREE( ppGatesLimit );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Transfers gates from table into the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Super_CheckTimeout( ProgressBar * pPro, Super_Man_t * pMan )
{
    abctime TimeNow = Abc_Clock();
    if ( TimeNow > pMan->TimePrint )
    {
        Extra_ProgressBarUpdate( pPro, ++pMan->TimeSec, NULL );
        pMan->TimePrint = Abc_Clock() + CLOCKS_PER_SEC;
    }
    if ( pMan->TimeStop && TimeNow > pMan->TimeStop )
    {
        printf ("Timeout!\n");
        return 1;
    }
    pMan->nTried++;
    return 0;
}


/**Function*************************************************************

  Synopsis    [Transfers gates from table into the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super_TranferGatesToArray( Super_Man_t * pMan )
{
    stmm_generator * gen;
    Super_Gate_t * pGate, * pList;
    ABC_PTRUINT_T Key;

    // put the gates fron the table into the array
    ABC_FREE( pMan->pGates );
    pMan->pGates = ABC_ALLOC( Super_Gate_t *, pMan->nAdded );
    pMan->nGates = 0;
    stmm_foreach_item( pMan->tTable, gen, (char **)&Key, (char **)&pList )
    {
        for ( pGate = pList; pGate; pGate = pGate->pNext )
            pMan->pGates[ pMan->nGates++ ] = pGate;
    }
//    assert( pMan->nGates == pMan->nAdded - pMan->nRemoved );
}

/**Function*************************************************************

  Synopsis    [Adds one supergate into the unique table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super_AddGateToTable( Super_Man_t * pMan, Super_Gate_t * pGate )
{
    Super_Gate_t ** ppList;
    ABC_PTRUINT_T Key;
//    Key = pGate->uTruth[0] + 2003 * pGate->uTruth[1];
    Key = pGate->uTruth[0] ^ pGate->uTruth[1];
    if ( !stmm_find_or_add( pMan->tTable, (char *)Key, (char ***)&ppList ) )
    {
        *ppList = NULL;
        pMan->nClasses++;
    }
    pGate->pNext = *ppList;
    *ppList = pGate;
    pMan->nAdded++;
}

/**Function*************************************************************

  Synopsis    [Check the manager's unique table for comparable gates.]

  Description [Returns 0 if the gate is dominated by others. Returns 1 
  if the gate is new or is better than the available ones. In this case, 
  cleans the table by removing the gates that are worse than the given one.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Super_CompareGates( Super_Man_t * pMan, unsigned uTruth[], float Area, float tPinDelaysRes[], int nPins )
{
    Super_Gate_t ** ppList, * pPrev, * pGate, * pGate2;
    int i, fNewIsBetter, fGateIsBetter;
    ABC_PTRUINT_T Key;

    // skip constant functions
    if ( pMan->nVarsMax < 6 )
    {
        if ( uTruth[0] == 0 || ~uTruth[0] == 0 )
            return 0;
    }
    else
    {
        if ( ( uTruth[0] == 0 && uTruth[1] == 0 ) || ( ~uTruth[0] == 0 && ~uTruth[1] == 0 ) )
            return 0;
    }

    // get hold of the place where the entry is stored
//    Key = uTruth[0] + 2003 * uTruth[1];
    Key = uTruth[0] ^ uTruth[1];
    if ( !stmm_find( pMan->tTable, (char *)Key, (char ***)&ppList ) )
        return 1; 
    // the entry with this truth table is found
    pPrev = NULL;
    for ( pGate = *ppList, pGate2 = pGate? pGate->pNext: NULL; pGate; 
        pGate = pGate2, pGate2 = pGate? pGate->pNext: NULL )
    {
        pMan->nLookups++;
        if ( pGate->uTruth[0] != uTruth[0] || pGate->uTruth[1] != uTruth[1] )
        {
            pMan->nAliases++;
            continue;
        }
        fGateIsBetter = 0;
        fNewIsBetter  = 0;
        if ( pGate->Area + SUPER_EPSILON < Area )
            fGateIsBetter = 1;
        else if ( pGate->Area > Area + SUPER_EPSILON )
            fNewIsBetter = 1;
        for ( i = 0; i < nPins; i++ )
        {
            if ( pGate->ptDelays[i] == SUPER_NO_VAR || tPinDelaysRes[i] == SUPER_NO_VAR )
                continue;
            if ( pGate->ptDelays[i] + SUPER_EPSILON < tPinDelaysRes[i] )
                fGateIsBetter = 1;
            else if ( pGate->ptDelays[i] > tPinDelaysRes[i] + SUPER_EPSILON )
                fNewIsBetter = 1;
            if ( fGateIsBetter && fNewIsBetter )
                break;
        }
        // consider 4 cases
        if ( fGateIsBetter && fNewIsBetter ) // Pareto points; save both
            pPrev = pGate;
        else if ( fNewIsBetter ) // gate is worse; remove the gate
        {
            if ( pPrev == NULL )
                *ppList = pGate->pNext;
            else
                pPrev->pNext = pGate->pNext;
            Extra_MmFixedEntryRecycle( pMan->pMem, (char *)pGate );
            pMan->nRemoved++;
        }
        else if ( fGateIsBetter ) // new is worse, already dominated no need to see others
            return 0;
        else // if ( !fGateIsBetter && !fNewIsBetter ) // they are identical, no need to see others
            return 0;
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Create a new supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Super_Gate_t * Super_CreateGateNew( Super_Man_t * pMan, Mio_Gate_t * pRoot, Super_Gate_t ** pSupers, int nSupers, 
    unsigned uTruth[], float Area, float tPinDelaysRes[], float tDelayMax, int nPins )
{
    Super_Gate_t * pSuper;
    pSuper = (Super_Gate_t *)Extra_MmFixedEntryFetch( pMan->pMem );
    memset( pSuper, 0, sizeof(Super_Gate_t) );
    pSuper->pRoot     = pRoot;
    pSuper->uTruth[0] = uTruth[0];
    pSuper->uTruth[1] = uTruth[1];
    memcpy( pSuper->ptDelays, tPinDelaysRes, sizeof(float) * nPins );
    pSuper->Area      = Area;
    pSuper->nFanins   = nSupers;
    memcpy( pSuper->pFanins, pSupers, sizeof(Super_Gate_t *) * nSupers );
    pSuper->pNext     = NULL;
    pSuper->tDelayMax = tDelayMax;
    return pSuper;
}

/**Function*************************************************************

  Synopsis    [Starts the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Super_Man_t * Super_ManStart()
{
    Super_Man_t * pMan;
    pMan = ABC_CALLOC( Super_Man_t, 1 );
    pMan->pMem     = Extra_MmFixedStart( sizeof(Super_Gate_t) );
    pMan->tTable   = stmm_init_table( st__ptrcmp, st__ptrhash );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Stops the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super_ManStop( Super_Man_t * pMan )
{
    Extra_MmFixedStop( pMan->pMem );
    if ( pMan->tTable ) stmm_free_table( pMan->tTable );
    ABC_FREE( pMan->pGates );
    ABC_FREE( pMan );
}





/**Function*************************************************************

  Synopsis    [Writes the supergate library into the file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Super_Write( Super_Man_t * pMan )
{
    Vec_Str_t * vStr;
    Super_Gate_t * pGateRoot, * pGate;
    stmm_generator * gen;
    int fZeroFound, v;
    abctime clk;
    ABC_PTRUINT_T Key;

    if ( pMan->nGates < 1 )
    {
        printf( "The generated library is empty. No output file written.\n" );
        return NULL;
    }

    // Filters the supergates by removing those that have fewer inputs than 
    // the given limit, provided that the inputs are not consequtive. 
    // For example, NAND2(a,c) is removed, but NAND2(a,b) is left, 
    // because a and b are consequtive.
    ABC_FREE( pMan->pGates );
    pMan->pGates = ABC_ALLOC( Super_Gate_t *, pMan->nAdded );
    pMan->nGates = 0;
    stmm_foreach_item( pMan->tTable, gen, (char **)&Key, (char **)&pGateRoot )
    {
        for ( pGate = pGateRoot; pGate; pGate = pGate->pNext )
        {
            // skip the elementary variables
            if ( pGate->pRoot == NULL )
                continue;
            // skip the non-consequtive gates
            fZeroFound = 0;
            for ( v = 0; v < pMan->nVarsMax; v++ )
                if ( pGate->ptDelays[v] < SUPER_NO_VAR + SUPER_EPSILON )
                    fZeroFound = 1;
                else if ( fZeroFound )
                    break;
            if ( v < pMan->nVarsMax )
                continue;
            // save the unique gate
            pMan->pGates[ pMan->nGates++ ] = pGate;
        }
    }

clk = Abc_Clock();
    // sort the supergates by truth table
    qsort( (void *)pMan->pGates, pMan->nGates, sizeof(Super_Gate_t *), 
            (int (*)(const void *, const void *)) Super_WriteCompare );
    assert( Super_WriteCompare( pMan->pGates, pMan->pGates + pMan->nGates - 1 ) <= 0 );
if ( pMan->fVerbose )
{
ABC_PRT( "Sorting", Abc_Clock() - clk );
}


    // write library in the old format
clk = Abc_Clock();
    if ( pMan->fWriteOldFormat )
        Super_WriteLibrary( pMan );
if ( pMan->fVerbose )
{
ABC_PRT( "Writing old format", Abc_Clock() - clk );
}

    // write the tree-like structure of supergates
clk = Abc_Clock();
    vStr = Super_WriteLibraryTreeStr( pMan );
if ( pMan->fVerbose )
{
ABC_PRT( "Writing new format", Abc_Clock() - clk );
}
    return vStr;
}


/**Function*************************************************************

  Synopsis    [Writes the file header.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super_WriteFileHeader( Super_Man_t * pMan, FILE * pFile )
{
    fprintf( pFile, "#\n" );
    fprintf( pFile, "# Supergate library derived for \"%s\" on %s.\n", pMan->pName, Extra_TimeStamp() );
    fprintf( pFile, "#\n" );
    fprintf( pFile, "# Command line: \"super -I %d -L %d -N %d -T %d -D %.2f -A %.2f %s %s\".\n", 
        pMan->nVarsMax, pMan->nLevels, pMan->nGatesMax, pMan->TimeLimit, pMan->tDelayMax, pMan->tAreaMax, (pMan->fSkipInv? "" : "-s"), pMan->pName );
    fprintf( pFile, "#\n" );
    fprintf( pFile, "# The number of inputs      = %10d.\n", pMan->nVarsMax );
    fprintf( pFile, "# The number of levels      = %10d.\n", pMan->nLevels );
    fprintf( pFile, "# The maximum delay         = %10.2f.\n", pMan->tDelayMax  );
    fprintf( pFile, "# The maximum area          = %10.2f.\n", pMan->tAreaMax );
    fprintf( pFile, "# The maximum runtime (sec) = %10d.\n", pMan->TimeLimit );
    fprintf( pFile, "#\n" );
    fprintf( pFile, "# The number of attempts    = %10d.\n", pMan->nTried  );
    fprintf( pFile, "# The number of supergates  = %10d.\n", pMan->nGates  );
    fprintf( pFile, "# The number of functions   = %10d.\n", pMan->nUnique );
    fprintf( pFile, "# The total functions       = %.0f (2^%d).\n", pow((double)2,pMan->nMints), pMan->nMints );
    fprintf( pFile, "#\n" );
    fprintf( pFile, "# Generation time           = %10.2f sec.\n", (float)(pMan->Time)/(float)(CLOCKS_PER_SEC) );
    fprintf( pFile, "#\n" );
    fprintf( pFile, "%s\n", pMan->pName );
    fprintf( pFile, "%d\n", pMan->nVarsMax );
    fprintf( pFile, "%d\n", pMan->nGates );
}
void Super_WriteFileHeaderStr( Super_Man_t * pMan, Vec_Str_t * vStr )
{
    char pBuffer[1000];
    sprintf( pBuffer, "#\n" );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "# Supergate library derived for \"%s\" on %s.\n", pMan->pName, Extra_TimeStamp() );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "#\n" );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "# Command line: \"super -I %d -L %d -N %d -T %d -D %.2f -A %.2f %s %s\".\n", 
        pMan->nVarsMax, pMan->nLevels, pMan->nGatesMax, pMan->TimeLimit, pMan->tDelayMax, pMan->tAreaMax, (pMan->fSkipInv? "" : "-s"), pMan->pName );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "#\n" );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "# The number of inputs      = %10d.\n", pMan->nVarsMax );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "# The number of levels      = %10d.\n", pMan->nLevels );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "# The maximum delay         = %10.2f.\n", pMan->tDelayMax  );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "# The maximum area          = %10.2f.\n", pMan->tAreaMax );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "# The maximum runtime (sec) = %10d.\n", pMan->TimeLimit );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "#\n" );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "# The number of attempts    = %10d.\n", pMan->nTried  );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "# The number of supergates  = %10d.\n", pMan->nGates  );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "# The number of functions   = %10d.\n", pMan->nUnique );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "# The total functions       = %.0f (2^%d).\n", pow((double)2,pMan->nMints), pMan->nMints );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "#\n" );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "# Generation time           = %10.2f sec.\n", (float)(pMan->Time)/(float)(CLOCKS_PER_SEC) );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "#\n" );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "%s\n", pMan->pName );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "%d\n", pMan->nVarsMax );
    Vec_StrPrintStr( vStr, pBuffer );
    sprintf( pBuffer, "%d\n", pMan->nGates );
    Vec_StrPrintStr( vStr, pBuffer );
}

/**Function*************************************************************

  Synopsis    [Compares two gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Super_WriteCompare( Super_Gate_t ** ppG1, Super_Gate_t ** ppG2 )
{
    unsigned * pTruth1 = (*ppG1)->uTruth;
    unsigned * pTruth2 = (*ppG2)->uTruth;
    if ( pTruth1[1] < pTruth2[1] )
        return -1;
    if ( pTruth1[1] > pTruth2[1] )
        return 1;
    if ( pTruth1[0] < pTruth2[0] )
        return -1;
    if ( pTruth1[0] > pTruth2[0] )
        return 1;
    return 0;
}
int Super_DelayCompare( Super_Gate_t ** ppG1, Super_Gate_t ** ppG2 )
{
    if ( (*ppG1)->tDelayMax < (*ppG2)->tDelayMax )
        return -1;
    if ( (*ppG1)->tDelayMax > (*ppG2)->tDelayMax )
        return 1;
    return 0;
}
int Super_AreaCompare( Super_Gate_t ** ppG1, Super_Gate_t ** ppG2 )
{
    if ( (*ppG1)->Area < (*ppG2)->Area )
        return -1;
    if ( (*ppG1)->Area > (*ppG2)->Area )
        return 1;
    return 0;
}






/**Function*************************************************************

  Synopsis    [Writes the gates into the file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super_WriteLibraryGateName_rec( Super_Gate_t * pGate, char * pBuffer )
{
    char Buffer[10];
    int i;

    if ( pGate->pRoot == NULL )
    {
        sprintf( Buffer, "%c", 'a' + pGate->Number );
        strcat( pBuffer, Buffer );
        return;
    }
    strcat( pBuffer, Mio_GateReadName(pGate->pRoot) );
    strcat( pBuffer, "(" );
    for ( i = 0; i < (int)pGate->nFanins; i++ )
    {
        if ( i )
            strcat( pBuffer, "," );
        Super_WriteLibraryGateName_rec( pGate->pFanins[i], pBuffer );
    }
    strcat( pBuffer, ")" );
}
char * Super_WriteLibraryGateName( Super_Gate_t * pGate )
{
    static char Buffer[2000];
    Buffer[0] = 0;
    Super_WriteLibraryGateName_rec( pGate, Buffer );
    return Buffer;
}
void Super_WriteLibraryGate( FILE * pFile, Super_Man_t * pMan, Super_Gate_t * pGate, int Num )
{
    int i;
    fprintf( pFile, "%04d  ", Num );                         // the number
    Extra_PrintBinary( pFile, pGate->uTruth, pMan->nMints ); // the truth table
    fprintf( pFile, "   %5.2f", pGate->tDelayMax );          // the max delay
    fprintf( pFile, "  " );                                  
    for ( i = 0; i < pMan->nVarsMax; i++ )                   // the pin-to-pin delays
        fprintf( pFile, " %5.2f", pGate->ptDelays[i]==SUPER_NO_VAR? 0.0 : pGate->ptDelays[i] );  
    fprintf( pFile, "   %5.2f", pGate->Area );               // the area
    fprintf( pFile, "   " );
    fprintf( pFile, "%s", Super_WriteLibraryGateName(pGate) );      // the symbolic expression
    fprintf( pFile, "\n" );
}
void Super_WriteLibrary( Super_Man_t * pMan )
{
    Super_Gate_t * pGate, * pGateNext;
    FILE * pFile;
    char * FileName;
    char * pNameGeneric;
    int i, Counter;

    FileName = ABC_ALLOC( char, 10000 );

    // get the file name
    pNameGeneric = Extra_FileNameGeneric( pMan->pName );
    sprintf( FileName, "%s.super_old", pNameGeneric );
    ABC_FREE( pNameGeneric );

    // count the number of unique functions
    pMan->nUnique = 1;
    Super_ManForEachGate( pMan->pGates, pMan->nGates, i, pGate )
    {
        if ( i == pMan->nGates - 1 )
            break;
        // print the newline if this gate is different from the following one
        pGateNext = pMan->pGates[i+1];
        if ( pGateNext->uTruth[0] != pGate->uTruth[0] || pGateNext->uTruth[1] != pGate->uTruth[1] )
            pMan->nUnique++;
    }

    // start the file
    pFile = fopen( FileName, "w" );
    Super_WriteFileHeader( pMan, pFile );

    // print the gates
    Counter = 0;
    Super_ManForEachGate( pMan->pGates, pMan->nGates, i, pGate )
    {
        Super_WriteLibraryGate( pFile, pMan, pGate, ++Counter );
        if ( i == pMan->nGates - 1 )
            break;
        // print the newline if this gate is different from the following one
        pGateNext = pMan->pGates[i+1];
        if ( pGateNext->uTruth[0] != pGate->uTruth[0] || pGateNext->uTruth[1] != pGate->uTruth[1] )
            fprintf( pFile, "\n" );
    }
    assert( Counter == pMan->nGates );
    fclose( pFile );

    if ( pMan->fVerbose )
    {
        printf( "The supergates are written using old format \"%s\" ", FileName );
        printf( "(%0.3f MB).\n", ((double)Extra_FileSize(FileName))/(1<<20) );
    }

    ABC_FREE( FileName );
}




/**Function*************************************************************

  Synopsis    [Recursively writes the gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super_WriteLibraryTreeFile_rec( FILE * pFile, Super_Man_t * pMan, Super_Gate_t * pSuper, int * pCounter )
{
    int nFanins, i;
    // skip an elementary variable and a gate that was already written
    if ( pSuper->fVar || pSuper->Number > 0 )
        return;
    // write the fanins
    nFanins = Mio_GateReadPinNum(pSuper->pRoot);
    for ( i = 0; i < nFanins; i++ )
        Super_WriteLibraryTreeFile_rec( pFile, pMan, pSuper->pFanins[i], pCounter );
    // finally write the gate
    pSuper->Number = (*pCounter)++;
    fprintf( pFile, "%s", pSuper->fSuper? "* " : "" );
    fprintf( pFile, "%s", Mio_GateReadName(pSuper->pRoot) );
    for ( i = 0; i < nFanins; i++ )
        fprintf( pFile, " %d", pSuper->pFanins[i]->Number );
    // write the formula 
    // this step is optional, the resulting library will work in any case
    // however, it may be helpful to for debugging to compare the same library 
    // written in the old format and written in the new format with formulas
//    fprintf( pFile, "    # %s", Super_WriteLibraryGateName( pSuper ) );
    fprintf( pFile, "\n" );
}
void Super_WriteLibraryTreeFile( Super_Man_t * pMan )
{
    Super_Gate_t * pSuper;
    FILE * pFile;
    char * FileName;
    char * pNameGeneric;
    int i, Counter;
    int posStart;

    FileName = ABC_ALLOC( char, 10000 );

    // get the file name
    pNameGeneric = Extra_FileNameGeneric( pMan->pName );
    sprintf( FileName, "%s.super", pNameGeneric );
    ABC_FREE( pNameGeneric );
 
    // write the elementary variables
    pFile = fopen( FileName, "wb" );
    Super_WriteFileHeader( pMan, pFile );
    // write the place holder for the number of lines
    posStart = ftell( pFile );
    fprintf( pFile, "         \n" );
    // mark the real supergates
    Super_ManForEachGate( pMan->pGates, pMan->nGates, i, pSuper )
        pSuper->fSuper = 1;
    // write the supergates
    Counter = pMan->nVarsMax;
    Super_ManForEachGate( pMan->pGates, pMan->nGates, i, pSuper )
        Super_WriteLibraryTreeFile_rec( pFile, pMan, pSuper, &Counter );
    fclose( pFile );
    // write the number of lines
    pFile = fopen( FileName, "rb+" );
    fseek( pFile, posStart, SEEK_SET );
    fprintf( pFile, "%d", Counter );
    fclose( pFile );

if ( pMan->fVerbose )
{
    printf( "The supergates are written using new format \"%s\" ", FileName );
    printf( "(%0.3f MB).\n", ((double)Extra_FileSize(FileName))/(1<<20) );
}

    ABC_FREE( FileName );
}


/**Function*************************************************************

  Synopsis    [Recursively writes the gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super_WriteLibraryTreeStr_rec( Vec_Str_t * vStr, Super_Man_t * pMan, Super_Gate_t * pSuper, int * pCounter )
{
    int nFanins, i;
    // skip an elementary variable and a gate that was already written
    if ( pSuper->fVar || pSuper->Number > 0 )
        return;
    // write the fanins
    nFanins = Mio_GateReadPinNum(pSuper->pRoot);
    for ( i = 0; i < nFanins; i++ )
        Super_WriteLibraryTreeStr_rec( vStr, pMan, pSuper->pFanins[i], pCounter );
    // finally write the gate
    pSuper->Number = (*pCounter)++;
//    fprintf( pFile, "%s", pSuper->fSuper? "* " : "" );
//    fprintf( pFile, "%s", Mio_GateReadName(pSuper->pRoot) );
//    for ( i = 0; i < nFanins; i++ )
//        fprintf( pFile, " %d", pSuper->pFanins[i]->Number );
    Vec_StrPrintStr( vStr, pSuper->fSuper? "* " : "" );
    Vec_StrPrintStr( vStr, Mio_GateReadName(pSuper->pRoot) );
    for ( i = 0; i < nFanins; i++ )
    {
        Vec_StrPrintStr( vStr, " " );
        Vec_StrPrintNum( vStr, pSuper->pFanins[i]->Number );    
    }
    // write the formula 
    // this step is optional, the resulting library will work in any case
    // however, it may be helpful to for debugging to compare the same library 
    // written in the old format and written in the new format with formulas
//    fprintf( pFile, "    # %s", Super_WriteLibraryGateName( pSuper ) );
//    fprintf( pFile, "\n" );
    Vec_StrPrintStr( vStr, "\n" );
}
Vec_Str_t * Super_WriteLibraryTreeStr( Super_Man_t * pMan )
{
    char pInsert[16];
    Vec_Str_t * vStr;
    Super_Gate_t * pSuper;
    int i, Counter;
    int posStart;
     // write the elementary variables
    vStr = Vec_StrAlloc( 1000 );
    Super_WriteFileHeaderStr( pMan, vStr );
    // write the place holder for the number of lines
    posStart = Vec_StrSize( vStr );
    for ( i = 0; i < 9; i++ )
        Vec_StrPush( vStr, ' ' );
    Vec_StrPush( vStr, '\n' );
    // mark the real supergates
    Super_ManForEachGate( pMan->pGates, pMan->nGates, i, pSuper )
        pSuper->fSuper = 1;
    // write the supergates
    Counter = pMan->nVarsMax;
    Super_ManForEachGate( pMan->pGates, pMan->nGates, i, pSuper )
        Super_WriteLibraryTreeStr_rec( vStr, pMan, pSuper, &Counter );
    Vec_StrPush( vStr, 0 );
    // write the number of lines
    sprintf( pInsert, "%d", Counter );
    for ( i = 0; i < (int)strlen(pInsert); i++ )
        Vec_StrWriteEntry( vStr, posStart + i, pInsert[i] );
    return vStr;
}
void Super_WriteLibraryTree( Super_Man_t * pMan )
{
    Vec_Str_t * vStr;
    char * pFileName = Extra_FileNameGenericAppend( pMan->pName, ".super" );
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {     
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }
    vStr = Super_WriteLibraryTreeStr( pMan );
    fwrite( Vec_StrArray(vStr), 1, Vec_StrSize(vStr), pFile );
    fclose( pFile );
    Vec_StrFree( vStr );
    // report the result of writing
    if ( pMan->fVerbose )
    {
        printf( "The supergates are written using new format \"%s\" ", pFileName );
        printf( "(%0.3f MB).\n", ((double)Extra_FileSize(pFileName))/(1<<20) );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

