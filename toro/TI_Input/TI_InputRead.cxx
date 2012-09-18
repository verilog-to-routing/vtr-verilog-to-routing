//===========================================================================//
// Purpose : Supporting methods for the TI_Input pre-processor class.  
//           These methods support reading all input options and data files.
//
//           Private methods include:
//           - ReadFileProcess_
//           - ReadFileParser_
//           - GetFileNameList_
//           - LoadOptionsFiles_
//           - ApplyCommandLineOptions_
//
//===========================================================================//

#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"
#include "TIO_StringText.h"

#include "TOP_OptionsFile.h"

#include "TAP_ArchitectureFile.h"
#include "TAP_ArchitectureHandler.h"

#include "TFP_FabricFile.h"
#include "TFP_FabricHandler.h"

#include "TCP_CircuitFile.h"
#include "TCP_CircuitHandler.h"

#include "TAXP_ArchitectureXmlFile.h"
#include "TAXP_ArchitectureXmlHandler.h"

#include "TCBP_CircuitBlifFile.h"
#include "TCBP_CircuitBlifHandler.h"

#include "TI_Input.h"

//===========================================================================//
// Method         : ReadFileProcess_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::ReadFileProcess_( 
      TI_Input_c::TI_FileMode_e fileMode,
      const char*               pszFileType,
            bool                showInfoMessage )
{
   bool ok = true;

   TI_FileNameList_t fileNameList;
   if( this->GetFileNameList_( fileMode, &fileNameList ))
   {
      for( size_t i = 0; i < fileNameList.GetLength( ); ++i )
      {
         const TC_Name_c& fileName = *fileNameList[i];
	 const char* pszFileName = fileName.GetName( );

         ok = this->ReadFileParser_( fileMode, pszFileType, pszFileName, 
                                     showInfoMessage );
         if( !ok )
            break;
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : ReadFileParser_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::ReadFileParser_(
      TI_Input_c::TI_FileMode_e fileMode,
      const char*               pszFileType, 
      const char*               pszFileName,
            bool                showInfoMessage )
{
   bool ok = true;

   if( showInfoMessage )
   {  
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      printHandler.Info( "Reading %s file '%s'...\n",
                         TIO_PSZ_STR( pszFileType ),
                         TIO_PSZ_STR( pszFileName ));
   }

   // Attempt to open and read the given input file
   TIO_FileHandler_c fileHandler;
   ok = fileHandler.Open( pszFileName, TIO_FILE_OPEN_READ, pszFileType );
   if( ok )
   {
      if( fileMode == TI_Input_c::TI_FILE_OPTIONS )
      {
         ok = fileHandler.ApplyPreProcessor( );
      }
   }

   if( ok )
   {
      if( fileMode == TI_Input_c::TI_FILE_OPTIONS )
      {
         TOP_OptionsFile_c optionsFile( fileHandler.GetFileStream( ),
                                        fileHandler.GetFileName( ),
                                        this->poptionsStore_ );
         ok = optionsFile.IsValid( );

         if( this->pcontrolSwitches_->messageOptions.trace.read.options )
	 {
            this->poptionsStore_->Print( );
	 }
      }

      if( fileMode == TI_Input_c::TI_FILE_XML )
      {
         TAXP_ArchitectureXmlHandler_c architectureXmlHandler( this->parchitectureSpec_ );
         TAXP_ArchitectureXmlFile_c architectureXmlFile( fileHandler.GetFileStream( ),
                                                         fileHandler.GetFileName( ),
                                                         &architectureXmlHandler,
                                                         this->parchitectureSpec_ );
         ok = architectureXmlFile.IsValid( );

         if( this->pcontrolSwitches_->messageOptions.trace.read.xml )
	 {
            this->parchitectureSpec_->PrintXML( );
	 }
      }

      if( fileMode == TI_Input_c::TI_FILE_BLIF )
      {
         TCBP_CircuitBlifFile_c circuitBlifFile( fileHandler.GetFileStream( ),
                                                 fileHandler.GetFileName( ),
                                                 this->pcircuitDesign_ );
         ok = circuitBlifFile.IsValid( );

         if( this->pcontrolSwitches_->messageOptions.trace.read.blif )
	 {
            this->pcircuitDesign_->PrintBLIF( );
	 }
      }

      if( fileMode == TI_Input_c::TI_FILE_ARCHITECTURE )
      {
         TAP_ArchitectureHandler_c architectureHandler( this->parchitectureSpec_ );
         TAP_ArchitectureFile_c architectureFile( fileHandler.GetFileStream( ),
                                                  fileHandler.GetFileName( ),
                                                  &architectureHandler,
                                                  this->parchitectureSpec_ );
         ok = architectureFile.IsValid( );

         if( this->pcontrolSwitches_->messageOptions.trace.read.architecture )
	 {
            this->parchitectureSpec_->Print( );
	 }
      }

      if( fileMode == TI_Input_c::TI_FILE_FABRIC )
      {
         TFP_FabricHandler_c fabricHandler( this->pfabricModel_ );
         TFP_FabricFile_c fabricFile( fileHandler.GetFileStream( ),
                                      fileHandler.GetFileName( ),
                                      &fabricHandler,
                                      this->pfabricModel_ );
         ok = fabricFile.IsValid( );

         if( this->pcontrolSwitches_->messageOptions.trace.read.fabric )
	 {
            this->pfabricModel_->Print( );
	 }
      }

      if( fileMode == TI_Input_c::TI_FILE_CIRCUIT )
      {
         TCP_CircuitFile_c circuitFile( fileHandler.GetFileStream( ),
                                        fileHandler.GetFileName( ),
                                        this->pcircuitDesign_ );
         ok = circuitFile.IsValid( );

         if( this->pcontrolSwitches_->messageOptions.trace.read.circuit )
	 {
            this->pcircuitDesign_->Print( );
	 }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : GetFileNameList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::GetFileNameList_(
      TI_Input_c::TI_FileMode_e fileMode,
      TI_FileNameList_t*        pfileNameList ) const
{
   if( pfileNameList )
   {
      pfileNameList->Clear( );

      const TOS_InputOptions_c& inputOptions = this->pcontrolSwitches_->inputOptions;

      switch( fileMode )
      {
      case TI_Input_c::TI_FILE_OPTIONS:
         pfileNameList->SetCapacity( inputOptions.optionsFileNameList.GetLength( ));
         pfileNameList->Add( inputOptions.optionsFileNameList );
         break;

      case TI_Input_c::TI_FILE_XML:
         pfileNameList->SetCapacity( 1 );
         pfileNameList->Add( inputOptions.srXmlFileName );
         break;

      case TI_Input_c::TI_FILE_BLIF:
         pfileNameList->SetCapacity( 1 );
         pfileNameList->Add( inputOptions.srBlifFileName );
         break;

      case TI_Input_c::TI_FILE_ARCHITECTURE:
         pfileNameList->SetCapacity( 1 );
         pfileNameList->Add( inputOptions.srArchitectureFileName );
         break;

      case TI_Input_c::TI_FILE_FABRIC:
         pfileNameList->SetCapacity( 1 );
         pfileNameList->Add( inputOptions.srFabricFileName );
         break;

      case TI_Input_c::TI_FILE_CIRCUIT:
         pfileNameList->SetCapacity( 1 );
         pfileNameList->Add( inputOptions.srCircuitFileName );
         break;
      }
   }
   return( pfileNameList && pfileNameList->IsValid( ) ? true : false );
}

//===========================================================================//
// Method         : LoadOptionsFiles_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::LoadOptionsFiles_( 
      void )
{
   bool ok = true;

   const TCL_CommandLine_c& commandLine = *this->pcommandLine_;
   const TOS_ControlSwitches_c& controlSwitches = commandLine.GetControlSwitches( );
   const TOS_InputOptions_c& inputOptions = controlSwitches.inputOptions;
   const TOS_OptionsNameList_t& optionsFileNameList = inputOptions.optionsFileNameList;

   if( optionsFileNameList.IsValid( ))
   {
      // First, define a default base name based on first options file name
      const string& srOptionsFileName = optionsFileNameList[0]->GetName( );
      TOS_ControlSwitches_c* pcontrolSwitches = &this->poptionsStore_->controlSwitches;
      this->BuildDefaultBaseName_( srOptionsFileName,
                                   &pcontrolSwitches->srDefaultBaseName );

      // Next, define and read the list of one or more options file names
      TOS_InputOptions_c* pinputOptions = &pcontrolSwitches->inputOptions;
      pinputOptions->optionsFileNameList = optionsFileNameList;

      bool showInfoMessage = false;
      ok = this->ReadFileProcess_( TI_Input_c::TI_FILE_OPTIONS, 
                                   TIO_SZ_INPUT_OPTIONS_DEF_TYPE,
                                   showInfoMessage );

      // Finally, override input options based on any command line options
      this->ApplyCommandLineOptions_( );
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      printHandler.Error( "Missing options file name, no options file read.\n" );
      ok = false;
   }
   return( ok );
}

//===========================================================================//
// Method         : ApplyCommandLineOptions_
// Purpose        : Overrides current options store per command line options.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TI_Input_c::ApplyCommandLineOptions_(
      void )
{
   const TCL_CommandLine_c& commandLine = *this->pcommandLine_;
   const TOS_ControlSwitches_c& controlSwitches = commandLine.GetControlSwitches( );
   const TOS_RulesSwitches_c& rulesSwitches = commandLine.GetRulesSwitches( );

   TOS_ControlSwitches_c* pcontrolSwitches = &this->poptionsStore_->controlSwitches;
   TOS_InputOptions_c* pinputOptions = &pcontrolSwitches->inputOptions;
   TOS_OutputOptions_c* poutputOptions = &pcontrolSwitches->outputOptions;
   TOS_ExecuteOptions_c* pexecuteOptions = &pcontrolSwitches->executeOptions;

   TOS_RulesSwitches_c* prulesSwitches = &this->poptionsStore_->rulesSwitches;
   TOS_PackOptions_c* ppackOptions = &prulesSwitches->packOptions;
   TOS_PlaceOptions_c* pplaceOptions = &prulesSwitches->placeOptions;
   TOS_RouteOptions_c* prouteOptions = &prulesSwitches->routeOptions;

   if( controlSwitches.inputOptions.srXmlFileName.length( ))
   {
      pinputOptions->srXmlFileName = controlSwitches.inputOptions.srXmlFileName;
      pinputOptions->xmlFileEnable = controlSwitches.inputOptions.xmlFileEnable;
   }
   if( controlSwitches.inputOptions.srBlifFileName.length( ))
   {
      pinputOptions->srBlifFileName = controlSwitches.inputOptions.srBlifFileName;
      pinputOptions->blifFileEnable = controlSwitches.inputOptions.blifFileEnable;
   }
   if( controlSwitches.inputOptions.srArchitectureFileName.length( ))
   {
      pinputOptions->srArchitectureFileName = controlSwitches.inputOptions.srArchitectureFileName;
      pinputOptions->architectureFileEnable = controlSwitches.inputOptions.architectureFileEnable;
   }
   if( controlSwitches.inputOptions.srFabricFileName.length( ))
   {
      pinputOptions->srFabricFileName = controlSwitches.inputOptions.srFabricFileName;
      pinputOptions->fabricFileEnable = controlSwitches.inputOptions.fabricFileEnable;
   }
   if( controlSwitches.inputOptions.srCircuitFileName.length( ))
   {
      pinputOptions->srCircuitFileName = controlSwitches.inputOptions.srCircuitFileName;
      pinputOptions->circuitFileEnable = controlSwitches.inputOptions.circuitFileEnable;
   }

   if( controlSwitches.outputOptions.srLogFileName.length( ))
   {
      poutputOptions->srLogFileName = controlSwitches.outputOptions.srLogFileName;
      poutputOptions->logFileEnable = true;
   }
   if( controlSwitches.outputOptions.srXmlFileName.length( ))
   {
      poutputOptions->srXmlFileName = controlSwitches.outputOptions.srXmlFileName;
      poutputOptions->xmlFileEnable = true;
   }
   if( controlSwitches.outputOptions.srBlifFileName.length( ))
   {
      poutputOptions->srBlifFileName = controlSwitches.outputOptions.srBlifFileName;
      poutputOptions->blifFileEnable = true;
   }
   if( controlSwitches.outputOptions.srArchitectureFileName.length( ))
   {
      poutputOptions->srArchitectureFileName = controlSwitches.outputOptions.srArchitectureFileName;
      poutputOptions->architectureFileEnable = true;
   }
   if( controlSwitches.outputOptions.srFabricFileName.length( ))
   {
      poutputOptions->srFabricFileName = controlSwitches.outputOptions.srFabricFileName;
      poutputOptions->fabricFileEnable = true;
   }
   if( controlSwitches.outputOptions.srCircuitFileName.length( ))
   {
      poutputOptions->srCircuitFileName = controlSwitches.outputOptions.srCircuitFileName;
      poutputOptions->circuitFileEnable = true;
   }
   if( controlSwitches.outputOptions.metricsFileEnable )
   {
      poutputOptions->metricsFileEnable = controlSwitches.outputOptions.metricsFileEnable;
   }

   if( controlSwitches.executeOptions.toolMask != TOS_EXECUTE_TOOL_UNDEFINED )
      pexecuteOptions->toolMask = controlSwitches.executeOptions.toolMask;

   if( rulesSwitches.packOptions.clusterNetsMode != TOS_PACK_CLUSTER_NETS_UNDEFINED )
      ppackOptions->clusterNetsMode = rulesSwitches.packOptions.clusterNetsMode;
   if( rulesSwitches.packOptions.affinityMode != TOS_PACK_AFFINITY_UNDEFINED )
      ppackOptions->affinityMode = rulesSwitches.packOptions.affinityMode;
   if( TCTF_IsNEQ( rulesSwitches.packOptions.areaWeight, 0.0 ))
      ppackOptions->areaWeight = rulesSwitches.packOptions.areaWeight;
   if( TCTF_IsNEQ( rulesSwitches.packOptions.netsWeight, 0.0 ))
      ppackOptions->netsWeight = rulesSwitches.packOptions.netsWeight;
   if( rulesSwitches.packOptions.costMode != TOS_PACK_COST_UNDEFINED )
      ppackOptions->costMode = rulesSwitches.packOptions.costMode;

   if( rulesSwitches.placeOptions.randomSeed != 0 )
      pplaceOptions->randomSeed = rulesSwitches.placeOptions.randomSeed;
   if( TCTF_IsNEQ( rulesSwitches.placeOptions.initTemp, 0.0 ))
      pplaceOptions->initTemp = rulesSwitches.placeOptions.initTemp;
   if( TCTF_IsNEQ( rulesSwitches.placeOptions.exitTemp, 0.0 ))
      pplaceOptions->exitTemp = rulesSwitches.placeOptions.exitTemp;
   if( TCTF_IsNEQ( rulesSwitches.placeOptions.reduceTemp, 0.0 ))
      pplaceOptions->reduceTemp = rulesSwitches.placeOptions.reduceTemp;
   if( TCTF_IsNEQ( rulesSwitches.placeOptions.innerNum, 0.0 ))
      pplaceOptions->innerNum = rulesSwitches.placeOptions.innerNum;
   if( rulesSwitches.placeOptions.costMode != TOS_PLACE_COST_UNDEFINED )
      pplaceOptions->costMode = rulesSwitches.placeOptions.costMode;
   if( TCTF_IsNEQ( rulesSwitches.placeOptions.timingCostFactor, 0.0 ))
      pplaceOptions->timingCostFactor = rulesSwitches.placeOptions.timingCostFactor;
   if( rulesSwitches.placeOptions.timingUpdateCount != 0 )
      pplaceOptions->timingUpdateCount = rulesSwitches.placeOptions.timingUpdateCount;
   if( TCTF_IsNEQ( rulesSwitches.placeOptions.slackInitWeight, 0.0 ))
      pplaceOptions->slackInitWeight = rulesSwitches.placeOptions.slackInitWeight;
   if( TCTF_IsNEQ( rulesSwitches.placeOptions.slackFinalWeight, 0.0 ))
      pplaceOptions->slackFinalWeight = rulesSwitches.placeOptions.slackFinalWeight;

   if( rulesSwitches.routeOptions.abstractMode != TOS_ROUTE_ABSTRACT_UNDEFINED )
      prouteOptions->abstractMode = rulesSwitches.routeOptions.abstractMode;
   if( rulesSwitches.routeOptions.windowSize != 0 )
      prouteOptions->windowSize = rulesSwitches.routeOptions.windowSize;
   if( rulesSwitches.routeOptions.channelWidth != 0 )
      prouteOptions->channelWidth = rulesSwitches.routeOptions.channelWidth;
   if( rulesSwitches.routeOptions.maxIterations != 0 )
      prouteOptions->maxIterations = rulesSwitches.routeOptions.maxIterations;
   if( TCTF_IsNEQ( rulesSwitches.routeOptions.histCongestionFactor, 0.0 ))
      prouteOptions->histCongestionFactor = rulesSwitches.routeOptions.histCongestionFactor;
   if( TCTF_IsNEQ( rulesSwitches.routeOptions.initCongestionFactor, 0.0 ))
      prouteOptions->initCongestionFactor = rulesSwitches.routeOptions.initCongestionFactor;
   if( TCTF_IsNEQ( rulesSwitches.routeOptions.presentCongestionFactor, 0.0 ))
      prouteOptions->presentCongestionFactor = rulesSwitches.routeOptions.presentCongestionFactor;
   if( TCTF_IsNEQ( rulesSwitches.routeOptions.bendCostFactor, 0.0 ))
      prouteOptions->bendCostFactor = rulesSwitches.routeOptions.bendCostFactor;
   if( rulesSwitches.routeOptions.resourceMode != TOS_ROUTE_RESOURCE_UNDEFINED )
      prouteOptions->resourceMode = rulesSwitches.routeOptions.resourceMode;
   if( rulesSwitches.routeOptions.costMode != TOS_ROUTE_COST_UNDEFINED )
      prouteOptions->costMode = rulesSwitches.routeOptions.costMode;
   if( TCTF_IsNEQ( rulesSwitches.routeOptions.timingAStarFactor, 0.0 ))
      prouteOptions->timingAStarFactor = rulesSwitches.routeOptions.timingAStarFactor;
   if( TCTF_IsNEQ( rulesSwitches.routeOptions.timingMaxCriticality, 0.0 ))
      prouteOptions->timingMaxCriticality = rulesSwitches.routeOptions.timingMaxCriticality;
   if( TCTF_IsNEQ( rulesSwitches.routeOptions.slackCriticality, 0.0 ))
      prouteOptions->slackCriticality = rulesSwitches.routeOptions.slackCriticality;
}
