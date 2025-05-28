/**CFile****************************************************************

  FileName    [bblif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Binary BLIF representation for logic networks.]

  Synopsis    [Main implementation module.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 28, 2009.]

  Revision    [$Id: bblif.c,v 1.00 2009/02/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/util/abc_global.h"
#include "bblif.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// vector of integers
typedef struct Vec_Int_t_ Vec_Int_t;
struct Vec_Int_t_ 
{
    int         nCap;
    int         nSize;
    int *       pArray;
};

// vector of characters
typedef struct Vec_Str_t_ Vec_Str_t;
struct Vec_Str_t_ 
{
    int         nCap;
    int         nSize;
    char *      pArray;
};

// network object
struct Bbl_Obj_t_
{
    int         Id;              // user ID
    int         Fnc;             // functionality
    unsigned    fCi     :  1;    // combinational input
    unsigned    fCo     :  1;    // combinational output
    unsigned    fBox    :  1;    // subcircuit
    unsigned    fMark   :  1;    // temporary mark
    unsigned    nFanins : 28;    // fanin number
    int         pFanins[0];      // fanin array
};

// object function
typedef struct Bbl_Fnc_t_ Bbl_Fnc_t;
struct Bbl_Fnc_t_
{
    int         nWords;          // word number
    int         pWords[0];       // word array
};

// object function
typedef struct Bbl_Ent_t_ Bbl_Ent_t;
struct Bbl_Ent_t_
{
    int         iFunc;           // function handle
    int         iNext;           // next entry handle
};

// data manager
struct Bbl_Man_t_
{
    // data pool
    Vec_Str_t * pName;           // design name
    Vec_Str_t * pObjs;           // vector of objects
    Vec_Str_t * pFncs;           // vector of functions
    // construction
    Vec_Int_t * vId2Obj;         // mapping user IDs into objects
    Vec_Int_t * vObj2Id;         // mapping objects into user IDs
    Vec_Int_t * vFaninNums;      // mapping user IDs into fanin number
    // file contents
    int         nFileSize;       // file size
    char *      pFileData;       // file contents
    // other data
    Vec_Str_t * pEnts;           // vector of entries
    int         SopMap[17][17];  // mapping vars x cubes into entry handles
};

static inline int         Bbl_ObjIsCi( Bbl_Obj_t * pObj )          { return pObj->fCi;                                        } 
static inline int         Bbl_ObjIsCo( Bbl_Obj_t * pObj )          { return pObj->fCo;                                        } 
static inline int         Bbl_ObjIsNode( Bbl_Obj_t * pObj )        { return!pObj->fCi && !pObj->fCo;                          } 

static inline int         Bbl_ObjFaninNum( Bbl_Obj_t * pObj )      { return pObj->nFanins;                                    } 
static inline Bbl_Obj_t * Bbl_ObjFanin( Bbl_Obj_t * pObj, int i )  { return (Bbl_Obj_t *)(((char *)pObj) - pObj->pFanins[i]); } 

static inline int         Bbl_ObjSize( Bbl_Obj_t * pObj )          { return sizeof(Bbl_Obj_t) + sizeof(int) * pObj->nFanins;  } 
static inline int         Bbl_FncSize( Bbl_Fnc_t * pFnc )          { return sizeof(Bbl_Fnc_t) + sizeof(int) * pFnc->nWords;   } 

static inline Bbl_Obj_t * Bbl_VecObj( Vec_Str_t * p, int h )       { return (Bbl_Obj_t *)(p->pArray + h);                     } 
static inline Bbl_Fnc_t * Bbl_VecFnc( Vec_Str_t * p, int h )       { return (Bbl_Fnc_t *)(p->pArray + h);                     } 
static inline Bbl_Ent_t * Bbl_VecEnt( Vec_Str_t * p, int h )       { return (Bbl_Ent_t *)(p->pArray + h);                     } 

static inline char *      Bbl_ManSop( Bbl_Man_t * p, int h )       { return (char *)Bbl_VecFnc(p->pFncs, h)->pWords;          } 
static inline Bbl_Obj_t * Bbl_ManObj( Bbl_Man_t * p, int Id )      { return Bbl_VecObj(p->pObjs, p->vId2Obj->pArray[Id]);     } 

#define Bbl_ManForEachObj_int( p, pObj, h )                \
    for ( h = 0; (h < p->nSize) && (pObj = Bbl_VecObj(p,h)); h += Bbl_ObjSize(pObj) )
#define Bbl_ManForEachFnc_int( p, pObj, h )                \
    for ( h = 0; (h < p->nSize) && (pObj = Bbl_VecFnc(p,h)); h += Bbl_FncSize(pObj) )
#define Bbl_ObjForEachFanin_int( pObj, pFanin, i )         \
    for ( i = 0; (i < (int)pObj->nFanins) && (pFanin = Bbl_ObjFanin(pObj,i)); i++ )

#define BBLIF_ALLOC(type, num)     ((type *) malloc(sizeof(type) * (num)))
#define BBLIF_CALLOC(type, num)     ((type *) calloc((num), sizeof(type)))
#define BBLIF_FALLOC(type, num)     ((type *) memset(malloc(sizeof(type) * (num)), 0xff, sizeof(type) * (num)))
#define BBLIF_FREE(obj)             ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#define BBLIF_REALLOC(type, obj, num)    \
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (num))) : \
         ((type *) malloc(sizeof(type) * (num))))

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntAlloc( int nCap )
{
    Vec_Int_t * p;
    p = BBLIF_ALLOC( Vec_Int_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? BBLIF_ALLOC( int, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given size and cleans it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntStart( int nSize )
{
    Vec_Int_t * p;
    p = Vec_IntAlloc( nSize );
    p->nSize = nSize;
    memset( p->pArray, 0, sizeof(int) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given size and cleans it.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntStartNatural( int nSize )
{
    Vec_Int_t * p;
    int i;
    p = Vec_IntAlloc( nSize );
    p->nSize = nSize;
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = i;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from an integer array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vec_IntAllocArray( int * pArray, int nSize )
{
    Vec_Int_t * p;
    p = BBLIF_ALLOC( Vec_Int_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = pArray;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntFree( Vec_Int_t * p )
{
    BBLIF_FREE( p->pArray );
    BBLIF_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntSize( Vec_Int_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntEntry( Vec_Int_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntWriteEntry( Vec_Int_t * p, int i, int Entry )
{
    assert( i >= 0 && i < p->nSize );
    p->pArray[i] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntAddToEntry( Vec_Int_t * p, int i, int Addition )
{
    assert( i >= 0 && i < p->nSize );
    p->pArray[i] += Addition;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntEntryLast( Vec_Int_t * p )
{
    assert( p->nSize > 0 );
    return p->pArray[p->nSize-1];
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntGrow( Vec_Int_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = BBLIF_REALLOC( int, p->pArray, nCapMin ); 
    assert( p->pArray );
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntFill( Vec_Int_t * p, int nSize, int Fill )
{
    int i;
    Vec_IntGrow( p, nSize );
    for ( i = 0; i < nSize; i++ )
        p->pArray[i] = Fill;
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntFillExtra( Vec_Int_t * p, int nSize, int Fill )
{
    int i;
    if ( p->nSize >= nSize )
        return;
    if ( nSize > 2 * p->nCap )
        Vec_IntGrow( p, nSize );
    else if ( nSize > p->nCap )
        Vec_IntGrow( p, 2 * p->nCap );
    for ( i = p->nSize; i < nSize; i++ )
        p->pArray[i] = Fill;
    p->nSize = nSize;
}

/**Function*************************************************************

  Synopsis    [Returns the entry even if the place not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntGetEntry( Vec_Int_t * p, int i )
{
    Vec_IntFillExtra( p, i + 1, 0 );
    return Vec_IntEntry( p, i );
}

/**Function*************************************************************

  Synopsis    [Inserts the entry even if the place does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntSetEntry( Vec_Int_t * p, int i, int Entry )
{
    Vec_IntFillExtra( p, i + 1, 0 );
    Vec_IntWriteEntry( p, i, Entry );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntShrink( Vec_Int_t * p, int nSizeNew )
{
    assert( p->nSize >= nSizeNew );
    p->nSize = nSizeNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntClear( Vec_Int_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntPush( Vec_Int_t * p, int Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_IntGrow( p, 16 );
        else
            Vec_IntGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}





/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Str_t * Vec_StrAlloc( int nCap )
{
    Vec_Str_t * p;
    p = BBLIF_ALLOC( Vec_Str_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? BBLIF_ALLOC( char, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from an array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Str_t * Vec_StrAllocArray( char * pArray, int nSize )
{
    Vec_Str_t * p;
    p = BBLIF_ALLOC( Vec_Str_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = pArray;
    return p;
}

/**Fnction*************************************************************

  Synopsis    [Returns a piece of memory.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Vec_StrFetch( Vec_Str_t * p, int nBytes )
{
    while ( p->nSize + nBytes > p->nCap )
    {
        p->pArray = BBLIF_REALLOC( char, p->pArray, 3 * p->nCap );
        p->nCap *= 3;
    }
    p->nSize += nBytes;
    return p->pArray + p->nSize - nBytes;
}

/**Fnction*************************************************************

  Synopsis    [Write vector into file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vec_StrWrite( FILE * pFile, Vec_Str_t * p )
{
    fwrite( &p->nSize, sizeof(int), 1, pFile );
    fwrite( p->pArray, sizeof(char), p->nSize, pFile );
}

/**Fnction*************************************************************

  Synopsis    [Write vector into file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Vec_StrRead( char ** ppStr )
{
    Vec_Str_t * p;
    char * pStr = *ppStr;
    p = Vec_StrAlloc( 0 );
    p->nSize = *(int *)pStr;
    p->pArray = pStr + sizeof(int);
    *ppStr = pStr + sizeof(int) + p->nSize * sizeof(char);
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_StrSize( Vec_Str_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_StrFree( Vec_Str_t * p )
{
    BBLIF_FREE( p->pArray );
    BBLIF_FREE( p );
}





/**Fnction*************************************************************

  Synopsis    [Returns the file size.]

  Description [The file should be closed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bbl_ManFileSize( char * pFileName )
{
    FILE * pFile;
    int nFileSize;
    pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Bbl_ManFileSize(): The file is unavailable (absent or open).\n" );
        return 0;
    }
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile ); 
    fclose( pFile );
    return nFileSize;
}

/**Fnction*************************************************************

  Synopsis    [Read data from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Bbl_ManFileRead( char * pFileName )
{
    FILE * pFile;
    char * pContents;
    int nFileSize;
    int RetValue;
    nFileSize = Bbl_ManFileSize( pFileName );
    pFile = fopen( pFileName, "rb" );
    pContents = BBLIF_ALLOC( char, nFileSize );
    RetValue = fread( pContents, nFileSize, 1, pFile );
    fclose( pFile );
    return pContents;
}



/**Fnction*************************************************************

  Synopsis    [Writes data into file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbl_ManDumpBinaryBlif( Bbl_Man_t * p, char * pFileName )
{
    FILE * pFile;
    pFile = fopen( pFileName, "wb" );
    Vec_StrWrite( pFile, p->pName );
    Vec_StrWrite( pFile, p->pObjs );
    Vec_StrWrite( pFile, p->pFncs );
    fclose( pFile );
}

/**Fnction*************************************************************

  Synopsis    [Creates manager after reading.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bbl_Man_t * Bbl_ManReadBinaryBlif( char * pFileName )
{
    Bbl_Man_t * p;
    Bbl_Obj_t * pObj;
    char * pBuffer;
    int h;
    p = BBLIF_ALLOC( Bbl_Man_t, 1 );
    memset( p, 0, sizeof(Bbl_Man_t) );
    p->nFileSize = Bbl_ManFileSize( pFileName );
    p->pFileData = Bbl_ManFileRead( pFileName );
    // extract three managers
    pBuffer = p->pFileData;
    p->pName = Vec_StrRead( &pBuffer );
    p->pObjs = Vec_StrRead( &pBuffer );
    p->pFncs = Vec_StrRead( &pBuffer );
    assert( pBuffer - p->pFileData == p->nFileSize );
    // remember original IDs in the objects
    p->vObj2Id = Vec_IntAlloc( 1000 );
    Bbl_ManForEachObj_int( p->pObjs, pObj, h )
    {
        Vec_IntPush( p->vObj2Id, pObj->Id );
        pObj->Id = Vec_IntSize(p->vObj2Id) - 1; 
    }
    return p;
}

/**Fnction*************************************************************

  Synopsis    [Prints stats of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbl_ManPrintStats( Bbl_Man_t * p )
{
    Bbl_Obj_t * pObj;
    Bbl_Fnc_t * pFnc;
    int h, nFuncs = 0, nNodes = 0, nObjs = 0;
    Bbl_ManForEachObj_int( p->pObjs, pObj, h )
        nObjs++, nNodes += Bbl_ObjIsNode(pObj);
    Bbl_ManForEachFnc_int( p->pFncs, pFnc, h )
        nFuncs++;
    printf( "Total objects = %7d.  Total nodes = %7d. Unique functions = %7d.\n", nObjs, nNodes, nFuncs );
    printf( "Name manager = %5.2f MB\n", 1.0*Vec_StrSize(p->pName)/(1 << 20) );
    printf( "Objs manager = %5.2f MB\n", 1.0*Vec_StrSize(p->pObjs)/(1 << 20) );
    printf( "Fncs manager = %5.2f MB\n", 1.0*Vec_StrSize(p->pFncs)/(1 << 20) );
}

/**Fnction*************************************************************

  Synopsis    [Deletes the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbl_ManStop( Bbl_Man_t * p )
{
    if ( p->vId2Obj )    Vec_IntFree( p->vId2Obj );
    if ( p->vObj2Id )    Vec_IntFree( p->vObj2Id );
    if ( p->vFaninNums ) Vec_IntFree( p->vFaninNums );
    if ( p->pFileData )
    {
        BBLIF_FREE( p->pFileData );
        p->pName->pArray = NULL;
        p->pObjs->pArray = NULL;
        p->pFncs->pArray = NULL;
    }
    if ( p->pEnts )
    Vec_StrFree( p->pEnts );
    Vec_StrFree( p->pName );
    Vec_StrFree( p->pObjs );
    Vec_StrFree( p->pFncs );
    BBLIF_FREE( p );
}

/**Fnction*************************************************************

  Synopsis    [Creates manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bbl_Man_t * Bbl_ManStart( char * pName )
{
    Bbl_Man_t * p;
    int nLength;
    p = BBLIF_ALLOC( Bbl_Man_t, 1 );
    memset( p, 0, sizeof(Bbl_Man_t) );
    nLength = pName? 4 * ((strlen(pName) + 1) / 4 + 1) : 0;
    p->pName  = Vec_StrAlloc( nLength );
    p->pName->nSize = p->pName->nCap;
    if ( pName )
        strcpy( p->pName->pArray, pName );
    p->pObjs = Vec_StrAlloc( 1 << 16 );
    p->pFncs = Vec_StrAlloc( 1 << 16 );
    p->pEnts = Vec_StrAlloc( 1 << 16 ); p->pEnts->nSize = 1;
    p->vId2Obj = Vec_IntStart( 1 << 10 );
    p->vFaninNums = Vec_IntStart( 1 << 10 );
    return p;
}




/**Function*************************************************************

  Synopsis    [Performs selection sort on the array of cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbl_ManSortCubes( char ** pCubes, int nCubes, int nVars )
{
    char * pTemp;
    int i, j, best_i;
    for ( i = 0; i < nCubes-1; i++ )
    {
        best_i = i;
        for (j = i+1; j < nCubes; j++)
            if ( memcmp( pCubes[j], pCubes[best_i], (size_t)nVars ) < 0 )
                best_i = j;
        pTemp = pCubes[i]; pCubes[i] = pCubes[best_i]; pCubes[best_i] = pTemp;
    }
}

/**Function*************************************************************

  Synopsis    [Sorts the cubes in the SOP to uniqify them to some extent.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Bbl_ManSortSop( char * pSop, int nVars )
{
    char ** pCubes, * pSopNew;
    int c, Length, nCubes;
    Length = strlen(pSop);
    assert( Length % (nVars + 3) == 0 );
    nCubes = Length / (nVars + 3);
    if ( nCubes < 2 )
    {
        pSopNew = BBLIF_ALLOC( char, Length + 1 );
        memcpy( pSopNew, pSop, (size_t)(Length + 1) );
        return pSopNew;
    }
    pCubes = BBLIF_ALLOC( char *, nCubes );
    for ( c = 0; c < nCubes; c++ )
        pCubes[c] = pSop + c * (nVars + 3);
    if ( nCubes < 300 )
        Bbl_ManSortCubes( pCubes, nCubes, nVars );
    pSopNew = BBLIF_ALLOC( char, Length + 1 );
    for ( c = 0; c < nCubes; c++ )
        memcpy( pSopNew + c * (nVars + 3), pCubes[c], (size_t)(nVars + 3) );
    BBLIF_FREE( pCubes );
    pSopNew[nCubes * (nVars + 3)] = 0;
    return pSopNew;
}

/**Fnction*************************************************************

  Synopsis    [Saves one entry.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bbl_ManCreateEntry( Bbl_Man_t * p, int iFunc, int iNext )
{
    Bbl_Ent_t * pEnt;
    pEnt = (Bbl_Ent_t *)Vec_StrFetch( p->pEnts, 2 * sizeof(int) );
    pEnt->iFunc = iFunc;
    pEnt->iNext = iNext;
    return (char *)pEnt - p->pEnts->pArray;
}

/**Function*************************************************************

  Synopsis    [Sorts the cubes in the SOP to uniqify them to some extent.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bbl_ManSopCheckUnique( Bbl_Man_t * p, char * pSop, int nVars, int nCubes, int iFunc )
{
    Bbl_Fnc_t * pFnc;
    Bbl_Ent_t * pEnt;
    int h, Length = strlen(pSop) + 1;
    int nWords = (Length / 4 + (Length % 4 > 0));
    if ( nVars > 16 )   nVars = 16;
    if ( nCubes > 16 )  nCubes = 16;
//    if ( nVars == 16 && nCubes == 16 )
//        return iFunc;
    for ( h = p->SopMap[nVars][nCubes]; h; h = pEnt->iNext )
    {
        pEnt = Bbl_VecEnt( p->pEnts, h );
        pFnc = Bbl_VecFnc( p->pFncs, pEnt->iFunc );
        assert( nVars == 16 || nCubes == 16 || pFnc->nWords == nWords );
        if ( pFnc->nWords == nWords && memcmp( pFnc->pWords, pSop, (size_t)Length ) == 0 )
            return pEnt->iFunc;
    }
    p->SopMap[nVars][nCubes] = Bbl_ManCreateEntry( p, iFunc, p->SopMap[nVars][nCubes] );
    return iFunc;
}

/**Fnction*************************************************************

  Synopsis    [Saves one SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bbl_ManSaveSop( Bbl_Man_t * p, char * pSop, int nVars )
{
    Bbl_Fnc_t * pFnc;
    char * pSopNew;
    int iFunc, Length = strlen(pSop) + 1;
    int nWords = Length / 4 + (Length % 4 > 0);
    // reorder cubes to semi-canicize SOPs
    pSopNew = Bbl_ManSortSop( pSop, nVars );
    // get the candidate location
    iFunc = Bbl_ManSopCheckUnique( p, pSopNew, nVars, Length / (nVars + 3), Vec_StrSize(p->pFncs) );
//    iFunc = Vec_StrSize(p->pFncs);
    if ( iFunc == Vec_StrSize(p->pFncs) )
    { // store this SOP
        pFnc = (Bbl_Fnc_t *)Vec_StrFetch( p->pFncs, sizeof(Bbl_Fnc_t) + nWords * sizeof(int) );
        pFnc->pWords[nWords-1] = 0;
        pFnc->nWords = nWords;
        strcpy( (char *)pFnc->pWords, pSopNew );
        assert( iFunc == (char *)pFnc - p->pFncs->pArray );
    }
    BBLIF_FREE( pSopNew );
    return iFunc;
}

/**Fnction*************************************************************

  Synopsis    [Adds one object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbl_ManCreateObject( Bbl_Man_t * p, Bbl_Type_t Type, int ObjId, int nFanins, char * pSop )
{
    Bbl_Obj_t * pObj;
    if ( Type == BBL_OBJ_CI && nFanins != 0 )
    {
        printf( "Attempting to create a combinational input with %d fanins (should be 0).\n", nFanins );
        return;
    }
    if ( Type == BBL_OBJ_CO && nFanins != 1 )
    {
        printf( "Attempting to create a combinational output with %d fanins (should be 1).\n", nFanins );
        return;
    }
    pObj = (Bbl_Obj_t *)Vec_StrFetch( p->pObjs, sizeof(Bbl_Obj_t) + nFanins * sizeof(int) );
    memset( pObj, 0, sizeof(Bbl_Obj_t) );
    Vec_IntSetEntry( p->vId2Obj, ObjId, (char *)pObj - p->pObjs->pArray );
    Vec_IntSetEntry( p->vFaninNums, ObjId, 0 );
    pObj->fCi = (Type == BBL_OBJ_CI);
    pObj->fCo = (Type == BBL_OBJ_CO);
    pObj->Id  = ObjId;
    pObj->Fnc = pSop? Bbl_ManSaveSop(p, pSop, nFanins) : -1;
    pObj->nFanins = nFanins;
}

/**Fnction*************************************************************

  Synopsis    [Creates fanin/fanout relationship between two objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbl_ManAddFanin( Bbl_Man_t * p, int ObjId, int FaninId )
{
    Bbl_Obj_t * pObj, * pFanin;
    int iFanin;
    pObj   = Bbl_ManObj( p, ObjId );
    if ( Bbl_ObjIsCi(pObj) )
    {
        printf( "Bbl_ManAddFanin(): Cannot add fanin of the combinational input (Id = %d).\n", ObjId );
        return;
    }
    pFanin = Bbl_ManObj( p, FaninId );
    if ( Bbl_ObjIsCo(pFanin) )
    {
        printf( "Bbl_ManAddFanin(): Cannot add fanout of the combinational output (Id = %d).\n", FaninId );
        return;
    }
    iFanin = Vec_IntEntry( p->vFaninNums, ObjId );
    if ( iFanin >= (int)pObj->nFanins )
    {
        printf( "Bbl_ManAddFanin(): Trying to add more fanins to object (Id = %d) than declared (%d).\n", ObjId, pObj->nFanins );
        return;
    }
    assert( iFanin < (int)pObj->nFanins );
    Vec_IntWriteEntry( p->vFaninNums, ObjId, iFanin+1 );
    pObj->pFanins[iFanin] = (char *)pObj - (char *)pFanin;
}


/**Fnction*************************************************************

  Synopsis    [Returns 1 if the manager was created correctly.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bbl_ManCheck( Bbl_Man_t * p )
{
    Bbl_Obj_t * pObj;
    int h, RetValue = 1;
    Bbl_ManForEachObj_int( p->pObjs, pObj, h )
    {
        if ( Bbl_ObjIsNode(pObj) && pObj->Fnc == -1 )
            RetValue = 0, printf( "Bbl_ManCheck(): Node %d does not have function specified.\n", pObj->Id );
        if ( Bbl_ObjIsCi(pObj) && pObj->Fnc != -1 )
            RetValue = 0, printf( "Bbl_ManCheck(): CI with %d has function specified.\n", pObj->Id );
        if ( Bbl_ObjIsCo(pObj) && pObj->Fnc != -1 )
            RetValue = 0, printf( "Bbl_ManCheck(): CO with %d has function specified.\n", pObj->Id );
        if ( Vec_IntEntry(p->vFaninNums, pObj->Id) != (int)pObj->nFanins )
            RetValue = 0, printf( "Bbl_ManCheck(): Object %d has less fanins (%d) than declared (%d).\n", 
                pObj->Id, Vec_IntEntry(p->vFaninNums, pObj->Id), pObj->nFanins );
    }
    return RetValue;
}


/**Fnction*************************************************************

  Synopsis    [Misc APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int         Bbl_ObjIsInput( Bbl_Obj_t * p )                      { return Bbl_ObjIsCi(p);                      }
int         Bbl_ObjIsOutput( Bbl_Obj_t * p )                     { return Bbl_ObjIsCo(p);                      }
int         Bbl_ObjIsLut( Bbl_Obj_t * p )                        { return Bbl_ObjIsNode(p);                    }
int         Bbl_ObjId( Bbl_Obj_t * p )                           { return p->Id;                               }
int         Bbl_ObjIdOriginal( Bbl_Man_t * pMan, Bbl_Obj_t * p ) { assert(0); return Vec_IntEntry(pMan->vObj2Id, p->Id);  }
int         Bbl_ObjFaninNumber( Bbl_Obj_t * p )                  { return Bbl_ObjFaninNum(p);                  }
char *      Bbl_ObjSop( Bbl_Man_t * pMan, Bbl_Obj_t * p )        { return Bbl_ManSop(pMan, p->Fnc);            }
int         Bbl_ObjIsMarked( Bbl_Obj_t * p )                     { return p->fMark;                            }
void        Bbl_ObjMark( Bbl_Obj_t * p )                         { p->fMark = 1;                               }
int         Bbl_ObjFncHandle( Bbl_Obj_t * p )                    { return p->Fnc;                              }

/**Fnction*************************************************************

  Synopsis    [Returns the name of the design.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Bbl_ManName( Bbl_Man_t * p )
{
    return p->pName->pArray;
}

/**Fnction*************************************************************

  Synopsis    [Returns the maximum handle of the SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bbl_ManFncSize( Bbl_Man_t * p )
{
    return p->pFncs->nSize;
}

/**Fnction*************************************************************

  Synopsis    [Returns the first object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bbl_Obj_t * Bbl_ManObjFirst( Bbl_Man_t * p )
{
    return Bbl_VecObj( p->pObjs, 0 );
}

/**Fnction*************************************************************

  Synopsis    [Returns the next object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bbl_Obj_t * Bbl_ManObjNext( Bbl_Man_t * p, Bbl_Obj_t * pObj )
{
    char * pNext = (char *)pObj + Bbl_ObjSize(pObj);
    char * pEdge = p->pObjs->pArray + p->pObjs->nSize;
    return (Bbl_Obj_t *)(pNext < pEdge ? pNext : NULL);
}

/**Fnction*************************************************************

  Synopsis    [Returns the first fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bbl_Obj_t * Bbl_ObjFaninFirst( Bbl_Obj_t * p )
{
    return Bbl_ObjFaninNum(p) ? Bbl_ObjFanin( p, 0 ) : NULL;
}

/**Fnction*************************************************************

  Synopsis    [Returns the next fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bbl_Obj_t * Bbl_ObjFaninNext( Bbl_Obj_t * p, Bbl_Obj_t * pPrev )
{
    Bbl_Obj_t * pFanin;
    int i;
    Bbl_ObjForEachFanin_int( p, pFanin, i )
        if ( pFanin == pPrev )
            break;
    return i < Bbl_ObjFaninNum(p) - 1 ? Bbl_ObjFanin( p, i+1 ) : NULL;
}

/**Fnction*************************************************************

  Synopsis    [Drives text BLIF file for debugging.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbl_ManDumpBlif( Bbl_Man_t * p, char * pFileName )
{
    FILE * pFile;
    Bbl_Obj_t * pObj, * pFanin;
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "# Test file written by Bbl_ManDumpBlif() in ABC.\n" );
    fprintf( pFile, ".model %s\n", Bbl_ManName(p) );
    // write objects
    Bbl_ManForEachObj( p, pObj )
    {
        if ( Bbl_ObjIsInput(pObj) )
            fprintf( pFile, ".inputs %d\n", Bbl_ObjId(pObj) );
        else if ( Bbl_ObjIsOutput(pObj) )
            fprintf( pFile, ".outputs %d\n", Bbl_ObjId(pObj) );
        else if ( Bbl_ObjIsLut(pObj) )
        {
            fprintf( pFile, ".names" );
            Bbl_ObjForEachFanin( pObj, pFanin )
                fprintf( pFile, " %d", Bbl_ObjId(pFanin) );
            fprintf( pFile, " %d\n", Bbl_ObjId(pObj) );
            fprintf( pFile, "%s", Bbl_ObjSop(p, pObj) );
        }
        else assert( 0 );
    }
    // write output drivers
    Bbl_ManForEachObj( p, pObj )
    {
        if ( !Bbl_ObjIsOutput(pObj) )
            continue;
        fprintf( pFile, ".names" );
        Bbl_ObjForEachFanin( pObj, pFanin )
            fprintf( pFile, " %d", Bbl_ObjId(pFanin) );
        fprintf( pFile, " %d\n", Bbl_ObjId(pObj) );
        fprintf( pFile, "1 1\n" );
    }
    fprintf( pFile, ".end\n" );
    fclose( pFile );
}

/**Fnction*************************************************************

  Synopsis    [Converting truth table into an SOP.]

  Description [The truth table is given as a bit-string pTruth
  composed of 2^nVars bits. The result is an SOP derived by
  collecting minterms appearing in the truth table. The SOP is 
  represented as a C-string, as documented in file "bblif.h".
  It is recommended to limit the use of this procedure to Boolean 
  functions up to 6 inputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Bbl_ManTruthToSop( unsigned * pTruth, int nVars )
{
    char * pResult, * pTemp;
    int nMints, nOnes, b, v;
    assert( nVars >= 0 && nVars <= 16 );
    nMints = (1 << nVars);
    // count the number of ones
    nOnes = 0;
    for ( b = 0; b < nMints; b++ )
        nOnes += ((pTruth[b>>5] >> (b&31)) & 1);
    // handle constants
    if ( nOnes == 0 || nOnes == nMints )
    {
        pResult = pTemp = BBLIF_ALLOC( char, nVars + 4 );        
        for ( v = 0; v < nVars; v++ )
            *pTemp++ = '-';
        *pTemp++ = ' ';
        *pTemp++ = nOnes? '1' : '0';
        *pTemp++ = '\n';
        *pTemp++ = 0;
        assert( pTemp - pResult == nVars + 4 );
        return pResult;
    }
    pResult = pTemp = BBLIF_ALLOC( char, nOnes * (nVars + 3) + 1 );        
    for ( b = 0; b < nMints; b++ )
    {
        if ( ((pTruth[b>>5] >> (b&31)) & 1) == 0 )
            continue;
        for ( v = 0; v < nVars; v++ )
            *pTemp++ = ((b >> v) & 1)? '1' : '0';
        *pTemp++ = ' ';
        *pTemp++ = '1';
        *pTemp++ = '\n';
    }
    *pTemp++ = 0;
    assert( pTemp - pResult == nOnes * (nVars + 3) + 1 );
    return pResult;
}

/**Function*************************************************************

  Synopsis    [Allocates the array of truth tables for the given number of vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Bbl_ManSopToTruthElem( int nVars, unsigned ** pVars )
{
    unsigned Masks[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    int i, k, nWords;
    nWords = (nVars <= 5 ? 1 : (1 << (nVars - 5)));
    for ( i = 0; i < nVars; i++ )
    {
        if ( i < 5 )
        {
            for ( k = 0; k < nWords; k++ )
                pVars[i][k] = Masks[i];
        }
        else
        {
            for ( k = 0; k < nWords; k++ )
                if ( k & (1 << (i-5)) )
                    pVars[i][k] = ~(unsigned)0;
                else
                    pVars[i][k] = 0;
        }
    }
}

/**Fnction*************************************************************

  Synopsis    [Converting SOP into a truth table.]

  Description [The SOP is represented as a C-string, as documented in 
  file "bblif.h". The truth table is returned as a bit-string composed 
  of 2^nVars bits. For functions of less than 6 variables, the full
  machine word is returned. (The truth table looks as if the function
  had 5 variables.) The use of this procedure should be limited to 
  Boolean functions with no more than 16 inputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Bbl_ManSopToTruth( char * pSop, int nVars )
{
    unsigned * pTruth, * pCube, * pVars[16];
    int nWords = nVars <= 5 ? 1 : (1 << (nVars - 5));
    int v, c, w, nCubes, fCompl = 0;
    if ( pSop == NULL )
        return NULL;
    if ( strlen(pSop) % (nVars + 3) != 0 )
    {
        printf( "Bbl_ManSopToTruth(): SOP is represented incorrectly.\n" );
        return NULL;
    }
    // create storage for TTs of the result, elementary variables and the temp cube
    pTruth   = BBLIF_ALLOC( unsigned, nWords );   
    pVars[0] = BBLIF_ALLOC( unsigned, nWords * (nVars+1) );   
    for ( v = 1; v < nVars; v++ )
        pVars[v] = pVars[v-1] + nWords;
    pCube = pVars[v-1] + nWords;
    Bbl_ManSopToTruthElem( nVars, pVars );
    // iterate through the cubes
    memset( pTruth, 0, sizeof(unsigned) * nWords );
    nCubes =  strlen(pSop) / (nVars + 3);
    for ( c = 0; c < nCubes; c++ )
    {
        fCompl = (pSop[nVars+1] == '0');
        memset( pCube, 0xff, sizeof(unsigned) * nWords );
        // iterate through the literals of the cube
        for ( v = 0; v < nVars; v++ )
            if ( pSop[v] == '1' )
                for ( w = 0; w < nWords; w++ )
                    pCube[w] &= pVars[v][w];
            else if ( pSop[v] == '0' )
                for ( w = 0; w < nWords; w++ )
                    pCube[w] &= ~pVars[v][w];
        // add cube to storage
        for ( w = 0; w < nWords; w++ )
            pTruth[w] |= pCube[w];
        // go to the next cube
        pSop += (nVars + 3);
    }
    BBLIF_FREE( pVars[0] );
    if ( fCompl )
        for ( w = 0; w < nWords; w++ )
            pTruth[w] = ~pTruth[w];
    return pTruth;
}


/**Fnction*************************************************************

  Synopsis    [Checks the truth table computation.]

  Description [We construct the logic network for the half-adder represnted
  using the BLIF file below]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbl_ManTestTruth( char * pSop, int nVars )
{
    unsigned * pTruth;
    char * pSopNew;
    pTruth = Bbl_ManSopToTruth( pSop, nVars );
    pSopNew = Bbl_ManTruthToSop( pTruth, nVars );
    printf( "Old SOP:\n%s\n", pSop );
    printf( "New SOP:\n%s\n", pSopNew );
    BBLIF_FREE( pSopNew );
    BBLIF_FREE( pTruth );
}

/**Fnction*************************************************************

  Synopsis    [This demo shows using the internal to construct a half-adder.]

  Description [We construct the logic network for the half-adder represnted
  using the BLIF file below]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbl_ManSimpleDemo()
{
/*
    # There are contents of a BLIF file representing a half-adder:
    .model hadder
    .inputs a     // ID = 1
    .inputs b     // ID = 2
    .inputs cin   // ID = 3
    .outputs s    // ID = 4
    .outputs cout // ID = 5
    .names a b cin s_driver   // ID = 6
    100 1
    010 1
    001 1
    111 1
    .names a b cin cout_driver // ID = 7
    -11 1
    1-1 1
    11- 1
    .names s_driver s
    1 1
    .names cout_driver cout
    1 1
    .end
*/
    Bbl_Man_t * p;
    // start the data manager
    p = Bbl_ManStart( "hadder" );
    // create CIs
    Bbl_ManCreateObject( p, BBL_OBJ_CI, 1, 0, NULL ); // a
    Bbl_ManCreateObject( p, BBL_OBJ_CI, 2, 0, NULL ); // b
    Bbl_ManCreateObject( p, BBL_OBJ_CI, 3, 0, NULL ); // cin
    // create COs
    Bbl_ManCreateObject( p, BBL_OBJ_CO, 4, 1, NULL ); // s
    Bbl_ManCreateObject( p, BBL_OBJ_CO, 5, 1, NULL ); // cout
    // create internal nodes
    Bbl_ManCreateObject( p, BBL_OBJ_NODE, 6, 3, "100 1\n010 1\n001 1\n111 1\n" ); // s_driver
    Bbl_ManCreateObject( p, BBL_OBJ_NODE, 7, 3, "-11 1\n1-1 1\n11- 1\n" );        // cout_driver
    // add fanins of node 6
    Bbl_ManAddFanin( p, 6, 1 ); // s_driver <- a
    Bbl_ManAddFanin( p, 6, 2 ); // s_driver <- b
    Bbl_ManAddFanin( p, 6, 3 ); // s_driver <- cin
    // add fanins of node 7
    Bbl_ManAddFanin( p, 7, 1 ); // cout_driver <- a
    Bbl_ManAddFanin( p, 7, 2 ); // cout_driver <- b
    Bbl_ManAddFanin( p, 7, 3 ); // cout_driver <- cin
    // add fanins of COs
    Bbl_ManAddFanin( p, 4, 6 ); // s <- s_driver
    Bbl_ManAddFanin( p, 5, 7 ); // cout <- cout_driver
    // sanity check
    Bbl_ManCheck( p );
    // write BLIF file as a sanity check
    Bbl_ManDumpBlif( p, "hadder.blif" );
    // write binary BLIF file
    Bbl_ManDumpBinaryBlif( p, "hadder.bblif" );
    // remove the manager
    Bbl_ManStop( p );


//    Bbl_ManTestTruth( "100 1\n010 1\n001 1\n111 1\n", 3 );
//    Bbl_ManTestTruth( "-11 0\n1-1 0\n11- 0\n", 3 );
//    Bbl_ManTestTruth( "--- 1\n", 3 );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

