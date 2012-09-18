//===========================================================================//
// Purpose : Supporting methods for the TO_Output post-processor class.
//           These methods support formatting and writing an options store
//           file.
//
//           Private methods include:
//           - WriteOptionsFile_
//
//===========================================================================//

#include "TIO_StringText.h"
#include "TIO_PrintHandler.h"

#include "TO_Output.h"

//===========================================================================//
// Method         : WriteOptionsFile_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TO_Output_c::WriteOptionsFile_( 
      const TOS_OptionsStore_c&  optionsStore,
      const TOS_OutputOptions_c& outputOptions ) const
{
   bool ok = true;

   const char* pszOptionsFileName = outputOptions.srOptionsFileName.data( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Info( "Writing %s file '%s'...\n",
                      TIO_SZ_OUTPUT_OPTIONS_DEF_TYPE,
		      TIO_PSZ_STR( pszOptionsFileName ));

   ok = printHandler.SetUserFileOutput( pszOptionsFileName );
   if( ok )
   {
      printHandler.DisableOutput( TIO_PRINT_OUTPUT_ALL );
      printHandler.EnableOutput( TIO_PRINT_OUTPUT_USER_FILE );

      this->WriteFileHeader_( 0, "Program Options", "//", "//" );
      optionsStore.Print( 0 );
      this->WriteFileFooter_( 0, "//", "//" );

      printHandler.EnableOutput( TIO_PRINT_OUTPUT_ALL );
      printHandler.DisableOutput( TIO_PRINT_OUTPUT_USER_FILE );
   }
   else
   {
      printHandler.Error( "Failed to open %s file '%s' in \"%s\" mode.\n",
			  TIO_SZ_OUTPUT_OPTIONS_DEF_TYPE,
			  TIO_PSZ_STR( pszOptionsFileName ),
                          "write" );
   }
   return( ok );
}
