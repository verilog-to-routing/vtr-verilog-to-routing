//===========================================================================//
// Purpose : Supporting methods for the TO_Output post-processor class.
//           These methods support formatting and writing an architecture
//           spec file.
//
//           Private methods include:
//           - WriteArchitectureFile_
//
//===========================================================================//

#include "TIO_StringText.h"
#include "TIO_PrintHandler.h"

#include "TO_Output.h"

//===========================================================================//
// Method         : WriteArchitectureFile_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TO_Output_c::WriteArchitectureFile_( 
      const TAS_ArchitectureSpec_c& architectureSpec,
      const TOS_OutputOptions_c&    outputOptions ) const
{
   bool ok = true;

   const char* pszArchitectureFileName = outputOptions.srArchitectureFileName.data( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Info( "Writing %s file '%s'...\n",
                      TIO_SZ_OUTPUT_ARCHITECTURE_DEF_TYPE,
		      TIO_PSZ_STR( pszArchitectureFileName ));

   ok = printHandler.SetUserFileOutput( pszArchitectureFileName );
   if( ok )
   {
      printHandler.DisableOutput( TIO_PRINT_OUTPUT_ALL );
      printHandler.EnableOutput( TIO_PRINT_OUTPUT_USER_FILE );

      this->WriteFileHeader_( 0, "Architecture Spec", "//", "//" );
      architectureSpec.Print( 0 );
      this->WriteFileFooter_( 0, "//", "//" );

      printHandler.EnableOutput( TIO_PRINT_OUTPUT_ALL );
      printHandler.DisableOutput( TIO_PRINT_OUTPUT_USER_FILE );
   }
   else
   {
      printHandler.Error( "Failed to open %s file '%s' in \"%s\" mode.\n",
			  TIO_SZ_OUTPUT_ARCHITECTURE_DEF_TYPE,
			  TIO_PSZ_STR( pszArchitectureFileName ),
                          "write" );
   }
   return( ok );
}
