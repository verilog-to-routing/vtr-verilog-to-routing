/**CFile****************************************************************

  FileName    [superAnd.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Pre-computation of supergates.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: superAnd.c,v 1.3 2004/06/28 14:20:25 alanmi Exp $]

***********************************************************************/

#include "superInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// the bit masks
#define SUPER_MASK(n)     ((~((unsigned)0)) >> (32-n))
#define SUPER_FULL         (~((unsigned)0))

// data structure for AND2 subgraph precomputation
typedef struct Super2_ManStruct_t_     Super2_Man_t;   // manager
typedef struct Super2_LibStruct_t_     Super2_Lib_t;   // library
typedef struct Super2_GateStruct_t_    Super2_Gate_t;  // supergate

struct Super2_ManStruct_t_
{
    Extra_MmFixed_t *   pMem;         // memory manager for all supergates
    stmm_table *        tTable;       // mapping of truth tables into gates
    int                 nTried;       // the total number of tried
};

struct Super2_LibStruct_t_
{
    int                 i;            // used to iterate through the table
    int                 k;            // used to iterate through the table
    int                 nInputs;      // the number of inputs
    int                 nMints;       // the number of minterms
    int                 nLevels;      // the number of logic levels
    int                 nGates;       // the number of gates in the library
    int                 nGatesAlloc;  // the number of allocated places
    Super2_Gate_t **    pGates;       // the gates themselves
    unsigned            uMaskBit;     // the mask used to determine the compl bit
};

struct Super2_GateStruct_t_
{
    unsigned            uTruth;       // the truth table of this supergate
    Super2_Gate_t *     pOne;         // the left wing
    Super2_Gate_t *     pTwo;         // the right wing
    Super2_Gate_t *     pNext;        // the next gate in the table
};


// manipulation of complemented attributes
#define Super2_IsComplement(p)    (((int)((ABC_PTRUINT_T) (p) & 01)))
#define Super2_Regular(p)         ((Super2_Gate_t *)((ABC_PTRUINT_T)(p) & ~01)) 
#define Super2_Not(p)             ((Super2_Gate_t *)((ABC_PTRUINT_T)(p) ^ 01)) 
#define Super2_NotCond(p,c)       ((Super2_Gate_t *)((ABC_PTRUINT_T)(p) ^ (c)))

// iterating through the gates in the library
#define Super2_LibForEachGate( Lib, Gate )                        \
    for ( Lib->i = 0;                                             \
          Lib->i < Lib->nGates && (Gate = Lib->pGates[Lib->i]);   \
          Lib->i++ )
#define Super2_LibForEachGate2( Lib, Gate2 )                      \
    for ( Lib->k = 0;                                             \
          Lib->k < Lib->i && (Gate2 = Lib->pGates[Lib->k]);       \
          Lib->k++ )

// static functions
static Super2_Man_t * Super2_ManStart();
static void           Super2_ManStop( Super2_Man_t * pMan );
static Super2_Lib_t * Super2_LibStart();
static Super2_Lib_t * Super2_LibDup( Super2_Lib_t * pLib );
static void           Super2_LibStop( Super2_Lib_t * pLib );
static void           Super2_LibAddGate( Super2_Lib_t * pLib, Super2_Gate_t * pGate );
static Super2_Lib_t * Super2_LibFirst( Super2_Man_t * pMan, int nInputs );
static Super2_Lib_t * Super2_LibCompute( Super2_Man_t * pMan, Super2_Lib_t * pLib );

static void           Super2_LibWrite( Super2_Lib_t * pLib );
static void           Super2_LibWriteGate( FILE * pFile, Super2_Lib_t * pLib, Super2_Gate_t * pGate );
static char *         Super2_LibWriteGate_rec( Super2_Gate_t * pGate, int fInv, int Level );
static int            Super2_LibWriteCompare( char * pStr1, char * pStr2 );
static int            Super2_LibCompareGates( Super2_Gate_t ** ppG1, Super2_Gate_t ** ppG2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Precomputes the library of AND2 gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super2_Precompute( int nInputs, int nLevels, int fVerbose )
{
    Super2_Man_t * pMan;
    Super2_Lib_t * pLibCur, * pLibNext;
    int Level;
    abctime clk;

    assert( nInputs < 6 );

    // start the manager
    pMan = Super2_ManStart();

    // get the starting supergates
    pLibCur = Super2_LibFirst( pMan, nInputs );

    // perform the computation of supergates
printf( "Computing supergates for %d inputs and %d levels:\n", nInputs, nLevels );
    for ( Level = 1; Level <= nLevels; Level++ )
    {
clk = Abc_Clock();
        pLibNext = Super2_LibCompute( pMan, pLibCur );
        pLibNext->nLevels = Level;
        Super2_LibStop( pLibCur );
        pLibCur = pLibNext;
printf( "Level %d:  Tried = %7d.  Computed = %7d.  ", Level, pMan->nTried, pLibCur->nGates );
ABC_PRT( "Runtime", Abc_Clock() - clk );
fflush( stdout );
    }

printf( "Writing the output file...\n" );
fflush( stdout );
    // write them into a file
    Super2_LibWrite( pLibCur );
    Super2_LibStop( pLibCur );

    // stop the manager
    Super2_ManStop( pMan );
}




/**Function*************************************************************

  Synopsis    [Starts the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Super2_Man_t * Super2_ManStart()
{
    Super2_Man_t * pMan;
    pMan = ABC_ALLOC( Super2_Man_t, 1 );
    memset( pMan, 0, sizeof(Super2_Man_t) );
    pMan->pMem   = Extra_MmFixedStart( sizeof(Super2_Gate_t) );
    pMan->tTable = stmm_init_table( st__ptrcmp, st__ptrhash );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Stops the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super2_ManStop( Super2_Man_t * pMan )
{
    Extra_MmFixedStop( pMan->pMem );
    stmm_free_table( pMan->tTable );
    ABC_FREE( pMan );
}

/**Function*************************************************************

  Synopsis    [Starts the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Super2_Lib_t * Super2_LibStart()
{
    Super2_Lib_t * pLib;
    pLib = ABC_ALLOC( Super2_Lib_t, 1 );
    memset( pLib, 0, sizeof(Super2_Lib_t) );
    return pLib;
}

/**Function*************************************************************

  Synopsis    [Duplicates the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Super2_Lib_t * Super2_LibDup( Super2_Lib_t * pLib )
{
    Super2_Lib_t * pLibNew;
    pLibNew = Super2_LibStart();
    pLibNew->nInputs     = pLib->nInputs;
    pLibNew->nMints      = pLib->nMints;
    pLibNew->nLevels     = pLib->nLevels;
    pLibNew->nGates      = pLib->nGates;
    pLibNew->uMaskBit    = pLib->uMaskBit;
    pLibNew->nGatesAlloc = 1000 + pLib->nGatesAlloc;
    pLibNew->pGates      = ABC_ALLOC( Super2_Gate_t *, pLibNew->nGatesAlloc );
    memcpy( pLibNew->pGates, pLib->pGates, pLibNew->nGates * sizeof(Super2_Gate_t *) );
    return pLibNew;
}

/**Function*************************************************************

  Synopsis    [Add gate to the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super2_LibAddGate( Super2_Lib_t * pLib, Super2_Gate_t * pGate )
{
    if ( pLib->nGates == pLib->nGatesAlloc )
    {
        pLib->pGates  = ABC_REALLOC( Super2_Gate_t *, pLib->pGates,  3 * pLib->nGatesAlloc );
        pLib->nGatesAlloc *= 3;
    }
    pLib->pGates[ pLib->nGates++ ] = pGate;
}

/**Function*************************************************************

  Synopsis    [Stops the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super2_LibStop( Super2_Lib_t * pLib )
{
    ABC_FREE( pLib->pGates );
    ABC_FREE( pLib );
}

/**Function*************************************************************

  Synopsis    [Derives the starting supergates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Super2_Lib_t * Super2_LibFirst( Super2_Man_t * pMan, int nInputs )
{
    Super2_Lib_t * pLib;
    int v, m;    

    // start the library
    pLib = Super2_LibStart();

    // create the starting supergates
    pLib->nInputs     = nInputs;
    pLib->nMints      = (1 << nInputs);
    pLib->nLevels     = 0;
    pLib->nGates      = nInputs + 1;
    pLib->nGatesAlloc = nInputs + 1;
    pLib->uMaskBit    = (1 << (pLib->nMints-1));
    pLib->pGates      = ABC_ALLOC( Super2_Gate_t *, nInputs + 1 );
    // add the constant 0
    pLib->pGates[0] = (Super2_Gate_t *)Extra_MmFixedEntryFetch( pMan->pMem );
    memset( pLib->pGates[0], 0, sizeof(Super2_Gate_t) );
    // add the elementary gates
    for ( v = 0; v < nInputs; v++ )
    {
        pLib->pGates[v+1] = (Super2_Gate_t *)Extra_MmFixedEntryFetch( pMan->pMem );
        memset( pLib->pGates[v+1], 0, sizeof(Super2_Gate_t) );
        pLib->pGates[v+1]->pTwo = (Super2_Gate_t *)(ABC_PTRUINT_T)v;
    }

    // set up their truth tables
    for ( m = 0; m < pLib->nMints; m++ )
        for ( v = 0; v < nInputs; v++ )
            if ( m & (1 << v) )
                pLib->pGates[v+1]->uTruth |= (1 << m);
    return pLib;
}

/**Function*************************************************************

  Synopsis    [Precomputes one level of supergates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Super2_Lib_t * Super2_LibCompute( Super2_Man_t * pMan, Super2_Lib_t * pLib )
{
    Super2_Lib_t * pLibNew;
    Super2_Gate_t * pGate1, * pGate2, * pGateNew;
    Super2_Gate_t ** ppGate;
    unsigned Mask = SUPER_MASK(pLib->nMints);
    unsigned uTruth, uTruthR, uTruth1, uTruth2, uTruth1c, uTruth2c;

    // start the new library
    pLibNew = Super2_LibDup( pLib );

    // reset the hash table
    stmm_free_table( pMan->tTable );
    pMan->tTable = stmm_init_table( st__ptrcmp, st__ptrhash );
    // set the starting things into the hash table
    Super2_LibForEachGate( pLibNew, pGate1 )
    {
        uTruthR = ((pGate1->uTruth & pLibNew->uMaskBit)? Mask & ~pGate1->uTruth : pGate1->uTruth);

        if ( stmm_lookup( pMan->tTable, (char *)(ABC_PTRUINT_T)uTruthR, (char **)&pGate2 ) )
        {
            printf( "New gate:\n" );
            Super2_LibWriteGate( stdout, pLibNew, pGate1 );
            printf( "Gate in the table:\n" );
            Super2_LibWriteGate( stdout, pLibNew, pGate2 );
            assert( 0 );
        }
        stmm_insert( pMan->tTable, (char *)(ABC_PTRUINT_T)uTruthR, (char *)(ABC_PTRUINT_T)pGate1 );
    }


    // set the number of gates tried
    pMan->nTried = pLibNew->nGates;

    // go through the gate pairs
    Super2_LibForEachGate( pLib, pGate1 )
    {
        if ( pLib->i && pLib->i % 300 == 0 )
        {
            printf( "Tried %5d first gates...\n", pLib->i );
            fflush( stdout );
        }

        Super2_LibForEachGate2( pLib, pGate2 )
        {
            uTruth1  = pGate1->uTruth;
            uTruth2  = pGate2->uTruth;
            uTruth1c = Mask & ~uTruth1;
            uTruth2c = Mask & ~uTruth2;

            // none complemented
            uTruth  = uTruth1  & uTruth2;
            uTruthR = ((uTruth & pLibNew->uMaskBit)? Mask & ~uTruth : uTruth);

            if ( !stmm_find_or_add( pMan->tTable, (char *)(ABC_PTRUINT_T)uTruthR, (char ***)&ppGate ) )
            {
                pGateNew = (Super2_Gate_t *)Extra_MmFixedEntryFetch( pMan->pMem );
                pGateNew->pOne  = pGate1;
                pGateNew->pTwo  = pGate2;
                pGateNew->uTruth = uTruth;
                *ppGate = pGateNew;
                Super2_LibAddGate( pLibNew, pGateNew );
            }                

            // one complemented
            uTruth  = uTruth1c & uTruth2;
            uTruthR = ((uTruth & pLibNew->uMaskBit)? Mask & ~uTruth : uTruth);

            if ( !stmm_find_or_add( pMan->tTable, (char *)(ABC_PTRUINT_T)uTruthR, (char ***)&ppGate ) )
            {
                pGateNew = (Super2_Gate_t *)Extra_MmFixedEntryFetch( pMan->pMem );
                pGateNew->pOne  = Super2_Not(pGate1);
                pGateNew->pTwo  = pGate2;
                pGateNew->uTruth = uTruth;
                *ppGate = pGateNew;
                Super2_LibAddGate( pLibNew, pGateNew );
            }                

            // another complemented
            uTruth  = uTruth1  & uTruth2c;
            uTruthR = ((uTruth & pLibNew->uMaskBit)? Mask & ~uTruth : uTruth);

            if ( !stmm_find_or_add( pMan->tTable, (char *)(ABC_PTRUINT_T)uTruthR, (char ***)&ppGate ) )
            {
                pGateNew = (Super2_Gate_t *)Extra_MmFixedEntryFetch( pMan->pMem );
                pGateNew->pOne  = pGate1;
                pGateNew->pTwo  = Super2_Not(pGate2);
                pGateNew->uTruth = uTruth;
                *ppGate = pGateNew;
                Super2_LibAddGate( pLibNew, pGateNew );
            }                

            // both complemented
            uTruth  = uTruth1c & uTruth2c;
            uTruthR = ((uTruth & pLibNew->uMaskBit)? Mask & ~uTruth : uTruth);

            if ( !stmm_find_or_add( pMan->tTable, (char *)(ABC_PTRUINT_T)uTruthR, (char ***)&ppGate ) )
            {
                pGateNew = (Super2_Gate_t *)Extra_MmFixedEntryFetch( pMan->pMem );
                pGateNew->pOne  = Super2_Not(pGate1);
                pGateNew->pTwo  = Super2_Not(pGate2);
                pGateNew->uTruth = uTruth;
                *ppGate = pGateNew;
                Super2_LibAddGate( pLibNew, pGateNew );
            }                

            pMan->nTried += 4;
        }
    }
    return pLibNew;
}


static unsigned s_uMaskBit;
static unsigned s_uMaskAll;

/**Function*************************************************************

  Synopsis    [Writes the library into the file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super2_LibWrite( Super2_Lib_t * pLib )
{
    Super2_Gate_t * pGate;
    FILE * pFile;
    char FileName[100];
    abctime clk;

    if ( pLib->nLevels > 5 )
    {
        printf( "Cannot write file for %d levels.\n", pLib->nLevels );
        return;
    }

clk = Abc_Clock();
    // sort the supergates by truth table
    s_uMaskBit = pLib->uMaskBit;
    s_uMaskAll = SUPER_MASK(pLib->nMints);
    qsort( (void *)pLib->pGates, pLib->nGates, sizeof(Super2_Gate_t *), 
            (int (*)(const void *, const void *)) Super2_LibCompareGates );
    assert( Super2_LibCompareGates( pLib->pGates, pLib->pGates + pLib->nGates - 1 ) < 0 );
ABC_PRT( "Sorting", Abc_Clock() - clk );


    // start the file
    sprintf( FileName, "superI%dL%d", pLib->nInputs, pLib->nLevels );
    pFile = fopen( FileName, "w" );
    fprintf( pFile, "# AND2/INV supergates derived on %s.\n", Extra_TimeStamp() );
    fprintf( pFile, "# Command line: \"super2 -i %d -l %d\".\n", pLib->nInputs, pLib->nLevels );
    fprintf( pFile, "# The number of inputs     = %6d.\n", pLib->nInputs );
    fprintf( pFile, "# The number of levels     = %6d.\n", pLib->nLevels );
    fprintf( pFile, "# The number of supergates = %6d.\n", pLib->nGates  );
    fprintf( pFile, "# The total functions      = %6d.\n", (1<<(pLib->nMints-1)) );
    fprintf( pFile, "\n" );
    fprintf( pFile, "%6d\n", pLib->nGates );

    // print the gates
    Super2_LibForEachGate( pLib, pGate )
        Super2_LibWriteGate( pFile, pLib, pGate );
    fclose( pFile );

    printf( "The supergates are written into file \"%s\" ", FileName );
    printf( "(%0.2f MB).\n", ((double)Extra_FileSize(FileName))/(1<<20) );
}

/**Function*************************************************************

  Synopsis    [Writes the gate into the file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Super2_LibCompareGates( Super2_Gate_t ** ppG1, Super2_Gate_t ** ppG2 )
{
    Super2_Gate_t * pG1  = *ppG1;
    Super2_Gate_t * pG2  = *ppG2;
    unsigned uTruth1, uTruth2;

    uTruth1 = (pG1->uTruth & s_uMaskBit)? s_uMaskAll & ~pG1->uTruth : pG1->uTruth;
    uTruth2 = (pG2->uTruth & s_uMaskBit)? s_uMaskAll & ~pG2->uTruth : pG2->uTruth;

    if ( uTruth1 < uTruth2 )
        return -1;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes the gate into the file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super2_LibWriteGate( FILE * pFile, Super2_Lib_t * pLib, Super2_Gate_t * pGate )
{
//    unsigned uTruthR;
    unsigned uTruth;
    int fInv;

    // check whether the gate need complementation
    fInv = (int)(pGate->uTruth & pLib->uMaskBit);
    uTruth = (fInv? ~pGate->uTruth : pGate->uTruth);
/*
    // reverse the truth table
    uTruthR = 0;
    for ( m = 0; m < pLib->nMints; m++ )
        if ( uTruth & (1 << m) )
            uTruthR |= (1 << (pLib->nMints-1-m));
*/
    // write the truth table
    Extra_PrintBinary( pFile, &uTruth, pLib->nMints );
    fprintf( pFile, "   " );
    // write the symbolic expression
    fprintf( pFile, "%s", Super2_LibWriteGate_rec( pGate, fInv, pLib->nLevels ) );
    fprintf( pFile, "\n" );
}


/**Function*************************************************************

  Synopsis    [Recursively writes the gate into the file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Super2_LibWriteGate_rec( Super2_Gate_t * pGate, int fInv, int Level )
{
    static char Buff01[  3], Buff02[  3];    // Max0              =  1
    static char Buff11[  6], Buff12[  6];    // Max1 = 2*Max0 + 2 =  4
    static char Buff21[ 12], Buff22[ 12];    // Max2 = 2*Max1 + 2 = 10
    static char Buff31[ 25], Buff32[ 25];    // Max3 = 2*Max2 + 2 = 22
    static char Buff41[ 50], Buff42[ 50];    // Max4 = 2*Max3 + 2 = 46
    static char Buff51[100], Buff52[100];    // Max5 = 2*Max4 + 2 = 94
    static char * pBuffs1[6] = { Buff01, Buff11, Buff21, Buff31, Buff41, Buff51 };
    static char * pBuffs2[6] = { Buff02, Buff12, Buff22, Buff32, Buff42, Buff52 };
    char * pBranch;
    char * pBuffer1 = pBuffs1[Level];
    char * pBuffer2 = pBuffs2[Level];
    Super2_Gate_t * pGateNext1, * pGateNext2;
    int fInvNext1, fInvNext2;
    int RetValue;

    // consider the last level
    assert( Level >= 0 );
    if ( pGate->pOne == NULL )
    {
        if ( pGate->uTruth == 0 )
        {
            pBuffer1[0] = (fInv? '1': '0');
            pBuffer1[1] = '$';
            pBuffer1[2] = 0;
        }
        else
        {
            pBuffer1[0] = (fInv? 'A' + ((int)(ABC_PTRUINT_T)pGate->pTwo): 'a' + ((int)(ABC_PTRUINT_T)pGate->pTwo));
            pBuffer1[1] = 0;
        }
        return pBuffer1;
    }
    assert( Level > 0 );


    // get the left branch
    pGateNext1 = Super2_Regular(pGate->pOne);
    fInvNext1  = Super2_IsComplement(pGate->pOne);
    pBranch    = Super2_LibWriteGate_rec(pGateNext1, fInvNext1, Level - 1);
    // copy into Buffer1
    strcpy( pBuffer1, pBranch );

    // get the right branch
    pGateNext2 = Super2_Regular(pGate->pTwo);
    fInvNext2  = Super2_IsComplement(pGate->pTwo);
    pBranch    = Super2_LibWriteGate_rec(pGateNext2, fInvNext2, Level - 1);

    // consider the case when comparison is not necessary
    if ( fInvNext1 ^ fInvNext2 )
    {
        if ( fInvNext1 > fInvNext2 )
            sprintf( pBuffer2, "%c%s%s%c", (fInv? '<': '('), pBuffer1, pBranch, (fInv? '>': ')') );
        else
            sprintf( pBuffer2, "%c%s%s%c", (fInv? '<': '('), pBranch, pBuffer1, (fInv? '>': ')') );
    }
    else
    {
        // compare the two branches
        RetValue = Super2_LibWriteCompare( pBuffer1, pBranch );
        if ( RetValue == 1 )
            sprintf( pBuffer2, "%c%s%s%c", (fInv? '<': '('), pBuffer1, pBranch, (fInv? '>': ')') );
        else // if ( RetValue == -1 )
        {
            sprintf( pBuffer2, "%c%s%s%c", (fInv? '<': '('), pBranch, pBuffer1, (fInv? '>': ')') );
            if ( RetValue == 0 )
                printf( "Strange!\n" );
        }
    }
    return pBuffer2;
}

/**Function*************************************************************

  Synopsis    [Compares the two branches of the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Super2_LibWriteCompare( char * pStr1, char * pStr2 )
{
    while ( 1 )
    {
        // skip extra symbols
        while ( *pStr1 && *pStr1 < 'A' )
            pStr1++;
        while ( *pStr2 && *pStr2 < 'A' )
            pStr2++;

        // check if any one is finished
        if ( *pStr1 == 0 || *pStr2 == 0 )
        {
            if ( *pStr2 )
                return 1;
            return -1;
        }

        // compare
        if ( *pStr1 == *pStr2 )
        {
            pStr1++;
            pStr2++;
        }
        else
        {
            if ( *pStr1 < *pStr2 )
                return 1;
            return -1;
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

