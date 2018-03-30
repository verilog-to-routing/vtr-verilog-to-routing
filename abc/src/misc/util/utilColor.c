/**CFile****************************************************************

  FileName    [utilColor.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Handling counter-examples.]

  Synopsis    [Handling counter-examples.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Feburary 13, 2011.]

  Revision    [$Id: utilColor.c,v 1.00 2011/02/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc/util/abc_global.h"


#ifdef WIN32
#include <windows.h>
#endif

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ColorTest()
{
#ifdef WIN32
    int x, y;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    printf( "Background     00   01   02   03   04   05   06   07   08   09   10   11   12   13   14   15\n" );
    for ( y = 0; y < 16; y++ )
    {
        printf( "Foreground %02d", y );
        for ( x = 0; x < 16; x++ )
        {
            printf( " " );
            SetConsoleTextAttribute( hConsole, (WORD)(16 * x + y) );
            printf( " Hi " );
            SetConsoleTextAttribute( hConsole, 7 );
        }
        printf( "\n" );
    }
#else
/*
  fg[Default]   = '[0m';    fg[DefaultBold] = '[1m'

  fg[Black]     = '[0;30m'; fg[DarkGray]    = '[1;30m'
  fg[Blue]      = '[0;34m'; fg[LightBlue]   = '[1;34m'
  fg[Green]     = '[0;32m'; fg[LightGreen]  = '[1;32m'
  fg[Cyan]      = '[0;36m'; fg[LightCyan]   = '[1;36m'
  fg[Red]       = '[0;31m'; fg[LightRed]    = '[1;31m'
  fg[Purple]    = '[0;35m'; fg[LightPurple] = '[1;35m'
  fg[Brown]     = '[0;33m'; fg[Yellow]      = '[1;33m'
  fg[LightGray] = '[0;37m'; fg[White]       = '[1;37m'

  bg[Black]     = '[0;40m'; hi[Underlined]  = '[4m'
  bg[Blue]      = '[0;44m'; hi[Blinking]    = '[5m'
  bg[Green]     = '[0;42m'; hi[Inverted]    = '[7m'
  bg[Cyan]      = '[0;46m'; hi[Concealed]   = '[8m'
  bg[Red]       = '[0;41m'
  bg[Purple]    = '[0;45m'
  bg[Brown]     = '[0;43m'
  bg[LightGray] = '[0;47m'
*/
    int x, y;
    printf( "Background       " );
    for ( x = 0; x < 8; x++ )
        printf( "  [1;4%dm", x );
    printf( "\n" );
    for ( y = 0; y < 2; y++ )
    {
        printf( "Foreground [%dm   ", y );
        for ( x = 0; x < 8; x++ )
            printf( "  \033[%d;3%dm\033[%dm  Hi  \033[0m", y&1, y>>1, x );
        printf( "\n" );
    }
    for ( y = 0; y < 16; y++ )
    {
        printf( "Foreground [%d;3%dm", y&1, y>>1 );
        for ( x = 0; x < 8; x++ )
            printf( "  \033[%d;3%dm\033[1;4%dm  Hi  \033[0m", y&1, y>>1, x );
        printf( "\n" );
    }
    printf( "\033[4mUnderlined\033[0m\n" );
    printf( "\033[5mBlinking  \033[0m\n" );
    printf( "\033[7mInverted  \033[0m\n" );
    printf( "\033[8mConcealed \033[0m\n" );
#endif
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

