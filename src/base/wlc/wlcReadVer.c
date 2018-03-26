/**CFile****************************************************************

  FileName    [wlcReadVer.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Parses several flavors of word-level Verilog.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcReadVer.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// Word-level Verilog file parser
#define WLV_PRS_MAX_LINE   10000

typedef struct Wlc_Prs_t_  Wlc_Prs_t;
struct Wlc_Prs_t_ 
{
    int                    nFileSize;
    char *                 pFileName;
    char *                 pBuffer;
    Vec_Int_t *            vLines;
    Vec_Int_t *            vStarts;
    Vec_Int_t *            vFanins;
    Wlc_Ntk_t *            pNtk;
    Mem_Flex_t *           pMemTable;
    Vec_Ptr_t *            vTables;
    int                    nConsts;
    int                    nNonZero[4];
    int                    nNegative[4];
    int                    nReverse[4];
    char                   sError[WLV_PRS_MAX_LINE];
};

static inline int          Wlc_PrsOffset( Wlc_Prs_t * p, char * pStr )  { return pStr - p->pBuffer;                     }
static inline char *       Wlc_PrsStr( Wlc_Prs_t * p, int iOffset )     { return p->pBuffer + iOffset;                  }
static inline int          Wlc_PrsStrCmp( char * pStr, char * pWhat )   { return !strncmp( pStr, pWhat, strlen(pWhat)); }

#define Wlc_PrsForEachLine( p, pLine, i )             \
    for ( i = 0; (i < Vec_IntSize((p)->vStarts)) && ((pLine) = Wlc_PrsStr(p, Vec_IntEntry((p)->vStarts, i))); i++ )
#define Wlc_PrsForEachLineStart( p, pLine, i, Start ) \
    for ( i = Start; (i < Vec_IntSize((p)->vStarts)) && ((pLine) = Wlc_PrsStr(p, Vec_IntEntry((p)->vStarts, i))); i++ )


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wlc_Prs_t * Wlc_PrsStart( char * pFileName, char * pStr )
{
    Wlc_Prs_t * p;
    if ( pFileName && !Extra_FileCheck( pFileName ) )
        return NULL;
    p = ABC_CALLOC( Wlc_Prs_t, 1 );
    p->pFileName = pFileName;
    p->pBuffer   = pStr ? Abc_UtilStrsav(pStr) : Extra_FileReadContents( pFileName );
    p->nFileSize = strlen(p->pBuffer);  assert( p->nFileSize > 0 );
    p->vLines    = Vec_IntAlloc( p->nFileSize / 50 );
    p->vStarts   = Vec_IntAlloc( p->nFileSize / 50 );
    p->vFanins   = Vec_IntAlloc( 100 );
    p->vTables   = Vec_PtrAlloc( 1000 );
    p->pMemTable = Mem_FlexStart();
    return p;
}
void Wlc_PrsStop( Wlc_Prs_t * p )
{
    if ( p->pNtk )
        Wlc_NtkFree( p->pNtk );
    if ( p->pMemTable )
        Mem_FlexStop( p->pMemTable, 0 );
    Vec_PtrFreeP( &p->vTables );
    Vec_IntFree( p->vLines );
    Vec_IntFree( p->vStarts );
    Vec_IntFree( p->vFanins );
    ABC_FREE( p->pBuffer );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prints the error message including the file name and line number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_PrsFindLine( Wlc_Prs_t * p, char * pCur )
{
    int Entry, iLine = 0;
    Vec_IntForEachEntry( p->vLines, Entry, iLine )
        if ( Entry > pCur - p->pBuffer )
            return iLine + 1;
    return -1;
}
int Wlc_PrsWriteErrorMessage( Wlc_Prs_t * p, char * pCur, const char * format, ... )
{
    char * pMessage;
    // derive message
    va_list args;
    va_start( args, format );
    pMessage = vnsprintf( format, args );
    va_end( args );
    // print messsage
    assert( strlen(pMessage) < WLV_PRS_MAX_LINE - 100 );
    assert( p->sError[0] == 0 );
    if ( pCur == NULL ) // the line number is not given
        sprintf( p->sError, "%s: %s\n", p->pFileName, pMessage );
    else                // print the error message with the line number
    {
        int iLine = Wlc_PrsFindLine( p, pCur );
        sprintf( p->sError, "%s (line %d): %s\n", p->pFileName, iLine, pMessage );
    }
    ABC_FREE( pMessage );
    return 0;
}
void Wlc_PrsPrintErrorMessage( Wlc_Prs_t * p )
{
    if ( p->sError[0] == 0 )
        return;
    fprintf( stdout, "%s", p->sError );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Wlc_PrsIsDigit( char * pStr )
{
    return (pStr[0] >= '0' && pStr[0] <= '9');
}
static inline int Wlc_PrsIsChar( char * pStr )
{
    return (pStr[0] >= 'a' && pStr[0] <= 'z') || 
           (pStr[0] >= 'A' && pStr[0] <= 'Z') || 
           (pStr[0] >= '0' && pStr[0] <= '9') || 
            pStr[0] == '_' || pStr[0] == '$' || pStr[0] == '\\';
}
static inline char * Wlc_PrsSkipSpaces( char * pStr )
{
    while ( *pStr && *pStr == ' ' )
        pStr++;
    return pStr;
}
static inline char * Wlc_PrsFindSymbol( char * pStr, char Symb )
{
    int fNotName = 1;
    for ( ; *pStr; pStr++ )
    {
        if ( fNotName && *pStr == Symb )
            return pStr;
        if ( pStr[0] == '\\' )
            fNotName = 0;
        else if ( !fNotName && *pStr == ' ' )
            fNotName = 1;
    }
    return NULL;
}
static inline char * Wlc_PrsFindSymbolTwo( char * pStr, char Symb, char Symb2 )
{
    for ( ; pStr[1]; pStr++ )
        if ( pStr[0] == Symb  && pStr[1] == Symb2 )
            return pStr;
    return NULL;
}
static inline char * Wlc_PrsFindClosingParenthesis( char * pStr, char Open, char Close )
{
    int Counter = 0;
    int fNotName = 1;
    assert( *pStr == Open );
    for ( ; *pStr; pStr++ )
    {
        if ( fNotName )
        {
            if ( *pStr == Open )
                Counter++;
            if ( *pStr == Close )
                Counter--;
            if ( Counter == 0 )
                return pStr;
        }
        if ( *pStr == '\\' )
            fNotName = 0;
        else if ( !fNotName && *pStr == ' ' )
            fNotName = 1;
    }
    return NULL;
}
int Wlc_PrsRemoveComments( Wlc_Prs_t * p )
{
    int fSpecifyFound = 0;
    char * pCur, * pNext, * pEnd = p->pBuffer + p->nFileSize;
    for ( pCur = p->pBuffer; pCur < pEnd; pCur++ )
    {
        // regular comment (//)
        if ( *pCur == '/' && pCur[1] == '/' )
        {
            if ( pCur + 5 < pEnd && pCur[2] == 'a' && pCur[3] == 'b' && pCur[4] == 'c' && pCur[5] == '2' )
                pCur[0] = pCur[1] = pCur[2] = pCur[3] = pCur[4] = pCur[5] = ' ';
            else
            {
                pNext = Wlc_PrsFindSymbol( pCur, '\n' );
                if ( pNext == NULL )
                    return Wlc_PrsWriteErrorMessage( p, pCur, "Cannot find end-of-line after symbols \"//\"." );
                for ( ; pCur < pNext; pCur++ )
                    *pCur = ' ';
            }
        }
        // skip preprocessor directive (`timescale, `celldefine, etc)
        else if ( *pCur == '`' )
        {
            pNext = Wlc_PrsFindSymbol( pCur, '\n' );
            if ( pNext == NULL )
                return Wlc_PrsWriteErrorMessage( p, pCur, "Cannot find end-of-line after symbols \"`\"." );
            for ( ; pCur < pNext; pCur++ )
                *pCur = ' ';
        }
        // regular comment (/* ... */)
        else if ( *pCur == '/' && pCur[1] == '*' )
        {
            pNext = Wlc_PrsFindSymbolTwo( pCur, '*', '/' );
            if ( pNext == NULL )
                return Wlc_PrsWriteErrorMessage( p, pCur, "Cannot find symbols \"*/\" after symbols \"/*\"." );
            // overwrite comment
            for ( ; pCur < pNext + 2; pCur++ )
                *pCur = ' ';
        }
        // 'specify' treated as comments
        else if ( *pCur == 's' && pCur[1] == 'p' && pCur[2] == 'e' && !strncmp(pCur, "specify", 7) )
        {
            for ( pNext = pCur; pNext < pEnd - 10; pNext++ )
                if ( *pNext == 'e' && pNext[1] == 'n' && pNext[2] == 'd' && !strncmp(pNext, "endspecify", 10) )
                {
                    // overwrite comment
                    for ( ; pCur < pNext + 10; pCur++ )
                        *pCur = ' ';
                    if ( fSpecifyFound == 0 )
                        Abc_Print( 0, "Ignoring specify/endspecify directives.\n" );
                    fSpecifyFound = 1;
                    break;
                }
        }
        // insert semicolons
        else if ( *pCur == 'e' && pCur[1] == 'n' && pCur[2] == 'd' && !strncmp(pCur, "endmodule", 9) )
            pCur[strlen("endmodule")] = ';';
        // overwrite end-of-lines with spaces (less checking to do later on)
        if ( *pCur == '\n' || *pCur == '\r'  || *pCur == '\t' )
            *pCur = ' ';
    }
    return 1;
}
int Wlc_PrsPrepare( Wlc_Prs_t * p )
{
    int fPrettyPrint = 0;
    int fNotName = 1;
    char * pTemp, * pPrev, * pThis;
    // collect info about lines
    assert( Vec_IntSize(p->vLines) == 0 );
    for ( pTemp = p->pBuffer; *pTemp; pTemp++ )
        if ( *pTemp == '\n' )
            Vec_IntPush( p->vLines, pTemp - p->pBuffer );
    // delete comments and insert breaks
    if ( !Wlc_PrsRemoveComments( p ) )
        return 0;
    // collect info about breaks
    assert( Vec_IntSize(p->vStarts) == 0 );
    for ( pPrev = pThis = p->pBuffer; *pThis; pThis++ )
    {
        if ( fNotName && *pThis == ';' )
        {
            *pThis = 0;
            Vec_IntPush( p->vStarts, Wlc_PrsOffset(p, Wlc_PrsSkipSpaces(pPrev)) );
            pPrev = pThis + 1;
        }
        if ( *pThis == '\\' )
            fNotName = 0;
        else if ( !fNotName && *pThis == ' ' )
            fNotName = 1;
    }

    if ( fPrettyPrint )
    {
        int i, k;
        // print the line types
        Wlc_PrsForEachLine( p, pTemp, i )
        {
            if ( Wlc_PrsStrCmp( pTemp, "module" ) )
                printf( "\n" );
            if ( !Wlc_PrsStrCmp( pTemp, "module" ) && !Wlc_PrsStrCmp( pTemp, "endmodule" ) )
                printf( "    " );
            printf( "%c", pTemp[0] );
            for ( k = 1; pTemp[k]; k++ )
                if ( pTemp[k] != ' ' || pTemp[k-1] != ' ' )
                    printf( "%c", pTemp[k] );
            printf( ";\n" );
        }
/*    
        // print the line types
        Wlc_PrsForEachLine( p, pTemp, i )
        {
            int k;  
            if ( !Wlc_PrsStrCmp( pTemp, "module" ) )
                continue;
            printf( "%3d : ", i );
            for ( k = 0; k < 40; k++ )
                printf( "%c", pTemp[k] ? pTemp[k] : ' ' );
            printf( "\n" );
        }
*/
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Modified version of strtok().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Wlc_PrsStrtok( char * s, const char * delim )
{
  const char *spanp;
  int c, sc;
  char *tok;
  static char *last;
  if (s == NULL && (s = last) == NULL)
      return NULL;
  // skip leading delimiters
cont:
  c = *s++;
  for (spanp = delim; (sc = *spanp++) != 0;) 
      if (c == sc)
          goto cont;
  if (c == 0)    // no non-delimiter characters 
      return (last = NULL);
//  tok = s - 1;
  if ( c != '\\' )
      tok = s - 1;
  else
      tok = s - 1;
  // go back to the first non-delimiter character
  s--;
  // find the token
  for (;;) 
  {
      c = *s++;
      if ( c == '\\' )  // skip blind characters
      {
          while ( c != ' ' )
              c = *s++;
          c = *s++;
      }
      spanp = delim;
      do {
          if ((sc = *spanp++) == c) 
          {
              if (c == 0)
                  s = NULL;
              else
                  s[-1] = 0;
              last = s;
              return (tok);
          }
      } while (sc != 0);
  }
  // not reached
  return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Wlc_PrsConvertInitValues( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj; 
    int i, k, Value, * pInits;
    char * pResult;
    Vec_Str_t * vStr = Vec_StrAlloc( 1000 );
    Vec_IntForEachEntry( p->vInits, Value, i )
    {
        if ( Value < 0 )
        {
            for ( k = 0; k < -Value; k++ )
                Vec_StrPush( vStr, '0' );
            continue;
        }
        pObj = Wlc_NtkObj( p, Value );
        Value = Wlc_ObjRange(pObj);
        while ( pObj->Type == WLC_OBJ_BUF )
            pObj = Wlc_NtkObj( p, Wlc_ObjFaninId0(pObj) );
        pInits = (pObj->Type == WLC_OBJ_CONST && !pObj->fXConst) ? Wlc_ObjConstValue(pObj) : NULL;
        for ( k = 0; k < Abc_MinInt(Value, Wlc_ObjRange(pObj)); k++ )
            Vec_StrPush( vStr, (char)(pInits ? '0' + Abc_InfoHasBit((unsigned *)pInits, k) : 'X') );
        // extend values with zero, in case the init value signal has different range compared to constant used
        for ( ; k < Value; k++ )
            Vec_StrPush( vStr, '0' );
        // update vInits to contain either number of values or PI index
        Vec_IntWriteEntry( p->vInits, i, (pInits || pObj->fXConst) ? -Value : Wlc_ObjCiId(pObj) );
    }
    Vec_StrPush( vStr, '\0' );
    pResult = Vec_StrReleaseArray( vStr );
    Vec_StrFree( vStr );
    return pResult;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline char * Wlc_PrsFindRange( char * pStr, int * End, int * Beg )
{
    *End = *Beg = 0;
    pStr = Wlc_PrsSkipSpaces( pStr );
    if ( pStr[0] != '[' )
        return pStr;
    pStr = Wlc_PrsSkipSpaces( pStr+1 );
    if ( !Wlc_PrsIsDigit(pStr) && pStr[0] != '-' )
        return NULL;
    *End = *Beg = atoi( pStr );
    if ( Wlc_PrsFindSymbol( pStr, ':' ) == NULL )
    {
        pStr = Wlc_PrsFindSymbol( pStr, ']' );
        if ( pStr == NULL )
            return NULL;
    }
    else
    {
        pStr = Wlc_PrsFindSymbol( pStr, ':' );
        pStr = Wlc_PrsSkipSpaces( pStr+1 );
        if ( !Wlc_PrsIsDigit(pStr) && pStr[0] != '-' )
            return NULL;
        *Beg = atoi( pStr );
        pStr = Wlc_PrsFindSymbol( pStr, ']' );
        if ( pStr == NULL )
            return NULL;
    }
    return pStr + 1;
}
static inline char * Wlc_PrsFindWord( char * pStr, char * pWord, int * fFound )
{
    *fFound = 0;
    pStr = Wlc_PrsSkipSpaces( pStr );
    if ( !Wlc_PrsStrCmp(pStr, pWord) )
        return pStr;
    *fFound = 1;
    return pStr + strlen(pWord);
}
static inline char * Wlc_PrsFindName( char * pStr, char ** ppPlace )
{
    static char Buffer[WLV_PRS_MAX_LINE];
    char * pThis = *ppPlace = Buffer;
    int fNotName = 1;
    pStr = Wlc_PrsSkipSpaces( pStr );
    if ( !Wlc_PrsIsChar(pStr) )
        return NULL;
//    while ( Wlc_PrsIsChar(pStr) )
//        *pThis++ = *pStr++;
    while ( *pStr )
    {
        if ( fNotName && !Wlc_PrsIsChar(pStr) )
            break;
        if ( *pStr == '\\' )
            fNotName = 0;
        else if ( !fNotName && *pStr == ' ' )
            fNotName = 1;
        *pThis++ = *pStr++;
    }
    *pThis = 0;
    return pStr;
}
static inline char * Wlc_PrsReadConstant( Wlc_Prs_t * p, char * pStr, Vec_Int_t * vFanins, int * pRange, int * pSigned, int * pXValue )
{
    int i, nDigits, nBits = atoi( pStr );
    *pRange = -1;
    *pSigned = 0;
    *pXValue = 0;
    pStr = Wlc_PrsSkipSpaces( pStr );
    if ( Wlc_PrsFindSymbol( pStr, '\'' ) == NULL )
    {
        // handle decimal number
        int Number = atoi( pStr );
        *pRange = Abc_Base2Log( Number+1 );
        assert( *pRange < 32 );
        while ( Wlc_PrsIsDigit(pStr) )
            pStr++;
        Vec_IntFill( vFanins, 1, Number );
        return pStr;
    }
    pStr = Wlc_PrsFindSymbol( pStr, '\'' );
    if ( pStr[1] == 's' )
    {
        *pSigned = 1;
        pStr++;
    }
    if ( pStr[1] == 'b' )
    {
        Vec_IntFill( vFanins, Abc_BitWordNum(nBits), 0 );
        for ( i = 0; i < nBits; i++ )
            if ( pStr[2+i] == '1' )
                Abc_InfoSetBit( (unsigned *)Vec_IntArray(vFanins), nBits-1-i );
            else if ( pStr[2+i] != '0' )
                return (char *)(ABC_PTRINT_T)Wlc_PrsWriteErrorMessage( p, pStr, "Wrong digit in binary constant \"%c\".", pStr[2+i] );
        *pRange = nBits;
        pStr += 2 + nBits;
        return pStr;
    }
    if ( pStr[1] != 'h' )
        return (char *)(ABC_PTRINT_T)Wlc_PrsWriteErrorMessage( p, pStr, "Expecting hexadecimal constant and not \"%c\".", pStr[1] );
    *pXValue = (pStr[2] == 'x' || pStr[2] == 'X');
    Vec_IntFill( vFanins, Abc_BitWordNum(nBits), 0 );
    nDigits = Abc_TtReadHexNumber( (word *)Vec_IntArray(vFanins), pStr+2 );
    if ( nDigits != (nBits + 3)/4 )
    {
//        return (char *)(ABC_PTRINT_T)Wlc_PrsWriteErrorMessage( p, pStr, "The length of a constant does not match." );
//        printf( "Warning: The length of a constant (%d hex digits) does not match the number of bits (%d).\n", nDigits, nBits );
    }
    *pRange = nBits;
    pStr += 2;
    while ( Wlc_PrsIsChar(pStr) )
        pStr++;
    return pStr;
}
static inline char * Wlc_PrsReadName( Wlc_Prs_t * p, char * pStr, Vec_Int_t * vFanins )
{
    int NameId, fFound, iObj;
    pStr = Wlc_PrsSkipSpaces( pStr );
    if ( Wlc_PrsIsDigit(pStr) )
    {
        char Buffer[100];
        int Range, Signed, XValue = 0;
        Vec_Int_t * vFanins = Vec_IntAlloc(0);
        pStr = Wlc_PrsReadConstant( p, pStr, vFanins, &Range, &Signed, &XValue );
        if ( pStr == NULL )
        {
            Vec_IntFree( vFanins );
            return 0;
        }
        // create new node
        iObj = Wlc_ObjAlloc( p->pNtk, WLC_OBJ_CONST, Signed, Range-1, 0 );
        Wlc_ObjAddFanins( p->pNtk, Wlc_NtkObj(p->pNtk, iObj), vFanins );
        Wlc_NtkObj(p->pNtk, iObj)->fXConst = XValue;
        Vec_IntFree( vFanins );
        // add node's name
        sprintf( Buffer, "_c%d_", p->nConsts++ );
        NameId = Abc_NamStrFindOrAdd( p->pNtk->pManName, Buffer, &fFound );
        if ( fFound )
            return (char *)(ABC_PTRINT_T)Wlc_PrsWriteErrorMessage( p, pStr, "Name %s is already used.", Buffer );
        assert( iObj == NameId );
    }
    else
    {
        char * pName;
        pStr = Wlc_PrsFindName( pStr, &pName );
        if ( pStr == NULL )
            return (char *)(ABC_PTRINT_T)Wlc_PrsWriteErrorMessage( p, pStr, "Cannot read name in assign-statement." );
        NameId = Abc_NamStrFindOrAdd( p->pNtk->pManName, pName, &fFound );
        if ( !fFound )
            return (char *)(ABC_PTRINT_T)Wlc_PrsWriteErrorMessage( p, pStr, "Name %s is used but not declared.", pName );
    }
    Vec_IntPush( vFanins, NameId );
    return Wlc_PrsSkipSpaces( pStr );
}
static inline int Wlc_PrsFindDefinition( Wlc_Prs_t * p, char * pStr, Vec_Int_t * vFanins, int * pXValue )
{
    char * pName;
    int Type = WLC_OBJ_NONE;
    int fRotating = 0;
    Vec_IntClear( vFanins );
    pStr = Wlc_PrsSkipSpaces( pStr );
    if ( pStr[0] != '=' )
        return 0;
    pStr = Wlc_PrsSkipSpaces( pStr+1 );
    if ( pStr[0] == '(' )
    {
        // consider rotating shifter
        if ( Wlc_PrsFindSymbolTwo(pStr, '>', '>') && Wlc_PrsFindSymbolTwo(pStr, '<', '<') )
        {
            // THIS IS A HACK TO DETECT rotating shifters
            char * pClose = Wlc_PrsFindClosingParenthesis( pStr, '(', ')' );
            if ( pClose == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStr, "Expecting closing parenthesis." );
            *pStr = ' '; *pClose = 0;
            pStr = Wlc_PrsSkipSpaces( pStr );
            fRotating = 1;
        }
        else
        {
            char * pClose = Wlc_PrsFindClosingParenthesis( pStr, '(', ')' );
            if ( pClose == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStr, "Expecting closing parenthesis." );
            *pStr = *pClose = ' ';
            pStr = Wlc_PrsSkipSpaces( pStr );
        }
    }
    if ( Wlc_PrsIsDigit(pStr) )
    {
        int Range, Signed;
        pStr = Wlc_PrsReadConstant( p, pStr, vFanins, &Range, &Signed, pXValue );
        if ( pStr == NULL )
            return 0;
        Type = WLC_OBJ_CONST;
    }
    else if ( pStr[0] == '!' || (pStr[0] == '~' && pStr[1] != '&' && pStr[1] != '|' && pStr[1] != '^') || pStr[0] == '@' || pStr[0] == '#' )
    {
        if ( pStr[0] == '!' )
            Type = WLC_OBJ_LOGIC_NOT;
        else if ( pStr[0] == '~' )
            Type = WLC_OBJ_BIT_NOT;
        else if ( pStr[0] == '@' )
            Type = WLC_OBJ_ARI_SQRT;
        else if ( pStr[0] == '#' )
            Type = WLC_OBJ_ARI_SQUARE;
        else assert( 0 );
        // skip parentheses
        pStr = Wlc_PrsSkipSpaces( pStr+1 );
        if ( pStr[0] == '(' )
        {
            char * pClose = Wlc_PrsFindClosingParenthesis( pStr, '(', ')' );
            if ( pClose == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStr, "Expecting closing parenthesis." );
            *pStr = *pClose = ' ';
        }
        if ( !(pStr = Wlc_PrsReadName(p, pStr, vFanins)) )
            return Wlc_PrsWriteErrorMessage( p, pStr, "Cannot read name after !." );
    }
    else if ( pStr[0] == '-' || 
              pStr[0] == '&' || pStr[0] == '|' || pStr[0] == '^' || 
              (pStr[0] == '~' && pStr[1] == '&') || 
              (pStr[0] == '~' && pStr[1] == '|') || 
              (pStr[0] == '~' && pStr[1] == '^') )
    {
        int shift = 1;
        if ( pStr[0] == '-' )
            Type = WLC_OBJ_ARI_MINUS;
        else if ( pStr[0] == '&' )
            Type = WLC_OBJ_REDUCT_AND;
        else if ( pStr[0] == '|' )
            Type = WLC_OBJ_REDUCT_OR;
        else if ( pStr[0] == '^' )
            Type = WLC_OBJ_REDUCT_XOR;
        else if ( pStr[0] == '~' && pStr[1] == '&' )
            {Type = WLC_OBJ_REDUCT_NAND; shift = 2;}
        else if ( pStr[0] == '~' && pStr[1] == '|' )
            {Type = WLC_OBJ_REDUCT_NOR; shift = 2;}
        else if ( pStr[0] == '~' && pStr[1] == '^' )
            {Type = WLC_OBJ_REDUCT_NXOR; shift = 2;}
        else assert( 0 );
        if ( !(pStr = Wlc_PrsReadName(p, pStr+shift, vFanins)) )
            return Wlc_PrsWriteErrorMessage( p, pStr, "Cannot read name after a unary operator." );
    }
    else if ( pStr[0] == '{' )
    {
        // THIS IS A HACK TO DETECT zero padding AND sign extension
        if ( Wlc_PrsFindSymbol(pStr+1, '{') )
        {
            if ( Wlc_PrsFindSymbol(pStr+1, '\'') )
                Type = WLC_OBJ_BIT_ZEROPAD;
            else
                Type = WLC_OBJ_BIT_SIGNEXT;
            pStr = Wlc_PrsFindSymbol(pStr+1, ',');
            if ( pStr == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStr, "Expecting one comma in this line." );
            if ( !(pStr = Wlc_PrsReadName(p, pStr+1, vFanins)) )
                return Wlc_PrsWriteErrorMessage( p, pStr, "Cannot read name in sign-extension." );
            pStr = Wlc_PrsSkipSpaces( pStr );
            if ( pStr[0] != '}' )
                return Wlc_PrsWriteErrorMessage( p, pStr, "There is no closing brace (})." );
        }
        else // concatenation
        {
            while ( 1 )
            {
                pStr = Wlc_PrsSkipSpaces( pStr+1 );
                if ( !(pStr = Wlc_PrsReadName(p, pStr, vFanins)) )
                    return Wlc_PrsWriteErrorMessage( p, pStr, "Cannot read name in concatenation." );
                if ( pStr[0] == '}' )
                    break;
                if ( pStr[0] != ',' )
                    return Wlc_PrsWriteErrorMessage( p, pStr, "Expected comma (,) in this place." );
            }
            Type = WLC_OBJ_BIT_CONCAT;
        }
        assert( pStr[0] == '}' );
        pStr++;
    }
    else
    {
        if ( !(pStr = Wlc_PrsReadName(p, pStr, vFanins)) )
            return 0;
        // get the next symbol
        if ( pStr[0] == 0 )
            Type = WLC_OBJ_BUF;
        else if ( pStr[0] == '?' )
        {
            if ( !(pStr = Wlc_PrsReadName(p, pStr+1, vFanins)) )
                return Wlc_PrsWriteErrorMessage( p, pStr, "Cannot read name in MUX." );
            if ( pStr[0] != ':' )
                return Wlc_PrsWriteErrorMessage( p, pStr, "MUX lacks the colon symbol (:)." );
            if ( !(pStr = Wlc_PrsReadName(p, pStr+1, vFanins)) )
                return Wlc_PrsWriteErrorMessage( p, pStr, "Cannot read name in MUX." );
            assert( Vec_IntSize(vFanins) == 3 );
            ABC_SWAP( int, Vec_IntArray(vFanins)[1], Vec_IntArray(vFanins)[2] );
            Type = WLC_OBJ_MUX;
        }
        else if ( pStr[0] == '[' )
        {
            int End, Beg; char * pLine = pStr;
            pStr = Wlc_PrsFindRange( pStr, &End, &Beg );
            if ( pStr == NULL )
                return Wlc_PrsWriteErrorMessage( p, pLine, "Non-standard range." );
            Vec_IntPushTwo( vFanins, End, Beg );
            Type = WLC_OBJ_BIT_SELECT;
        }
        else 
        {
                 if ( pStr[0] == '>' && pStr[1] == '>' && pStr[2] != '>' ) pStr += 2, Type = fRotating ? WLC_OBJ_ROTATE_R : WLC_OBJ_SHIFT_R;
            else if ( pStr[0] == '>' && pStr[1] == '>' && pStr[2] == '>' ) pStr += 3, Type = WLC_OBJ_SHIFT_RA;      
            else if ( pStr[0] == '<' && pStr[1] == '<' && pStr[2] != '<' ) pStr += 2, Type = fRotating ? WLC_OBJ_ROTATE_L : WLC_OBJ_SHIFT_L;
            else if ( pStr[0] == '<' && pStr[1] == '<' && pStr[2] == '<' ) pStr += 3, Type = WLC_OBJ_SHIFT_LA;      
            else if ( pStr[0] == '&' && pStr[1] != '&'                   ) pStr += 1, Type = WLC_OBJ_BIT_AND;       
            else if ( pStr[0] == '|' && pStr[1] != '|'                   ) pStr += 1, Type = WLC_OBJ_BIT_OR;        
            else if ( pStr[0] == '^' && pStr[1] != '^'                   ) pStr += 1, Type = WLC_OBJ_BIT_XOR;       
            else if ( pStr[0] == '~' && pStr[1] == '&'                   ) pStr += 2, Type = WLC_OBJ_BIT_NAND;       
            else if ( pStr[0] == '~' && pStr[1] == '|'                   ) pStr += 2, Type = WLC_OBJ_BIT_NOR;       
            else if ( pStr[0] == '~' && pStr[1] == '^'                   ) pStr += 2, Type = WLC_OBJ_BIT_NXOR;       
            else if ( pStr[0] == '=' && pStr[1] == '>'                   ) pStr += 2, Type = WLC_OBJ_LOGIC_IMPL;     
            else if ( pStr[0] == '&' && pStr[1] == '&'                   ) pStr += 2, Type = WLC_OBJ_LOGIC_AND;     
            else if ( pStr[0] == '|' && pStr[1] == '|'                   ) pStr += 2, Type = WLC_OBJ_LOGIC_OR;      
            else if ( pStr[0] == '^' && pStr[1] == '^'                   ) pStr += 2, Type = WLC_OBJ_LOGIC_XOR;      
            else if ( pStr[0] == '=' && pStr[1] == '='                   ) pStr += 2, Type = WLC_OBJ_COMP_EQU;      
            else if ( pStr[0] == '!' && pStr[1] == '='                   ) pStr += 2, Type = WLC_OBJ_COMP_NOTEQU;      
            else if ( pStr[0] == '<' && pStr[1] != '='                   ) pStr += 1, Type = WLC_OBJ_COMP_LESS;     
            else if ( pStr[0] == '>' && pStr[1] != '='                   ) pStr += 1, Type = WLC_OBJ_COMP_MORE;     
            else if ( pStr[0] == '<' && pStr[1] == '='                   ) pStr += 2, Type = WLC_OBJ_COMP_LESSEQU;
            else if ( pStr[0] == '>' && pStr[1] == '='                   ) pStr += 2, Type = WLC_OBJ_COMP_MOREEQU;  
            else if ( pStr[0] == '+'                                     ) pStr += 1, Type = WLC_OBJ_ARI_ADD;       
            else if ( pStr[0] == '-'                                     ) pStr += 1, Type = WLC_OBJ_ARI_SUB;       
            else if ( pStr[0] == '*' && pStr[1] != '*'                   ) pStr += 1, Type = WLC_OBJ_ARI_MULTI;     
            else if ( pStr[0] == '/'                                     ) pStr += 1, Type = WLC_OBJ_ARI_DIVIDE;        
            else if ( pStr[0] == '%'                                     ) pStr += 1, Type = WLC_OBJ_ARI_REM;   
            else if ( pStr[0] == '*' && pStr[1] == '*'                   ) pStr += 2, Type = WLC_OBJ_ARI_POWER;
            else return Wlc_PrsWriteErrorMessage( p, pStr, "Unsupported operation (%c).", pStr[0] );
            if ( !(pStr = Wlc_PrsReadName(p, pStr+1, vFanins)) )
                return 0;
            pStr = Wlc_PrsSkipSpaces( pStr );
            if ( Type == WLC_OBJ_ARI_ADD && pStr[0] == '+' )
            {
                if ( !(pStr = Wlc_PrsReadName(p, pStr+1, vFanins)) )
                    return 0;
                pStr = Wlc_PrsSkipSpaces( pStr );
            }
            if ( pStr[0] )
                printf( "Warning: Trailing symbols \"%s\" in line %d.\n", pStr, Wlc_PrsFindLine(p, pStr) );
        }
    }
    // make sure there is nothing left there
    if ( pStr )
    {
        pStr = Wlc_PrsFindName( pStr, &pName );
        if ( pStr != NULL )
            return Wlc_PrsWriteErrorMessage( p, pStr, "Name %s is left at the end of the line.", pName );   
    }
    return Type;
}
int Wlc_PrsReadDeclaration( Wlc_Prs_t * p, char * pStart )
{
    int fFound = 0, Type = WLC_OBJ_NONE, iObj; char * pLine;
    int Signed = 0, Beg = 0, End = 0, NameId, fIsPo = 0;
    if ( Wlc_PrsStrCmp( pStart, "input" ) )
        pStart += strlen("input"), Type = WLC_OBJ_PI;
    else if ( Wlc_PrsStrCmp( pStart, "output" ) )
        pStart += strlen("output"), fIsPo = 1;
    pStart = Wlc_PrsSkipSpaces( pStart );
    if ( Wlc_PrsStrCmp( pStart, "wire" ) )
        pStart += strlen("wire");
    else if ( Wlc_PrsStrCmp( pStart, "reg" ) )
        pStart += strlen("reg");
    // read 'signed'
    pStart = Wlc_PrsFindWord( pStart, "signed", &Signed );
    // read range
    pLine = pStart;
    pStart = Wlc_PrsFindRange( pStart, &End, &Beg );
    if ( pStart == NULL )
        return Wlc_PrsWriteErrorMessage( p, pLine, "Non-standard range." );
    if ( End != 0 && Beg != 0 )
    {
        if ( p->nNonZero[0]++ == 0 )
        {
            p->nNonZero[1] = End;
            p->nNonZero[2] = Beg;
            p->nNonZero[3] = Wlc_PrsFindLine(p, pStart);
        }
    }
    if ( End < 0 || Beg < 0  )
    {
        if ( p->nNegative[0]++ == 0 )
        {
            p->nNegative[1] = End;
            p->nNegative[2] = Beg;
            p->nNegative[3] = Wlc_PrsFindLine(p, pStart);
        }
    }
    if ( End < Beg )
    {
        if ( p->nReverse[0]++ == 0 )
        {
            p->nReverse[1] = End;
            p->nReverse[2] = Beg;
            p->nReverse[3] = Wlc_PrsFindLine(p, pStart);
        }
    }
    while ( 1 )
    {
        char * pName; int XValue;
        // read name
        pStart = Wlc_PrsFindName( pStart, &pName );
        if ( pStart == NULL )
            return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read name in declaration." );
        NameId = Abc_NamStrFindOrAdd( p->pNtk->pManName, pName, &fFound );
        if ( fFound )
            return Wlc_PrsWriteErrorMessage( p, pStart, "Name %s is declared more than once.", pName );
        iObj = Wlc_ObjAlloc( p->pNtk, Type, Signed, End, Beg );
        if ( fIsPo ) Wlc_ObjSetCo( p->pNtk, Wlc_NtkObj(p->pNtk, iObj), 0 );
        assert( iObj == NameId );
        // check next definition
        pStart = Wlc_PrsSkipSpaces( pStart );
        if ( pStart[0] == ',' )
        {
            pStart++;
            continue;
        }
        // check definition
        Type = Wlc_PrsFindDefinition( p, pStart, p->vFanins, &XValue );
        if ( Type )
        {
            Wlc_Obj_t * pObj = Wlc_NtkObj( p->pNtk, iObj );
            Wlc_ObjUpdateType( p->pNtk, pObj, Type );
            Wlc_ObjAddFanins( p->pNtk, pObj, p->vFanins );
            pObj->fXConst = XValue;
        }
        break;
    }
    return 1;
}
int Wlc_PrsDerive( Wlc_Prs_t * p )
{
    Wlc_Obj_t * pObj;
    char * pStart, * pName;
    int i;
    // go through the directives
    Wlc_PrsForEachLine( p, pStart, i )
    {
startword:
        if ( Wlc_PrsStrCmp( pStart, "module" ) )
        {
            // get module name
            pName = Wlc_PrsStrtok( pStart + strlen("module"), " \r\n\t(,)" );
            if ( pName == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read model name." );
            // THIS IS A HACK to skip definitions of modules beginning with "CPL_"
            if ( Wlc_PrsStrCmp( pName, "CPL_" ) )
            {
                while ( ++i < Vec_IntSize(p->vStarts) )
                {
                    pStart = Wlc_PrsStr(p, Vec_IntEntry(p->vStarts, i));
                    pStart = strstr( pStart, "endmodule" );
                    if ( pStart != NULL )
                        break;
                }
                continue;
            }
            if ( Wlc_PrsStrCmp( pName, "table" ) )
            {
                // THIS IS A HACK to detect table module descriptions
                int Width1 = -1, Width2 = -1;
                int v, b, Value, nBits, nInts;
                unsigned * pTable;
                Vec_Int_t * vValues = Vec_IntAlloc( 256 );
                Wlc_PrsForEachLineStart( p, pStart, i, i+1 )
                {
                    if ( Wlc_PrsStrCmp( pStart, "endcase" ) )
                        break;
                    pStart = Wlc_PrsFindSymbol( pStart, '\'' );
                    if ( pStart == NULL )
                        continue;
                    Width1 = atoi(pStart-1);
                    pStart = Wlc_PrsFindSymbol( pStart+2, '\'' );
                    if ( pStart == NULL )
                        continue;
                    Width2 = atoi(pStart-1);
                    Value = 0;
                    Abc_TtReadHexNumber( (word *)&Value, pStart+2 );
                    Vec_IntPush( vValues, Value );
                }
                //Vec_IntPrint( vValues );
                nBits = Abc_Base2Log( Vec_IntSize(vValues) );
                if ( Vec_IntSize(vValues) != (1 << nBits) )
                {
                    Vec_IntFree( vValues );
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read module \"%s\".", pName );
                }
                assert( Width1 == nBits );
                // create bitmap
                nInts = Abc_BitWordNum( Width2 * Vec_IntSize(vValues) );
                pTable = (unsigned *)Mem_FlexEntryFetch( p->pMemTable, nInts * sizeof(unsigned) );
                memset( pTable, 0, nInts * sizeof(unsigned) );
                Vec_IntForEachEntry( vValues, Value, v )
                    for ( b = 0; b < Width2; b++ )
                        if ( (Value >> b) & 1 )
                            Abc_InfoSetBit( pTable, v * Width2 + b );
                Vec_PtrPush( p->vTables, pTable );
                Vec_IntFree( vValues );
                continue;
            }
            if ( p->pNtk != NULL )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Network is already defined." );
            p->pNtk = Wlc_NtkAlloc( pName, Vec_IntSize(p->vStarts) );
            p->pNtk->pManName = Abc_NamStart( Vec_IntSize(p->vStarts), 20 );
            p->pNtk->pMemTable = p->pMemTable; p->pMemTable = NULL;
            p->pNtk->vTables = p->vTables; p->vTables = NULL;
            // read the argument definitions
            while ( (pName = Wlc_PrsStrtok( NULL, "(,)" )) )
            {
                pName = Wlc_PrsSkipSpaces( pName );
                if ( Wlc_PrsStrCmp( pName, "input" ) || Wlc_PrsStrCmp( pName, "output" ) || Wlc_PrsStrCmp( pName, "wire" ) )
                {
                    if ( !Wlc_PrsReadDeclaration( p, pName ) )
                        return 0;
                }
            }
        }
        else if ( Wlc_PrsStrCmp( pStart, "endmodule" ) )
        {
            Vec_Int_t * vTemp = Vec_IntStartNatural( Wlc_NtkObjNumMax(p->pNtk) );
            Vec_IntAppend( &p->pNtk->vNameIds, vTemp );
            Vec_IntFree( vTemp );
            if ( p->pNtk->vInits )
            {
                // move FO/FI to be part of CI/CO
                assert( (Vec_IntSize(&p->pNtk->vFfs) & 1) == 0 );
                assert( Vec_IntSize(&p->pNtk->vFfs) == 2 * Vec_IntSize(p->pNtk->vInits) );
                Wlc_NtkForEachFf( p->pNtk, pObj, i )
                    if ( i & 1 )
                        Wlc_ObjSetCo( p->pNtk, pObj, 1 );
                    else
                        Wlc_ObjSetCi( p->pNtk, pObj );
                Vec_IntClear( &p->pNtk->vFfs );
                // convert init values into binary string
                //Vec_IntPrint( &p->pNtk->vInits );
                p->pNtk->pInits = Wlc_PrsConvertInitValues( p->pNtk );
                //printf( "%s", p->pNtk->pInits );
            }
            break;
        }
        // these are read as part of the interface
        else if ( Wlc_PrsStrCmp( pStart, "input" ) || Wlc_PrsStrCmp( pStart, "output" ) || Wlc_PrsStrCmp( pStart, "wire" ) || Wlc_PrsStrCmp( pStart, "reg" ) )
        {
            if ( !Wlc_PrsReadDeclaration( p, pStart ) )
                return 0;
        }
        else if ( Wlc_PrsStrCmp( pStart, "assign" ) )
        {
            int Type, NameId, fFound, XValue = 0;
            pStart += strlen("assign");
            // read name
            pStart = Wlc_PrsFindName( pStart, &pName );
            if ( pStart == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read name after assign." );
            NameId = Abc_NamStrFindOrAdd( p->pNtk->pManName, pName, &fFound );
            if ( !fFound )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Name %s is not declared.", pName );
            // read definition
            Type = Wlc_PrsFindDefinition( p, pStart, p->vFanins, &XValue );
            if ( Type )
            {
                pObj = Wlc_NtkObj( p->pNtk, NameId );
                Wlc_ObjUpdateType( p->pNtk, pObj, Type );
                Wlc_ObjAddFanins( p->pNtk, pObj, p->vFanins );
                pObj->fXConst = XValue;
            }
            else
                return 0;
        }
        else if ( Wlc_PrsStrCmp( pStart, "table" ) )
        {
            // THIS IS A HACK to detect tables
            int NameId, fFound, iTable = atoi( pStart + strlen("table") );
            // find opening
            pStart = Wlc_PrsFindSymbol( pStart, '(' );
            if ( pStart == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read table." );
            // read input
            pStart = Wlc_PrsFindName( pStart+1, &pName );
            if ( pStart == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read name after assign." );
            NameId = Abc_NamStrFindOrAdd( p->pNtk->pManName, pName, &fFound );
            if ( !fFound )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Name %s is not declared.", pName );
            // save inputs
            Vec_IntClear( p->vFanins );
            Vec_IntPush( p->vFanins, NameId );
            Vec_IntPush( p->vFanins, iTable );
            // find comma
            pStart = Wlc_PrsFindSymbol( pStart, ',' );
            if ( pStart == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read table." );
            // read output
            pStart = Wlc_PrsFindName( pStart+1, &pName );
            if ( pStart == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read name after assign." );
            NameId = Abc_NamStrFindOrAdd( p->pNtk->pManName, pName, &fFound );
            if ( !fFound )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Name %s is not declared.", pName );
            pObj = Wlc_NtkObj( p->pNtk, NameId );
            Wlc_ObjUpdateType( p->pNtk, pObj, WLC_OBJ_TABLE );
            Wlc_ObjAddFanins( p->pNtk, pObj, p->vFanins );
        }
        else if ( Wlc_PrsStrCmp( pStart, "always" ) )
        {
            // THIS IS A HACK to detect always statement representing combinational MUX
            int NameId, NameIdOut = -1, fFound, nValues, fDefaultFound = 0;
            // find control
            pStart = Wlc_PrsFindWord( pStart, "case", &fFound );
            if ( pStart == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read case statement." );
            // read the name
            pStart = Wlc_PrsFindSymbol( pStart, '(' );
            if ( pStart == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read table." );
            pStart = Wlc_PrsFindSymbol( pStart+1, '(' );
            if ( pStart == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read table." );
            pStart = Wlc_PrsFindName( pStart+1, &pName );
            if ( pStart == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read name after case." );
            NameId = Abc_NamStrFindOrAdd( p->pNtk->pManName, pName, &fFound );
            if ( !fFound )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Name %s is not declared.", pName );
            Vec_IntClear( p->vFanins );
            Vec_IntPush( p->vFanins, NameId );
            // read data inputs
            pObj = Wlc_NtkObj( p->pNtk, NameId );
            if ( pObj == NULL )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot find the object in case statement." );
            // remember the number of values
            nValues = (1 << Wlc_ObjRange(pObj));
            while ( 1 )
            {                
                // find opening
                pStart = Wlc_PrsFindSymbol( pStart, ':' );
                if ( pStart == NULL )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot find colon in the case statement." );
                // find output name
                pStart = Wlc_PrsFindName( pStart+1, &pName );
                if ( pStart == NULL )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read name after case." );
                NameIdOut = Abc_NamStrFindOrAdd( p->pNtk->pManName, pName, &fFound );
                if ( !fFound )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Name %s is not declared.", pName );
                // find equality
                pStart = Wlc_PrsFindSymbol( pStart, '=' );
                if ( pStart == NULL )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot find equality in the case statement." );
                // find input name
                pStart = Wlc_PrsSkipSpaces( pStart+1 );
                pStart = Wlc_PrsReadName( p, pStart, p->vFanins );
                if ( pStart == NULL )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read name inside case statement." );
                // consider default
                if ( fDefaultFound )
                {
                    int EntryLast = Vec_IntEntryLast( p->vFanins );
                    if (nValues != Vec_IntSize(p->vFanins)-2)
                        Vec_IntFillExtra( p->vFanins, nValues + 1, EntryLast );
                    else
                        Vec_IntPop(p->vFanins);
                    // get next line and check its opening character
                    pStart = Wlc_PrsStr(p, Vec_IntEntry(p->vStarts, ++i));
                    pStart = Wlc_PrsSkipSpaces( pStart );
                }
                else
                {
                    // get next line and check its opening character
                    pStart = Wlc_PrsStr(p, Vec_IntEntry(p->vStarts, ++i));
                    pStart = Wlc_PrsSkipSpaces( pStart );
                    if ( Wlc_PrsIsDigit(pStart) )
                        continue;
                    if ( Wlc_PrsStrCmp( pStart, "default" ) )
                    {
                        fDefaultFound = 1;
                        continue;
                    }
                }
                // find closing
                pStart = Wlc_PrsFindWord( pStart, "endcase", &fFound );
                if ( pStart == NULL )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read case statement." );
                // find closing
                pStart = Wlc_PrsFindWord( pStart, "end", &fFound );
                if ( pStart == NULL )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read case statement." );
                pStart = Wlc_PrsSkipSpaces( pStart );
                break;
            }
            // check range of the control
            if ( nValues != Vec_IntSize(p->vFanins) - 1 )
                return Wlc_PrsWriteErrorMessage( p, pStart, "The number of values in the case statement is wrong.", pName );
            if ( Wlc_ObjRange(pObj) == 1 )
            {
//                return Wlc_PrsWriteErrorMessage( p, pStart, "Always-statement with 1-bit control is not bit-blasted correctly.", pName );
                printf( "Warning:  Case-statement with 1-bit control is treated as a 2:1 MUX (correct for unsigned signals only).\n" );
            }
            pObj = Wlc_NtkObj( p->pNtk, NameIdOut );
            Wlc_ObjUpdateType( p->pNtk, pObj, WLC_OBJ_MUX );
            Wlc_ObjAddFanins( p->pNtk, pObj, p->vFanins );
            goto startword;
        }
        else if ( Wlc_PrsStrCmp( pStart, "CPL_FF" ) )
        {
            int NameId = -1, NameIdIn = -1, NameIdOut = -1, fFound, nBits = 1, fFlopIn, fFlopOut;
            pStart += strlen("CPL_FF");
            if ( pStart[0] == '#' )
                nBits = atoi(pStart+1);
            // read names
            while ( 1 )
            {
                pStart = Wlc_PrsFindSymbol( pStart, '.' );
                if ( pStart == NULL )
                    break;
                pStart = Wlc_PrsSkipSpaces( pStart+1 );
                if ( pStart[0] != 'd' && (pStart[0] != 'q' || pStart[1] == 'b') && strncmp(pStart, "arstval", 7) )
                    continue;
                fFlopIn = (pStart[0] == 'd');
                fFlopOut = (pStart[0] == 'q');
                pStart = Wlc_PrsFindSymbol( pStart, '(' );
                if ( pStart == NULL )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read opening parenthesis in the flop description." );
                pStart = Wlc_PrsFindName( pStart+1, &pName );
                if ( pStart == NULL )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read name inside flop description." );
                if ( fFlopIn )
                    NameIdIn = Abc_NamStrFindOrAdd( p->pNtk->pManName, pName, &fFound );
                else if ( fFlopOut ) 
                    NameIdOut = Abc_NamStrFindOrAdd( p->pNtk->pManName, pName, &fFound );
                else
                    NameId = Abc_NamStrFindOrAdd( p->pNtk->pManName, pName, &fFound );
                if ( !fFound )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Name %s is not declared.", pName );
            }
            if ( NameIdIn == -1 || NameIdOut == -1 )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Name of flop input or flop output is missing." );
            // create flop output
            pObj = Wlc_NtkObj( p->pNtk, NameIdOut );
            Wlc_ObjUpdateType( p->pNtk, pObj, WLC_OBJ_FO );
            Vec_IntPush( &p->pNtk->vFfs, NameIdOut );
            if ( nBits != Wlc_ObjRange(pObj) )
                printf( "Warning!  Flop input has bit-width (%d) that differs from the flop declaration (%d)\n", Wlc_ObjRange(pObj), nBits );
            // create flop input
            pObj = Wlc_NtkObj( p->pNtk, NameIdIn );
            Vec_IntPush( &p->pNtk->vFfs, NameIdIn );
            if ( nBits != Wlc_ObjRange(pObj) )
                printf( "Warning!  Flop output has bit-width (%d) that differs from the flop declaration (%d)\n", Wlc_ObjRange(pObj), nBits );
            // save flop init value
            if ( NameId == -1 )
                printf( "Initial value of flop \"%s\" is not specified. Zero is assumed.\n", Abc_NamStr(p->pNtk->pManName, NameIdOut) );
            else
            {
                pObj = Wlc_NtkObj( p->pNtk, NameId );
                if ( nBits != Wlc_ObjRange(pObj) )
                    printf( "Warning!  Flop init signal bit-width (%d) is different from the flop declaration (%d)\n", Wlc_ObjRange(pObj), nBits );
            }
            if ( p->pNtk->vInits == NULL )
                p->pNtk->vInits = Vec_IntAlloc( 100 );
            Vec_IntPush( p->pNtk->vInits, NameId > 0 ? NameId : -nBits );
        }
        else if ( Wlc_PrsStrCmp( pStart, "CPL_MEM_" ) )
        {
            int * pNameId = NULL, NameOutput, NameMi = -1, NameMo = -1, NameAddr = -1, NameDi = -1, NameDo = -1, fFound, fRead = 1;
            pStart += strlen("CPL_MEM_");
            if ( pStart[0] == 'W' )
                fRead = 0;
            // read names
            while ( 1 )
            {
                pStart = Wlc_PrsFindSymbol( pStart, '.' );
                if ( pStart == NULL )
                    break;
                pStart = Wlc_PrsSkipSpaces( pStart+1 );
                if ( !strncmp(pStart, "mem_data_in", 11) )
                    pNameId = &NameMi;
                else if ( !strncmp(pStart, "data_in", 7) )
                    pNameId = &NameDi;
                else if ( !strncmp(pStart, "data_out", 8) )
                    pNameId = fRead ? &NameDo : &NameMo;
                else if ( !strncmp(pStart, "addr_in", 7) )
                    pNameId = &NameAddr;
                else
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read name of the input/output port." );
                pStart = Wlc_PrsFindSymbol( pStart, '(' );
                if ( pStart == NULL )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read opening parenthesis in the flop description." );
                pStart = Wlc_PrsFindName( pStart+1, &pName );
                if ( pStart == NULL )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read name inside flop description." );
                *pNameId = Abc_NamStrFindOrAdd( p->pNtk->pManName, pName, &fFound );
                if ( !fFound )
                    return Wlc_PrsWriteErrorMessage( p, pStart, "Name %s is not declared.", pName );
            }
            if ( fRead && (NameMi == -1 || NameAddr == -1 || NameDo == -1) )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Name of one of signals of read port is missing." );
            if ( !fRead && (NameMi == -1 || NameAddr == -1 || NameDi == -1 || NameMo == -1) )
                return Wlc_PrsWriteErrorMessage( p, pStart, "Name of one of signals of write port is missing." );
            // create output
            NameOutput = fRead ? NameDo : NameMo;
            pObj = Wlc_NtkObj( p->pNtk, NameOutput );
            Wlc_ObjUpdateType( p->pNtk, pObj, fRead ? WLC_OBJ_READ : WLC_OBJ_WRITE );
            Vec_IntClear( p->vFanins );
            Vec_IntPush( p->vFanins, NameMi );
            Vec_IntPush( p->vFanins, NameAddr );
            if ( !fRead )
                Vec_IntPush( p->vFanins, NameDi );
            Wlc_ObjAddFanins( p->pNtk, pObj, p->vFanins );
        }
        else if ( pStart[0] == '(' && pStart[1] == '*' ) // skip comments
        {
            while ( *pStart++ != ')' );
            pStart = Wlc_PrsSkipSpaces( pStart );
            goto startword;
        }
        else if ( pStart[0] != '`' )
        {
            int iLine = Wlc_PrsFindLine(p, pStart);
            pStart = Wlc_PrsFindName( pStart, &pName );
            return Wlc_PrsWriteErrorMessage( p, pStart, "Cannot read line %d beginning with %s.", iLine, (!pName || !pName[0]) ? "\"?\"" : pName );
        }
    }
    if ( p->nNonZero[0] )
    {
        printf( "Warning: Input file contains %d objects with non-zero-based ranges.\n", p->nNonZero[0] );
        printf( "For example, signal with range [%d:%d] is declared in line %d.\n", p->nNonZero[1], p->nNonZero[2], p->nNonZero[3] );
    }
    if ( p->nNegative[0] )
    {
        printf( "Warning: Input file contains %d objects with negative ranges.\n", p->nNegative[0] );
        printf( "For example, signal with range [%d:%d] is declared in line %d.\n", p->nNegative[1], p->nNegative[2], p->nNegative[3] );
    }
    if ( p->nReverse[0] )
    {
        printf( "Warning: Input file contains %d objects with reversed ranges.\n", p->nReverse[0] );
        printf( "For example, signal with range [%d:%d] is declared in line %d.\n", p->nReverse[1], p->nReverse[2], p->nReverse[3] );
    }
    return 1;
}
Wlc_Ntk_t * Wlc_ReadVer( char * pFileName, char * pStr )
{
    Wlc_Prs_t * p;
    Wlc_Ntk_t * pNtk = NULL;
    // either file name or string is given
    assert( (pFileName == NULL) != (pStr == NULL) );
    // start the parser 
    p = Wlc_PrsStart( pFileName, pStr );
    if ( p == NULL )
        return NULL;
    // detect lines
    if ( !Wlc_PrsPrepare( p ) )
        goto finish;
    // parse models
    if ( !Wlc_PrsDerive( p ) )
        goto finish;
    // derive topological order
    pNtk = Wlc_NtkDupDfs( p->pNtk, 0, 1 );
    pNtk->pSpec = Abc_UtilStrsav( pFileName );
finish:
    Wlc_PrsPrintErrorMessage( p );
    Wlc_PrsStop( p );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_ReadWordTest( char * pFileName )
{
    Gia_Man_t * pNew;
    Wlc_Ntk_t * pNtk = Wlc_ReadVer( pFileName, NULL );
    if ( pNtk == NULL )
        return;
    Wlc_WriteVer( pNtk, "test.v", 0, 0 );

    pNew = Wlc_NtkBitBlast( pNtk, NULL );
    Gia_AigerWrite( pNew, "test.aig", 0, 0 );
    Gia_ManStop( pNew );

    Wlc_NtkFree( pNtk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

