/**CFile****************************************************************

  FileName    [verParse.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Performs some Verilog parsing tasks.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 19, 2006.]

  Revision    [$Id: verParse.c,v 1.00 2006/08/19 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ver.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Skips the comments of they are present.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_ParseSkipComments( Ver_Man_t * pMan )
{
    Ver_Stream_t * p = pMan->pReader;
    char Symbol;
    // skip spaces
    Ver_StreamSkipChars( p, " \t\n\r" );
    if ( !Ver_StreamIsOkey(pMan->pReader) )
        return 1;
    // read the first symbol
    Symbol = Ver_StreamScanChar( p );
    if ( Symbol != '/' )
        return 1;
    Ver_StreamPopChar( p );
    // read the second symbol
    Symbol = Ver_StreamScanChar( p );
    if ( Symbol == '/' )
    { // skip till the end of line
        Ver_StreamSkipToChars( p, "\n" );
        return Ver_ParseSkipComments( pMan );
    }
    if ( Symbol == '*' )
    { // skip till the next occurrence of */
        Ver_StreamPopChar( p );
        do {
            Ver_StreamSkipToChars( p, "*" );
            Ver_StreamPopChar( p );
        } while ( Ver_StreamScanChar( p ) != '/' );
        Ver_StreamPopChar( p );
        return Ver_ParseSkipComments( pMan );
    }
    sprintf( pMan->sError, "Cannot parse after symbol \"/\"." );
    Ver_ParsePrintErrorMessage( pMan );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Parses a Verilog name that can be being with a slash.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ver_ParseGetName( Ver_Man_t * pMan )
{
    Ver_Stream_t * p = pMan->pReader;
    char Symbol;
    char * pWord;
    pMan->fNameLast = 0;
    if ( !Ver_StreamIsOkey(p) )
        return NULL;
    if ( !Ver_ParseSkipComments( pMan ) )
        return NULL;
    Symbol = Ver_StreamScanChar( p );
    if ( Symbol == '\\' )
    {
        pMan->fNameLast = 1;
        Ver_StreamPopChar( p );
        pWord = Ver_StreamGetWord( p, " \r\n" );
        Ver_StreamSkipChars( p, " \r\n" );
        if ( Ver_StreamScanChar(p) == '[' )
        {
            char This, * pEnd = pWord + strlen( pWord );
            while ( (This = Ver_StreamPopChar(p)) != ']' )
                *pEnd++ = This;
            *pEnd++ = This;
            *pEnd = 0;
        }
    }
    else
        pWord = Ver_StreamGetWord( p, " \t\n\r(),;" );
    if ( Ver_StreamIsOkey(p) && !Ver_ParseSkipComments( pMan ) )
        return NULL;
    return pWord;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

