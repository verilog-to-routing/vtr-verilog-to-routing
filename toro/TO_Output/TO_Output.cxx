//===========================================================================//
// Purpose : Method definitions for the TO_Output post-processor class.
//
//           Public methods include:
//           - TO_Output_c, ~TO_Output_c
//           - Init
//           - Apply
//           - IsValid
//
//           Private methods include:
//           - WriteFileHeader_
//           - WriteFileFooter_
//
//===========================================================================//

#include "TC_StringUtils.h"

#include "TIO_StringText.h"
#include "TIO_SkinHandler.h"
#include "TIO_PrintHandler.h"

#include "TO_Output.h"

//===========================================================================//
// Method         : TO_Output_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TO_Output_c::TO_Output_c( 
      void )
      :
      pcommandLine_( 0 ),
      poptionsStore_( 0 ),
      pcontrolSwitches_( 0 ),
      prulesSwitches_( 0 ),
      pfloorplanStore_( 0 ),
      parchitectureSpec_( 0 ),
      pfabricModel_( 0 ),
      pcircuitDesign_( 0 )
{
}

//===========================================================================//
// Method         : TO_Output_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TO_Output_c::~TO_Output_c( 
      void )
{
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TO_Output_c::Init( 
      const TCL_CommandLine_c&    commandLine,
      const TOS_OptionsStore_c&   optionsStore,
      const TFS_FloorplanStore_c& floorplanStore )
{
   this->pcommandLine_ = &commandLine;

   this->poptionsStore_ = &optionsStore;
   this->pcontrolSwitches_ = &optionsStore.controlSwitches;
   this->prulesSwitches_ = &optionsStore.rulesSwitches;

   this->pfloorplanStore_ = &floorplanStore;
   this->parchitectureSpec_ = &floorplanStore.architectureSpec;
   this->pfabricModel_ = &floorplanStore.fabricModel;
   this->pcircuitDesign_ = &floorplanStore.circuitDesign;

   return( this->IsValid( ));
}

//===========================================================================//
// Method         : Apply
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TO_Output_c::Apply( 
      const TCL_CommandLine_c&    commandLine,
      const TOS_OptionsStore_c&   optionsStore,
      const TFS_FloorplanStore_c& floorplanStore )
{
   bool ok = this->Init( commandLine, optionsStore, floorplanStore );
   if( ok )
   {
      ok = this->Apply( );
   }
   return( ok );
}

//===========================================================================//
bool TO_Output_c::Apply( 
      void ) 
{
   bool ok = true;

   if( this->IsValid( ))
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      printHandler.Info( "Post-processing...\n" );

      const TAS_ArchitectureSpec_c& architectureSpec = *this->parchitectureSpec_;
      const TFM_FabricModel_c& fabricModel = *this->pfabricModel_;
      const TFV_FabricView_c& fabricView = *this->pfabricModel_->GetFabricView( );
      const TCD_CircuitDesign_c& circuitDesign = *this->pcircuitDesign_;

      const TOS_OptionsStore_c& optionsStore = *this->poptionsStore_;
      const TOS_InputOptions_c& inputOptions = this->pcontrolSwitches_->inputOptions;
      const TOS_OutputOptions_c& outputOptions = this->pcontrolSwitches_->outputOptions;
      if( ok && outputOptions.optionsFileEnable )
      { 
         ok = this->WriteOptionsFile_( optionsStore, outputOptions );
      }
      if( ok && outputOptions.xmlFileEnable )
      { 
         ok = this->WriteXmlFile_( architectureSpec, outputOptions );
      }
      if( ok && outputOptions.blifFileEnable )
      { 
	 ok = this->WriteBlifFile_( circuitDesign, outputOptions );
      }
      if( ok && outputOptions.architectureFileEnable )
      { 
         ok = this->WriteArchitectureFile_( architectureSpec, outputOptions );
      }
      if( ok && outputOptions.fabricFileEnable )
      { 
         ok = this->WriteFabricFile_( fabricModel, outputOptions );
      }
      if( ok && outputOptions.circuitFileEnable )
      { 
         ok = this->WriteCircuitFile_( circuitDesign, outputOptions );
      }
      if( ok && outputOptions.laffFileEnable )
      { 
         ok = this->WriteLaffFile_( fabricView, outputOptions );
      }
      if( ok && outputOptions.metricsEmailEnable )
      { 
         ok = this->SendMetricsEmail_( inputOptions, outputOptions );
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : IsValid
// Author         : Jon Sykes
//---------------------------------------------------------------------------//
// Version history
// 11/19/02 jsykes: Original
//===========================================================================//
bool TO_Output_c::IsValid( 
      void ) const
{
   return( this->pcommandLine_ &&
           this->poptionsStore_ && 
           this->pfloorplanStore_ ?
           true : false );
} 

//===========================================================================//
// Method         : WriteFileHeader_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteFileHeader_( 
            FILE* pfile,
      const char* pszFileType,
      const char* pszPrefix,
      const char* pszPostfix,
      const char* pszMessage ) const
{
   TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );
   const char* pszProgramTitle = skinHandler.GetProgramTitle( );
   const char* pszProgramCopyright = skinHandler.GetCopyright( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   size_t spaceLen = 0;
   size_t separatorLen = TIO_FORMAT_BANNER_LEN_DEF;

   char szSeparator[TIO_FORMAT_STRING_LEN_HEAD_FOOT];
   char szDateTimeStamp[TIO_FORMAT_STRING_LEN_DATE_TIME];

   TC_FormatStringFilled( '_', szSeparator, separatorLen );
   TC_AddStringPrefix( szSeparator, pszPrefix );
   TC_AddStringPostfix( szSeparator, pszPostfix );

   TC_FormatStringDateTimeStamp( szDateTimeStamp, TIO_FORMAT_BANNER_LEN_DEF );

   printHandler.WriteCenter( pfile, spaceLen, szSeparator, separatorLen );
   printHandler.WriteCenter( pfile, spaceLen, pszProgramTitle, separatorLen,
                             pszPrefix, pszPostfix );
   printHandler.WriteCenter( pfile, spaceLen, TIO_SZ_BUILD_VERSION, separatorLen,
                             pszPrefix, pszPostfix );
   printHandler.WriteCenter( pfile, spaceLen, pszProgramCopyright, separatorLen,
                             pszPrefix, pszPostfix );
   printHandler.WriteCenter( pfile, spaceLen, szSeparator, separatorLen );
   printHandler.WriteCenter( pfile, spaceLen, szDateTimeStamp, separatorLen,
                             pszPrefix, pszPostfix );
   printHandler.WriteCenter( pfile, spaceLen, pszFileType, separatorLen,
                             pszPrefix, pszPostfix );
   printHandler.WriteCenter( pfile, spaceLen, szSeparator, separatorLen );
   
   if( pszMessage )
   {
      printHandler.WriteCenter( pfile, spaceLen, pszMessage, separatorLen,
                                pszPrefix, pszPostfix );
      printHandler.WriteCenter( pfile, spaceLen, szSeparator, separatorLen );
   }
}

//===========================================================================//
// Method         : WriteFileFooter_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteFileFooter_( 
            FILE* pfile,
      const char* pszPrefix,
      const char* pszPostfix ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   size_t spaceLen = 0;
   size_t separatorLen = TIO_FORMAT_BANNER_LEN_DEF;

   char szSeparator[TIO_FORMAT_STRING_LEN_HEAD_FOOT];

   TC_FormatStringFilled( '_', szSeparator, separatorLen );
   TC_AddStringPrefix( szSeparator, pszPrefix );
   TC_AddStringPostfix( szSeparator, pszPostfix );

   printHandler.WriteCenter( pfile, spaceLen, szSeparator, separatorLen );
}
