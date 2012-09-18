//===========================================================================//
// Purpose : Supporting methods for the TO_Output post-processor class.
//           These methods support formatting and writing a fabric model
//           file.
//
//           Private methods include:
//           - WriteFabricFile_
//
//===========================================================================//

#include "TIO_StringText.h"
#include "TIO_PrintHandler.h"

#include "TO_Output.h"

//===========================================================================//
// Method         : WriteFabricFile_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TO_Output_c::WriteFabricFile_( 
      const TFM_FabricModel_c&   fabricModel,
      const TOS_OutputOptions_c& outputOptions ) const
{
   bool ok = true;

   const char* pszFabricFileName = outputOptions.srFabricFileName.data( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Info( "Writing %s file '%s'...\n",
                      TIO_SZ_OUTPUT_FABRIC_DEF_TYPE,
		      TIO_PSZ_STR( pszFabricFileName ));

   ok = printHandler.SetUserFileOutput( pszFabricFileName );
   if( ok )
   {
      printHandler.DisableOutput( TIO_PRINT_OUTPUT_ALL );
      printHandler.EnableOutput( TIO_PRINT_OUTPUT_USER_FILE );

      this->WriteFileHeader_( 0, "Fabric Model", "//", "//" );
      fabricModel.Print( 0 );
      this->WriteFileFooter_( 0, "//", "//" );

      printHandler.EnableOutput( TIO_PRINT_OUTPUT_ALL );
      printHandler.DisableOutput( TIO_PRINT_OUTPUT_USER_FILE );
   }
   else
   {
      printHandler.Error( "Failed to open %s file '%s' in \"%s\" mode.\n",
			  TIO_SZ_OUTPUT_FABRIC_DEF_TYPE,
			  TIO_PSZ_STR( pszFabricFileName ),
                          "write" );
   }
   return( ok );
}
