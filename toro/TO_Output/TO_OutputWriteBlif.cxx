//===========================================================================//
// Purpose : Supporting methods for the TO_Output post-processor class.
//           These methods support formatting and writing a circuit BLIF 
//           file.
//
//           Private methods include:
//           - WriteBlifFile_
//
//===========================================================================//

#include "TIO_StringText.h"
#include "TIO_PrintHandler.h"

#include "TO_Output.h"

//===========================================================================//
// Method         : WriteBlifFile_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TO_Output_c::WriteBlifFile_( 
      const TCD_CircuitDesign_c& circuitDesign,
      const TOS_OutputOptions_c& outputOptions ) const
{
   bool ok = true;

   const char* pszBlifFileName = outputOptions.srBlifFileName.data( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Info( "Writing %s file '%s'...\n",
                      TIO_SZ_OUTPUT_BLIF_DEF_TYPE,
		      TIO_PSZ_STR( pszBlifFileName ));

   ok = printHandler.SetUserFileOutput( pszBlifFileName );
   if( ok )
   {
      printHandler.DisableOutput( TIO_PRINT_OUTPUT_ALL );
      printHandler.EnableOutput( TIO_PRINT_OUTPUT_USER_FILE );

      this->WriteFileHeader_( 0, "Circuit Design (VPR's BLIF)", "#", "#" );
      circuitDesign.PrintBLIF( 0 );
      this->WriteFileFooter_( 0, "#", "#" );

      printHandler.EnableOutput( TIO_PRINT_OUTPUT_ALL );
      printHandler.DisableOutput( TIO_PRINT_OUTPUT_USER_FILE );
   }
   else
   {
      printHandler.Error( "Failed to open %s file '%s' in \"%s\" mode.\n",
			  TIO_SZ_OUTPUT_BLIF_DEF_TYPE,
			  TIO_PSZ_STR( pszBlifFileName ),
                          "write" );
   }
   return( ok );
}
