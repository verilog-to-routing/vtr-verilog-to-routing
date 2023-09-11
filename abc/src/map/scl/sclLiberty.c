/**CFile****************************************************************

  FileName    [sclLiberty.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Standard-cell library representation.]

  Synopsis    [Liberty parser.]

  Author      [Alan Mishchenko, Niklas Een]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 24, 2012.]

  Revision    [$Id: sclLiberty.c,v 1.0 2012/08/24 00:00:00 alanmi Exp $]

***********************************************************************/
#include <string.h>
#ifdef _WIN32
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#else 
#include <fnmatch.h>
#endif

#include "sclLib.h"
#include "misc/st/st.h"
#include "map/mio/mio.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// #define ABC_MAX_LIB_STR_LEN 5000

// entry types
typedef enum { 
    SCL_LIBERTY_NONE = 0,        // 0:  unknown
    SCL_LIBERTY_PROC,            // 1:  procedure :  key(head){body}
    SCL_LIBERTY_EQUA,            // 2:  equation  :  key:head;
    SCL_LIBERTY_LIST             // 3:  list      :  key(head) 
} Scl_LibertyType_t;

typedef struct Scl_Pair_t_ Scl_Pair_t;
struct Scl_Pair_t_
{
    int             Beg;          // item beginning
    int             End;          // item end
};

typedef struct Scl_Item_t_ Scl_Item_t;
struct Scl_Item_t_
{
    int             Type;         // Scl_LibertyType_t
    int             iLine;        // file line where the item's spec begins
    Scl_Pair_t      Key;          // key part
    Scl_Pair_t      Head;         // head part 
    Scl_Pair_t      Body;         // body part
    int             Next;         // next item in the list 
    int             Child;        // first child item 
};

typedef struct Scl_Tree_t_ Scl_Tree_t;
struct Scl_Tree_t_
{
    char *          pFileName;    // input Liberty file name
    char *          pContents;    // file contents
    long            nContents;    // file size
    int             nLines;       // line counter
    int             nItems;       // number of items
    int             nItermAlloc;  // number of items allocated
    Scl_Item_t *    pItems;       // the items
    char *          pError;       // the error string
    abctime         clkStart;     // beginning time
    Vec_Str_t *     vBuffer;      // temp string buffer
};


static inline int          Scl_LibertyGlobMatch(const char * pattern, const char * string) {
    #ifdef _WIN32
    return PathMatchSpec(string, pattern);
    #else
    return fnmatch(pattern, string, 0) == 0;
    #endif
}
static inline Scl_Item_t *  Scl_LibertyRoot( Scl_Tree_t * p )                                      { return p->pItems;                                                 }
static inline Scl_Item_t *  Scl_LibertyItem( Scl_Tree_t * p, int v )                               { assert( v < p->nItems ); return v < 0 ? NULL : p->pItems + v;     }
static inline int           Scl_LibertyCompare( Scl_Tree_t * p, Scl_Pair_t Pair, char * pStr )     { return strncmp( p->pContents+Pair.Beg, pStr, Pair.End-Pair.Beg ) || ((int)strlen(pStr) != Pair.End-Pair.Beg); }
static inline void          Scl_PrintWord( FILE * pFile, Scl_Tree_t * p, Scl_Pair_t Pair )         { char * pBeg = p->pContents+Pair.Beg, * pEnd = p->pContents+Pair.End; while ( pBeg < pEnd ) fputc( *pBeg++, pFile ); }
static inline void          Scl_PrintSpace( FILE * pFile, int nOffset )                            { int i; for ( i = 0; i < nOffset; i++ ) fputc(' ', pFile);         }
static inline int           Scl_LibertyItemId( Scl_Tree_t * p, Scl_Item_t * pItem )                { return pItem - p->pItems;                                         }

#define Scl_ItemForEachChild( p, pItem, pChild ) \
    for ( pChild = Scl_LibertyItem(p, pItem->Child); pChild; pChild = Scl_LibertyItem(p, pChild->Next) )
#define Scl_ItemForEachChildName( p, pItem, pChild, pName ) \
    for ( pChild = Scl_LibertyItem(p, pItem->Child); pChild; pChild = Scl_LibertyItem(p, pChild->Next) ) if ( Scl_LibertyCompare(p, pChild->Key, pName) ) {} else

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prints parse tree in Liberty format.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Scl_LibertyParseDumpItem( FILE * pFile, Scl_Tree_t * p, Scl_Item_t * pItem, int nOffset )
{
    if ( pItem->Type == SCL_LIBERTY_PROC )
    {
        Scl_PrintSpace( pFile, nOffset );
        Scl_PrintWord( pFile, p, pItem->Key );
        fprintf( pFile, "(" );
        Scl_PrintWord( pFile, p, pItem->Head );
        fprintf( pFile, ") {\n" );
        if ( Scl_LibertyItem(p, pItem->Child) )
            Scl_LibertyParseDumpItem( pFile, p, Scl_LibertyItem(p, pItem->Child), nOffset + 2 );
        Scl_PrintSpace( pFile, nOffset );
        fprintf( pFile, "}\n" );
    }
    else if ( pItem->Type == SCL_LIBERTY_EQUA )
    {
        Scl_PrintSpace( pFile, nOffset );
        Scl_PrintWord( pFile, p, pItem->Key );
        fprintf( pFile, " : " );
        Scl_PrintWord( pFile, p, pItem->Head );
        fprintf( pFile, ";\n" );
    }
    else if ( pItem->Type == SCL_LIBERTY_LIST )
    {
        Scl_PrintSpace( pFile, nOffset );
        Scl_PrintWord( pFile, p, pItem->Key );
        fprintf( pFile, "(" );
        Scl_PrintWord( pFile, p, pItem->Head );
        fprintf( pFile, ");\n" );
    }
    else assert( 0 );
    if ( Scl_LibertyItem(p, pItem->Next) )
        Scl_LibertyParseDumpItem( pFile, p, Scl_LibertyItem(p, pItem->Next), nOffset );
}
int Scl_LibertyParseDump( Scl_Tree_t * p, char * pFileName )
{
    FILE * pFile;
    if ( pFileName == NULL )
        pFile = stdout;
    else
    {
        pFile = fopen( pFileName, "w" );
        if ( pFile == NULL )
        {
            printf( "Scl_LibertyParseDump(): The output file is unavailable (absent or open).\n" );
            return 0;
        }
    }
    Scl_LibertyParseDumpItem( pFile, p, Scl_LibertyRoot(p), 0 );
    if ( pFile != stdout )
        fclose( pFile );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Gets the name to write.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_LibertyCountItems( char * pBeg, char * pEnd )
{
    int Counter = 0;
    for ( ; pBeg < pEnd; pBeg++ )
        Counter += (*pBeg == '(' || *pBeg == ':');
    return Counter;
}
// removes C-style comments
/*
void Scl_LibertyWipeOutComments( char * pBeg, char * pEnd )
{
    char * pCur, * pStart;
    for ( pCur = pBeg; pCur < pEnd; pCur++ )
    if ( pCur[0] == '/' && pCur[1] == '*' )
        for ( pStart = pCur; pCur < pEnd; pCur++ )
        if ( pCur[0] == '*' && pCur[1] == '/' )
        {
            for ( ; pStart < pCur + 2; pStart++ )
            if ( *pStart != '\n' ) *pStart = ' ';
            break;
        }
}
*/
void Scl_LibertyWipeOutComments( char * pBeg, char * pEnd )
{
    char * pCur, * pStart;
    for ( pCur = pBeg; pCur < pEnd-1; pCur++ )
        if ( pCur[0] == '/' && pCur[1] == '*' )
        {
            for ( pStart = pCur; pCur < pEnd-1; pCur++ )
                if ( pCur[0] == '*' && pCur[1] == '/' )
                {
                    for ( ; pStart < pCur + 2; pStart++ )
                    if ( *pStart != '\n' ) *pStart = ' ';
                    break;
                }
        }
        else if ( pCur[0] == '/' && pCur[1] == '/' )
        {
            for ( pStart = pCur; pCur < pEnd; pCur++ )
                if ( pCur[0] == '\n' || pCur == pEnd-1 )
                {
                    for ( ; pStart < pCur; pStart++ ) *pStart = ' ';
                    break;
                }
        }
}
static inline int Scl_LibertyCharIsSpace( char c )
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\\';
}
static inline int Scl_LibertySkipSpaces( Scl_Tree_t * p, char ** ppPos, char * pEnd, int fStopAtNewLine )
{
    char * pPos = *ppPos;
    for ( ; pPos < pEnd; pPos++ )
    {
        if ( *pPos == '\n' )
        {
            p->nLines++;
            if ( fStopAtNewLine )
                break;
        }        
        if ( !Scl_LibertyCharIsSpace(*pPos) )
            break;
    }
    *ppPos = pPos;
    return pPos == pEnd;
}
// skips entry delimited by " :;(){}" and returns 1 if reached the end
static inline int Scl_LibertySkipEntry( char ** ppPos, char * pEnd )
{
    char * pPos = *ppPos;
    if ( *pPos == '\"' )
    {
        for ( pPos++; pPos < pEnd; pPos++ )
            if ( *pPos == '\"' )
            {
                pPos++;
                break;
            }
    }
    else
    {
        for ( ; pPos < pEnd; pPos++ )
            if ( *pPos == ' ' || *pPos == '\r' || *pPos == '\n' || *pPos == '\t' ||
                 *pPos == ':' || *pPos == ';'  || 
                 *pPos == '(' || *pPos == ')'  || 
                 *pPos == '{' || *pPos == '}' )
                break;
    }
    *ppPos = pPos;
    return pPos == pEnd;
}
// finds the matching closing symbol
static inline char * Scl_LibertyFindMatch( char * pPos, char * pEnd )
{
    int Counter = 0;
    assert( *pPos == '(' || *pPos == '{' );
    if ( *pPos == '(' )
    {
        for ( ; pPos < pEnd; pPos++ )
        {
            if ( *pPos == '(' )
                Counter++;
            if ( *pPos == ')' )
                Counter--;
            if ( Counter == 0 )
                break;
        }
    }
    else
    {
        for ( ; pPos < pEnd; pPos++ )
        {
            if ( *pPos == '{' )
                Counter++;
            if ( *pPos == '}' )
                Counter--;
            if ( Counter == 0 )
                break;
        }
    }
    assert( *pPos == ')' || *pPos == '}' );
    return pPos;
}
// trims spaces around the head
static inline Scl_Pair_t Scl_LibertyUpdateHead( Scl_Tree_t * p, Scl_Pair_t Head )
{
    Scl_Pair_t Res;
    char * pBeg = p->pContents + Head.Beg;
    char * pEnd = p->pContents + Head.End;
    char * pFirstNonSpace = NULL;
    char * pLastNonSpace = NULL;
    char * pChar;
    for ( pChar = pBeg; pChar < pEnd; pChar++ )
    {
        if ( *pChar == '\n' )
            p->nLines++;
        if ( Scl_LibertyCharIsSpace(*pChar) )
            continue;
        pLastNonSpace = pChar;
        if ( pFirstNonSpace == NULL )
            pFirstNonSpace = pChar;
    }
    if ( pFirstNonSpace == NULL || pLastNonSpace == NULL )
        return Head;
    assert( pFirstNonSpace && pLastNonSpace );
    Res.Beg = pFirstNonSpace - p->pContents;
    Res.End = pLastNonSpace  - p->pContents + 1;
    return Res;
}
// returns new item
static inline Scl_Item_t * Scl_LibertyNewItem( Scl_Tree_t * p, int Type )
{
    p->pItems[p->nItems].iLine = p->nLines;
    p->pItems[p->nItems].Type  = Type;
    p->pItems[p->nItems].Child = -1;
    p->pItems[p->nItems].Next  = -1;
    return p->pItems + p->nItems++;
}


/**Function*************************************************************

  Synopsis    [Gets the name to write.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Scl_LibertyReadString( Scl_Tree_t * p, Scl_Pair_t Pair )   
{ 
    // static char Buffer[ABC_MAX_LIB_STR_LEN]; 
    char * Buffer;
    if ( Pair.End - Pair.Beg + 2 > Vec_StrSize(p->vBuffer) )
        Vec_StrFill( p->vBuffer, Pair.End - Pair.Beg + 100, '\0' );
    Buffer = Vec_StrArray( p->vBuffer );
    strncpy( Buffer, p->pContents+Pair.Beg, Pair.End-Pair.Beg ); 
    if ( Pair.Beg < Pair.End && Buffer[0] == '\"' )
    {
        assert( Buffer[Pair.End-Pair.Beg-1] == '\"' );
        Buffer[Pair.End-Pair.Beg-1] = 0;
        return Buffer + 1;
    }
    Buffer[Pair.End-Pair.Beg] = 0;
    return Buffer;
}
int Scl_LibertyItemNum( Scl_Tree_t * p, Scl_Item_t * pRoot, char * pName )
{
    Scl_Item_t * pItem;
    int Counter = 0;
    Scl_ItemForEachChildName( p, pRoot, pItem, pName )
        Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns free item.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_LibertyBuildItem( Scl_Tree_t * p, char ** ppPos, char * pEnd )
{
    Scl_Item_t * pItem;
    Scl_Pair_t Key, Head, Body;
    char * pNext, * pStop;
    Key.End = 0;
    if ( Scl_LibertySkipSpaces( p, ppPos, pEnd, 0 ) )
        return -2;
    Key.Beg = *ppPos - p->pContents;
    if ( Scl_LibertySkipEntry( ppPos, pEnd ) )
        goto exit;
    Key.End = *ppPos - p->pContents;
    if ( Scl_LibertySkipSpaces( p, ppPos, pEnd, 0 ) )
        goto exit;
    pNext = *ppPos;
    if ( *pNext == ':' )
    {
        *ppPos = pNext + 1;
        if ( Scl_LibertySkipSpaces( p, ppPos, pEnd, 0 ) )
            goto exit;
        Head.Beg = *ppPos - p->pContents;
        if ( Scl_LibertySkipEntry( ppPos, pEnd ) )
            goto exit;
        Head.End = *ppPos - p->pContents;
        if ( Scl_LibertySkipSpaces( p, ppPos, pEnd, 1 ) )
            goto exit;
        pNext = *ppPos;
        while ( *pNext == '+' || *pNext == '-' || *pNext == '*' || *pNext == '/' )
        {
        (*ppPos) += 1;
            if ( Scl_LibertySkipSpaces( p, ppPos, pEnd, 0 ) )
                goto exit;
            if ( Scl_LibertySkipEntry( ppPos, pEnd ) )
                goto exit;
            Head.End = *ppPos - p->pContents;
            if ( Scl_LibertySkipSpaces( p, ppPos, pEnd, 1 ) )
                goto exit;
        pNext = *ppPos;
        }
        if ( *pNext != ';' && *pNext != '\n' )
            goto exit;
        *ppPos = pNext + 1;
        // end of equation
        pItem = Scl_LibertyNewItem( p, SCL_LIBERTY_EQUA );
        pItem->Key  = Key;
        pItem->Head = Scl_LibertyUpdateHead( p, Head );
        pItem->Next = Scl_LibertyBuildItem( p, ppPos, pEnd );
        if ( pItem->Next == -1 )
            goto exit;
        return Scl_LibertyItemId( p, pItem );
    }
    if ( *pNext == '(' )
    {
        pStop = Scl_LibertyFindMatch( pNext, pEnd );
        Head.Beg = pNext - p->pContents + 1;
        Head.End = pStop - p->pContents;
        *ppPos = pStop + 1;
        if ( Scl_LibertySkipSpaces( p, ppPos, pEnd, 0 ) )
        {
            // end of list
            pItem = Scl_LibertyNewItem( p, SCL_LIBERTY_LIST );
            pItem->Key  = Key;
            pItem->Head = Scl_LibertyUpdateHead( p, Head );
            return Scl_LibertyItemId( p, pItem );
        }
        pNext = *ppPos;
        if ( *pNext == '{' ) // beginning of body
        {
            pStop = Scl_LibertyFindMatch( pNext, pEnd );
            Body.Beg = pNext - p->pContents + 1;
            Body.End = pStop - p->pContents;
            // end of body
            pItem = Scl_LibertyNewItem( p, SCL_LIBERTY_PROC );
            pItem->Key  = Key;
            pItem->Head = Scl_LibertyUpdateHead( p, Head );
            pItem->Body = Body;
            *ppPos = pNext + 1;
            pItem->Child = Scl_LibertyBuildItem( p, ppPos, pStop );
            if ( pItem->Child == -1 )
                goto exit;
            *ppPos = pStop + 1;
            pItem->Next = Scl_LibertyBuildItem( p, ppPos, pEnd );
            if ( pItem->Next == -1 )
                goto exit;
            return Scl_LibertyItemId( p, pItem );
        }
        // end of list
        if ( *pNext == ';' )
            *ppPos = pNext + 1;
        pItem = Scl_LibertyNewItem( p, SCL_LIBERTY_LIST );
        pItem->Key  = Key;
        pItem->Head = Scl_LibertyUpdateHead( p, Head );
        pItem->Next = Scl_LibertyBuildItem( p, ppPos, pEnd );
        if ( pItem->Next == -1 )
            goto exit;
        return Scl_LibertyItemId( p, pItem );
    }
    if ( *pNext == ';' )
    {
        *ppPos = pNext + 1;
        return Scl_LibertyBuildItem(p, ppPos, pEnd);
    }
exit:
    if ( p->pError == NULL )
    {
        p->pError = ABC_ALLOC( char, 1000 );
        sprintf( p->pError, "File \"%s\". Line %6d. Failed to parse entry \"%s\".\n", 
            p->pFileName, p->nLines, Scl_LibertyReadString(p, Key) );
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [File management.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Scl_LibertyFixFileName( char * pFileName )
{
    char * pHead;
    for ( pHead = pFileName; *pHead; pHead++ )
        if ( *pHead == '>' )
            *pHead = '\\';
}
long Scl_LibertyFileSize( char * pFileName )
{
    FILE * pFile;
    long nFileSize;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Scl_LibertyFileSize(): The input file is unavailable (absent or open).\n" );
        return 0;
    }
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile ); 
    fclose( pFile );
    return nFileSize;
}
char * Scl_LibertyFileContents( char * pFileName, long nContents )
{
    FILE * pFile = fopen( pFileName, "rb" );
    char * pContents = ABC_ALLOC( char, nContents+1 );
    int RetValue = 0;
    RetValue = fread( pContents, nContents, 1, pFile );
    fclose( pFile );
    pContents[nContents] = 0;
    return pContents;
}
void Scl_LibertyStringDump( char * pFileName, Vec_Str_t * vStr )
{
    FILE * pFile = fopen( pFileName, "wb" );
    int RetValue = 0;
    if ( pFile == NULL )
    {
        printf( "Scl_LibertyStringDump(): The output file is unavailable.\n" );
        return;
    }
    RetValue = fwrite( Vec_StrArray(vStr), 1, Vec_StrSize(vStr), pFile );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Starts the parsing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Scl_Tree_t * Scl_LibertyStart( char * pFileName )
{
    Scl_Tree_t * p;
    long RetValue;
    // read the file into the buffer
    Scl_LibertyFixFileName( pFileName );
    RetValue = Scl_LibertyFileSize( pFileName );
    if ( RetValue == 0 )
        return NULL;
    // start the manager
    p = ABC_ALLOC( Scl_Tree_t, 1 );
    memset( p, 0, sizeof(Scl_Tree_t) );
    p->clkStart  = Abc_Clock();
    p->nContents = RetValue;
    p->pContents = Scl_LibertyFileContents( pFileName, p->nContents );
    // other 
    p->pFileName = Abc_UtilStrsav( pFileName );
    p->nItermAlloc = 10 + Scl_LibertyCountItems( p->pContents, p->pContents+p->nContents );
    p->pItems = ABC_CALLOC( Scl_Item_t, p->nItermAlloc );
    p->nItems = 0;
    p->nLines = 1;
    p->vBuffer = Vec_StrStart( 10 );
    return p;
}
void Scl_LibertyStop( Scl_Tree_t * p, int fVerbose )
{
    if ( fVerbose )
    {
        printf( "Memory = %7.2f MB. ", 1.0 * (p->nContents + p->nItermAlloc * sizeof(Scl_Item_t))/(1<<20) );
        ABC_PRT( "Time", Abc_Clock() - p->clkStart );
    }
    Vec_StrFree( p->vBuffer );
    ABC_FREE( p->pFileName );
    ABC_FREE( p->pContents );
    ABC_FREE( p->pItems );
    ABC_FREE( p->pError );
    ABC_FREE( p );
}
Scl_Tree_t * Scl_LibertyParse( char * pFileName, int fVerbose )
{
    Scl_Tree_t * p;
    char * pPos;
    if ( (p = Scl_LibertyStart(pFileName)) == NULL )
        return NULL;
    pPos = p->pContents;
    Scl_LibertyWipeOutComments( p->pContents, p->pContents+p->nContents );
    if ( (!Scl_LibertyBuildItem( p, &pPos, p->pContents + p->nContents )) == 0 )
    {
        if ( p->pError ) printf( "%s", p->pError );
        printf( "Parsing failed.  " );
        Abc_PrintTime( 1, "Parsing time", Abc_Clock() - p->clkStart );
    }
    else if ( fVerbose )
    {
        printf( "Parsing finished successfully.  " );
        Abc_PrintTime( 1, "Parsing time", Abc_Clock() - p->clkStart );
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Fetching attributes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Scl_LibertyReadCellIsFlop( Scl_Tree_t * p, Scl_Item_t * pCell )
{
    Scl_Item_t * pAttr;
    Scl_ItemForEachChild( p, pCell, pAttr )
        if ( !Scl_LibertyCompare(p, pAttr->Key, "ff") ||
             !Scl_LibertyCompare(p, pAttr->Key, "latch") )
            return 1;
    return 0;
}
int Scl_LibertyReadCellIsDontUse( Scl_Tree_t * p, Scl_Item_t * pCell, SC_DontUse dont_use )
{
    Scl_Item_t * pAttr;
    Scl_ItemForEachChild( p, pCell, pAttr )
    {
        if ( !Scl_LibertyCompare(p, pAttr->Key, "dont_use") )
            return 1;
        const char * cell_name = Scl_LibertyReadString(p, pCell->Head);
        for (int i = 0; i < dont_use.size; i++) {
            if (Scl_LibertyGlobMatch(dont_use.dont_use_list[i], cell_name)) {
                return 1;
            }
        }
    }
    return 0;
}
char * Scl_LibertyReadCellArea( Scl_Tree_t * p, Scl_Item_t * pCell )
{
    Scl_Item_t * pArea;
    Scl_ItemForEachChildName( p, pCell, pArea, "area" )
        return Scl_LibertyReadString(p, pArea->Head);
    return 0;
}
char * Scl_LibertyReadCellLeakage( Scl_Tree_t * p, Scl_Item_t * pCell )
{
    Scl_Item_t * pItem, * pChild;
    Scl_ItemForEachChildName( p, pCell, pItem, "cell_leakage_power" )
        return Scl_LibertyReadString(p, pItem->Head);
    // look for another type
    Scl_ItemForEachChildName( p, pCell, pItem, "leakage_power" )
    {
        Scl_ItemForEachChildName( p, pItem, pChild, "when" )
            break;
        if ( pChild && !Scl_LibertyCompare(p, pChild->Key, "when") )
            continue;
        Scl_ItemForEachChildName( p, pItem, pChild, "value" )
            return Scl_LibertyReadString(p, pChild->Head);
    }
    return 0;
}
char * Scl_LibertyReadPinFormula( Scl_Tree_t * p, Scl_Item_t * pPin )
{
    Scl_Item_t * pFunc;
    Scl_ItemForEachChildName( p, pPin, pFunc, "function" )
        return Scl_LibertyReadString(p, pFunc->Head);
    return NULL;
}
int Scl_LibertyReadCellIsThreeState( Scl_Tree_t * p, Scl_Item_t * pCell )
{
    Scl_Item_t * pPin, * pItem;
    Scl_ItemForEachChildName( p, pCell, pPin, "pin" )
        Scl_ItemForEachChildName( p, pPin, pItem, "three_state" )
            return 1;
    return 0;
}
int Scl_LibertyReadCellOutputNum( Scl_Tree_t * p, Scl_Item_t * pCell )
{
    Scl_Item_t * pPin;
    int Counter = 0;
    Scl_ItemForEachChildName( p, pCell, pPin, "pin" )
        if ( Scl_LibertyReadPinFormula(p, pPin) )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Parses the standard cell library in Liberty format.]

  Description [Writes the resulting file in Genlib format.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Scl_LibertyReadGenlibStr( Scl_Tree_t * p, int fVerbose, SC_DontUse dont_use )
{
    Vec_Str_t * vStr;
    Scl_Item_t * pCell, * pOutput, * pInput;
    char * pFormula;
    vStr = Vec_StrAlloc( 1000 );
    Vec_StrPrintStr( vStr, "GATE          _const0_  0.000000  z=CONST0;\n" );
    Vec_StrPrintStr( vStr, "GATE          _const1_  0.000000  z=CONST1;\n" );
    Scl_ItemForEachChildName( p, Scl_LibertyRoot(p), pCell, "cell" )
    {
        if ( Scl_LibertyReadCellIsFlop(p, pCell) )
        {
            if ( fVerbose )  printf( "Scl_LibertyReadGenlib() skipped sequential cell \"%s\".\n", Scl_LibertyReadString(p, pCell->Head) );
            continue;
        }
        if ( Scl_LibertyReadCellIsDontUse(p, pCell, dont_use) )
        {
            if ( fVerbose )  printf( "Scl_LibertyReadGenlib() skipped cell \"%s\" due to dont_use attribute.\n", Scl_LibertyReadString(p, pCell->Head) );
            continue;
        }
        if ( Scl_LibertyReadCellIsThreeState(p, pCell) )
        {
            if ( fVerbose )  printf( "Scl_LibertyReadGenlib() skipped three-state cell \"%s\".\n", Scl_LibertyReadString(p, pCell->Head) );
            continue;
        }
        if ( Scl_LibertyReadCellOutputNum(p, pCell) == 0 )
        {
            if ( fVerbose )  printf( "Scl_LibertyReadGenlib() skipped cell \"%s\" without logic function.\n", Scl_LibertyReadString(p, pCell->Head) );
            continue;
        }
        // iterate through output pins
        Scl_ItemForEachChildName( p, pCell, pOutput, "pin" )
        {
            if ( !(pFormula = Scl_LibertyReadPinFormula(p, pOutput)) ) 
                continue;
            if ( !strcmp(pFormula, "0") || !strcmp(pFormula, "1") )
            {
                if ( fVerbose ) printf( "Scl_LibertyReadGenlib() skipped cell \"%s\" with constant formula \"%s\".\n", Scl_LibertyReadString(p, pCell->Head), pFormula );
                break;
            }
            Vec_StrPrintStr( vStr, "GATE " );
            Vec_StrPrintStr( vStr, Scl_LibertyReadString(p, pCell->Head) );
            Vec_StrPrintStr( vStr, " " );
            Vec_StrPrintStr( vStr, Scl_LibertyReadCellArea(p, pCell) );
            Vec_StrPrintStr( vStr, " " );
            Vec_StrPrintStr( vStr, Scl_LibertyReadString(p, pOutput->Head) );
            Vec_StrPrintStr( vStr, "=" );
            Vec_StrPrintStr( vStr, pFormula );
            Vec_StrPrintStr( vStr, ";\n" );
            // iterate through input pins
            Scl_ItemForEachChildName( p, pCell, pInput, "pin" )
            {
                if ( Scl_LibertyReadPinFormula(p, pInput) == NULL )
                    continue;
                Vec_StrPrintStr( vStr, "  PIN " );
                Vec_StrPrintStr( vStr, Scl_LibertyReadString(p, pInput->Head) );
                Vec_StrPrintStr( vStr, " UNKNOWN  1  999  1.00  0.00  1.00  0.00\n" );
            }
        }
    }
    Vec_StrPrintStr( vStr, "\n.end\n" );
    Vec_StrPush( vStr, '\0' );
//    printf( "%s", Vec_StrArray(vStr) );
    return vStr;
}

/**Function*************************************************************

  Synopsis    [Enabling debug output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
//#define SCL_DEBUG
#ifdef SCL_DEBUG
static inline void Vec_StrPutI_( Vec_Str_t * vOut, int Val )     {  printf( "%d ",  Val );        Vec_StrPutI( vOut, Val );  }
static inline void Vec_StrPutW_( Vec_Str_t * vOut, word Val )    {  printf( "%lu ", (long)Val );  Vec_StrPutW( vOut, Val );  }
static inline void Vec_StrPutF_( Vec_Str_t * vOut, float Val )   {  printf( "%f ",  Val );        Vec_StrPutF( vOut, Val );  }
static inline void Vec_StrPutS_( Vec_Str_t * vOut, char * Val )  {  printf( "%s ",  Val );        Vec_StrPutS( vOut, Val );  }
static inline void Vec_StrPut_( Vec_Str_t * vOut )               {  printf( "\n" ); }
#else
static inline void Vec_StrPutI_( Vec_Str_t * vOut, int Val )     { Vec_StrPutI( vOut, Val );  }
static inline void Vec_StrPutW_( Vec_Str_t * vOut, word Val )    { Vec_StrPutW( vOut, Val );  }
static inline void Vec_StrPutF_( Vec_Str_t * vOut, float Val )   { Vec_StrPutF( vOut, Val );  }
static inline void Vec_StrPutS_( Vec_Str_t * vOut, char * Val )  { Vec_StrPutS( vOut, Val );  }
static inline void Vec_StrPut_( Vec_Str_t * vOut )               { }
#endif

/**Function*************************************************************

  Synopsis    [Parsing Liberty into internal data representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Scl_LibertyReadDefaultWireLoad( Scl_Tree_t * p )
{
    Scl_Item_t * pItem;
    Scl_ItemForEachChildName( p, Scl_LibertyRoot(p), pItem, "default_wire_load" )
        return Scl_LibertyReadString(p, pItem->Head);
    return "";
}
char * Scl_LibertyReadDefaultWireLoadSel( Scl_Tree_t * p )
{
    Scl_Item_t * pItem;
    Scl_ItemForEachChildName( p, Scl_LibertyRoot(p), pItem, "default_wire_load_selection" )
        return Scl_LibertyReadString(p, pItem->Head);
    return "";
}
float Scl_LibertyReadDefaultMaxTrans( Scl_Tree_t * p )
{
    Scl_Item_t * pItem;
    Scl_ItemForEachChildName( p, Scl_LibertyRoot(p), pItem, "default_max_transition" )
        return atof(Scl_LibertyReadString(p, pItem->Head));
    return 0;
}
int Scl_LibertyReadTimeUnit( Scl_Tree_t * p )
{
    Scl_Item_t * pItem;
    Scl_ItemForEachChildName( p, Scl_LibertyRoot(p), pItem, "time_unit" )
    {
        char * pUnit = Scl_LibertyReadString(p, pItem->Head);
        // 9=1ns, 10=100ps, 11=10ps, 12=1ps
        if ( !strcmp(pUnit, "1ns") )
            return 9;
        if ( !strcmp(pUnit, "100ps") )
            return 10;
        if ( !strcmp(pUnit, "10ps") )
            return 11;
        if ( !strcmp(pUnit, "1ps") )
            return 12;
        break;
    }
    printf( "Liberty parser cannot read \"time_unit\".  Assuming   time_unit : \"1ns\".\n" );
    return 9;
}
void Scl_LibertyReadLoadUnit( Scl_Tree_t * p, Vec_Str_t * vOut )
{
    Scl_Item_t * pItem;
    Scl_ItemForEachChildName( p, Scl_LibertyRoot(p), pItem, "capacitive_load_unit" )
    {
        // expecting (1.00,ff) or (1, pf) ... 12 or 15 for 'pf' or 'ff'
        char * pHead   = Scl_LibertyReadString(p, pItem->Head);
        float First    = atof(strtok(pHead, " \t\n\r\\\","));
        char * pSecond = strtok(NULL, " \t\n\r\\\",");
        Vec_StrPutF_( vOut, First );
        if ( pSecond && !strcmp(pSecond, "pf") )
            Vec_StrPutI_( vOut, 12 );
        else if ( pSecond && !strcmp(pSecond, "ff") )
            Vec_StrPutI_( vOut, 15 );
        else break;
        return;
    }
    printf( "Liberty parser cannot read \"capacitive_load_unit\". Assuming   capacitive_load_unit(1, pf).\n" );
    Vec_StrPutF_( vOut, 1.0 );
    Vec_StrPutI_( vOut, 12 );
}
void Scl_LibertyReadWireLoad( Scl_Tree_t * p, Vec_Str_t * vOut )
{
    Scl_Item_t * pItem, * pChild;
    Vec_StrPutI_( vOut, Scl_LibertyItemNum(p, Scl_LibertyRoot(p), "wire_load") );
    Vec_StrPut_( vOut );
    Scl_ItemForEachChildName( p, Scl_LibertyRoot(p), pItem, "wire_load" )
    {
        Vec_StrPutS_( vOut, Scl_LibertyReadString(p, pItem->Head) );
        Scl_ItemForEachChildName( p, pItem, pChild, "capacitance" )
            Vec_StrPutF_( vOut, atof(Scl_LibertyReadString(p, pChild->Head)) );
        Scl_ItemForEachChildName( p, pItem, pChild, "slope" )
            Vec_StrPutF_( vOut, atof(Scl_LibertyReadString(p, pChild->Head)) );
        Vec_StrPut_( vOut );
        Vec_StrPutI_( vOut, Scl_LibertyItemNum(p, pItem, "fanout_length") );
        Vec_StrPut_( vOut );
        Scl_ItemForEachChildName( p, pItem, pChild, "fanout_length" )
        {
            char * pHead  = Scl_LibertyReadString(p, pChild->Head);
            int    First  = atoi( strtok(pHead, " ,") );
            float  Second = atof( strtok(NULL, " ") );
            Vec_StrPutI_( vOut, First );
            Vec_StrPutF_( vOut, Second );
            Vec_StrPut_( vOut );
        }
        Vec_StrPut_( vOut );
    }
    Vec_StrPut_( vOut );
}
void Scl_LibertyReadWireLoadSelect( Scl_Tree_t * p, Vec_Str_t * vOut )
{
    Scl_Item_t * pItem, * pChild;
    Vec_StrPutI_( vOut, Scl_LibertyItemNum(p, Scl_LibertyRoot(p), "wire_load_selection") );
    Vec_StrPut_( vOut );
    Scl_ItemForEachChildName( p, Scl_LibertyRoot(p), pItem, "wire_load_selection" )
    {
        Vec_StrPutS_( vOut, Scl_LibertyReadString(p, pItem->Head) );
        Vec_StrPut_( vOut );
        Vec_StrPutI_( vOut, Scl_LibertyItemNum(p, pItem, "wire_load_from_area") );
        Vec_StrPut_( vOut );
        Scl_ItemForEachChildName( p, pItem, pChild, "wire_load_from_area" )
        {
            char * pHead  = Scl_LibertyReadString(p, pChild->Head);
            float  First  = atof( strtok(pHead, " ,") );
            float  Second = atof( strtok(NULL, " ,") );
            char * pThird = strtok(NULL, " ");
            if ( pThird[0] == '\"' )
                assert(pThird[strlen(pThird)-1] == '\"'), pThird[strlen(pThird)-1] = 0, pThird++;
            Vec_StrPutF_( vOut, First );
            Vec_StrPutF_( vOut, Second );
            Vec_StrPutS_( vOut, pThird );
            Vec_StrPut_( vOut );
        }
        Vec_StrPut_( vOut );
    }
    Vec_StrPut_( vOut );
}
int Scl_LibertyReadDeriveStrength( Scl_Tree_t * p, Scl_Item_t * pCell )
{
    Scl_Item_t * pItem;
    Scl_ItemForEachChildName( p, pCell, pItem, "drive_strength" )
        return atoi(Scl_LibertyReadString(p, pItem->Head));
    return 0;
}
int Scl_LibertyReadPinDirection( Scl_Tree_t * p, Scl_Item_t * pPin )
{
    Scl_Item_t * pItem;
    Scl_ItemForEachChildName( p, pPin, pItem, "direction" )
    {
        char * pToken = Scl_LibertyReadString(p, pItem->Head);
        if ( !strcmp(pToken, "input") )
            return 0;
        if ( !strcmp(pToken, "output") )
            return 1;
        if ( !strcmp(pToken, "internal") )
            return 2;
        break;
    }
    return -1;
}
float Scl_LibertyReadPinCap( Scl_Tree_t * p, Scl_Item_t * pPin, char * pName )
{
    Scl_Item_t * pItem;
    Scl_ItemForEachChildName( p, pPin, pItem, pName )
        return atof(Scl_LibertyReadString(p, pItem->Head));
    return 0;
}
Scl_Item_t * Scl_LibertyReadPinTiming( Scl_Tree_t * p, Scl_Item_t * pPinOut, char * pNameIn )
{
    Scl_Item_t * pTiming, * pPinIn;
    Scl_ItemForEachChildName( p, pPinOut, pTiming, "timing" )
        Scl_ItemForEachChildName( p, pTiming, pPinIn, "related_pin" )
            if ( !strcmp(Scl_LibertyReadString(p, pPinIn->Head), pNameIn) )
                return pTiming;
    return NULL;
}
Vec_Ptr_t * Scl_LibertyReadPinTimingAll( Scl_Tree_t * p, Scl_Item_t * pPinOut, char * pNameIn )
{
    Vec_Ptr_t * vTimings;
    Scl_Item_t * pTiming, * pPinIn;
    vTimings = Vec_PtrAlloc( 16 );
    Scl_ItemForEachChildName( p, pPinOut, pTiming, "timing" )
        Scl_ItemForEachChildName( p, pTiming, pPinIn, "related_pin" )
            if ( !strcmp(Scl_LibertyReadString(p, pPinIn->Head), pNameIn) )
                Vec_PtrPush( vTimings, pTiming );
    return vTimings;
}
int Scl_LibertyReadTimingSense( Scl_Tree_t * p, Scl_Item_t * pPin )
{
    Scl_Item_t * pItem;
    Scl_ItemForEachChildName( p, pPin, pItem, "timing_sense" )
    {
        char * pToken = Scl_LibertyReadString(p, pItem->Head);
        if ( !strcmp(pToken, "positive_unate") )
            return sc_ts_Pos;
        if ( !strcmp(pToken, "negative_unate") )
            return sc_ts_Neg;
        if ( !strcmp(pToken, "non_unate") )
            return sc_ts_Non;
        break;
    }
    return sc_ts_Non;
}
Vec_Flt_t * Scl_LibertyReadFloatVec( char * pName )
{
    char * pToken;
    Vec_Flt_t * vValues = Vec_FltAlloc( 100 );
    for ( pToken = strtok(pName, " \t\n\r\\\","); pToken; pToken = strtok(NULL, " \t\n\r\\\",") )
        Vec_FltPush( vValues, atof(pToken) );
    return vValues;
}

void Scl_LibertyDumpTables( Vec_Str_t * vOut, Vec_Flt_t * vInd1, Vec_Flt_t * vInd2, Vec_Flt_t * vValues )
{
    int i; float Entry;
    // write entries
    Vec_StrPutI_( vOut, Vec_FltSize(vInd1) );
    Vec_FltForEachEntry( vInd1, Entry, i )
        Vec_StrPutF_( vOut, Entry );
    Vec_StrPut_( vOut );
    // write entries
    Vec_StrPutI_( vOut, Vec_FltSize(vInd2) );
    Vec_FltForEachEntry( vInd2, Entry, i )
        Vec_StrPutF_( vOut, Entry );
    Vec_StrPut_( vOut );
    Vec_StrPut_( vOut );
    // write entries
    assert( Vec_FltSize(vInd1) * Vec_FltSize(vInd2) == Vec_FltSize(vValues) );
    Vec_FltForEachEntry( vValues, Entry, i )
    {
        Vec_StrPutF_( vOut, Entry );
        if ( i % Vec_FltSize(vInd2) == Vec_FltSize(vInd2)-1 )
            Vec_StrPut_( vOut );
    }
    // dump approximations
    Vec_StrPut_( vOut );
    for ( i = 0; i < 3; i++ ) 
        Vec_StrPutF_( vOut, 0 );
    for ( i = 0; i < 4; i++ ) 
        Vec_StrPutF_( vOut, 0 );
    for ( i = 0; i < 6; i++ ) 
        Vec_StrPutF_( vOut, 0 );
    Vec_StrPut_( vOut );
    Vec_StrPut_( vOut );
}
int Scl_LibertyScanTable( Scl_Tree_t * p, Vec_Ptr_t * vOut, Scl_Item_t * pTiming, char * pName, Vec_Ptr_t * vTemples )
{
    Vec_Flt_t * vIndex1 = NULL;
    Vec_Flt_t * vIndex2 = NULL;
    Vec_Flt_t * vValues = NULL;
    Vec_Flt_t * vInd1, * vInd2;
    Scl_Item_t * pItem, * pTable = NULL;
    char * pThis, * pTempl = NULL;
    int iPlace, i;
    float Entry;
    // find the table
    Scl_ItemForEachChildName( p, pTiming, pTable, pName )
        break;
    if ( pTable == NULL )
        return 0;
    // find the template
    pTempl = Scl_LibertyReadString(p, pTable->Head);
    if ( pTempl == NULL || pTempl[0] == 0 )
    {
        // read the numbers
        Scl_ItemForEachChild( p, pTable, pItem )
        {
            if ( !Scl_LibertyCompare(p, pItem->Key, "index_1") )
                assert(vIndex1 == NULL), vIndex1 = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
            else if ( !Scl_LibertyCompare(p, pItem->Key, "index_2") )
                assert(vIndex2 == NULL), vIndex2 = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
            else if ( !Scl_LibertyCompare(p, pItem->Key, "values") )
                assert(vValues == NULL), vValues = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
        }
        if ( vIndex1 == NULL || vIndex2 == NULL || vValues == NULL )
            { printf( "Incomplete table specification\n" ); return 0; }
        // dump the table
        vInd1 = vIndex1;
        vInd2 = vIndex2;
        // write entries
        Vec_PtrPush( vOut, vInd1 );
        Vec_PtrPush( vOut, vInd2 );
        Vec_PtrPush( vOut, vValues );
    }
    else if ( !strcmp(pTempl, "scalar") )
    {
        Scl_ItemForEachChild( p, pTable, pItem )
            if ( !Scl_LibertyCompare(p, pItem->Key, "values") )
            {
                assert(vValues == NULL);
                vValues = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
                assert( Vec_FltSize(vValues) == 1 );
                // write entries
                Vec_PtrPush( vOut, Vec_IntStart(1) );
                Vec_PtrPush( vOut, Vec_IntStart(1) );
                Vec_PtrPush( vOut, vValues );
                break;
            }
            else
            { printf( "Cannot read \"scalar\" template\n" ); return 0; }
    }
    else
    {
        // fetch the template
        iPlace = -1;
        Vec_PtrForEachEntry( char *, vTemples, pThis, i )
            if ( i % 4 == 0 && !strcmp(pTempl, pThis) )
            {  
                iPlace = i;
                break;
            }
        if ( iPlace == -1 )
            { printf( "Template cannot be found in the template library\n" ); return 0; }
        // read the numbers
        Scl_ItemForEachChild( p, pTable, pItem )
        {
            if ( !Scl_LibertyCompare(p, pItem->Key, "index_1") )
                assert(vIndex1 == NULL), vIndex1 = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
            else if ( !Scl_LibertyCompare(p, pItem->Key, "index_2") )
                assert(vIndex2 == NULL), vIndex2 = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
            else if ( !Scl_LibertyCompare(p, pItem->Key, "values") )
                assert(vValues == NULL), vValues = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
        }
        // check the template style
        vInd1 = (Vec_Flt_t *)Vec_PtrEntry( vTemples, iPlace + 2 ); // slew
        vInd2 = (Vec_Flt_t *)Vec_PtrEntry( vTemples, iPlace + 3 ); // load
        if ( Vec_PtrEntry(vTemples, iPlace + 1) == NULL ) // normal order (vIndex1 is slew; vIndex2 is load)
        {
            assert( !vIndex1 || Vec_FltSize(vIndex1) == Vec_FltSize(vInd1) );
            assert( !vIndex2 || Vec_FltSize(vIndex2) == Vec_FltSize(vInd2) );
            vInd1 = vIndex1 ? vIndex1 : vInd1;
            vInd2 = vIndex2 ? vIndex2 : vInd2;
            // write entries
            Vec_PtrPush( vOut, Vec_FltDup(vInd1) );
            Vec_PtrPush( vOut, Vec_FltDup(vInd2) );
            Vec_PtrPush( vOut, Vec_FltDup(vValues) );
        }
        else  // reverse order (vIndex2 is slew; vIndex1 is load)
        {
            Vec_Flt_t * vValues2 = Vec_FltAlloc( Vec_FltSize(vValues) );
            assert( !vIndex2 || Vec_FltSize(vIndex2) == Vec_FltSize(vInd1) );
            assert( !vIndex1 || Vec_FltSize(vIndex1) == Vec_FltSize(vInd2) );
            vInd1 = vIndex2 ? vIndex2 : vInd1;
            vInd2 = vIndex1 ? vIndex1 : vInd2;
            // write entries -- transpose
            assert( Vec_FltSize(vInd1) * Vec_FltSize(vInd2) == Vec_FltSize(vValues) );
            Vec_FltForEachEntry( vValues, Entry, i )
            {
                int x = i % Vec_FltSize(vInd2);
                int y = i / Vec_FltSize(vInd2);
                Entry = Vec_FltEntry( vValues, x * Vec_FltSize(vInd1) + y );
                Vec_FltPush( vValues2, Entry );
            }
            assert( Vec_FltSize(vValues) == Vec_FltSize(vValues2) );
            // write entries
            Vec_PtrPush( vOut, Vec_FltDup(vInd1) );
            Vec_PtrPush( vOut, Vec_FltDup(vInd2) );
            Vec_PtrPush( vOut, vValues2 );
        }
        Vec_FltFreeP( &vIndex1 );
        Vec_FltFreeP( &vIndex2 );
        Vec_FltFreeP( &vValues );
    }
    return 1;
}
int Scl_LibertyComputeWorstCase( Vec_Ptr_t * vTables, Vec_Flt_t ** pvInd0, Vec_Flt_t ** pvInd1, Vec_Flt_t ** pvValues )
{
    Vec_Flt_t * vInd0, * vInd1, * vValues;
    Vec_Flt_t * vind0, * vind1, * vvalues;
    int i, k, nTriples = Vec_PtrSize(vTables) / 3;
    float Entry;
    assert( Vec_PtrSize(vTables) > 0 && Vec_PtrSize(vTables) % 3 == 0 );
    if ( nTriples == 1 )
    {
        *pvInd0   = (Vec_Flt_t *)Vec_PtrEntry(vTables, 0);
        *pvInd1   = (Vec_Flt_t *)Vec_PtrEntry(vTables, 1);
        *pvValues = (Vec_Flt_t *)Vec_PtrEntry(vTables, 2);
        Vec_PtrShrink( vTables, 0 );
        return 1;
    }
    vInd0   = Vec_FltDup( (Vec_Flt_t *)Vec_PtrEntry(vTables, 0) );
    vInd1   = Vec_FltDup( (Vec_Flt_t *)Vec_PtrEntry(vTables, 1) );
    vValues = Vec_FltDup( (Vec_Flt_t *)Vec_PtrEntry(vTables, 2) );
    for ( i = 1; i < nTriples; i++ )
    {
        vind0   = (Vec_Flt_t *)Vec_PtrEntry(vTables, i*3+0);
        vind1   = (Vec_Flt_t *)Vec_PtrEntry(vTables, i*3+1);
        vvalues = (Vec_Flt_t *)Vec_PtrEntry(vTables, i*3+2);
        // check equality of indexes
        if ( !Vec_FltEqual(vind0, vInd0) )
            continue;//return 0;
        if ( !Vec_FltEqual(vind1, vInd1) )
            continue;//return 0;
//        Vec_FltForEachEntry( vvalues, Entry, k )
//            Vec_FltAddToEntry( vValues, k, Entry );
        Vec_FltForEachEntry( vvalues, Entry, k )
            if ( Vec_FltEntry(vValues, k) < Entry )
                Vec_FltWriteEntry( vValues, k, Entry );
    }
//    Vec_FltForEachEntry( vValues, Entry, k )
//        Vec_FltWriteEntry( vValues, k, Entry/nTriples );
    // return the result
    *pvInd0 = vInd0;
    *pvInd1 = vInd1;
    *pvValues = vValues;
    return 1;
}

int Scl_LibertyReadTable( Scl_Tree_t * p, Vec_Str_t * vOut, Scl_Item_t * pTiming, char * pName, Vec_Ptr_t * vTemples )
{
    Vec_Flt_t * vIndex1 = NULL;
    Vec_Flt_t * vIndex2 = NULL;
    Vec_Flt_t * vValues = NULL;
    Vec_Flt_t * vInd1, * vInd2;
    Scl_Item_t * pItem, * pTable = NULL;
    char * pThis, * pTempl = NULL;
    int iPlace, i;
    float Entry;
    // find the table
    Scl_ItemForEachChildName( p, pTiming, pTable, pName )
        break;
    if ( pTable == NULL )
        return 0;
    // find the template
    pTempl = Scl_LibertyReadString(p, pTable->Head);
    if ( pTempl == NULL || pTempl[0] == 0 )
    {
        // read the numbers
        Scl_ItemForEachChild( p, pTable, pItem )
        {
            if ( !Scl_LibertyCompare(p, pItem->Key, "index_1") )
                assert(vIndex1 == NULL), vIndex1 = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
            else if ( !Scl_LibertyCompare(p, pItem->Key, "index_2") )
                assert(vIndex2 == NULL), vIndex2 = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
            else if ( !Scl_LibertyCompare(p, pItem->Key, "values") )
                assert(vValues == NULL), vValues = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
        }
        if ( vIndex1 == NULL || vIndex2 == NULL || vValues == NULL )
            { printf( "Incomplete table specification\n" ); return 0; }
        // dump the table
        vInd1 = vIndex1;
        vInd2 = vIndex2;
        // write entries
        Vec_StrPutI_( vOut, Vec_FltSize(vInd1) );
        Vec_FltForEachEntry( vInd1, Entry, i )
            Vec_StrPutF_( vOut, Entry );
        Vec_StrPut_( vOut );
        // write entries
        Vec_StrPutI_( vOut, Vec_FltSize(vInd2) );
        Vec_FltForEachEntry( vInd2, Entry, i )
            Vec_StrPutF_( vOut, Entry );
        Vec_StrPut_( vOut );
        Vec_StrPut_( vOut );
        // write entries
        assert( Vec_FltSize(vInd1) * Vec_FltSize(vInd2) == Vec_FltSize(vValues) );
        Vec_FltForEachEntry( vValues, Entry, i )
        {
            Vec_StrPutF_( vOut, Entry );
            if ( i % Vec_FltSize(vInd2) == Vec_FltSize(vInd2)-1 )
                Vec_StrPut_( vOut );
        }
    }
    else
    {
        // fetch the template
        iPlace = -1;
        Vec_PtrForEachEntry( char *, vTemples, pThis, i )
            if ( i % 4 == 0 && !strcmp(pTempl, pThis) )
            {  
                iPlace = i;
                break;
            }
        if ( iPlace == -1 )
            { printf( "Template cannot be found in the template library\n" ); return 0; }
        // read the numbers
        Scl_ItemForEachChild( p, pTable, pItem )
        {
            if ( !Scl_LibertyCompare(p, pItem->Key, "index_1") )
                assert(vIndex1 == NULL), vIndex1 = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
            else if ( !Scl_LibertyCompare(p, pItem->Key, "index_2") )
                assert(vIndex2 == NULL), vIndex2 = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
            else if ( !Scl_LibertyCompare(p, pItem->Key, "values") )
                assert(vValues == NULL), vValues = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
        }
        // check the template style
        vInd1 = (Vec_Flt_t *)Vec_PtrEntry( vTemples, iPlace + 2 ); // slew
        vInd2 = (Vec_Flt_t *)Vec_PtrEntry( vTemples, iPlace + 3 ); // load
        if ( Vec_PtrEntry(vTemples, iPlace + 1) == NULL ) // normal order (vIndex1 is slew; vIndex2 is load)
        {
            assert( !vIndex1 || Vec_FltSize(vIndex1) == Vec_FltSize(vInd1) );
            assert( !vIndex2 || Vec_FltSize(vIndex2) == Vec_FltSize(vInd2) );
            vInd1 = vIndex1 ? vIndex1 : vInd1;
            vInd2 = vIndex2 ? vIndex2 : vInd2;
            // write entries
            Vec_StrPutI_( vOut, Vec_FltSize(vInd1) );
            Vec_FltForEachEntry( vInd1, Entry, i )
                Vec_StrPutF_( vOut, Entry );
            Vec_StrPut_( vOut );
            // write entries
            Vec_StrPutI_( vOut, Vec_FltSize(vInd2) );
            Vec_FltForEachEntry( vInd2, Entry, i )
                Vec_StrPutF_( vOut, Entry );
            Vec_StrPut_( vOut );
            Vec_StrPut_( vOut );
            // write entries
            assert( Vec_FltSize(vInd1) * Vec_FltSize(vInd2) == Vec_FltSize(vValues) );
            Vec_FltForEachEntry( vValues, Entry, i )
            {
                Vec_StrPutF_( vOut, Entry );
                if ( i % Vec_FltSize(vInd2) == Vec_FltSize(vInd2)-1 )
                    Vec_StrPut_( vOut );
            }
        }
        else  // reverse order (vIndex2 is slew; vIndex1 is load)
        {
            assert( !vIndex2 || Vec_FltSize(vIndex2) == Vec_FltSize(vInd1) );
            assert( !vIndex1 || Vec_FltSize(vIndex1) == Vec_FltSize(vInd2) );
            vInd1 = vIndex2 ? vIndex2 : vInd1;
            vInd2 = vIndex1 ? vIndex1 : vInd2;
            // write entries
            Vec_StrPutI_( vOut, Vec_FltSize(vInd1) );
            Vec_FltForEachEntry( vInd1, Entry, i )
                Vec_StrPutF_( vOut, Entry );
            Vec_StrPut_( vOut );
            // write entries
            Vec_StrPutI_( vOut, Vec_FltSize(vInd2) );
            Vec_FltForEachEntry( vInd2, Entry, i )
                Vec_StrPutF_( vOut, Entry );
            Vec_StrPut_( vOut );
            Vec_StrPut_( vOut );
            // write entries -- transpose
            assert( Vec_FltSize(vInd1) * Vec_FltSize(vInd2) == Vec_FltSize(vValues) );
            Vec_FltForEachEntry( vValues, Entry, i )
            {
                int x = i % Vec_FltSize(vInd2);
                int y = i / Vec_FltSize(vInd2);
                Entry = Vec_FltEntry( vValues, x * Vec_FltSize(vInd1) + y );
                Vec_StrPutF_( vOut, Entry );
                if ( i % Vec_FltSize(vInd2) == Vec_FltSize(vInd2)-1 )
                    Vec_StrPut_( vOut );
            }
        }
    }
    Vec_StrPut_( vOut );
    for ( i = 0; i < 3; i++ ) 
        Vec_StrPutF_( vOut, 0 );
    for ( i = 0; i < 4; i++ ) 
        Vec_StrPutF_( vOut, 0 );
    for ( i = 0; i < 6; i++ ) 
        Vec_StrPutF_( vOut, 0 );
    Vec_FltFreeP( &vIndex1 );
    Vec_FltFreeP( &vIndex2 );
    Vec_FltFreeP( &vValues );
    Vec_StrPut_( vOut );
    Vec_StrPut_( vOut );
    return 1;
}
void Scl_LibertyPrintTemplates( Vec_Ptr_t * vRes )
{
    Vec_Flt_t * vArray; int i;
    assert( Vec_PtrSize(vRes) % 4 == 0 );
    printf( "There are %d slew/load templates\n", Vec_PtrSize(vRes) % 4 );
    Vec_PtrForEachEntry( Vec_Flt_t *, vRes, vArray, i )
    {
        if ( i % 4 == 0 )
            printf( "%s\n", (char *)vArray );
        else if ( i % 4 == 1 )
            printf( "%d\n", (int)(vArray != NULL) );
        else if ( i % 4 == 2 || i % 4 == 3 )
            Vec_FltPrint( vArray );
        if ( i % 4 == 3 )
            printf( "\n" );
    }
}
Vec_Ptr_t * Scl_LibertyReadTemplates( Scl_Tree_t * p )
{
    Vec_Ptr_t * vRes = NULL;
    Vec_Flt_t * vIndex1, * vIndex2;
    Scl_Item_t * pTempl, * pItem;
    char * pVar1, * pVar2;
    int fFlag0, fFlag1;
    vRes = Vec_PtrAlloc( 100 );
    Scl_ItemForEachChildName( p, Scl_LibertyRoot(p), pTempl, "lu_table_template" )
    {
        pVar1 = pVar2 = NULL;
        vIndex1 = vIndex2 = NULL;
        Scl_ItemForEachChild( p, pTempl, pItem )
        {
            if ( !Scl_LibertyCompare(p, pItem->Key, "index_1") )
                assert(vIndex1 == NULL), vIndex1 = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
            else if ( !Scl_LibertyCompare(p, pItem->Key, "index_2") )
                assert(vIndex2 == NULL), vIndex2 = Scl_LibertyReadFloatVec( Scl_LibertyReadString(p, pItem->Head) );
            else if ( !Scl_LibertyCompare(p, pItem->Key, "variable_1") )
                assert(pVar1 == NULL), pVar1 = Abc_UtilStrsav( Scl_LibertyReadString(p, pItem->Head) );
            else if ( !Scl_LibertyCompare(p, pItem->Key, "variable_2") )
                assert(pVar2 == NULL), pVar2 = Abc_UtilStrsav( Scl_LibertyReadString(p, pItem->Head) );
        }
        if ( pVar1 == NULL || pVar2 == NULL )
        {
            ABC_FREE( pVar1 );  
            ABC_FREE( pVar2 );
            Vec_FltFreeP( &vIndex1 );
            Vec_FltFreeP( &vIndex2 );
            continue;
        }
        assert( pVar1 != NULL && pVar2 != NULL );
        fFlag0 = (!strcmp(pVar1, "input_net_transition") && !strcmp(pVar2, "total_output_net_capacitance"));
        fFlag1 = (!strcmp(pVar2, "input_net_transition") && !strcmp(pVar1, "total_output_net_capacitance"));
        ABC_FREE( pVar1 );  
        ABC_FREE( pVar2 );
        if ( !fFlag0 && !fFlag1 )
        {
            Vec_FltFreeP( &vIndex1 );
            Vec_FltFreeP( &vIndex2 );
            continue;
        }
        Vec_PtrPush( vRes, Abc_UtilStrsav( Scl_LibertyReadString(p, pTempl->Head) ) );
        Vec_PtrPush( vRes, fFlag0 ? NULL : (void *)(ABC_PTRINT_T)1 );
        Vec_PtrPush( vRes, fFlag0 ? vIndex1 : vIndex2 );
        Vec_PtrPush( vRes, fFlag0 ? vIndex2 : vIndex1 );
    }
    if ( Vec_PtrSize(vRes) == 0 )
        Abc_Print( 0, "Templates are not defined.\n" );
    // print templates
//    printf( "Found %d templates\n", Vec_PtrSize(vRes)/4 );
//    Scl_LibertyPrintTemplates( vRes );
    return vRes;
}
Vec_Str_t * Scl_LibertyReadSclStr( Scl_Tree_t * p, int fVerbose, int fVeryVerbose, SC_DontUse dont_use )
{
    int fUseFirstTable = 0;
    Vec_Str_t * vOut;
    Vec_Ptr_t * vNameIns, * vTemples = NULL;
    Scl_Item_t * pCell, * pPin, * pTiming;
    Vec_Wrd_t * vTruth;
    char * pFormula, * pName;
    int i, k, Counter, nOutputs, nCells;
    int nSkipped[4] = {0};

    // read delay-table templates
    vTemples = Scl_LibertyReadTemplates( p );

    // start the library
    vOut = Vec_StrAlloc( 10000 );
    Vec_StrPutI_( vOut, ABC_SCL_CUR_VERSION );

    // top level information
    Vec_StrPut_( vOut );
    Vec_StrPutS_( vOut, Scl_LibertyReadString(p, Scl_LibertyRoot(p)->Head) );
    Vec_StrPutS_( vOut, Scl_LibertyReadDefaultWireLoad(p) );
    Vec_StrPutS_( vOut, Scl_LibertyReadDefaultWireLoadSel(p) );
    Vec_StrPutF_( vOut, Scl_LibertyReadDefaultMaxTrans(p) );
    Vec_StrPutI_( vOut, Scl_LibertyReadTimeUnit(p) );
    Scl_LibertyReadLoadUnit( p, vOut );
    Vec_StrPut_( vOut );
    Vec_StrPut_( vOut );

    // read wire loads
    Scl_LibertyReadWireLoad( p, vOut );
    Scl_LibertyReadWireLoadSelect( p, vOut );

    // count cells
    nCells = 0;
    Scl_ItemForEachChildName( p, Scl_LibertyRoot(p), pCell, "cell" )
    {
        if ( Scl_LibertyReadCellIsFlop(p, pCell) )
        {
            if ( fVeryVerbose )  printf( "Scl_LibertyReadGenlib() skipped sequential cell \"%s\".\n", Scl_LibertyReadString(p, pCell->Head) );
            nSkipped[0]++;
            continue;
        }
        if ( Scl_LibertyReadCellIsDontUse(p, pCell, dont_use) )
        {
            if ( fVeryVerbose )  printf( "Scl_LibertyReadGenlib() skipped cell \"%s\" due to dont_use attribute.\n", Scl_LibertyReadString(p, pCell->Head) );
            nSkipped[3]++;
            continue;
        }
        if ( Scl_LibertyReadCellIsThreeState(p, pCell) )
        {
            if ( fVeryVerbose )  printf( "Scl_LibertyReadGenlib() skipped three-state cell \"%s\".\n", Scl_LibertyReadString(p, pCell->Head) );
            nSkipped[1]++;
            continue;
        }
        if ( (Counter = Scl_LibertyReadCellOutputNum(p, pCell)) == 0 )
        {
            if ( fVeryVerbose )  printf( "Scl_LibertyReadGenlib() skipped cell \"%s\" without logic function.\n", Scl_LibertyReadString(p, pCell->Head) );
            nSkipped[2]++;
            continue;
        }
        nCells++;
    }
    // read cells
    Vec_StrPutI_( vOut, nCells );
    Vec_StrPut_( vOut );
    Vec_StrPut_( vOut );
    Scl_ItemForEachChildName( p, Scl_LibertyRoot(p), pCell, "cell" )
    {
        if ( Scl_LibertyReadCellIsFlop(p, pCell) )
            continue;
        if ( Scl_LibertyReadCellIsDontUse(p, pCell, dont_use) )
            continue;
        if ( Scl_LibertyReadCellIsThreeState(p, pCell) )
            continue;
        if ( (Counter = Scl_LibertyReadCellOutputNum(p, pCell)) == 0 )
            continue;
        // top level information
        Vec_StrPutS_( vOut, Scl_LibertyReadString(p, pCell->Head) );
        pName = Scl_LibertyReadCellArea(p, pCell);
        Vec_StrPutF_( vOut, pName ? atof(pName) : 1 );
        pName = Scl_LibertyReadCellLeakage(p, pCell);
        Vec_StrPutF_( vOut, pName ? atof(pName) : 0 );
        Vec_StrPutI_( vOut, Scl_LibertyReadDeriveStrength(p, pCell) );
        // pin count
        nOutputs = Scl_LibertyReadCellOutputNum( p, pCell );
        Vec_StrPutI_( vOut, Scl_LibertyItemNum(p, pCell, "pin") - nOutputs );
        Vec_StrPutI_( vOut, nOutputs );
        Vec_StrPut_( vOut );
        Vec_StrPut_( vOut );

        // input pins
        vNameIns = Vec_PtrAlloc( 16 );
        Scl_ItemForEachChildName( p, pCell, pPin, "pin" )
        {
            float CapOne, CapRise, CapFall;
            if ( Scl_LibertyReadPinFormula(p, pPin) != NULL ) // skip output pin
                continue;
            assert( Scl_LibertyReadPinDirection(p, pPin) == 0 || Scl_LibertyReadPinDirection(p, pPin) == 2);
            pName = Scl_LibertyReadString(p, pPin->Head);
            Vec_PtrPush( vNameIns, Abc_UtilStrsav(pName) );
            Vec_StrPutS_( vOut, pName );
            CapOne  = Scl_LibertyReadPinCap( p, pPin, "capacitance" );
            CapRise = Scl_LibertyReadPinCap( p, pPin, "rise_capacitance" );
            CapFall = Scl_LibertyReadPinCap( p, pPin, "fall_capacitance" );
            if ( CapRise == 0 )
                CapRise = CapOne;
            if ( CapFall == 0 )
                CapFall = CapOne;
            Vec_StrPutF_( vOut, CapRise );
            Vec_StrPutF_( vOut, CapFall );
            Vec_StrPut_( vOut );
        }
        Vec_StrPut_( vOut );
        // output pins
        Scl_ItemForEachChildName( p, pCell, pPin, "pin" )
        {
            if ( !Scl_LibertyReadPinFormula(p, pPin) ) // skip input pin
                continue;
            if (Scl_LibertyReadPinDirection(p, pPin) == 2) // skip internal pin
                continue;
            assert( Scl_LibertyReadPinDirection(p, pPin) == 1 );
            pName = Scl_LibertyReadString(p, pPin->Head);
            Vec_StrPutS_( vOut, pName );
            Vec_StrPutF_( vOut, Scl_LibertyReadPinCap( p, pPin, "max_capacitance" ) );
            Vec_StrPutF_( vOut, Scl_LibertyReadPinCap( p, pPin, "max_transition" ) );
            Vec_StrPutI_( vOut, Vec_PtrSize(vNameIns) );
            pFormula = Scl_LibertyReadPinFormula(p, pPin);
            Vec_StrPutS_( vOut, pFormula );
            // write truth table
            vTruth = Mio_ParseFormulaTruth( pFormula, (char **)Vec_PtrArray(vNameIns), Vec_PtrSize(vNameIns) );
            if ( vTruth == NULL )
                return NULL;
            for ( i = 0; i < Abc_Truth6WordNum(Vec_PtrSize(vNameIns)); i++ )
                Vec_StrPutW_( vOut, Vec_WrdEntry(vTruth, i) );
            Vec_WrdFree( vTruth );
            Vec_StrPut_( vOut );
            Vec_StrPut_( vOut );

            // write the delay tables
            if ( fUseFirstTable )
            {
                Vec_PtrForEachEntry( char *, vNameIns, pName, i )
                {
                    pTiming = Scl_LibertyReadPinTiming( p, pPin, pName );
                    Vec_StrPutS_( vOut, pName );
                    Vec_StrPutI_( vOut, (int)(pTiming != NULL) );
                    if ( pTiming == NULL ) // output does not depend on input
                        continue;
                    Vec_StrPutI_( vOut, Scl_LibertyReadTimingSense(p, pTiming) );
                    Vec_StrPut_( vOut );
                    Vec_StrPut_( vOut );
                    // some cells only have 'rise' or 'fall' but not both - here we work around this
                    if ( !Scl_LibertyReadTable( p, vOut, pTiming, "cell_rise",           vTemples ) )
                        if ( !Scl_LibertyReadTable( p, vOut, pTiming, "cell_fall",       vTemples ) )
                                { printf( "Table cannot be found\n" ); return NULL; }                              
                    if ( !Scl_LibertyReadTable( p, vOut, pTiming, "cell_fall",           vTemples ) )
                        if ( !Scl_LibertyReadTable( p, vOut, pTiming, "cell_rise",       vTemples ) )
                                { printf( "Table cannot be found\n" ); return NULL; }                              
                    if ( !Scl_LibertyReadTable( p, vOut, pTiming, "rise_transition",     vTemples ) )
                        if ( !Scl_LibertyReadTable( p, vOut, pTiming, "fall_transition", vTemples ) )
                                { printf( "Table cannot be found\n" ); return NULL; }                              
                    if ( !Scl_LibertyReadTable( p, vOut, pTiming, "fall_transition",     vTemples ) )
                        if ( !Scl_LibertyReadTable( p, vOut, pTiming, "rise_transition", vTemples ) )
                                { printf( "Table cannot be found\n" ); return NULL; }  
                }
                continue;
            }

            // write the timing tables
            Vec_PtrForEachEntry( char *, vNameIns, pName, i )
            {
                Vec_Ptr_t * vTables[4];
                Vec_Ptr_t * vTimings;
                vTimings = Scl_LibertyReadPinTimingAll( p, pPin, pName );
                Vec_StrPutS_( vOut, pName );
                Vec_StrPutI_( vOut, (int)(Vec_PtrSize(vTimings) != 0) );
                if ( Vec_PtrSize(vTimings) == 0 ) // output does not depend on input
                {
                    Vec_PtrFree( vTimings );
                    continue;
                }
                Vec_StrPutI_( vOut, Scl_LibertyReadTimingSense(p, (Scl_Item_t *)Vec_PtrEntry(vTimings, 0)) );
                Vec_StrPut_( vOut );
                Vec_StrPut_( vOut );
                // collect the timing tables
                for ( k = 0; k < 4; k++ )
                    vTables[k] = Vec_PtrAlloc( 16 );
                Vec_PtrForEachEntry( Scl_Item_t *, vTimings, pTiming, k )
                {
                    // some cells only have 'rise' or 'fall' but not both - here we work around this
                    if ( !Scl_LibertyScanTable( p, vTables[0], pTiming, "cell_rise",           vTemples ) )
                        if ( !Scl_LibertyScanTable( p, vTables[0], pTiming, "cell_fall",       vTemples ) )
                                { printf( "Table cannot be found\n" ); return NULL; }                              
                    if ( !Scl_LibertyScanTable( p, vTables[1], pTiming, "cell_fall",           vTemples ) )
                        if ( !Scl_LibertyScanTable( p, vTables[1], pTiming, "cell_rise",       vTemples ) )
                                { printf( "Table cannot be found\n" ); return NULL; }                              
                    if ( !Scl_LibertyScanTable( p, vTables[2], pTiming, "rise_transition",     vTemples ) )
                        if ( !Scl_LibertyScanTable( p, vTables[2], pTiming, "fall_transition", vTemples ) )
                                { printf( "Table cannot be found\n" ); return NULL; }                              
                    if ( !Scl_LibertyScanTable( p, vTables[3], pTiming, "fall_transition",     vTemples ) )
                        if ( !Scl_LibertyScanTable( p, vTables[3], pTiming, "rise_transition", vTemples ) )
                                { printf( "Table cannot be found\n" ); return NULL; }  
                }
                Vec_PtrFree( vTimings );
                // compute worse case of the tables
                for ( k = 0; k < 4; k++ )
                {
                    Vec_Flt_t * vInd0, * vInd1, * vValues;
                    if ( !Scl_LibertyComputeWorstCase( vTables[k], &vInd0, &vInd1, &vValues ) )
                        { printf( "Table indexes have different values\n" ); return NULL; }  
                    Vec_VecFree( (Vec_Vec_t *)vTables[k] );
                    Scl_LibertyDumpTables( vOut, vInd0, vInd1, vValues );
                    Vec_FltFree( vInd0 );
                    Vec_FltFree( vInd1 );
                    Vec_FltFree( vValues );
                }
            }
        }
        Vec_StrPut_( vOut );
        Vec_PtrFreeFree( vNameIns );
    }
    // free templates
    if ( vTemples )
    {
        Vec_Flt_t * vArray;
        assert( Vec_PtrSize(vTemples) % 4 == 0 );
        Vec_PtrForEachEntry( Vec_Flt_t *, vTemples, vArray, i )
        {
            if ( vArray == NULL )
                continue;
            if ( i % 4 == 0 )
                ABC_FREE( vArray );
            else if ( i % 4 == 2 || i % 4 == 3 )
                Vec_FltFree( vArray );
        }
        Vec_PtrFree( vTemples );
    }
    if ( fVerbose )
    {
        printf( "Library \"%s\" from \"%s\" has %d cells ", 
            Scl_LibertyReadString(p, Scl_LibertyRoot(p)->Head), p->pFileName, nCells );
        printf( "(%d skipped: %d seq; %d tri-state; %d no func; %d dont_use).  ", 
            nSkipped[0]+nSkipped[1]+nSkipped[2], nSkipped[0], nSkipped[1], nSkipped[2], nSkipped[3] );
        Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
    }
    return vOut;
}
SC_Lib * Abc_SclReadLiberty( char * pFileName, int fVerbose, int fVeryVerbose, SC_DontUse dont_use )
{
    SC_Lib * pLib;
    Scl_Tree_t * p;
    Vec_Str_t * vStr;
    p = Scl_LibertyParse( pFileName, fVeryVerbose );
    if ( p == NULL )
        return NULL;
//    Scl_LibertyParseDump( p, "temp_.lib" );
    // collect relevant data
    vStr = Scl_LibertyReadSclStr( p, fVerbose, fVeryVerbose, dont_use );
    Scl_LibertyStop( p, fVeryVerbose );
    if ( vStr == NULL )
        return NULL;
    // construct SCL data-structure
    pLib = Abc_SclReadFromStr( vStr );
    if ( pLib == NULL )
        return NULL;
    pLib->pFileName = Abc_UtilStrsav( pFileName );
    Abc_SclLibNormalize( pLib );
    Vec_StrFree( vStr );
//    printf( "Average slew = %.2f ps\n", Abc_SclComputeAverageSlew(pLib) );
    return pLib;
}

/**Function*************************************************************

  Synopsis    [Experiments with Liberty parsing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Scl_LibertyTest()
{
    char * pFileName = "bwrc.lib";
    int fVerbose = 1;
    int fVeryVerbose = 0;
    Scl_Tree_t * p;
    Vec_Str_t * vStr;
//    return;
    p = Scl_LibertyParse( pFileName, fVeryVerbose );
    if ( p == NULL )
        return;
//    Scl_LibertyParseDump( p, "temp_.lib" );
    SC_DontUse dont_use = {0};
    vStr = Scl_LibertyReadSclStr( p, fVerbose, fVeryVerbose, dont_use);
    Scl_LibertyStringDump( "test_scl.lib", vStr );
    Vec_StrFree( vStr );
    Scl_LibertyStop( p, fVerbose );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

