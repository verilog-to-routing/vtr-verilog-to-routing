/*===========================================================================
 * Purpose : Test scaffold for the "TIO_PrintHandler_c" C->C++ interface.
 *
 *           Functions include:
 *           - main
 *
 *===========================================================================*/

#include "TIO_PrintHandlerExtern.h"

/*===========================================================================*/
/* Function       : main
 * Author         : Jeff Rudolph
 *---------------------------------------------------------------------------/
 * Version history
 * 05/01/12 jeffr : Original
 *===========================================================================*/
int main( int argc, char *argv[] )
{
   unsigned char ok = 1;

   char* pszLogFileName = "test.log";

   unsigned char enableTimeStamps = 1;
   unsigned char enableFileLines = 0;
   unsigned long maxWarningCount = 1000;
   unsigned long maxErrorCount = 1;

   const char* pszFileName = 0;
   unsigned int lineNum = 0;

   PrintHandlerNew( pszLogFileName );
   PrintHandlerInit( enableTimeStamps, enableFileLines,
                     maxWarningCount, maxErrorCount );

   /* PrintHandlerFilter( TIO_MESSAGE_INFO, TIO_FILTER_ACCEPT, ".*Wor.*" ); */
   /* PrintHandlerFilter( TIO_MESSAGE_INFO, TIO_FILTER_REJECT, ".*Wor.*" ); */
   PrintHandlerFilter( TIO_MESSAGE_WARNING, TIO_FILTER_ACCEPT, "^PACK .*" );
   PrintHandlerFilter( TIO_MESSAGE_ERROR, TIO_FILTER_ACCEPT, "^PLACE .*" );
   PrintHandlerFilter( TIO_MESSAGE_TRACE, TIO_FILTER_ACCEPT, "^PACK .*" );
   /* PrintHandlerFilter( TIO_MESSAGE_TRACE, TIO_FILTER_REJECT, "^PACK .*" ); */

   PrintHandlerMessage( TIO_MESSAGE_INFO, pszFileName, lineNum, "Hello World\n" );
   PrintHandlerMessage( TIO_MESSAGE_INFO, pszFileName, lineNum, "Play it again, %s\n", "Sam" );
   PrintHandlerMessage( TIO_MESSAGE_WARNING, pszFileName, lineNum, "PACK - Display this warning\n" );
   PrintHandlerMessage( TIO_MESSAGE_WARNING, pszFileName, lineNum, "PLACE - Do not display this warning\n" );
   PrintHandlerMessage( TIO_MESSAGE_TRACE, pszFileName, lineNum, "PACK - %s\n", "Display this trace" );
   PrintHandlerMessage( TIO_MESSAGE_TRACE, pszFileName, lineNum, "PLACE - Do not display this trace\n" );

   PrintHandlerDelete( );
 
   return( ok ? 0 : 1 );
}
