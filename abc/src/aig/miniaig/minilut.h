/**CFile****************************************************************

  FileName    [minilut.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic representation of LUT mapped network.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 29, 2012.]

  Revision    [$Id: minilut.h,v 1.00 2012/09/29 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef MINI_LUT__mini_lut_h
#define MINI_LUT__mini_lut_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define MINI_LUT_NULL       (0x7FFFFFFF)
#define MINI_LUT_NULL2      (0x7FFFFFFE)
#define MINI_LUT_START_SIZE (0x000000FF)

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Mini_Lut_t_       Mini_Lut_t;
struct Mini_Lut_t_ 
{
    int           nCap;
    int           nSize;
    int           nRegs;
    int           LutSize;
    int *         pArray;
    unsigned *    pTruths;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// memory management
#define MINI_LUT_ALLOC(type, num)     ((type *) malloc(sizeof(type) * (num)))
#define MINI_LUT_CALLOC(type, num)    ((type *) calloc((num), sizeof(type)))
#define MINI_LUT_FALLOC(type, num)    ((type *) memset(malloc(sizeof(type) * (num)), 0xff, sizeof(type) * (num)))
#define MINI_LUT_FREE(obj)            ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#define MINI_LUT_REALLOC(type, obj, num) \
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (num))) : \
         ((type *) malloc(sizeof(type) * (num))))

// compute truth table size measured in unsigned's
static int  Mini_LutWordNum( int LutSize ) 
{
    return LutSize > 5 ? 1 << (LutSize-5) : 1;
}

// internal procedures
static void Mini_LutGrow( Mini_Lut_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray  = MINI_LUT_REALLOC( int,      p->pArray,  nCapMin * p->LutSize ); 
    p->pTruths = MINI_LUT_REALLOC( unsigned, p->pTruths, nCapMin * Mini_LutWordNum(p->LutSize) ); 
    p->nCap   = nCapMin;
    assert( p->pArray );
    assert( p->pTruths );
}
static void Mini_LutPush( Mini_Lut_t * p, int nVars, int * pVars, unsigned * pTruth )
{
    int i, nWords = Mini_LutWordNum(p->LutSize);
    if ( p->nSize == p->nCap )
    {
        assert( p->LutSize*p->nSize < MINI_LUT_NULL/2 );
        if ( p->nCap < MINI_LUT_START_SIZE )
            Mini_LutGrow( p, MINI_LUT_START_SIZE );
        else
            Mini_LutGrow( p, 2 * p->nCap );
    }
    for ( i = 0; i < nVars; i++ )
        p->pArray[p->LutSize * p->nSize + i] = pVars[i];
    for ( ; i < p->LutSize; i++ )
        p->pArray[p->LutSize * p->nSize + i] = MINI_LUT_NULL;
    for ( i = 0; i < nWords; i++ )
        p->pTruths[nWords * p->nSize + i] = pTruth? pTruth[i] : 0;
    p->nSize++;
}

// accessing fanins
static int Mini_LutNodeFanin( Mini_Lut_t * p, int Id, int k )
{
    assert( Id >= 0 && Id < p->nSize );
    return p->pArray[p->LutSize*Id+k];
}
static unsigned * Mini_LutNodeTruth( Mini_Lut_t * p, int Id )    
{ 
    assert( Id >= 0 && Id < p->nSize ); 
    return p->pTruths + Id * Mini_LutWordNum(p->LutSize);
}

// working with LUTs
static int      Mini_LutNodeConst0()                           { return 0;                    }
static int      Mini_LutNodeConst1()                           { return 1;                    }

static int      Mini_LutNodeNum( Mini_Lut_t * p )              { return p->nSize;             }
static int      Mini_LutNodeIsConst( Mini_Lut_t * p, int Id )  { assert( Id >= 0 ); return Id == 0 || Id == 1; }
static int      Mini_LutNodeIsPi( Mini_Lut_t * p, int Id )     { assert( Id >= 0 ); return Id > 1 && Mini_LutNodeFanin( p, Id, 0 ) == MINI_LUT_NULL; }
static int      Mini_LutNodeIsPo( Mini_Lut_t * p, int Id )     { assert( Id >= 0 ); return Id > 1 && Mini_LutNodeFanin( p, Id, 0 ) != MINI_LUT_NULL && Mini_LutNodeFanin( p, Id, 1 ) == MINI_LUT_NULL2; }
static int      Mini_LutNodeIsNode( Mini_Lut_t * p, int Id )   { assert( Id >= 0 ); return Id > 1 && Mini_LutNodeFanin( p, Id, 0 ) != MINI_LUT_NULL && Mini_LutNodeFanin( p, Id, 1 ) != MINI_LUT_NULL2; }

static int      Mini_LutSize( Mini_Lut_t * p )                 { return p->LutSize;           }

// working with sequential AIGs
static int      Mini_LutRegNum( Mini_Lut_t * p )               { return p->nRegs;             }
static void     Mini_LutSetRegNum( Mini_Lut_t * p, int n )     { p->nRegs = n;                }

// iterators through objects
#define Mini_LutForEachPi( p, i )    for (i = 2; i < Mini_LutNodeNum(p); i++) if ( !Mini_LutNodeIsPi(p, i) )   {} else 
#define Mini_LutForEachPo( p, i )    for (i = 2; i < Mini_LutNodeNum(p); i++) if ( !Mini_LutNodeIsPo(p, i) )   {} else 
#define Mini_LutForEachNode( p, i )  for (i = 2; i < Mini_LutNodeNum(p); i++) if ( !Mini_LutNodeIsNode(p, i) ) {} else

// iterator through fanins
#define Mini_LutForEachFanin( p, i, Fan, k ) for (k = 0; (k < p->LutSize) && (Fan = Mini_LutNodeFanin(p, i, k)) < MINI_LUT_NULL2; k++) 

// constructor/destructor
static Mini_Lut_t * Mini_LutStart( int LutSize )
{
    Mini_Lut_t * p; int i;
    assert( LutSize >= 2 && LutSize <= 16 );
    p = MINI_LUT_CALLOC( Mini_Lut_t, 1 );
    p->LutSize = LutSize;
    p->nCap    = MINI_LUT_START_SIZE;
    p->pArray  = MINI_LUT_ALLOC( int,      p->nCap * p->LutSize );
    p->pTruths = MINI_LUT_ALLOC( unsigned, p->nCap * Mini_LutWordNum(p->LutSize) );
    Mini_LutPush( p, 0, NULL, NULL ); // const0
    Mini_LutPush( p, 0, NULL, NULL ); // const1
    for ( i = 0; i < Mini_LutWordNum(p->LutSize); i++ )
        p->pTruths[i] = 0;
    for ( i = 0; i < Mini_LutWordNum(p->LutSize); i++ )
        p->pTruths[Mini_LutWordNum(p->LutSize) + i] = ~0;
    return p;
}
static void Mini_LutStop( Mini_Lut_t * p )
{
    MINI_LUT_FREE( p->pArray );
    MINI_LUT_FREE( p->pTruths );
    MINI_LUT_FREE( p );
}
static void Mini_LutPrintStats( Mini_Lut_t * p )
{
    int i, nPis, nPos, nNodes;
    nPis = 0;
    Mini_LutForEachPi( p, i )
        nPis++;
    nPos = 0;
    Mini_LutForEachPo( p, i )
        nPos++;
    nNodes = 0;
    Mini_LutForEachNode( p, i )
        nNodes++;
    printf( "PI = %d. PO = %d. LUT = %d.\n", nPis, nPos, nNodes );
}

// serialization
static void Mini_LutDump( Mini_Lut_t * p, char * pFileName )
{
    FILE * pFile;
    int RetValue;
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file for writing \"%s\".\n", pFileName );
        return;
    }
    RetValue = (int)fwrite( &p->nSize,   sizeof(int), 1, pFile );
    RetValue = (int)fwrite( &p->nRegs,   sizeof(int), 1, pFile );
    RetValue = (int)fwrite( &p->LutSize, sizeof(int), 1, pFile );
    RetValue = (int)fwrite( p->pArray,   sizeof(int), p->nSize * p->LutSize, pFile );
    RetValue = (int)fwrite( p->pTruths,  sizeof(int), p->nSize * Mini_LutWordNum(p->LutSize), pFile );
    fclose( pFile );
}
static Mini_Lut_t * Mini_LutLoad( char * pFileName )
{
    Mini_Lut_t * p;
    FILE * pFile;
    int RetValue, nSize;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file for reading \"%s\".\n", pFileName );
        return NULL;
    }
    RetValue = (int)fread( &nSize, sizeof(int), 1, pFile );
    p = MINI_LUT_CALLOC( Mini_Lut_t, 1 );
    p->nSize   = p->nCap = nSize;
    RetValue = (int)fread( &p->nRegs,   sizeof(int), 1, pFile );
    RetValue = (int)fread( &p->LutSize, sizeof(int), 1, pFile );
    p->pArray  = MINI_LUT_ALLOC( int,      p->nCap * p->LutSize );
    p->pTruths = MINI_LUT_ALLOC( unsigned, p->nCap * Mini_LutWordNum(p->LutSize) );
    RetValue = (int)fread( p->pArray,   sizeof(int), p->nCap * p->LutSize, pFile );
    RetValue = (int)fread( p->pTruths,  sizeof(int), p->nCap * Mini_LutWordNum(p->LutSize), pFile );
    fclose( pFile );
    return p;
}


// creating nodes 
// (constant nodes are created when LUT manager is created)
static int Mini_LutCreatePi( Mini_Lut_t * p )
{
    Mini_LutPush( p, 0, NULL, NULL );
    return p->nSize - 1;
}
static int Mini_LutCreatePo( Mini_Lut_t * p, int Var0 )
{
    assert( Var0 >= 0 && Var0 < p->nSize );
    Mini_LutPush( p, 1, &Var0, NULL );
    // mark PO by setting its 2nd fanin to the special number
    p->pArray[p->LutSize*(p->nSize - 1)+1] = MINI_LUT_NULL2;
    return p->nSize - 1;
}

// create LUT
static int Mini_LutCreateNode( Mini_Lut_t * p, int nVars, int * pVars, unsigned * pTruth )
{
    assert( nVars >= 0 && nVars <= p->LutSize );
    Mini_LutPush( p, nVars, pVars, pTruth );
    return p->nSize - 1;
}

// procedure to check the topological order during AIG construction
static int Mini_LutCheck( Mini_Lut_t * p )
{
    int status = 1;
    int i, k, iFaninVar;
    Mini_LutForEachNode( p, i )
    {
        for ( k = 0; k < p->LutSize; k++ )
        {
            iFaninVar = Mini_LutNodeFanin( p, i, k );
            if ( iFaninVar == MINI_LUT_NULL )
                continue;
            if ( iFaninVar >= p->LutSize * i )
                printf( "Fanin %d of LUT node %d is not in a topological order.\n", k, i ), status = 0;
        }
    }
    Mini_LutForEachPo( p, i )
    {
        iFaninVar = Mini_LutNodeFanin( p, i, 0 );
        if ( iFaninVar >= p->LutSize * i )
            printf( "Fanin %d of PO node %d is not in a topological order.\n", k, i ), status = 0;
    }
    return status;
}



////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

