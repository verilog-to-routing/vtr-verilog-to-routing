/**CFile****************************************************************

  FileName    [miniaig.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic AIG package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 29, 2012.]

  Revision    [$Id: miniaig.h,v 1.00 2012/09/29 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef MINI_AIG__mini_aig_h
#define MINI_AIG__mini_aig_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef _VERIFIC_DATABASE_H_
ABC_NAMESPACE_HEADER_START
#endif

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define MINI_AIG_NULL       (0x7FFFFFFF)
#define MINI_AIG_START_SIZE (0x000000FF)

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Mini_Aig_t_       Mini_Aig_t;
struct Mini_Aig_t_ 
{
    int           nCap;
    int           nSize;
    int           nRegs;
    int *         pArray;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// memory management
#define MINI_AIG_ALLOC(type, num)     ((type *) malloc(sizeof(type) * (num)))
#define MINI_AIG_CALLOC(type, num)    ((type *) calloc((num), sizeof(type)))
#define MINI_AIG_FALLOC(type, num)    ((type *) memset(malloc(sizeof(type) * (num)), 0xff, sizeof(type) * (num)))
#define MINI_AIG_FREE(obj)            ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#define MINI_AIG_REALLOC(type, obj, num) \
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (num))) : \
         ((type *) malloc(sizeof(type) * (num))))

// internal procedures
static void Mini_AigGrow( Mini_Aig_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = MINI_AIG_REALLOC( int, p->pArray, nCapMin ); 
    assert( p->pArray );
    p->nCap   = nCapMin;
}
static void Mini_AigPush( Mini_Aig_t * p, int Lit0, int Lit1 )
{
    if ( p->nSize + 2 > p->nCap )
    {
        assert( p->nSize < MINI_AIG_NULL/4 );
        if ( p->nCap < MINI_AIG_START_SIZE )
            Mini_AigGrow( p, MINI_AIG_START_SIZE );
        else
            Mini_AigGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Lit0;
    p->pArray[p->nSize++] = Lit1;
}

// accessing fanins
static int Mini_AigNodeFanin0( Mini_Aig_t * p, int Id )
{
    assert( Id >= 0 && 2*Id < p->nSize );
    assert( p->pArray[2*Id] == MINI_AIG_NULL || p->pArray[2*Id] < 2*Id );
    return p->pArray[2*Id];
}
static int Mini_AigNodeFanin1( Mini_Aig_t * p, int Id )
{
    assert( Id >= 0 && 2*Id < p->nSize );
    assert( p->pArray[2*Id+1] == MINI_AIG_NULL || p->pArray[2*Id+1] < 2*Id );
    return p->pArray[2*Id+1];
}
static void Mini_AigFlipLastPo( Mini_Aig_t * p )
{
    assert( p->pArray[p->nSize-1] == MINI_AIG_NULL );
    assert( p->pArray[p->nSize-2] != MINI_AIG_NULL );
    p->pArray[p->nSize-2] ^= 1;
}

// working with variables and literals
static int      Mini_AigVar2Lit( int Var, int fCompl )         { return Var + Var + fCompl;   }
static int      Mini_AigLit2Var( int Lit )                     { return Lit >> 1;             }
static int      Mini_AigLitIsCompl( int Lit )                  { return Lit & 1;              }
static int      Mini_AigLitNot( int Lit )                      { return Lit ^ 1;              }
static int      Mini_AigLitNotCond( int Lit, int c )           { return Lit ^ (int)(c > 0);   }
static int      Mini_AigLitRegular( int Lit )                  { return Lit & ~01;            }

static int      Mini_AigLitConst0()                            { return 0;                    }
static int      Mini_AigLitConst1()                            { return 1;                    }
static int      Mini_AigLitIsConst0( int Lit )                 { return Lit == 0;             }
static int      Mini_AigLitIsConst1( int Lit )                 { return Lit == 1;             }
static int      Mini_AigLitIsConst( int Lit )                  { return Lit == 0 || Lit == 1; }

static int      Mini_AigNodeNum( Mini_Aig_t * p )              { return p->nSize/2;           }
static int      Mini_AigNodeIsConst( Mini_Aig_t * p, int Id )  { assert( Id >= 0 ); return Id == 0; }
static int      Mini_AigNodeIsPi( Mini_Aig_t * p, int Id )     { assert( Id >= 0 ); return Id > 0 && Mini_AigNodeFanin0( p, Id ) == MINI_AIG_NULL; }
static int      Mini_AigNodeIsPo( Mini_Aig_t * p, int Id )     { assert( Id >= 0 ); return Id > 0 && Mini_AigNodeFanin0( p, Id ) != MINI_AIG_NULL && Mini_AigNodeFanin1( p, Id ) == MINI_AIG_NULL; }
static int      Mini_AigNodeIsAnd( Mini_Aig_t * p, int Id )    { assert( Id >= 0 ); return Id > 0 && Mini_AigNodeFanin0( p, Id ) != MINI_AIG_NULL && Mini_AigNodeFanin1( p, Id ) != MINI_AIG_NULL; }

// working with sequential AIGs
static int      Mini_AigRegNum( Mini_Aig_t * p )               { return p->nRegs;             }
static void     Mini_AigSetRegNum( Mini_Aig_t * p, int n )     { p->nRegs = n;                }

// iterators through objects
#define Mini_AigForEachPi( p, i )  for (i = 1; i < Mini_AigNodeNum(p); i++) if ( !Mini_AigNodeIsPi(p, i) ) {} else 
#define Mini_AigForEachPo( p, i )  for (i = 1; i < Mini_AigNodeNum(p); i++) if ( !Mini_AigNodeIsPo(p, i) ) {} else 
#define Mini_AigForEachAnd( p, i ) for (i = 1; i < Mini_AigNodeNum(p); i++) if ( !Mini_AigNodeIsAnd(p, i) ) {} else


// constructor/destructor
static Mini_Aig_t * Mini_AigStart()
{
    Mini_Aig_t * p;
    p = MINI_AIG_CALLOC( Mini_Aig_t, 1 );
    p->nCap   = MINI_AIG_START_SIZE;
    p->pArray = MINI_AIG_ALLOC( int, p->nCap );
    Mini_AigPush( p, MINI_AIG_NULL, MINI_AIG_NULL );
    return p;
}
static Mini_Aig_t * Mini_AigStartSupport( int nIns, int nObjsAlloc )
{
    Mini_Aig_t * p; int i;
    assert( 1+nIns < nObjsAlloc );
    p = MINI_AIG_CALLOC( Mini_Aig_t, 1 );
    p->nCap   = 2*nObjsAlloc;
    p->nSize  = 2*(1+nIns);
    p->pArray = MINI_AIG_ALLOC( int, p->nCap );
    for ( i = 0; i < p->nSize; i++ )
        p->pArray[i] = MINI_AIG_NULL;
    return p;
}
static Mini_Aig_t * Mini_AigStartArray( int nIns, int n0InAnds, int * p0InAnds, int nOuts, int * pOuts )
{
    Mini_Aig_t * p; int i;
    assert( 1+nIns <= n0InAnds );
    p = MINI_AIG_CALLOC( Mini_Aig_t, 1 );
    p->nCap   = 2*(n0InAnds + nOuts);
    p->nSize  = 2*(n0InAnds + nOuts);
    p->pArray = MINI_AIG_ALLOC( int, p->nCap );
    for ( i = 0; i < 2*(1 + nIns); i++ )
        p->pArray[i] = MINI_AIG_NULL;
    memcpy( p->pArray + 2*(1 + nIns), p0InAnds + 2*(1 + nIns), 2*(n0InAnds - 1 - nIns)*sizeof(int) );
    for ( i = 0; i < nOuts; i++ )
    {
        p->pArray[2*(n0InAnds + i)+0] = pOuts[i];
        p->pArray[2*(n0InAnds + i)+1] = MINI_AIG_NULL;
    }
    return p;
}
static Mini_Aig_t * Mini_AigDup( Mini_Aig_t * p )
{
    Mini_Aig_t * pNew;
    pNew = MINI_AIG_CALLOC( Mini_Aig_t, 1 );
    pNew->nCap   = p->nCap;
    pNew->nSize  = p->nSize;
    pNew->pArray = MINI_AIG_ALLOC( int, p->nSize );
    memcpy( pNew->pArray, p->pArray, p->nSize * sizeof(int) );
    return pNew;
}
static void Mini_AigStop( Mini_Aig_t * p )
{
    MINI_AIG_FREE( p->pArray );
    MINI_AIG_FREE( p );
}
static int Mini_AigPiNum( Mini_Aig_t * p )
{
    int i, nPis = 0;
    Mini_AigForEachPi( p, i )
        nPis++;
    return nPis;
}
static int Mini_AigPoNum( Mini_Aig_t * p )
{
    int i, nPos = 0;
    Mini_AigForEachPo( p, i )
        nPos++;
    return nPos;
}
static int Mini_AigAndNum( Mini_Aig_t * p )
{
    int i, nNodes = 0;
    Mini_AigForEachAnd( p, i )
        nNodes++;
    return nNodes;
}
static int Mini_AigXorNum( Mini_Aig_t * p )
{
    int i, nNodes = 0;
    Mini_AigForEachAnd( p, i )
        nNodes += p->pArray[2*i] > p->pArray[2*i+1];
    return nNodes;
}
static int Mini_AigLevelNum( Mini_Aig_t * p )
{
    int i, Level = 0;
    int * pLevels = MINI_AIG_CALLOC( int, Mini_AigNodeNum(p) );
    Mini_AigForEachAnd( p, i )
    {
        int Lel0 = pLevels[Mini_AigLit2Var(Mini_AigNodeFanin0(p, i))];
        int Lel1 = pLevels[Mini_AigLit2Var(Mini_AigNodeFanin1(p, i))];
        pLevels[i] = 1 + (Lel0 > Lel1 ? Lel0 : Lel1);
    }
    Mini_AigForEachPo( p, i )
    {
        int Lel0 = pLevels[Mini_AigLit2Var(Mini_AigNodeFanin0(p, i))];
        Level = Level > Lel0 ? Level : Lel0;
    }
    MINI_AIG_FREE( pLevels );
    return Level;
}
static void Mini_AigPrintStats( Mini_Aig_t * p )
{
    printf( "MiniAIG stats:  PI = %d  PO = %d  FF = %d  AND = %d\n", Mini_AigPiNum(p), Mini_AigPoNum(p), Mini_AigRegNum(p), Mini_AigAndNum(p) );
}

// serialization
static void Mini_AigDump( Mini_Aig_t * p, char * pFileName )
{
    FILE * pFile;
    int RetValue;
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file for writing \"%s\".\n", pFileName );
        return;
    }
    RetValue = (int)fwrite( &p->nSize, sizeof(int), 1, pFile );
    RetValue = (int)fwrite( &p->nRegs, sizeof(int), 1, pFile );
    RetValue = (int)fwrite( p->pArray, sizeof(int), p->nSize, pFile );
    fclose( pFile );
}
static Mini_Aig_t * Mini_AigLoad( char * pFileName )
{
    Mini_Aig_t * p;
    FILE * pFile;
    int RetValue, nSize;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file for reading \"%s\".\n", pFileName );
        return NULL;
    }
    RetValue = (int)fread( &nSize, sizeof(int), 1, pFile );
    p = MINI_AIG_CALLOC( Mini_Aig_t, 1 );
    p->nSize = p->nCap = nSize;
    p->pArray = MINI_AIG_ALLOC( int, p->nCap );
    RetValue = (int)fread( &p->nRegs, sizeof(int), 1, pFile );
    RetValue = (int)fread( p->pArray, sizeof(int), p->nSize, pFile );
    fclose( pFile );
    return p;
}


// creating nodes 
// (constant node is created when AIG manager is created)
static int Mini_AigCreatePi( Mini_Aig_t * p )
{
    int Lit = p->nSize;
    Mini_AigPush( p, MINI_AIG_NULL, MINI_AIG_NULL );
    return Lit;
}
static int Mini_AigCreatePo( Mini_Aig_t * p, int Lit0 )
{
    int Lit = p->nSize;
    assert( Lit0 >= 0 && Lit0 < Lit );
    Mini_AigPush( p, Lit0, MINI_AIG_NULL );
    return Lit;
}

// boolean operations
static int Mini_AigAnd( Mini_Aig_t * p, int Lit0, int Lit1 )
{
    int Lit = p->nSize;
    assert( Lit0 >= 0 && Lit0 < Lit );
    assert( Lit1 >= 0 && Lit1 < Lit );
    if ( Lit0 < Lit1 )
        Mini_AigPush( p, Lit0, Lit1 );
    else
        Mini_AigPush( p, Lit1, Lit0 );
    return Lit;
}
static int Mini_AigOr( Mini_Aig_t * p, int Lit0, int Lit1 )
{
    return Mini_AigLitNot( Mini_AigAnd( p, Mini_AigLitNot(Lit0), Mini_AigLitNot(Lit1) ) );
}
static int Mini_AigMux( Mini_Aig_t * p, int LitC, int Lit1, int Lit0 )
{
    int Res0 = Mini_AigAnd( p, LitC, Lit1 );
    int Res1 = Mini_AigAnd( p, Mini_AigLitNot(LitC), Lit0 );
    return Mini_AigOr( p, Res0, Res1 );
}
static int Mini_AigXor( Mini_Aig_t * p, int Lit0, int Lit1 )
{
    return Mini_AigMux( p, Lit0, Mini_AigLitNot(Lit1), Lit1 );
}
static int Mini_AigXorSpecial( Mini_Aig_t * p, int Lit0, int Lit1 )
{
    int Lit = p->nSize;
    assert( Lit0 >= 0 && Lit0 < Lit );
    assert( Lit1 >= 0 && Lit1 < Lit );
    if ( Lit0 > Lit1 )
        Mini_AigPush( p, Lit0, Lit1 );
    else
        Mini_AigPush( p, Lit1, Lit0 );
    return Lit;
}
static int Mini_AigAndMulti( Mini_Aig_t * p, int * pLits, int nLits )
{
    int i;
    assert( nLits > 0 );
    while ( nLits > 1 )
    {
        for ( i = 0; i < nLits/2; i++ )
            pLits[i] = Mini_AigAnd(p, pLits[2*i], pLits[2*i+1]);
        if ( nLits & 1 )
            pLits[i++] = pLits[nLits-1];
        nLits = i;
    }
    return pLits[0];
}
static int Mini_AigMuxMulti( Mini_Aig_t * p, int * pCtrl, int nCtrl, int * pData, int nData )
{
    int i, c;
    assert( nData > 0 );
    if ( nCtrl == 0 )
        return pData[0];
    assert( nData <= (1 << nCtrl) );
    for ( c = 0; c < nCtrl; c++ )
    {
        for ( i = 0; i < nData/2; i++ )
            pData[i] = Mini_AigMux( p, pCtrl[c], pData[2*i+1], pData[2*i] );
        if ( nData & 1 )
            pData[i++] = Mini_AigMux( p, pCtrl[c], 0, pData[nData-1] );
        nData = i;
    }
    assert( nData == 1 );
    return pData[0];
}
static int Mini_AigMuxMulti_rec( Mini_Aig_t * p, int * pCtrl, int * pData, int nData )
{
    int Res0, Res1;
    assert( nData > 0 );
    if ( nData == 1 )
        return pData[0];
    assert( nData % 2 == 0 );
    Res0 = Mini_AigMuxMulti_rec( p, pCtrl+1, pData,         nData/2 );
    Res1 = Mini_AigMuxMulti_rec( p, pCtrl+1, pData+nData/2, nData/2 );
    return Mini_AigMux( p, pCtrl[0], Res1, Res0 );
}
static Mini_Aig_t * Mini_AigTransformXor( Mini_Aig_t * p )
{
    Mini_Aig_t * pNew = Mini_AigStart();
    int i, * pCopy = MINI_AIG_ALLOC( int, Mini_AigNodeNum(p) );
    Mini_AigForEachPi( p, i )
        pCopy[i] = Mini_AigCreatePi(pNew);
    Mini_AigForEachAnd( p, i )
    {
        int iLit0 = Mini_AigNodeFanin0(p, i);
        int iLit1 = Mini_AigNodeFanin1(p, i);
        iLit0 = Mini_AigLitNotCond( pCopy[Mini_AigLit2Var(iLit0)], Mini_AigLitIsCompl(iLit0) );
        iLit1 = Mini_AigLitNotCond( pCopy[Mini_AigLit2Var(iLit1)], Mini_AigLitIsCompl(iLit1) );
        if ( iLit0 <= iLit1 )
            pCopy[i] = Mini_AigAnd( pNew, iLit0, iLit1 );
        else
            pCopy[i] = Mini_AigXor( pNew, iLit0, iLit1 );
    }
    Mini_AigForEachPo( p, i )
    {
        int iLit0 = Mini_AigNodeFanin0( p, i );
        iLit0 = Mini_AigLitNotCond( pCopy[Mini_AigLit2Var(iLit0)], Mini_AigLitIsCompl(iLit0) );
        pCopy[i] = Mini_AigCreatePo( pNew, iLit0 );
    }
    MINI_AIG_FREE( pCopy );
    return pNew;
}


static unsigned s_MiniTruths5[5] = {
    0xAAAAAAAA,
    0xCCCCCCCC,
    0xF0F0F0F0,
    0xFF00FF00,
    0xFFFF0000,
};
static inline int Mini_AigTt5HasVar( unsigned t, int iVar )
{
    return ((t << (1<<iVar)) & s_MiniTruths5[iVar]) != (t & s_MiniTruths5[iVar]);
}
static inline unsigned Mini_AigTt5Cofactor0( unsigned t, int iVar )
{
    assert( iVar >= 0 && iVar < 6 );
    return (t & ~s_MiniTruths5[iVar]) | ((t & ~s_MiniTruths5[iVar]) << (1<<iVar));
}
static inline unsigned Mini_AigTt5Cofactor1( unsigned t, int iVar )
{
    assert( iVar >= 0 && iVar < 6 );
    return (t & s_MiniTruths5[iVar]) | ((t & s_MiniTruths5[iVar]) >> (1<<iVar));
}
static inline int Mini_AigAndProp( Mini_Aig_t * p, int iLit0, int iLit1 )  
{ 
    if ( iLit0 < 2 )
        return iLit0 ? iLit1 : 0;
    if ( iLit1 < 2 )
        return iLit1 ? iLit0 : 0;
    if ( iLit0 == iLit1 )
        return iLit1;
    if ( iLit0 == Mini_AigLitNot(iLit1) )
        return 0;
    return Mini_AigAnd( p, iLit0, iLit1 );
}
static inline int Mini_AigMuxProp( Mini_Aig_t * p, int iCtrl, int iData1, int iData0 )  
{ 
    int iTemp0 = Mini_AigAndProp( p, Mini_AigLitNot(iCtrl), iData0 );
    int iTemp1 = Mini_AigAndProp( p, iCtrl, iData1 );
    return Mini_AigLitNot( Mini_AigAndProp( p, Mini_AigLitNot(iTemp0), Mini_AigLitNot(iTemp1) ) );
}
static inline int Mini_AigTruth( Mini_Aig_t * p, int * pVarLits, int nVars, unsigned Truth )
{
    int Var, Lit0, Lit1; 
    if ( Truth == 0 )
        return 0;
    if ( ~Truth == 0 )
        return 1;
    assert( nVars > 0 );
    // find the topmost var
    for ( Var = nVars-1; Var >= 0; Var-- )
        if ( Mini_AigTt5HasVar( Truth, Var ) )
             break;
    assert( Var >= 0 );
    // cofactor
    Lit0 = Mini_AigTruth( p, pVarLits, Var, Mini_AigTt5Cofactor0(Truth, Var) );
    Lit1 = Mini_AigTruth( p, pVarLits, Var, Mini_AigTt5Cofactor1(Truth, Var) );
    return Mini_AigMuxProp( p, pVarLits[Var], Lit1, Lit0 );
}
static char * Mini_AigPhase( Mini_Aig_t * p )
{
    char * pRes = MINI_AIG_CALLOC( char, Mini_AigNodeNum(p) );
    int i;
    Mini_AigForEachAnd( p, i )
    {
        int iFaninLit0 = Mini_AigNodeFanin0( p, i );
        int iFaninLit1 = Mini_AigNodeFanin1( p, i );
        int Phase0 = pRes[Mini_AigLit2Var(iFaninLit0)] ^ Mini_AigLitIsCompl(iFaninLit0);
        int Phase1 = pRes[Mini_AigLit2Var(iFaninLit1)] ^ Mini_AigLitIsCompl(iFaninLit1);
        pRes[i] = Phase0 & Phase1;
    }
    return pRes;
}

// procedure to check the topological order during AIG construction
static int Mini_AigCheck( Mini_Aig_t * p )
{
    int status = 1;
    int i, iFaninLit0, iFaninLit1;
    Mini_AigForEachAnd( p, i )
    {
        // get the fanin literals of this AND node
        iFaninLit0 = Mini_AigNodeFanin0( p, i );
        iFaninLit1 = Mini_AigNodeFanin1( p, i );
        // compare the fanin literals with the literal of the current node (2 * i)
        if ( iFaninLit0 >= 2 * i )
            printf( "Fanin0 of AND node %d is not in a topological order.\n", i ), status = 0;
        if ( iFaninLit1 >= 2 * i )
            printf( "Fanin0 of AND node %d is not in a topological order.\n", i ), status = 0;
    }
    Mini_AigForEachPo( p, i )
    {
        // get the fanin literal of this PO node
        iFaninLit0 = Mini_AigNodeFanin0( p, i );
        // compare the fanin literal with the literal of the current node (2 * i)
        if ( iFaninLit0 >= 2 * i )
            printf( "Fanin0 of PO node %d is not in a topological order.\n", i ), status = 0;
    }
    return status;
}

// procedure to dump MiniAIG into a Verilog file
static void Mini_AigDumpVerilog( char * pFileName, char * pModuleName, Mini_Aig_t * p )
{
    int i, k, iFaninLit0, iFaninLit1, Length = strlen(pModuleName), nPos = Mini_AigPoNum(p); 
    char * pObjIsPi = MINI_AIG_CALLOC( char, Mini_AigNodeNum(p) );
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL ) { printf( "Cannot open output file %s\n", pFileName ); MINI_AIG_FREE( pObjIsPi ); return; }
    // write interface
    //fprintf( pFile, "// This MiniAIG dump was produced by ABC on %s\n\n", Extra_TimeStamp() );
    fprintf( pFile, "module %s (\n", pModuleName );
    if ( Mini_AigPiNum(p) > 0 )
    {
        fprintf( pFile, "%*sinput wire", Length+10, "" );
        k = 0;
        Mini_AigForEachPi( p, i )
        {
            if ( k++ % 12 == 0 ) fprintf( pFile, "\n%*s", Length+10, "" );
            fprintf( pFile, "i%d, ", i );
            pObjIsPi[i] = 1;
        }
    }
    fprintf( pFile, "\n%*soutput wire", Length+10, "" );
    k = 0;
    Mini_AigForEachPo( p, i )
    {
        if ( k++ % 12 == 0 ) fprintf( pFile, "\n%*s", Length+10, "" );
        fprintf( pFile, "o%d%s", i, k==nPos ? "":", " );
    }
    fprintf( pFile, "\n%*s);\n\n", Length+8, "" );
    // write LUTs
    Mini_AigForEachAnd( p, i )
    {
        iFaninLit0 = Mini_AigNodeFanin0( p, i );
        iFaninLit1 = Mini_AigNodeFanin1( p, i );
        fprintf( pFile, "  assign n%d = ", i );
        fprintf( pFile, "%s%c%d", (iFaninLit0 & 1) ? "~":"", pObjIsPi[iFaninLit0 >> 1] ? 'i':'n', iFaninLit0 >> 1 );
        fprintf( pFile, " & " );
        fprintf( pFile, "%s%c%d", (iFaninLit1 & 1) ? "~":"", pObjIsPi[iFaninLit1 >> 1] ? 'i':'n', iFaninLit1 >> 1  );
        fprintf( pFile, ";\n" );
    }
    // write assigns
    fprintf( pFile, "\n" );
    Mini_AigForEachPo( p, i )
    {
        iFaninLit0 = Mini_AigNodeFanin0( p, i );
        fprintf( pFile, "  assign o%d = ", i );
        fprintf( pFile, "%s%c%d", (iFaninLit0 & 1) ? "~":"", pObjIsPi[iFaninLit0 >> 1] ? 'i':'n', iFaninLit0 >> 1 );
        fprintf( pFile, ";\n" );
    }
    fprintf( pFile, "\nendmodule // %s \n\n\n", pModuleName );
    MINI_AIG_FREE( pObjIsPi );
    fclose( pFile );
}

// procedure to dump MiniAIG into a BLIF file
static void Mini_AigDumpBlif( char * pFileName, char * pModuleName, Mini_Aig_t * p, int fVerbose )
{
    int i, k, iFaninLit0, iFaninLit1;
    char * pObjIsPi = MINI_AIG_FALLOC( char, Mini_AigNodeNum(p) );
    FILE * pFile = fopen( pFileName, "wb" );
    assert( Mini_AigPiNum(p) <= 26 );
    if ( pFile == NULL ) { printf( "Cannot open output file %s\n", pFileName ); MINI_AIG_FREE( pObjIsPi ); return; }
    // write interface
    //fprintf( pFile, "// This MiniAIG dump was produced by ABC on %s\n\n", Extra_TimeStamp() );
    fprintf( pFile, ".model %s\n", pModuleName );
    if ( Mini_AigPiNum(p) )
    {
        k = 0;
        fprintf( pFile, ".inputs" );
        Mini_AigForEachPi( p, i )
        {
            pObjIsPi[i] = k;
            fprintf( pFile, " %c", (char)('a'+k++) );
        }
    }
    k = 0;
    fprintf( pFile, "\n.outputs" );
    Mini_AigForEachPo( p, i )
        fprintf( pFile, " o%d", k++ );
    fprintf( pFile, "\n\n" );
    // write LUTs
    Mini_AigForEachAnd( p, i )
    {
        iFaninLit0 = Mini_AigNodeFanin0( p, i );
        iFaninLit1 = Mini_AigNodeFanin1( p, i );
        fprintf( pFile, ".names" );
        if ( pObjIsPi[iFaninLit0 >> 1] >= 0 )
            fprintf( pFile, " %c", (char)('a'+pObjIsPi[iFaninLit0 >> 1]) );
        else
            fprintf( pFile, " n%d", iFaninLit0 >> 1 );
        if ( pObjIsPi[iFaninLit1 >> 1] >= 0 )
            fprintf( pFile, " %c", (char)('a'+pObjIsPi[iFaninLit1 >> 1]) );
        else
            fprintf( pFile, " n%d", iFaninLit1 >> 1 );
        fprintf( pFile, " n%d\n", i );
        if ( iFaninLit0 < iFaninLit1 )
            fprintf( pFile, "%d%d 1\n", !(iFaninLit0 & 1), !(iFaninLit1 & 1) );
        else if ( !(iFaninLit0 & 1) == !(iFaninLit1 & 1) )
            fprintf( pFile, "10 1\n01 1\n" );
        else
            fprintf( pFile, "00 1\n11 1\n" );
    }
    // write assigns
    fprintf( pFile, "\n" );
    k = 0;
    Mini_AigForEachPo( p, i )
    {
        iFaninLit0 = Mini_AigNodeFanin0( p, i );
        fprintf( pFile, ".names" );
        if ( pObjIsPi[iFaninLit0 >> 1] >= 0 )
            fprintf( pFile, " %c", (char)('a'+pObjIsPi[iFaninLit0 >> 1]) );
        else
            fprintf( pFile, " n%d", iFaninLit0 >> 1 );
        fprintf( pFile, " o%d\n", k++ );
        fprintf( pFile, "%d 1\n", !(iFaninLit0 & 1) );
    }
    fprintf( pFile, ".end\n\n" );
    MINI_AIG_FREE( pObjIsPi );
    fclose( pFile );
    if ( fVerbose )
        printf( "Written MiniAIG into the BLIF file \"%s\".\n", pFileName );
}


// checks if MiniAIG is normalized (first inputs, then internal nodes, then outputs)
static int Mini_AigIsNormalized( Mini_Aig_t * p )
{
    int nCiNum = Mini_AigPiNum(p);
    int nCoNum = Mini_AigPoNum(p);
    int i, nOffset = 1;
    for ( i = 0; i < nCiNum; i++ )
        if ( !Mini_AigNodeIsPi( p, nOffset+i ) )
            return 0;
    nOffset = Mini_AigNodeNum(p) - nCoNum;
    for ( i = 0; i < nCoNum; i++ )
        if ( !Mini_AigNodeIsPo( p, nOffset+i ) )
            return 0;
    return 1;
}


////////////////////////////////////////////////////////////////////////
///         MiniAIG reading from / write into AIGER                  ///
////////////////////////////////////////////////////////////////////////

static unsigned Mini_AigerReadUnsigned( FILE * pFile )
{
    unsigned x = 0, i = 0;
    unsigned char ch;
    while ((ch = fgetc(pFile)) & 0x80)
        x |= (ch & 0x7f) << (7 * i++);
    return x | (ch << (7 * i));
}
static void Mini_AigerWriteUnsigned( FILE * pFile, unsigned x )
{
    unsigned char ch;
    while (x & ~0x7f)
    {
        ch = (x & 0x7f) | 0x80;
        fputc( ch, pFile );
        x >>= 7;
    }
    ch = x;
    fputc( ch, pFile );
}
static int * Mini_AigerReadInt( char * pFileName, int * pnObjs, int * pnIns, int * pnLatches, int * pnOuts, int * pnAnds )
{
    int i, Temp, nTotal, nObjs, nIns, nLatches, nOuts, nAnds, * pObjs;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Mini_AigerRead(): Cannot open the output file \"%s\".\n", pFileName );
        return NULL;
    }
    if ( fgetc(pFile) != 'a' || fgetc(pFile) != 'i' || fgetc(pFile) != 'g' )
    {
        fprintf( stdout, "Mini_AigerRead(): Can only read binary AIGER.\n" );
        fclose( pFile );
        return NULL;
    }
    if ( fscanf(pFile, "%d %d %d %d %d", &nTotal, &nIns, &nLatches, &nOuts, &nAnds) != 5 )
    {
        fprintf( stdout, "Mini_AigerRead(): Cannot read the header line.\n" );
        fclose( pFile );
        return NULL;
    }
    if ( nTotal != nIns + nLatches + nAnds )
    {
        fprintf( stdout, "The number of objects does not match.\n" );
        fclose( pFile );
        return NULL;
    }
    nObjs = 1 + nIns + 2*nLatches + nOuts + nAnds;
    pObjs = MINI_AIG_CALLOC( int, nObjs * 2 );
    for ( i = 0; i <= nIns + nLatches; i++ )
        pObjs[2*i] = pObjs[2*i+1] = MINI_AIG_NULL;
    // read flop input literals
    for ( i = 0; i < nLatches; i++ )
    {
        while ( fgetc(pFile) != '\n' );
        fscanf( pFile, "%d", &Temp );
        pObjs[2*(nObjs-nLatches+i)+0] = Temp;
        pObjs[2*(nObjs-nLatches+i)+1] = MINI_AIG_NULL;
    }
    // read output literals
    for ( i = 0; i < nOuts; i++ )
    {
        while ( fgetc(pFile) != '\n' );
        fscanf( pFile, "%d", &Temp );
        pObjs[2*(nObjs-nOuts-nLatches+i)+0] = Temp;
        pObjs[2*(nObjs-nOuts-nLatches+i)+1] = MINI_AIG_NULL;
    }
    // read the binary part
    while ( fgetc(pFile) != '\n' );
    for ( i = 0; i < nAnds; i++ )
    {
        int uLit  = 2*(1+nIns+nLatches+i);
        int uLit1 = uLit  - Mini_AigerReadUnsigned( pFile );
        int uLit0 = uLit1 - Mini_AigerReadUnsigned( pFile );
        pObjs[uLit+0] = uLit0;
        pObjs[uLit+1] = uLit1;
    }
    fclose( pFile );
    if ( pnObjs )    *pnObjs = nObjs;
    if ( pnIns )     *pnIns  = nIns;
    if ( pnLatches ) *pnLatches = nLatches;
    if ( pnOuts )    *pnOuts = nOuts;
    if ( pnAnds )    *pnAnds = nAnds;
    return pObjs;
}
static Mini_Aig_t * Mini_AigerRead( char * pFileName, int fVerbose )
{
    Mini_Aig_t * p;
    int nObjs, nIns, nLatches, nOuts, nAnds, * pObjs = Mini_AigerReadInt( pFileName, &nObjs, &nIns, &nLatches, &nOuts, &nAnds );
    if ( pObjs == NULL )
        return NULL;
    p = MINI_AIG_CALLOC( Mini_Aig_t, 1 );
    p->nCap   = 2*nObjs;
    p->nSize  = 2*nObjs;
    p->nRegs  = nLatches;
    p->pArray = pObjs;
    if ( fVerbose )
        printf( "Loaded MiniAIG from the AIGER file \"%s\".\n", pFileName );
    return p;
}

static void Mini_AigerWriteInt( char * pFileName, int * pObjs, int nObjs, int nIns, int nLatches, int nOuts, int nAnds )
{
    FILE * pFile = fopen( pFileName, "wb" ); int i;
    if ( pFile == NULL )
    {
        fprintf( stdout, "Mini_AigerWrite(): Cannot open the output file \"%s\".\n", pFileName );
        return;
    }
    fprintf( pFile, "aig %d %d %d %d %d\n", nIns + nLatches + nAnds, nIns, nLatches, nOuts, nAnds );
    for ( i = 0; i < nLatches; i++ )
        fprintf( pFile, "%d\n", pObjs[2*(nObjs-nLatches+i)+0] );
    for ( i = 0; i < nOuts; i++ )
        fprintf( pFile, "%d\n", pObjs[2*(nObjs-nOuts-nLatches+i)+0] );
    for ( i = 0; i < nAnds; i++ )
    {
        int uLit  = 2*(1+nIns+nLatches+i);
        int uLit0 = pObjs[uLit+0];
        int uLit1 = pObjs[uLit+1];
        Mini_AigerWriteUnsigned( pFile, uLit  - uLit1 );
        Mini_AigerWriteUnsigned( pFile, uLit1 - uLit0 );
    }
    fprintf( pFile, "c\n" );
    fclose( pFile );
}
static void Mini_AigerWrite( char * pFileName, Mini_Aig_t * p, int fVerbose )
{
    int i, nIns = 0, nOuts = 0, nAnds = 0;
    assert( Mini_AigIsNormalized(p) );
    for ( i = 1; i < Mini_AigNodeNum(p); i++ )
    {
        if ( Mini_AigNodeIsPi(p, i) )
            nIns++;
        else if ( Mini_AigNodeIsPo(p, i) )
            nOuts++;
        else 
            nAnds++;
    }
    Mini_AigerWriteInt( pFileName, p->pArray, p->nSize/2, nIns - p->nRegs, p->nRegs, nOuts - p->nRegs, nAnds );
    if ( fVerbose )
        printf( "Written MiniAIG into the AIGER file \"%s\".\n", pFileName );
}
static void Mini_AigerTest( char * pFileNameIn, char * pFileNameOut )
{
    Mini_Aig_t * p = Mini_AigerRead( pFileNameIn, 1 );
    if ( p == NULL )
        return;
    printf( "Finished reading input file \"%s\".\n", pFileNameIn );
    Mini_AigerWrite( pFileNameOut, p, 1 );
    printf( "Finished writing output file \"%s\".\n", pFileNameOut );
    Mini_AigStop( p );
}

/*
int main( int argc, char ** argv )
{
    if ( argc != 3 )
        return 0;
    Mini_AigerTest( argv[1], argv[2] );
    return 1;
}
*/

/*
#include "aig/miniaig/miniaig.h"

// this procedure creates a MiniAIG for function F = a*b + ~c and writes it into a file "test.aig"
void Mini_AigTest()
{
    Mini_Aig_t * p = Mini_AigStart();    // create empty AIG manager (contains only const0 node)
    int litApos = Mini_AigCreatePi( p ); // create input A (returns pos lit of A)
    int litBpos = Mini_AigCreatePi( p ); // create input B (returns pos lit of B)
    int litCpos = Mini_AigCreatePi( p ); // create input C (returns pos lit of C)
    int litCneg = Mini_AigLitNot( litCpos ); // neg lit of C
    int litAnd  = Mini_AigAnd( p, litApos, litBpos ); // lit for a*b
    int litOr   = Mini_AigOr( p, litAnd, litCneg );   // lit for a*b + ~c
    Mini_AigCreatePo( p, litOr );                     // create primary output
    Mini_AigerWrite( "test.aig", p, 1 );              // write the result into a file
    Mini_AigStop( p );                                // deallocate MiniAIG
}
*/

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

#ifndef _VERIFIC_DATABASE_H_
ABC_NAMESPACE_HEADER_END
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

