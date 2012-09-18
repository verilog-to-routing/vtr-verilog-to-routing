//===========================================================================//
// Purpose : Supporting methods for the TO_Output post-processor class.
//           These methods support formatting and writing a metrics file.
//
//           Private methods include:
//           - WriteMetricsFile_
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TO_Output.h"

//===========================================================================//
// Method         : WriteMetricsFile_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TO_Output_c::WriteMetricsFile_( 
      void ) const
{
   bool ok = true;

// TBD ???
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Warning( "Metrics file will be coming to a theater near you soon...\n" );

   return( ok );
}
