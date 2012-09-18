//===========================================================================//
// Purpose : Method definitions for the TOS_ExecuteOptions class.
//
//           Public methods include:
//           - TOS_ExecuteOptions_c, ~TOS_ExecuteOptions_c
//           - Print
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TOS_StringUtils.h"
#include "TOS_ExecuteOptions.h"

//===========================================================================//
// Method         : TOS_ExecuteOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_ExecuteOptions_c::TOS_ExecuteOptions_c( 
      void )
      :
      maxWarningCount( 0 ),
      maxErrorCount( 0 ),
      toolMask( 0 )
{
}

//===========================================================================//
TOS_ExecuteOptions_c::TOS_ExecuteOptions_c( 
      unsigned long maxWarningCount_,
      unsigned long maxErrorCount_,
      int           toolMask_ )
      :
      maxWarningCount( maxWarningCount_ ),
      maxErrorCount( maxErrorCount_ ),
      toolMask( toolMask_ )
{
}

//===========================================================================//
// Method         : ~TOS_ExecuteOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_ExecuteOptions_c::~TOS_ExecuteOptions_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExecuteOptions_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srToolMask;
   TOS_ExtractStringExecuteToolMask( this->toolMask, &srToolMask );

   printHandler.Write( pfile, spaceLen, "HALT_MAX_WARNINGS          = %lu\n", this->maxWarningCount );
   printHandler.Write( pfile, spaceLen, "HALT_MAX_ERRORS            = %lu\n", this->maxErrorCount );

   printHandler.Write( pfile, spaceLen, "EXECUTE_MODE               = { %s}\n", TIO_SR_STR( srToolMask ));
}
