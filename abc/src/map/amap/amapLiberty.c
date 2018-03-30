/**CFile****************************************************************

  FileHead    [amapLiberty.c]

  SystemHead  [ABC: Logic synthesis and verification system.]

  PackageHead [Technology mapper for standard cells.]

  Synopsis    [Liberty parser.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapLiberty.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "amapInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define ABC_MAX_LIB_STR_LEN 5000

// entry types
typedef enum { 
    AMAP_LIBERTY_NONE = 0,        // 0:  unknown
    AMAP_LIBERTY_PROC,            // 1:  procedure :  key(head){body}
    AMAP_LIBERTY_EQUA,            // 2:  equation  :  key:head;
    AMAP_LIBERTY_LIST             // 3:  list      :  key(head) 
} Amap_LibertyType_t;

typedef struct Amap_Pair_t_ Amap_Pair_t;
struct Amap_Pair_t_
{
    int             Beg;          // item beginning
    int             End;          // item end
};

typedef struct Amap_Item_t_ Amap_Item_t;
struct Amap_Item_t_
{
    int             Type;         // Amap_LibertyType_t
    int             iLine;        // file line where the item's spec begins
    Amap_Pair_t     Key;          // key part
    Amap_Pair_t     Head;         // head part 
    Amap_Pair_t     Body;         // body part
    int             Next;         // next item in the list 
    int             Child;        // first child item 
};

typedef struct Amap_Tree_t_ Amap_Tree_t;
struct Amap_Tree_t_
{
    char *          pFileName;    // input Liberty file name
    char *          pContents;    // file contents
    int             nContents;    // file size
    int             nLines;       // line counter
    int             nItems;       // number of items
    int             nItermAlloc;  // number of items allocated
    Amap_Item_t *   pItems;       // the items
    char *          pError;       // the error string
};

static inline Amap_Item_t *  Amap_LibertyRoot( Amap_Tree_t * p )                                       { return p->pItems;                                                 }
static inline Amap_Item_t *  Amap_LibertyItem( Amap_Tree_t * p, int v )                                { assert( v < p->nItems ); return v < 0 ? NULL : p->pItems + v;     }
static inline int            Amap_LibertyCompare( Amap_Tree_t * p, Amap_Pair_t Pair, char * pStr )     { return strncmp( p->pContents+Pair.Beg, pStr, Pair.End-Pair.Beg ); }
static inline void           Amap_PrintWord( FILE * pFile, Amap_Tree_t * p, Amap_Pair_t Pair )         { char * pBeg = p->pContents+Pair.Beg, * pEnd = p->pContents+Pair.End; while ( pBeg < pEnd ) fputc( *pBeg++, pFile ); }
static inline void           Amap_PrintSpace( FILE * pFile, int nOffset )                              { int i; for ( i = 0; i < nOffset; i++ ) fputc(' ', pFile);         }
static inline int            Amap_LibertyItemId( Amap_Tree_t * p, Amap_Item_t * pItem )                { return pItem - p->pItems;                                         }

#define Amap_ItemForEachChild( p, pItem, pChild ) \
    for ( pChild = Amap_LibertyItem(p, pItem->Child); pChild; pChild = Amap_LibertyItem(p, pChild->Next) )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prints parse tree in Liberty format.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_LibertyPrintLibertyItem( FILE * pFile, Amap_Tree_t * p, Amap_Item_t * pItem, int nOffset )
{
    if ( pItem->Type == AMAP_LIBERTY_PROC )
    {
        Amap_PrintSpace( pFile, nOffset );
        Amap_PrintWord( pFile, p, pItem->Key );
        fprintf( pFile, "(" );
        Amap_PrintWord( pFile, p, pItem->Head );
        fprintf( pFile, ") {\n" );
        if ( Amap_LibertyItem(p, pItem->Child) )
            Amap_LibertyPrintLibertyItem( pFile, p, Amap_LibertyItem(p, pItem->Child), nOffset + 1 );
        Amap_PrintSpace( pFile, nOffset );
        fprintf( pFile, "}\n" );
    }
    else if ( pItem->Type == AMAP_LIBERTY_EQUA )
    {
        Amap_PrintSpace( pFile, nOffset );
        Amap_PrintWord( pFile, p, pItem->Key );
        fprintf( pFile, " : " );
        Amap_PrintWord( pFile, p, pItem->Head );
        fprintf( pFile, ";\n" );
    }
    else if ( pItem->Type == AMAP_LIBERTY_LIST )
    {
        Amap_PrintSpace( pFile, nOffset );
        Amap_PrintWord( pFile, p, pItem->Key );
        fprintf( pFile, "(" );
        Amap_PrintWord( pFile, p, pItem->Head );
        fprintf( pFile, ");\n" );
    }
    else assert( 0 );
    if ( Amap_LibertyItem(p, pItem->Next) )
        Amap_LibertyPrintLibertyItem( pFile, p, Amap_LibertyItem(p, pItem->Next), nOffset );
}

/**Function*************************************************************

  Synopsis    [Prints parse tree in Liberty format.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibertyPrintLiberty( Amap_Tree_t * p, char * pFileName )
{
    FILE * pFile;
    if ( pFileName == NULL )
        pFile = stdout;
    else
    {
        pFile = fopen( pFileName, "w" );
        if ( pFile == NULL )
        {
            printf( "Amap_LibertyPrintLiberty(): The output file is unavailable (absent or open).\n" );
            return 0;
        }
    }
    Amap_LibertyPrintLibertyItem( pFile, p, Amap_LibertyRoot(p), 0 );
    if ( pFile != stdout )
        fclose( pFile );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Returns the time stamp.]

  Description [The file should be closed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Amap_LibertyTimeStamp()
{
    static char Buffer[100];
    char * TimeStamp;
    time_t ltime;
    // get the current time
    time( &ltime );
    TimeStamp = asctime( localtime( &ltime ) );
    TimeStamp[ strlen(TimeStamp) - 1 ] = 0;
    assert( strlen(TimeStamp) < 100 );
    strcpy( Buffer, TimeStamp );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Returns cell's function.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibertyCellIsFlop( Amap_Tree_t * p, Amap_Item_t * pCell )
{
    Amap_Item_t * pAttr;
    Amap_ItemForEachChild( p, pCell, pAttr )
        if ( !Amap_LibertyCompare(p, pAttr->Key, "ff") ||
             !Amap_LibertyCompare(p, pAttr->Key, "latch") )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns pin's function.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Item_t * Amap_LibertyPinFunction( Amap_Tree_t * p, Amap_Item_t * pPin )
{
    Amap_Item_t * pFunc;
    Amap_ItemForEachChild( p, pPin, pFunc )
        if ( !Amap_LibertyCompare(p, pFunc->Key, "function") )
            return pFunc;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns output pin(s).]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Item_t * Amap_LibertyCellOutput( Amap_Tree_t * p, Amap_Item_t * pCell )
{
    Amap_Item_t * pPin;
    Amap_ItemForEachChild( p, pCell, pPin )
    {
        if ( Amap_LibertyCompare(p, pPin->Key, "pin") )
            continue;
        if ( Amap_LibertyPinFunction(p, pPin) )
            return pPin;
    }
    return NULL;
}
Vec_Ptr_t * Amap_LibertyCellOutputs( Amap_Tree_t * p, Amap_Item_t * pCell )
{
    Amap_Item_t * pPin;
    Vec_Ptr_t * vOutPins;
    vOutPins = Vec_PtrAlloc( 2 );
    Amap_ItemForEachChild( p, pCell, pPin )
    {
        if ( Amap_LibertyCompare(p, pPin->Key, "pin") )
            continue;
        if ( Amap_LibertyPinFunction(p, pPin) )
            Vec_PtrPush( vOutPins, pPin );
    }
    return vOutPins;
}

/**Function*************************************************************

  Synopsis    [Returns cell's area.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Item_t * Amap_LibertyCellArea( Amap_Tree_t * p, Amap_Item_t * pCell )
{
    Amap_Item_t * pArea;
    Amap_ItemForEachChild( p, pCell, pArea )
    {
        if ( Amap_LibertyCompare(p, pArea->Key, "area") )
            continue;
        return pArea;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Count cell's output pins (pins with a logic function).]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibertyCellCountOutputs( Amap_Tree_t * p, Amap_Item_t * pCell )
{
    Amap_Item_t * pPin;
    int Counter = 0;
    Amap_ItemForEachChild( p, pCell, pPin )
    {
        if ( Amap_LibertyCompare(p, pPin->Key, "pin") )
            continue;
        if ( Amap_LibertyPinFunction(p, pPin) )
            Counter++;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Gets the name to write.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Amap_LibertyGetString( Amap_Tree_t * p, Amap_Pair_t Pair )   
{ 
    static char Buffer[ABC_MAX_LIB_STR_LEN]; 
    assert( Pair.End-Pair.Beg < ABC_MAX_LIB_STR_LEN );
    strncpy( Buffer, p->pContents+Pair.Beg, Pair.End-Pair.Beg ); 
    Buffer[Pair.End-Pair.Beg] = 0;
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Gets the name to write.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Amap_LibertyGetStringFormula( Amap_Tree_t * p, Amap_Pair_t Pair )   
{ 
    static char Buffer[ABC_MAX_LIB_STR_LEN]; 
    assert( Pair.End-Pair.Beg-2 < ABC_MAX_LIB_STR_LEN );
    strncpy( Buffer, p->pContents+Pair.Beg+1, Pair.End-Pair.Beg-2 ); 
    Buffer[Pair.End-Pair.Beg-2] = 0;
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Prints parse tree in Genlib format.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibertyPrintGenlib( Amap_Tree_t * p, char * pFileName, int fVerbose )
{
    FILE * pFile;
    Vec_Ptr_t * vOutputs;
    Amap_Item_t * pCell, * pArea, * pFunc, * pPin, * pOutput;
    char * pForm;
    int i, Counter;
    if ( pFileName == NULL )
        pFile = stdout;
    else
    {
        pFile = fopen( pFileName, "w" );
        if ( pFile == NULL )
        {
            printf( "Amap_LibertyPrintGenlib(): The output file is unavailable (absent or open).\n" );
            return 0;
        }
    }
    fprintf( pFile, "# This Genlib file was generated by ABC on %s\n", Amap_LibertyTimeStamp() );
    fprintf( pFile, "# The standard cell library \"%s\" is from Liberty file \"%s\"\n", Amap_LibertyGetString(p, Amap_LibertyRoot(p)->Head), p->pFileName );
    fprintf( pFile, "# (To find out more about Genlib format, google for \"sis_paper.ps\")\n" );

    fprintf( pFile, "GATE  " );
    fprintf( pFile, "%16s  ", "_const0_" );
    fprintf( pFile, "%f  ",   0.0 );
    fprintf( pFile, "%s=",    "z" );
    fprintf( pFile, "%s;\n",  "CONST0" );

    fprintf( pFile, "GATE  " );
    fprintf( pFile, "%16s  ", "_const1_" );
    fprintf( pFile, "%f  ",   0.0 );
    fprintf( pFile, "%s=",    "z" );
    fprintf( pFile, "%s;\n",  "CONST1" );

    Amap_ItemForEachChild( p, Amap_LibertyRoot(p), pCell )
    {
/*
    if ( strcmp(Amap_LibertyGetString(p, pCell->Head), "HA1SVTX1") == 0 )
    {
        int s = 0;
    }
*/
        if ( Amap_LibertyCompare(p, pCell->Key, "cell") )
            continue;
        if ( Amap_LibertyCellIsFlop(p, pCell) )
        {
            if ( fVerbose )
                printf( "Amap_LibertyPrintGenlib() skipped sequential cell \"%s\".\n", Amap_LibertyGetString(p, pCell->Head) );
            continue;
        }
        Counter = Amap_LibertyCellCountOutputs( p, pCell );
        if ( Counter == 0 )
        {
            if ( fVerbose )
                printf( "Amap_LibertyPrintGenlib() skipped cell \"%s\" without logic function.\n", Amap_LibertyGetString(p, pCell->Head) );
            continue;
        }
/*
        if ( Counter > 1 )
        {
            if ( fVerbose )
                printf( "Amap_LibertyPrintGenlib() skipped multi-output cell \"%s\".\n", Amap_LibertyGetString(p, pCell->Head) );
            continue;
        }
*/
        pArea = Amap_LibertyCellArea( p, pCell );
        if ( pArea == NULL )
        {
            if ( fVerbose )
                printf( "Amap_LibertyPrintGenlib() skipped cell \"%s\" with unspecified area.\n", Amap_LibertyGetString(p, pCell->Head) );
            continue;
        }
//        pOutput = Amap_LibertyCellOutput( p, pCell );
        vOutputs = Amap_LibertyCellOutputs( p, pCell );
        Vec_PtrForEachEntry( Amap_Item_t *, vOutputs, pOutput, i )
        {
            pFunc   = Amap_LibertyPinFunction( p, pOutput );
            pForm   = Amap_LibertyGetStringFormula( p, pFunc->Head );
            if ( !strcmp(pForm, "0") || !strcmp(pForm, "1") )
            {
                if ( fVerbose )
                    printf( "Amap_LibertyPrintGenlib() skipped cell \"%s\" with constant formula \"%s\".\n", Amap_LibertyGetString(p, pCell->Head), pForm );
                continue;
            }
            fprintf( pFile, "GATE  " );
            fprintf( pFile, "%16s  ", Amap_LibertyGetString(p, pCell->Head) );
            fprintf( pFile, "%f  ",   atof(Amap_LibertyGetString(p, pArea->Head)) );
            fprintf( pFile, "%s=",    Amap_LibertyGetString(p, pOutput->Head) );
            fprintf( pFile, "%s;\n",  Amap_LibertyGetStringFormula(p, pFunc->Head) );
            Amap_ItemForEachChild( p, pCell, pPin )
                if ( Vec_PtrFind(vOutputs, pPin) == -1 && !Amap_LibertyCompare(p, pPin->Key, "pin") )
                    fprintf( pFile, "    PIN  %13s  UNKNOWN  1  999  1.00  0.00  1.00  0.00\n", Amap_LibertyGetString(p, pPin->Head) );
        }
        Vec_PtrFree( vOutputs );
    }
    if ( pFile != stdout )
        fclose( pFile );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prints parse tree in Genlib format.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Amap_LibertyPrintGenlibStr( Amap_Tree_t * p, int fVerbose )
{
    Vec_Str_t * vStr;
    char Buffer[100];
    Vec_Ptr_t * vOutputs;
    Amap_Item_t * pCell, * pArea, * pFunc, * pPin, * pOutput;
    int i, Counter;
    char * pForm;

    vStr = Vec_StrAlloc( 1000 );

    Vec_StrPrintStr( vStr, "GATE          _const0_  0.000000  z=CONST0;\n" );
    Vec_StrPrintStr( vStr, "GATE          _const1_  0.000000  z=CONST1;\n" );
    Amap_ItemForEachChild( p, Amap_LibertyRoot(p), pCell )
    {
        if ( Amap_LibertyCompare(p, pCell->Key, "cell") )
            continue;
        if ( Amap_LibertyCellIsFlop(p, pCell) )
        {
            if ( fVerbose )
                printf( "Amap_LibertyPrintGenlib() skipped sequential cell \"%s\".\n", Amap_LibertyGetString(p, pCell->Head) );
            continue;
        }
        Counter = Amap_LibertyCellCountOutputs( p, pCell );
        if ( Counter == 0 )
        {
            if ( fVerbose )
                printf( "Amap_LibertyPrintGenlib() skipped cell \"%s\" without logic function.\n", Amap_LibertyGetString(p, pCell->Head) );
            continue;
        }
        pArea = Amap_LibertyCellArea( p, pCell );
        if ( pArea == NULL )
        {
            if ( fVerbose )
                printf( "Amap_LibertyPrintGenlib() skipped cell \"%s\" with unspecified area.\n", Amap_LibertyGetString(p, pCell->Head) );
            continue;
        }
        vOutputs = Amap_LibertyCellOutputs( p, pCell );
        Vec_PtrForEachEntry( Amap_Item_t *, vOutputs, pOutput, i )
        {
            pFunc   = Amap_LibertyPinFunction( p, pOutput );
            pForm   = Amap_LibertyGetStringFormula( p, pFunc->Head );
            if ( !strcmp(pForm, "0") || !strcmp(pForm, "1") )
            {
                if ( fVerbose )
                    printf( "Amap_LibertyPrintGenlib() skipped cell \"%s\" with constant formula \"%s\".\n", Amap_LibertyGetString(p, pCell->Head), pForm );
                continue;
            }
/*
            fprintf( pFile, "GATE  " );
            fprintf( pFile, "%16s  ", Amap_LibertyGetString(p, pCell->Head) );
            fprintf( pFile, "%f  ",   atof(Amap_LibertyGetString(p, pArea->Head)) );
            fprintf( pFile, "%s=",    Amap_LibertyGetString(p, pOutput->Head) );
            fprintf( pFile, "%s;\n",  Amap_LibertyGetStringFormula(p, pFunc->Head) );
            Amap_ItemForEachChild( p, pCell, pPin )
                if ( Vec_PtrFind(vOutputs, pPin) == -1 && !Amap_LibertyCompare(p, pPin->Key, "pin") )
                    fprintf( pFile, "    PIN  %13s  UNKNOWN  1  999  1.00  0.00  1.00  0.00\n", Amap_LibertyGetString(p, pPin->Head) );
*/
            Vec_StrPrintStr( vStr, "GATE " );
            Vec_StrPrintStr( vStr, Amap_LibertyGetString(p, pCell->Head) );
            Vec_StrPrintStr( vStr, " " );
            sprintf( Buffer, "%f", atof(Amap_LibertyGetString(p, pArea->Head)) );
            Vec_StrPrintStr( vStr, Buffer );
            Vec_StrPrintStr( vStr, " " );
            Vec_StrPrintStr( vStr, Amap_LibertyGetString(p, pOutput->Head) );
            Vec_StrPrintStr( vStr, "=" );
            Vec_StrPrintStr( vStr, Amap_LibertyGetStringFormula(p, pFunc->Head) );
            Vec_StrPrintStr( vStr, ";\n" );
            Amap_ItemForEachChild( p, pCell, pPin )
                if ( Vec_PtrFind(vOutputs, pPin) == -1 && !Amap_LibertyCompare(p, pPin->Key, "pin") )
                {
                    Vec_StrPrintStr( vStr, "  PIN " );
                    Vec_StrPrintStr( vStr, Amap_LibertyGetString(p, pPin->Head) );
                    Vec_StrPrintStr( vStr, " UNKNOWN  1  999  1.00  0.00  1.00  0.00\n" );
                }
        }
        Vec_PtrFree( vOutputs );
    }
    Vec_StrPrintStr( vStr, "\n.end\n" );
    Vec_StrPush( vStr, '\0' );
//    printf( "%s", Vec_StrArray(vStr) );
    return vStr;
}


/**Function*************************************************************

  Synopsis    [Returns the file size.]

  Description [The file should be closed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibertyFileSize( char * pFileName )
{
    FILE * pFile;
    int nFileSize;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Amap_LibertyFileSize(): The input file is unavailable (absent or open).\n" );
        return 0;
    }
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile ); 
    fclose( pFile );
    return nFileSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_LibertyFixFileHead( char * pFileName )
{
    char * pHead;
    for ( pHead = pFileName; *pHead; pHead++ )
        if ( *pHead == '>' )
            *pHead = '\\';
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibertyCountItems( char * pBeg, char * pEnd )
{
    int Counter = 0;
    for ( ; pBeg < pEnd; pBeg++ )
        Counter += (*pBeg == '(' || *pBeg == ':');
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Removes C-style comments.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_LibertyWipeOutComments( char * pBeg, char * pEnd )
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

/**Function*************************************************************

  Synopsis    [Returns 1 if the character is space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Amap_LibertyCharIsSpace( char c )
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\\';
}

/**Function*************************************************************

  Synopsis    [Skips spaces.]

  Description [Returns 1 if reached the end.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Amap_LibertySkipSpaces( Amap_Tree_t * p, char ** ppPos, char * pEnd, int fStopAtNewLine )
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
        if ( !Amap_LibertyCharIsSpace(*pPos) )
            break;
    }
    *ppPos = pPos;
    return pPos == pEnd;
}

/**Function*************************************************************

  Synopsis    [Skips entry delimited by " :;(){}" ]

  Description [Returns 1 if reached the end.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Amap_LibertySkipEntry( char ** ppPos, char * pEnd )
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

/**Function*************************************************************

  Synopsis    [Find the matching closing symbol.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char * Amap_LibertyFindMatch( char * pPos, char * pEnd )
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

/**Function*************************************************************

  Synopsis    [Find the matching closing symbol.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Amap_Pair_t Amap_LibertyUpdateHead( Amap_Tree_t * p, Amap_Pair_t Head )
{
    Amap_Pair_t Res;
    char * pBeg = p->pContents + Head.Beg;
    char * pEnd = p->pContents + Head.End;
    char * pFirstNonSpace = NULL;
    char * pLastNonSpace = NULL;
    char * pChar;
    for ( pChar = pBeg; pChar < pEnd; pChar++ )
    {
        if ( *pChar == '\n' )
            p->nLines++;
        if ( Amap_LibertyCharIsSpace(*pChar) )
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

/**Function*************************************************************

  Synopsis    [Returns free item.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Amap_Item_t * Amap_LibertyNewItem( Amap_Tree_t * p, int Type )
{
    p->pItems[p->nItems].iLine = p->nLines;
    p->pItems[p->nItems].Type  = Type;
    p->pItems[p->nItems].Child = -1;
    p->pItems[p->nItems].Next  = -1;
    return p->pItems + p->nItems++;
}

/**Function*************************************************************

  Synopsis    [Returns free item.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibertyBuildItem( Amap_Tree_t * p, char ** ppPos, char * pEnd )
{
    Amap_Item_t * pItem;
    Amap_Pair_t Key, Head, Body;
    char * pNext, * pStop;
    Key.End = 0;
    if ( Amap_LibertySkipSpaces( p, ppPos, pEnd, 0 ) )
        return -2;
    Key.Beg = *ppPos - p->pContents;
    if ( Amap_LibertySkipEntry( ppPos, pEnd ) )
        goto exit;
    Key.End = *ppPos - p->pContents;
    if ( Amap_LibertySkipSpaces( p, ppPos, pEnd, 0 ) )
        goto exit;
    pNext = *ppPos;
    if ( *pNext == ':' )
    {
        *ppPos = pNext + 1;
        if ( Amap_LibertySkipSpaces( p, ppPos, pEnd, 0 ) )
            goto exit;
        Head.Beg = *ppPos - p->pContents;
        if ( Amap_LibertySkipEntry( ppPos, pEnd ) )
            goto exit;
        Head.End = *ppPos - p->pContents;
        if ( Amap_LibertySkipSpaces( p, ppPos, pEnd, 1 ) )
            goto exit;
        pNext = *ppPos;
        if ( *pNext != ';' && *pNext != '\n' )
            goto exit;
        *ppPos = pNext + 1;
        // end of equation
        pItem = Amap_LibertyNewItem( p, AMAP_LIBERTY_EQUA );
        pItem->Key  = Key;
        pItem->Head = Amap_LibertyUpdateHead( p, Head );
        pItem->Next = Amap_LibertyBuildItem( p, ppPos, pEnd );
        if ( pItem->Next == -1 )
            goto exit;
        return Amap_LibertyItemId( p, pItem );
    }
    if ( *pNext == '(' )
    {
        pStop = Amap_LibertyFindMatch( pNext, pEnd );
        Head.Beg = pNext - p->pContents + 1;
        Head.End = pStop - p->pContents;
        *ppPos = pStop + 1;
        if ( Amap_LibertySkipSpaces( p, ppPos, pEnd, 0 ) )
        {
            // end of list
            pItem = Amap_LibertyNewItem( p, AMAP_LIBERTY_LIST );
            pItem->Key  = Key;
            pItem->Head = Amap_LibertyUpdateHead( p, Head );
            return Amap_LibertyItemId( p, pItem );
        }
        pNext = *ppPos;
        if ( *pNext == '{' ) // beginning of body
        {
            pStop = Amap_LibertyFindMatch( pNext, pEnd );
            Body.Beg = pNext - p->pContents + 1;
            Body.End = pStop - p->pContents;
            // end of body
            pItem = Amap_LibertyNewItem( p, AMAP_LIBERTY_PROC );
            pItem->Key  = Key;
            pItem->Head = Amap_LibertyUpdateHead( p, Head );
            pItem->Body = Body;
            *ppPos = pNext + 1;
            pItem->Child = Amap_LibertyBuildItem( p, ppPos, pStop );
            if ( pItem->Child == -1 )
                goto exit;
            *ppPos = pStop + 1;
            pItem->Next = Amap_LibertyBuildItem( p, ppPos, pEnd );
            if ( pItem->Next == -1 )
                goto exit;
            return Amap_LibertyItemId( p, pItem );
        }
        // end of list
        if ( *pNext == ';' )
            *ppPos = pNext + 1;
        pItem = Amap_LibertyNewItem( p, AMAP_LIBERTY_LIST );
        pItem->Key  = Key;
        pItem->Head = Amap_LibertyUpdateHead( p, Head );
        pItem->Next = Amap_LibertyBuildItem( p, ppPos, pEnd );
        if ( pItem->Next == -1 )
            goto exit;
        return Amap_LibertyItemId( p, pItem );
    }
exit:
    if ( p->pError == NULL )
    {
        p->pError = ABC_ALLOC( char, 1000 );
        sprintf( p->pError, "File \"%s\". Line %6d. Failed to parse entry \"%s\".\n", 
            p->pFileName, p->nLines, Amap_LibertyGetString(p, Key) );
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Starts the parsing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Tree_t * Amap_LibertyStart( char * pFileName )
{
    FILE * pFile;
    Amap_Tree_t * p;
    int RetValue;
    // start the manager
    p = ABC_ALLOC( Amap_Tree_t, 1 );
    memset( p, 0, sizeof(Amap_Tree_t) );
    // read the file into the buffer
    Amap_LibertyFixFileHead( pFileName );
    p->nContents = Amap_LibertyFileSize( pFileName );
    if ( p->nContents == 0 )
    {
        ABC_FREE( p );
        return NULL;
    }
    pFile = fopen( pFileName, "rb" );
    p->pContents = ABC_ALLOC( char, p->nContents+1 );
    RetValue = fread( p->pContents, p->nContents, 1, pFile );
    fclose( pFile );
    p->pContents[p->nContents] = 0;
    // other 
    p->pFileName = Abc_UtilStrsav( pFileName );
    p->nItermAlloc = 10 + Amap_LibertyCountItems( p->pContents, p->pContents+p->nContents );
    p->pItems = ABC_CALLOC( Amap_Item_t, p->nItermAlloc );
    p->nItems = 0;
    p->nLines = 1;
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the parsing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_LibertyStop( Amap_Tree_t * p )
{
    ABC_FREE( p->pFileName );
    ABC_FREE( p->pContents );
    ABC_FREE( p->pItems );
    ABC_FREE( p->pError );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Parses the standard cell library in Liberty format.]

  Description [Writes the resulting file in Genlib format.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibertyParse( char * pFileName, int fVerbose )
{
    Amap_Tree_t * p;
    char * pPos;
    abctime clk = Abc_Clock();
    int RetValue;
    p = Amap_LibertyStart( pFileName );
    if ( p == NULL )
        return 0;
    pPos = p->pContents;
    Amap_LibertyWipeOutComments( p->pContents, p->pContents+p->nContents );
    if ( Amap_LibertyBuildItem( p, &pPos, p->pContents + p->nContents ) == 0 )
    {
        if ( fVerbose )
        printf( "Parsing finished successfully.\n" );
//        Amap_LibertyPrintLiberty( p, "temp_.lib" );
        Amap_LibertyPrintGenlib( p, Extra_FileNameGenericAppend(pFileName, ".genlib"), fVerbose );
        RetValue = 1;
    }
    else
    {
        if ( p->pError )
            printf( "%s", p->pError );
        if ( fVerbose )
        printf( "Parsing failed.\n" );
        RetValue = 0;
    }
    if ( fVerbose )
    {
    printf( "Memory = %7.2f MB. ", 1.0*(p->nContents+p->nItermAlloc*sizeof(Amap_Item_t))/(1<<20) );
    ABC_PRT( "Time", Abc_Clock() - clk );
    }
    Amap_LibertyStop( p );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Parses the standard cell library in Liberty format.]

  Description [Writes the resulting file in Genlib format.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Amap_LibertyParseStr( char * pFileName, int fVerbose )
{
    Amap_Tree_t * p;
    Vec_Str_t * vStr = NULL;
    char * pPos;
    abctime clk = Abc_Clock();
    int RetValue;
    p = Amap_LibertyStart( pFileName );
    if ( p == NULL )
        return 0;
    pPos = p->pContents;
    Amap_LibertyWipeOutComments( p->pContents, p->pContents+p->nContents );
    if ( Amap_LibertyBuildItem( p, &pPos, p->pContents + p->nContents ) == 0 )
    {
        if ( fVerbose )
        printf( "Parsing finished successfully.\n" );
//        Amap_LibertyPrintLiberty( p, "temp_.lib" );
        vStr = Amap_LibertyPrintGenlibStr( p, fVerbose );
        RetValue = 1;
    }
    else
    {
        if ( p->pError )
            printf( "%s", p->pError );
        if ( fVerbose )
        printf( "Parsing failed.\n" );
        RetValue = 0;
    }
    if ( fVerbose )
    {
    printf( "Memory = %7.2f MB. ", 1.0*(p->nContents+p->nItermAlloc*sizeof(Amap_Item_t))/(1<<20) );
    ABC_PRT( "Time", Abc_Clock() - clk );
    }
    Amap_LibertyStop( p );
    return vStr;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

