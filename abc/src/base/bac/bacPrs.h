/**CFile****************************************************************

  FileName    [bacPrs.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Parser declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bacPrs.h,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__prs__prs_h
#define ABC__base__prs__prs_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"
#include "misc/util/utilNam.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START 

// parser name types
typedef enum { 
    BAC_PRS_NAME = 0,        // 0:  name/variable
    BAC_PRS_SLICE,           // 1:  slice
    BAC_PRS_CONST,           // 2:  constant
    BAC_PRS_CONCAT,          // 3:  concatentation
} Psr_ManType_t; 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// network
typedef struct Psr_Ntk_t_ Psr_Ntk_t;
struct Psr_Ntk_t_
{
    // general info
    int          iModuleName;
    unsigned     fMapped : 1;
    unsigned     fSlices : 1;
    unsigned     fHasC0s : 1;
    unsigned     fHasC1s : 1;
    unsigned     fHasCXs : 1;
    unsigned     fHasCZs : 1;
    Abc_Nam_t *  pStrs;
    // interface
    Vec_Int_t    vOrder;     // order of signals
    // signal names
    Vec_Int_t    vInouts;    // inouts 
    Vec_Int_t    vInputs;    // inputs 
    Vec_Int_t    vOutputs;   // outputs
    Vec_Int_t    vWires;     // wires  
    // signal ranges
    Vec_Int_t    vInoutsR;   // inouts 
    Vec_Int_t    vInputsR;   // inputs 
    Vec_Int_t    vOutputsR;  // outputs
    Vec_Int_t    vWiresR;    // wires  
    // slices/concatenations/objects
    Vec_Int_t    vSlices;    // NameId + RangeId
    Vec_Int_t    vConcats;   // array of NameId/SliceId/ConstId
    Vec_Int_t    vBoxes;     // ModuleId + InstId + array of pairs {FormNameId, ActSignalId(NameId/SliceId/ConstId/ConcatId)}
    Vec_Int_t    vObjs;      // box handles
};

// parser
typedef struct Psr_Man_t_ Psr_Man_t;
struct Psr_Man_t_
{
    // input data
    char *       pName;       // file name
    char *       pBuffer;     // file contents
    char *       pLimit;      // end of file
    char *       pCur;        // current position
    Abc_Nam_t *  pStrs;       // string manager
    Psr_Ntk_t *  pNtk;        // current network
    Vec_Ptr_t *  vNtks;       // input networks
    // temporary data
    Vec_Str_t    vCover;      // one SOP cover
    Vec_Int_t    vTemp;       // array of tokens
    Vec_Int_t    vTemp2;      // array of tokens
    // statistics
    Vec_Int_t    vKnown;
    Vec_Int_t    vFailed;
    Vec_Int_t    vSucceeded;
    // error handling
    int          fUsingTemp2; // vTemp2 is in use
    char ErrorStr[1000];      // error
};

static inline Psr_Ntk_t * Psr_ManNtk( Vec_Ptr_t * vPrs, int i )        { return i >= 0 && i < Vec_PtrSize(vPrs) ? (Psr_Ntk_t *)Vec_PtrEntry(vPrs, i) : NULL; }
static inline Psr_Ntk_t * Psr_ManRoot( Vec_Ptr_t * vPrs )              { return Psr_ManNtk(vPrs, 0);                             }
static inline Abc_Nam_t * Psr_ManNameMan( Vec_Ptr_t * vPrs )           { return Psr_ManRoot(vPrs)->pStrs;                        }

static inline int         Psr_NtkId( Psr_Ntk_t * p )                   { return p->iModuleName;                                  }
static inline int         Psr_NtkPioNum( Psr_Ntk_t * p )               { return Vec_IntSize(&p->vInouts);                        }
static inline int         Psr_NtkPiNum( Psr_Ntk_t * p )                { return Vec_IntSize(&p->vInputs);                        }
static inline int         Psr_NtkPoNum( Psr_Ntk_t * p )                { return Vec_IntSize(&p->vOutputs);                       }
static inline int         Psr_NtkBoxNum( Psr_Ntk_t * p )               { return Vec_IntSize(&p->vObjs);                          }
static inline int         Psr_NtkObjNum( Psr_Ntk_t * p )               { return Psr_NtkPioNum(p) + Psr_NtkPiNum(p) + Psr_NtkPoNum(p) + Psr_NtkBoxNum(p); }
static inline char *      Psr_NtkStr( Psr_Ntk_t * p, int h )           { return Abc_NamStr(p->pStrs, h);                         }
static inline char *      Psr_NtkName( Psr_Ntk_t * p )                 { return Psr_NtkStr(p, Psr_NtkId(p));                     }
static inline int         Psr_NtkSigName( Psr_Ntk_t * p, int i )       { if (!p->fSlices) return i; assert(Abc_Lit2Att2(i) == BAC_PRS_NAME); return Abc_Lit2Var2(i); }

static inline int         Psr_SliceName( Psr_Ntk_t * p, int h )        { return Vec_IntEntry(&p->vSlices, h);                    }
static inline int         Psr_SliceRange( Psr_Ntk_t * p, int h )       { return Vec_IntEntry(&p->vSlices, h+1);                  }

static inline int         Psr_CatSize( Psr_Ntk_t * p, int h )          { return Vec_IntEntry(&p->vConcats, h);                   }
static inline int *       Psr_CatArray( Psr_Ntk_t * p, int h )         { return Vec_IntEntryP(&p->vConcats, h+1);                }
static inline Vec_Int_t * Psr_CatSignals( Psr_Ntk_t * p, int h )       { static Vec_Int_t V; V.nSize = V.nCap = Psr_CatSize(p, h); V.pArray = Psr_CatArray(p, h); return &V; }

static inline int         Psr_BoxHand( Psr_Ntk_t * p, int i )          { return Vec_IntEntry(&p->vObjs, i);                      }
static inline int         Psr_BoxSize( Psr_Ntk_t * p, int i )          { return Vec_IntEntry(&p->vBoxes, Psr_BoxHand(p, i))-2;   }
static inline int         Psr_BoxIONum( Psr_Ntk_t * p, int i )         { return Psr_BoxSize(p, i) / 2;                           }
static inline int         Psr_BoxNtk( Psr_Ntk_t * p, int i )           { return Vec_IntEntry(&p->vBoxes, Psr_BoxHand(p, i)+1);   }
static inline void        Psr_BoxSetNtk( Psr_Ntk_t * p, int i, int m ) { Vec_IntWriteEntry(&p->vBoxes, Psr_BoxHand(p, i)+1, m);  }
static inline int         Psr_BoxName( Psr_Ntk_t * p, int i )          { return Vec_IntEntry(&p->vBoxes, Psr_BoxHand(p, i)+2);   }
static inline int         Psr_BoxIsNode( Psr_Ntk_t * p, int i )        { return!Vec_IntEntry(&p->vBoxes, Psr_BoxHand(p, i)+3);   } // no formal names
static inline int *       Psr_BoxArray( Psr_Ntk_t * p, int i )         { return Vec_IntEntryP(&p->vBoxes, Psr_BoxHand(p, i)+3);  }
static inline Vec_Int_t * Psr_BoxSignals( Psr_Ntk_t * p, int i )       { static Vec_Int_t V; V.nSize = V.nCap = Psr_BoxSize(p, i); V.pArray = Psr_BoxArray(p, i); return &V; }

#define Psr_ManForEachNameVec( vVec, p, pName, i )   \
    for ( i = 0; (i < Vec_IntSize(vVec)) && ((pName) = Abc_NamStr(p->pStrs, Vec_IntEntry(vVec,i))); i++ )

#define Psr_NtkForEachPio( p, NameId, i )            \
    for ( i = 0; i < Psr_NtkPioNum(p) && ((NameId) = Vec_IntEntry(&p->vInouts, i)); i++ )
#define Psr_NtkForEachPi( p, NameId, i )             \
    for ( i = 0; i < Psr_NtkPiNum(p) && ((NameId) = Vec_IntEntry(&p->vInputs, i)); i++ )
#define Psr_NtkForEachPo( p, NameId, i )             \
    for ( i = 0; i < Psr_NtkPoNum(p) && ((NameId) = Vec_IntEntry(&p->vOutputs, i)); i++ )
#define Psr_NtkForEachBox( p, vVec, i )              \
    for ( i = 0; i < Psr_NtkBoxNum(p) && ((vVec) = Psr_BoxSignals(p, i)); i++ )

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// create error message
static inline int Psr_ManErrorSet( Psr_Man_t * p, char * pError, int Value )
{
    assert( !p->ErrorStr[0] );
    sprintf( p->ErrorStr, "%s", pError );
    return Value;
}
// clear error message
static inline void Psr_ManErrorClear( Psr_Man_t * p )
{
    p->ErrorStr[0] = '\0';
}
// print error message
static inline int Psr_ManErrorPrint( Psr_Man_t * p )
{
    char * pThis; int iLine = 0;
    if ( !p->ErrorStr[0] ) return 1;
    for ( pThis = p->pBuffer; pThis < p->pCur; pThis++ )
        iLine += (int)(*pThis == '\n');
    printf( "Line %d: %s\n", iLine, p->ErrorStr );
    return 0;
}

// parsing network
static inline void Psr_ManInitializeNtk( Psr_Man_t * p, int iName, int fSlices )
{
    assert( p->pNtk == NULL );
    p->pNtk = ABC_CALLOC( Psr_Ntk_t, 1 );
    p->pNtk->iModuleName = iName;
    p->pNtk->fSlices = fSlices;
    p->pNtk->pStrs = Abc_NamRef( p->pStrs );
    Vec_PtrPush( p->vNtks, p->pNtk );
}
static inline void Psr_ManFinalizeNtk( Psr_Man_t * p )
{
    assert( p->pNtk != NULL );
    p->pNtk = NULL;
}

// parsing slice/concatentation/box
static inline int Psr_NtkAddSlice( Psr_Ntk_t * p, int Name, int Range )
{
    int Value = Vec_IntSize(&p->vSlices);
    Vec_IntPushTwo( &p->vSlices, Name, Range );
    return Value;
}
static inline int Psr_NtkAddConcat( Psr_Ntk_t * p, Vec_Int_t * vTemp )
{
    int Value;
    if ( !(Vec_IntSize(&p->vConcats) & 1) )
        Vec_IntPush(&p->vConcats, -1);
    Value = Vec_IntSize(&p->vConcats);
    assert( Value & 1 );
    Vec_IntPush( &p->vConcats, Vec_IntSize(vTemp) );
    Vec_IntAppend( &p->vConcats, vTemp );
    return Value;
}
static inline void Psr_NtkAddBox( Psr_Ntk_t * p, int ModName, int InstName, Vec_Int_t * vTemp )
{
    int Value;
    assert( Vec_IntSize(vTemp) % 2 == 0 );
    if ( !(Vec_IntSize(&p->vBoxes) & 1) )
        Vec_IntPush(&p->vBoxes, -1);
    Value = Vec_IntSize(&p->vBoxes);
    assert( Value & 1 );
    Vec_IntPush( &p->vObjs, Value );
    // create entry
    Vec_IntPush( &p->vBoxes, Vec_IntSize(vTemp)+2 );
    Vec_IntPush( &p->vBoxes, ModName );
    Vec_IntPush( &p->vBoxes, InstName );
    Vec_IntAppend( &p->vBoxes, vTemp );
}


static inline char * Psr_ManLoadFile( char * pFileName, char ** ppLimit )
{
    char * pBuffer;
    int nFileSize, RetValue;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open input file.\n" );
        return NULL;
    }
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    pBuffer = ABC_ALLOC( char, nFileSize + 16 );
    pBuffer[0] = '\n';
    RetValue = fread( pBuffer+1, nFileSize, 1, pFile );
    fclose( pFile );
    // terminate the string with '\0'
    pBuffer[nFileSize + 1] = '\n';
    pBuffer[nFileSize + 2] = '\0';
    *ppLimit = pBuffer + nFileSize + 3;
    return pBuffer;
}
static inline Psr_Man_t * Psr_ManAlloc( char * pFileName )
{
    Psr_Man_t * p;
    char * pBuffer, * pLimit;
    pBuffer = Psr_ManLoadFile( pFileName, &pLimit );
    if ( pBuffer == NULL )
        return NULL;
    p = ABC_CALLOC( Psr_Man_t, 1 );
    p->pName   = pFileName;
    p->pBuffer = pBuffer;
    p->pLimit  = pLimit;
    p->pCur    = pBuffer;
    p->pStrs   = Abc_NamStart( 1000, 24 );
    p->vNtks   = Vec_PtrAlloc( 100 );
    return p;
}

static inline void Psr_NtkFree( Psr_Ntk_t * p )
{
    if ( p->pStrs )
        Abc_NamDeref( p->pStrs );
    Vec_IntErase( &p->vOrder );
    Vec_IntErase( &p->vInouts );
    Vec_IntErase( &p->vInputs );
    Vec_IntErase( &p->vOutputs );
    Vec_IntErase( &p->vWires );
    Vec_IntErase( &p->vInoutsR );
    Vec_IntErase( &p->vInputsR );
    Vec_IntErase( &p->vOutputsR );
    Vec_IntErase( &p->vWiresR );
    Vec_IntErase( &p->vSlices );
    Vec_IntErase( &p->vConcats );
    Vec_IntErase( &p->vBoxes );
    Vec_IntErase( &p->vObjs );
    ABC_FREE( p );
}

static inline void Psr_ManVecFree( Vec_Ptr_t * vPrs )
{
    Psr_Ntk_t * pNtk; int i;
    Vec_PtrForEachEntry( Psr_Ntk_t *, vPrs, pNtk, i )
        Psr_NtkFree( pNtk );
    Vec_PtrFree( vPrs );
}

static inline void Psr_ManFree( Psr_Man_t * p )
{
    if ( p->pStrs )
        Abc_NamDeref( p->pStrs );
    if ( p->vNtks )
        Psr_ManVecFree( p->vNtks );
    // temporary
    Vec_StrErase( &p->vCover );
    Vec_IntErase( &p->vTemp );
    Vec_IntErase( &p->vTemp2 );
    Vec_IntErase( &p->vKnown );
    Vec_IntErase( &p->vFailed );
    Vec_IntErase( &p->vSucceeded );
    ABC_FREE( p->pBuffer );
    ABC_FREE( p );
}

static inline int Psr_NtkMemory( Psr_Ntk_t * p )
{
    int nMem = sizeof(Psr_Ntk_t);
    nMem += Vec_IntMemory( &p->vOrder );
    nMem += Vec_IntMemory( &p->vInouts );
    nMem += Vec_IntMemory( &p->vInputs );
    nMem += Vec_IntMemory( &p->vOutputs );
    nMem += Vec_IntMemory( &p->vWires );
    nMem += Vec_IntMemory( &p->vInoutsR );
    nMem += Vec_IntMemory( &p->vInputsR );
    nMem += Vec_IntMemory( &p->vOutputsR );
    nMem += Vec_IntMemory( &p->vWiresR );
    nMem += Vec_IntMemory( &p->vSlices );
    nMem += Vec_IntMemory( &p->vBoxes );
    nMem += Vec_IntMemory( &p->vConcats );
    return nMem;
}
static inline int Psr_ManMemory( Vec_Ptr_t * vPrs )
{
    Psr_Ntk_t * pNtk; int i;
    int nMem = Vec_PtrMemory(vPrs);
    Vec_PtrForEachEntry( Psr_Ntk_t *, vPrs, pNtk, i )
        nMem += Psr_NtkMemory( pNtk );
    nMem += Abc_NamMemUsed(Psr_ManNameMan(vPrs));
    return nMem;
}



////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== bac.c ========================================================*/


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

