/**CFile****************************************************************

  FileName    [pla.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SOP manager.]

  Synopsis    [Scalable SOP transformations.]

  Author      [Alan Mishchenko]

  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - March 18, 2015.]

  Revision    [$Id: pla.h,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__wlc__wlc_h
#define ABC__base__wlc__wlc_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"
#include "misc/extra/extra.h"
#include "base/main/mainInt.h"
//#include "misc/util/utilTruth.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

#define MASK55 ABC_CONST(0x5555555555555555)

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// file types
typedef enum {
    PLA_FILE_FD = 0,
    PLA_FILE_F,
    PLA_FILE_FR,
    PLA_FILE_FDR,
    PLA_FILE_NONE
} Pla_File_t;

// literal types
typedef enum {
    PLA_LIT_DASH = 0,
    PLA_LIT_ZERO,
    PLA_LIT_ONE,
    PLA_LIT_FULL
} Pla_Lit_t;


typedef struct Pla_Man_t_ Pla_Man_t;
struct Pla_Man_t_
{
    char *           pName;      // model name
    char *           pSpec;      // input file
    Pla_File_t       Type;       // file type
    int              nIns;       // inputs
    int              nOuts;      // outputs
    int              nInWords;   // words of input data
    int              nOutWords;  // words of output data
    Vec_Int_t        vCubes;     // cubes
    Vec_Int_t        vHashes;    // hash values
    Vec_Wrd_t        vInBits;    // input bits
    Vec_Wrd_t        vOutBits;   // output bits
    Vec_Wec_t        vCubeLits;  // cubes as interger arrays
    Vec_Wec_t        vOccurs;    // occurence counters for the literals
    Vec_Int_t        vDivs;      // divisor definitions
};

static inline char * Pla_ManName( Pla_Man_t * p )                    { return p->pName;                   }
static inline int    Pla_ManInNum( Pla_Man_t * p )                   { return p->nIns;                    }
static inline int    Pla_ManOutNum( Pla_Man_t * p )                  { return p->nOuts;                   }
static inline int    Pla_ManCubeNum( Pla_Man_t * p )                 { return Vec_IntSize( &p->vCubes );  }
static inline int    Pla_ManDivNum( Pla_Man_t * p )                  { return Vec_IntSize( &p->vDivs );   }

static inline word * Pla_CubeIn( Pla_Man_t * p, int i )              { return Vec_WrdEntryP(&p->vInBits,  i * p->nInWords);  }
static inline word * Pla_CubeOut( Pla_Man_t * p, int i )             { return Vec_WrdEntryP(&p->vOutBits, i * p->nOutWords); }

static inline int    Pla_CubeNum( int hCube )                        { return hCube >> 8;    }
static inline int    Pla_CubeLit( int hCube )                        { return hCube & 0xFF;  }
static inline int    Pla_CubeHandle( int iCube, int iLit )           { assert( !(iCube >> 24) && !(iLit >> 8) ); return iCube << 8 | iLit; }

// read/write/flip i-th bit of a bit string table
static inline int    Pla_TtGetBit( word * p, int i )                 { return (int)(p[i>>6] >> (i & 63)) & 1;      }
static inline void   Pla_TtSetBit( word * p, int i )                 { p[i>>6] |= (((word)1)<<(i & 63));           }
static inline void   Pla_TtXorBit( word * p, int i )                 { p[i>>6] ^= (((word)1)<<(i & 63));           }

// read/write/flip i-th literal in a cube
static inline int    Pla_CubeGetLit( word * p, int i )               { return (int)(p[i>>5] >> ((i<<1) & 63)) & 3; }
static inline void   Pla_CubeSetLit( word * p, int i, Pla_Lit_t d )  { p[i>>5] |= (((word)d)<<((i<<1) & 63));      }
static inline void   Pla_CubeXorLit( word * p, int i, Pla_Lit_t d )  { p[i>>5] ^= (((word)d)<<((i<<1) & 63));      }


////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

#define Pla_ForEachCubeIn( p, pCube, i )                            \
    for ( i = 0; (i < Pla_ManCubeNum(p))  && (((pCube) = Pla_CubeIn(p, i)), 1); i++ )
#define Pla_ForEachCubeInStart( p, pCube, i, Start )                \
    for ( i = Start; (i < Pla_ManCubeNum(p))  && (((pCube) = Pla_CubeIn(p, i)), 1); i++ )

#define Pla_ForEachCubeOut( p, pCube, i )                           \
    for ( i = 0; (i < Pla_ManCubeNum(p))  && (((pCube) = Pla_CubeOut(p, i)), 1); i++ )
#define Pla_ForEachCubeInOut( p, pCubeIn, pCubeOut, i )             \
    for ( i = 0; (i < Pla_ManCubeNum(p))  && (((pCubeIn) = Pla_CubeIn(p, i)), 1)  && (((pCubeOut) = Pla_CubeOut(p, i)), 1); i++ )

#define Pla_CubeForEachLit( nVars, pCube, Lit, i )                  \
    for ( i = 0; (i < nVars)  && (((Lit) = Pla_CubeGetLit(pCube, i)), 1); i++ )
#define Pla_CubeForEachLitIn( p, pCube, Lit, i )                    \
    Pla_CubeForEachLit( Pla_ManInNum(p), pCube, Lit, i )
#define Pla_CubeForEachLitOut( p, pCube, Lit, i )                   \
    Pla_CubeForEachLit( Pla_ManOutNum(p), pCube, Lit, i )


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks if cubes are distance-1.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Pla_OnlyOneOne( word t )
{
    return t ? ((t & (t-1)) == 0) : 0;
}
static inline int Pla_CubesAreDistance1( word * p, word * q, int nWords )
{
    word Test; int c, fFound = 0;
    for ( c = 0; c < nWords; c++ )
    {
        if ( p[c] == q[c] )
            continue;
        if ( fFound )
            return 0;
        // check if the number of 1s is one, which means exactly one different literal (0/1, -/1, -/0)
        Test = ((p[c] ^ q[c]) | ((p[c] ^ q[c]) >> 1)) & MASK55;
        if ( !Pla_OnlyOneOne(Test) )
            return 0;
        fFound = 1;
    }
    return fFound;
}
static inline int Pla_CubesAreConsensus( word * p, word * q, int nWords, int * piVar )
{
    word Test; int c, fFound = 0;
    for ( c = 0; c < nWords; c++ )
    {
        if ( p[c] == q[c] )
            continue;
        if ( fFound )
            return 0;
        // check if the number of 1s is one, which means exactly one opposite literal (0/1) but may have other diffs (-/0 or -/1)
        Test = ((p[c] ^ q[c]) & ((p[c] ^ q[c]) >> 1)) & MASK55;
        if ( !Pla_OnlyOneOne(Test) )
            return 0;
        fFound = 1;
        if ( piVar ) *piVar = c * 32 + Abc_Tt6FirstBit(Test)/2;
    }
    return fFound;
}
static inline int Pla_TtCountOnesOne( word x )
{
    x = x - ((x >> 1) & ABC_CONST(0x5555555555555555));
    x = (x & ABC_CONST(0x3333333333333333)) + ((x >> 2) & ABC_CONST(0x3333333333333333));
    x = (x + (x >> 4)) & ABC_CONST(0x0F0F0F0F0F0F0F0F);
    x = x + (x >> 8);
    x = x + (x >> 16);
    x = x + (x >> 32);
    return (int)(x & 0xFF);
}
static inline int Pla_TtCountOnes( word * p, int nWords )
{
    int i, Count = 0;
    for ( i = 0; i < nWords; i++ )
        Count += Pla_TtCountOnesOne( p[i] );
    return Count;
}

/**Function*************************************************************

  Synopsis    [Manager APIs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Pla_Man_t * Pla_ManAlloc( char * pFileName, int nIns, int nOuts, int nCubes )
{
    Pla_Man_t * p = ABC_CALLOC( Pla_Man_t, 1 );
    p->pName = Extra_FileDesignName( pFileName );
    p->pSpec = Abc_UtilStrsav( pFileName );
    p->nIns  = nIns;
    p->nOuts = nOuts;
    p->nInWords  = Abc_Bit6WordNum( 2*nIns );
    p->nOutWords = Abc_Bit6WordNum( 2*nOuts );
    Vec_IntFillNatural( &p->vCubes, nCubes );
    Vec_WrdFill( &p->vInBits,  Pla_ManCubeNum(p) * p->nInWords,  0 );
    Vec_WrdFill( &p->vOutBits, Pla_ManCubeNum(p) * p->nOutWords, 0 );
    return p;
}
static inline void Pla_ManFree( Pla_Man_t * p )
{
    Vec_IntErase( &p->vCubes );
    Vec_IntErase( &p->vHashes );
    Vec_WrdErase( &p->vInBits );
    Vec_WrdErase( &p->vOutBits );
    Vec_WecErase( &p->vCubeLits );
    Vec_WecErase( &p->vOccurs );
    Vec_IntErase( &p->vDivs );
    ABC_FREE( p->pName );
    ABC_FREE( p->pSpec );
    ABC_FREE( p );
}
static inline int Pla_ManLitInNum( Pla_Man_t * p )
{
    word * pCube; int i, k, Lit, Count = 0;
    Pla_ForEachCubeIn( p, pCube, i )
        Pla_CubeForEachLitIn( p, pCube, Lit, k )
            Count += Lit != PLA_LIT_DASH;
    return Count;
}
static inline int Pla_ManLitOutNum( Pla_Man_t * p )
{
    word * pCube; int i, k, Lit, Count = 0;
    Pla_ForEachCubeOut( p, pCube, i )
        Pla_CubeForEachLitOut( p, pCube, Lit, k )
            Count += Lit == PLA_LIT_ONE;
    return Count;
}
static inline void Pla_ManPrintStats( Pla_Man_t * p, int fVerbose )
{
    printf( "%-16s :  ",     Pla_ManName(p) );
    printf( "In =%4d  ",     Pla_ManInNum(p) );
    printf( "Out =%4d  ",    Pla_ManOutNum(p) );
    printf( "Cube =%8d  ",   Pla_ManCubeNum(p) );
    printf( "LitIn =%8d  ",  Pla_ManLitInNum(p) );
    printf( "LitOut =%8d  ", Pla_ManLitOutNum(p) );
    printf( "Div =%6d  ",    Pla_ManDivNum(p) );
    printf( "\n" );
}



/*=== plaHash.c ========================================================*/
extern int                 Pla_ManHashDist1NumTest( Pla_Man_t * p );
extern void                Pla_ManComputeDist1Test( Pla_Man_t * p );
/*=== plaMan.c ========================================================*/
extern Vec_Bit_t *         Pla_ManPrimesTable( int nVars );
extern Pla_Man_t *         Pla_ManPrimesDetector( int nVars );
extern Pla_Man_t *         Pla_ManGenerate( int nIns, int nOuts, int nCubes, int fVerbose );
extern void                Pla_ManConvertFromBits( Pla_Man_t * p );
extern void                Pla_ManConvertToBits( Pla_Man_t * p );
extern int                 Pla_ManDist1NumTest( Pla_Man_t * p );
/*=== plaMerge.c ========================================================*/
extern int                 Pla_ManDist1Merge( Pla_Man_t * p );
/*=== plaSimple.c ========================================================*/
extern int                 Pla_ManFxPerformSimple( int nVars );
/*=== plaRead.c ========================================================*/
extern Pla_Man_t *         Pla_ReadPla( char * pFileName );
/*=== plaWrite.c ========================================================*/
extern void                Pla_WritePla( Pla_Man_t * p, char * pFileName );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
