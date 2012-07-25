/*===========================================================================*/
/* Purpose : Function definitions for the TIO_PrintHandler class external 
 *           'C' interface.
 *
 *           - PrintHandlerNew
 *           - PrintHandlerDelete
 *           - PrintHandlerInit
 *           - PrintHandlerFilter
 *           - PrintHandlerMessage
 *
 *===========================================================================*/

#include <stdarg.h>

#include "TIO_SkinHandler.h"
#include "TIO_PrintHandler.h"
#include "TIO_PrintHandlerExtern.h"



/*===========================================================================*/
/* Function       : PrintHandlerNew
 * Purpose        : Provides an external C interface to the TIO_PrintHandler.
 * Author         : Jeff Rudolph
 *---------------------------------------------------------------------------
 * Version history
 * 07/02/12 jeffr : Original
 *===========================================================================*/
void PrintHandlerNew( 
      char* pszLogFileName )
{
   /* Allocate a skin handler 'singleton' for program messasge handling */
   TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );
   skinHandler.Set( TIO_SkinHandler_c::TIO_SKIN_VPR );

   /* Allocate a print handler 'singleton' for program message handling */
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.SetStdioOutput( stdout );

   /* Display optional print handler program banner */
   /* jluu removed optional program banner */
   /* printHandler.WriteBanner( ); */

   /* Apply optional print handler file logging and include program banner */
   if ( pszLogFileName )
   {
      printHandler.SetLogFileOutput( pszLogFileName );

      printHandler.DisableOutput( TIO_PRINT_OUTPUT_STDIO | TIO_PRINT_OUTPUT_CUSTOM );
      printHandler.WriteBanner( );
      printHandler.EnableOutput( TIO_PRINT_OUTPUT_STDIO | TIO_PRINT_OUTPUT_CUSTOM );
   }
}

/*===========================================================================*/
/* Function       : PrintHandlerDelete
 * Purpose        : Provides an external C interface to the TIO_PrintHandler.
 * Author         : Jeff Rudolph
 *---------------------------------------------------------------------------
 * Version history
 * 07/02/12 jeffr : Original
 *===========================================================================*/
void PrintHandlerDelete( void )
{
   TIO_PrintHandler_c::GetInstance( );
   TIO_PrintHandler_c::DeleteInstance( );

   TIO_SkinHandler_c::GetInstance( );
   TIO_SkinHandler_c::DeleteInstance( );
}
 
/*===========================================================================*/
/* Function       : PrintHandlerInit
 * Purpose        : Provides an external C interface to the TIO_PrintHandler.
 * Author         : Jeff Rudolph
 *---------------------------------------------------------------------------
 * Version history
 * 07/02/12 jeffr : Original
 *===========================================================================*/
void PrintHandlerInit( 
      unsigned char enableTimeStamps,
      unsigned long maxWarningCount,
      unsigned long maxErrorCount )
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   /* Enable optional print handler message time stamps */
   printHandler.SetTimeStampsEnabled( static_cast< bool >( enableTimeStamps == (unsigned char) 0 ));

   /* Define optional print handler max warning/error counts */
   printHandler.SetMaxWarningCount( maxWarningCount );
   printHandler.SetMaxErrorCount( maxErrorCount );
}

/*===========================================================================*/
/* Function       : PrintHandlerFilter
 * Purpose        : Provides an external C interface to the TIO_PrintHandler.
 * Author         : Jeff Rudolph
 *---------------------------------------------------------------------------
 * Version history
 * 07/02/12 jeffr : Original
 *===========================================================================*/
void PrintHandlerFilter( 
      TIO_MessageMode_t messageMode,
      TIO_FilterMode_t  filterMode,
      char*             pszFilter )
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   switch( messageMode )
   {
   case TIO_MESSAGE_INFO:
      switch( filterMode )
      {
      case TIO_FILTER_ACCEPT:
         printHandler.AddInfoAcceptRegExp( pszFilter );
         break;
      case TIO_FILTER_REJECT:
         printHandler.AddInfoRejectRegExp( pszFilter );
         break;
	  default: /* do nothing */;
      }
      break;
   case TIO_MESSAGE_WARNING:
      switch( filterMode )
      {
      case TIO_FILTER_ACCEPT:
         printHandler.AddWarningAcceptRegExp( pszFilter );
         break;
      case TIO_FILTER_REJECT:
         printHandler.AddWarningRejectRegExp( pszFilter );
         break;
	  default: /* do nothing */;
      }
      break;
   case TIO_MESSAGE_ERROR:
      switch( filterMode )
      {
      case TIO_FILTER_ACCEPT:
         printHandler.AddErrorAcceptRegExp( pszFilter );
         break;
      case TIO_FILTER_REJECT:
         printHandler.AddErrorRejectRegExp( pszFilter );
         break;
	  default: /* do nothing */;
      }
      break;
   case TIO_MESSAGE_TRACE:
      switch( filterMode )
      {
      case TIO_FILTER_ACCEPT:
         printHandler.AddTraceAcceptRegExp( pszFilter );
         break;
      case TIO_FILTER_REJECT:
         printHandler.AddTraceRejectRegExp( pszFilter );
         break;
	  default: /* do nothing */;
      }
      break;
	default: /* do nothing */;
   }
}

/*===========================================================================*/
/* Function       : PrintHandlerMessage
 * Purpose        : Provides an external C interface to the TIO_PrintHandler.
 * Author         : Jeff Rudolph
 *---------------------------------------------------------------------------
 * Version history
 * 07/02/12 jeffr : Original
 *===========================================================================*/
unsigned char PrintHandlerMessage( 
      TIO_MessageMode_t messageMode,
      char*             pszMessage,
      ... )
{
   va_list vaArgs;                      /* Make a variable argument list */
   va_start( vaArgs, pszMessage );      /* Initialize variable argument list */

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   unsigned char ok = true;
   switch( messageMode )
   {
   case TIO_MESSAGE_INFO:
      printHandler.Info( pszMessage, vaArgs );
      break;
   case TIO_MESSAGE_WARNING:
      ok = printHandler.Warning( pszMessage, vaArgs );
      break;
   case TIO_MESSAGE_ERROR:
      ok = printHandler.Error( pszMessage, vaArgs );
      break;
   case TIO_MESSAGE_TRACE:
      printHandler.Trace( pszMessage, vaArgs );
      break;
   default: /* do nothing */;
   }
   va_end( vaArgs );                    /* Reset variable argument list */

   return( ok );
}
