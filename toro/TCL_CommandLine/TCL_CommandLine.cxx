//===========================================================================//
// Purpose : Method definitions for the TCL_CommandLine class.
//
//           Public methods include:
//           - TCL_CommandLine, ~TCL_CommandLine
//           - Parse
//           - ExtractArgString
//
//           Private methods include:
//           - ParseBool_
//           - ParseExecuteToolMode_
//           - ParsePlaceCostMode_
//           - ParseRouteAbstractMode_
//           - ParseRouteResourceMode_
//           - ParseRouteCostMode_
//
//===========================================================================//

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TCL_CommandLine.h"

//===========================================================================//
// Method         : TCL_CommandLine_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TCL_CommandLine_c::TCL_CommandLine_c( 
      void )
      :
      argc_( 0 ),
      pargv_( 0 )
{
}

//===========================================================================//
TCL_CommandLine_c::TCL_CommandLine_c( 
      int   argc,
      char* argv[],
      bool* pok )
      :
      argc_( 0 ),
      pargv_( 0 )
{
   bool ok = this->Parse( argc, argv );
   if( pok )
   {
      *pok = ok;
   }
}

//===========================================================================//
// Method         : ~TCL_CommandLine_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TCL_CommandLine_c::~TCL_CommandLine_c( 
      void )
{
}

//===========================================================================//
// Method         : Parse
// Purpose        : Handle parsing the given program command-line arguments
//                  for required and optional switches.  An optional usage
//                  help message is displayed if necessary.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TCL_CommandLine_c::Parse(
      int   argc,
      char* argv[] )
{
   // Save the original command-line arguments
   this->argc_ = argc;
   this->pargv_ = argv;

   bool ok = true;
   bool showHelp = false;
   bool showVersion = false;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   TOS_InputOptions_c* pinputOptions = &this->controlSwitches_.inputOptions;
   TOS_OutputOptions_c* poutputOptions = &this->controlSwitches_.outputOptions;
   TOS_MessageOptions_c* pmessageOptions = &this->controlSwitches_.messageOptions;
   TOS_ExecuteOptions_c* pexecuteOptions = &this->controlSwitches_.executeOptions;

   TOS_PackOptions_c* ppackOptions = &this->rulesSwitches_.packOptions;
   TOS_PlaceOptions_c* pplaceOptions = &this->rulesSwitches_.placeOptions;
   TOS_RouteOptions_c* prouteOptions = &this->rulesSwitches_.routeOptions;

   // Iterate, parsing any command-line arguments
   argc = ( --argc <= 0 ? 0 : argc );

   while( ok && argc )
   {
      --argc; 
      ++argv;

      if(( TC_stricmp( *argv, "-help" ) == 0 ) || 
         ( TC_stricmp( *argv, "-h" ) == 0 ))
      {
         argc = 0;
         ok = false;
         showHelp = true;
      }
      else if(( TC_stricmp( *argv, "-version" ) == 0 ) || 
              ( TC_stricmp( *argv, "-v" ) == 0 ))
      {
         argc = 0;
         ok = false;
         showVersion = true;
      }
      else if(( TC_stricmp( *argv, "-options" ) == 0 ) || 
              ( TC_stricmp( *argv, "-o" ) == 0 ) ||
              ( TC_stricmp( *argv, "-i" ) == 0 ))
      { 
         while( ok && argc )
         {
            argc = ok ? --argc : 1;
            ok = *++argv ? true : false;   
            if( !ok )
               break;

            pinputOptions->optionsFileNameList.Add( *argv );

            if( *( argv + 1 ) && **( argv + 1 ) == '-' )
               break;
         }
      }
      else if(( TC_stricmp( *argv, "-xml" ) == 0 ) ||
              ( TC_stricmp( *argv, "-x" ) == 0 ))
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            pinputOptions->srXmlFileName = *argv;
	    pinputOptions->xmlFileEnable = ok;
	 }
      }
      else if(( TC_stricmp( *argv, "-blif" ) == 0 ) ||
              ( TC_stricmp( *argv, "-b" ) == 0 ))
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            pinputOptions->srBlifFileName = *argv;
	    pinputOptions->blifFileEnable = ok;
	 }
      }
      else if(( TC_stricmp( *argv, "-architecture" ) == 0 ) ||
              ( TC_stricmp( *argv, "-a" ) == 0 ))
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            pinputOptions->srArchitectureFileName = *argv;
	    pinputOptions->architectureFileEnable = ok;
	 }
      }
      else if(( TC_stricmp( *argv, "-fabric" ) == 0 ) ||
              ( TC_stricmp( *argv, "-f" ) == 0 ))
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            pinputOptions->srFabricFileName = *argv;
	    pinputOptions->fabricFileEnable = ok;
	 }
      }
      else if(( TC_stricmp( *argv, "-circuit" ) == 0 ) ||
              ( TC_stricmp( *argv, "-c" ) == 0 ))
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            pinputOptions->srCircuitFileName = *argv;
	    pinputOptions->circuitFileEnable = ok;
	 }
      }
      else if(( TC_stricmp( *argv, "+architecture" ) == 0 ) ||
              ( TC_stricmp( *argv, "+a" ) == 0 ))
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            poutputOptions->srArchitectureFileName = *argv;
	    pinputOptions->architectureFileEnable = ok;
	 }
      }
      else if(( TC_stricmp( *argv, "+fabric" ) == 0 ) ||
              ( TC_stricmp( *argv, "+f" ) == 0 ))
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            poutputOptions->srFabricFileName = *argv;
	    pinputOptions->fabricFileEnable = ok;
	 }
      }
      else if(( TC_stricmp( *argv, "+circuit" ) == 0 ) ||
              ( TC_stricmp( *argv, "+c" ) == 0 ))
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            poutputOptions->srCircuitFileName = *argv;
	    pinputOptions->circuitFileEnable = ok;
	 }
      }
      else if(( TC_stricmp( *argv, "-log" ) == 0 ) ||
              ( TC_stricmp( *argv, "-l" ) == 0 ) ||
              ( TC_stricmp( *argv, "+log" ) == 0 ) ||
              ( TC_stricmp( *argv, "+l" ) == 0 ))
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            poutputOptions->srLogFileName = *argv;
	 }
      }
      else if(( TC_stricmp( *argv, "-execute" ) == 0 ) || 
              ( TC_stricmp( *argv, "-e" ) == 0 ))
      { 
         while( ok && argc )
         {
            argc = ok ? --argc : 1;
            ok = *++argv ? true : false;   
            if( !ok )
               break;

            ok = this->ParseExecuteToolMode_( *argv, &pexecuteOptions->toolMask );
            if( !ok )
            {
               printHandler.Error( "Invalid argument '%s'!\n", *argv );
               argc = 0;
               break;
            }
            if( *( argv + 1 ) && **( argv + 1 ) == '-' )
               break;
         }
      }

      // VPR file name options...
      else if( TC_stricmp( *argv, "-xml_file" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            pinputOptions->srXmlFileName = *argv;
      }
      else if( TC_stricmp( *argv, "-blif_file" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            pinputOptions->srBlifFileName = *argv;
      }
      else if( TC_stricmp( *argv, "-net_file" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            poutputOptions->srVPR_NetFileName = *argv;
      }
      else if( TC_stricmp( *argv, "-place_file" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            poutputOptions->srVPR_PlaceFileName = *argv;
      }
      else if( TC_stricmp( *argv, "-route_file" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            poutputOptions->srVPR_RouteFileName = *argv;
      }
      else if( TC_stricmp( *argv, "-sdc_file" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            poutputOptions->srVPR_SDC_FileName = *argv;
      }
      else if( TC_stricmp( *argv, "-outfile_prefix" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            poutputOptions->srVPR_OutputFilePrefix = *argv;
      }

      // VPR general options...
      else if( TC_stricmp( *argv, "-nodisp" ) == 0 )
      { 
         printHandler.Error( "Invalid argument '%s'!\n", *argv );
         argc = 0;
      }
      else if( TC_stricmp( *argv, "-auto" ) == 0 )
      { 
         printHandler.Error( "Invalid argument '%s'!\n", *argv );
         argc = 0;
      }
      else if( TC_stricmp( *argv, "-pack" ) == 0 )
      {
         pexecuteOptions->toolMask |= TOS_EXECUTE_TOOL_PACK;
      }
      else if( TC_stricmp( *argv, "-place" ) == 0 )
      {
         pexecuteOptions->toolMask |= TOS_EXECUTE_TOOL_PLACE;
      }
      else if( TC_stricmp( *argv, "-route" ) == 0 )
      {
         pexecuteOptions->toolMask |= TOS_EXECUTE_TOOL_ROUTE;
      }
      else if( TC_stricmp( *argv, "-timing_analysis" ) == 0 )
      {
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            bool enabled;
            if( this->ParseBool_( *argv, &enabled ))
            {
               ppackOptions->costMode = ( enabled ? TOS_PACK_COST_TIMING_DRIVEN : 
                                                    TOS_PACK_COST_ROUTABILITY_DRIVEN );
	       pplaceOptions->costMode = ( enabled ? TOS_PLACE_COST_PATH_TIMING_DRIVEN : 
                                                     TOS_PLACE_COST_ROUTABILITY_DRIVEN );
	       prouteOptions->costMode = ( enabled ? TOS_ROUTE_COST_TIMING_DRIVEN : 
					             TOS_ROUTE_COST_BREADTH_FIRST );
            }
            else
            {
               printHandler.Error( "Invalid argument '%s'!\n", *argv );
               argc = 0;
	       ok = false;
            }
	 }
      }
      else if( TC_stricmp( *argv, "-timing_analyze_only_with_net_delay" ) == 0 )
      {
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            ppackOptions->fixedDelay = atof( *argv );
            pplaceOptions->fixedDelay = atof( *argv );
            prouteOptions->fixedDelay = atof( *argv );
         }
      }
      else if( TC_stricmp( *argv, "-full_stats" ) == 0 )
      {
         poutputOptions->metricsFileEnable = true;
      }
      else if( TC_stricmp( *argv, "-echo_file" ) == 0 )
      {
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            bool enabled;
            if( this->ParseBool_( *argv, &enabled ))
            {
               pmessageOptions->trace.vpr.echoFile = enabled;
            }
            else
            {
               printHandler.Error( "Invalid argument '%s'!\n", *argv );
               argc = 0;
	       ok = false;
            }
	 }
      }

      // VPR packer options...
      else if( TC_stricmp( *argv, "-cluster_only" ) == 0 )
      {
         pexecuteOptions->toolMask = TOS_EXECUTE_TOOL_PACK;
      }
      else if( TC_stricmp( *argv, "-connection_driven_clustering" ) == 0 )
      {
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            bool enabled;
            if( this->ParseBool_( *argv, &enabled ))
            {
               ppackOptions->clusterNetsMode = ( enabled ? TOS_PACK_CLUSTER_NETS_MIN_CONNECTIONS :
                                                           TOS_PACK_CLUSTER_NETS_MAX_CONNECTIONS );
            }
            else
            {
               printHandler.Error( "Invalid argument '%s'!\n", *argv );
               argc = 0;
	       ok = false;
            }
	 }
      }
      else if( TC_stricmp( *argv, "-allow_unrelated_clustering" ) == 0 )
      {
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            bool enabled;
            if( this->ParseBool_( *argv, &enabled ))
            {
               ppackOptions->affinityMode = ( enabled ? TOS_PACK_AFFINITY_NONE :
                                                        TOS_PACK_AFFINITY_ANY );
            }
            else
            {
               printHandler.Error( "Invalid argument '%s'!\n", *argv );
               argc = 0;
	       ok = false;
            }
	 }
      }
      else if( TC_stricmp( *argv, "-alpha_clustering" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            ppackOptions->areaWeight = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-beta_clustering" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            ppackOptions->netsWeight = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-timing_driven_clustering" ) == 0 )
      {
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            bool enabled;
            if( this->ParseBool_( *argv, &enabled ))
            {
               ppackOptions->costMode = ( enabled ? TOS_PACK_COST_TIMING_DRIVEN :
                                                    TOS_PACK_COST_ROUTABILITY_DRIVEN );
            }
            else
            {
               printHandler.Error( "Invalid argument '%s'!\n", *argv );
               argc = 0;
	       ok = false;
            }
	 }
      }

      // VPR placer options...
      else if( TC_stricmp( *argv, "-seed" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            pplaceOptions->randomSeed = atoi( *argv );
      }
      else if( TC_stricmp( *argv, "-init_t" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            pplaceOptions->initTemp = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-exit_t" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            pplaceOptions->exitTemp = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-alpha_t" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            pplaceOptions->reduceTemp = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-inner_num" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            pplaceOptions->innerNum = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-fix_pins" ) == 0 )
      { 
         printHandler.Error( "Invalid argument '%s'!\n", *argv );
         argc = 0;
      }
      else if( TC_stricmp( *argv, "-place_algorithm" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            ok = this->ParsePlaceCostMode_( *argv, &pplaceOptions->costMode );
            if( !ok )
            {
               printHandler.Error( "Invalid argument '%s'!\n", *argv );
               argc = 0;
            }
	 }
      }
      else if( TC_stricmp( *argv, "-timing_tradeoff" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            pplaceOptions->timingCostFactor = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-recompute_crit_iter" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            pplaceOptions->timingUpdateInt = atoi( *argv );
      }
      else if( TC_stricmp( *argv, "-inner_loop_recompute_divider" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            pplaceOptions->timingUpdateCount = atoi( *argv );
      }
      else if( TC_stricmp( *argv, "-td_place_exp_first" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            pplaceOptions->slackInitWeight = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-td_place_exp_last" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            pplaceOptions->slackFinalWeight = atof( *argv );
      }

      // VPR router options...
      else if( TC_stricmp( *argv, "-route_type" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            ok = this->ParseRouteAbstractMode_( *argv, &prouteOptions->abstractMode );
            if( !ok )
            {
               printHandler.Error( "Invalid argument '%s'!\n", *argv );
               argc = 0;
            }
	 }
      }
      else if( TC_stricmp( *argv, "-bb_factor" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            prouteOptions->windowSize = atoi( *argv );
      }
      else if( TC_stricmp( *argv, "-route_chan_width" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            prouteOptions->channelWidth = atoi( *argv );
      }
      else if( TC_stricmp( *argv, "-max_router_iterations" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            prouteOptions->maxIterations = atoi( *argv );
      }
      else if( TC_stricmp( *argv, "-acc_fac" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            prouteOptions->histCongestionFactor = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-initial_pres_fac" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            prouteOptions->initCongestionFactor = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-first_iter_pres_fac" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            prouteOptions->initCongestionFactor = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-pres_fac_mult" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            prouteOptions->presentCongestionFactor = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-bend_cost" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            prouteOptions->bendCostFactor = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-base_cost_type" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            ok = this->ParseRouteResourceMode_( *argv, &prouteOptions->resourceMode );
            if( !ok )
            {
               printHandler.Error( "Invalid argument '%s'!\n", *argv );
               argc = 0;
            }
	 }
      }
      else if( TC_stricmp( *argv, "-router_algorithm" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
	 {
            ok = this->ParseRouteCostMode_( *argv, &prouteOptions->costMode );
            if( !ok )
            {
               printHandler.Error( "Invalid argument '%s'!\n", *argv );
               argc = 0;
            }
	 }
      }
      else if( TC_stricmp( *argv, "-astar_fac" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            prouteOptions->timingAStarFactor = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-max_criticality" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            prouteOptions->timingMaxCriticality = atof( *argv );
      }
      else if( TC_stricmp( *argv, "-criticality_exp" ) == 0 )
      { 
         argc = ok ? --argc : 1;
         ok = *++argv ? true : false;   
         if( ok )
            prouteOptions->slackCriticality = atof( *argv );
      }
      else
      {
         printHandler.Error( "Invalid argument '%s'!\n", *argv );
         argc = 0;
         ok = false;
      }
   }

   if( !ok && ( argc > 0 ))   
   {
      // Stopped parsing prematurely, probably due to missing arguments
      printHandler.Error( "Missing argument(s)!\n" );
      showHelp = true;
      ok = false;
   }
   else if( ok && !pinputOptions->optionsFileNameList.IsValid( ))
   {
      // Report detected missing options file argument
      printHandler.Error( "Missing options file name argument!\n" );
      showHelp = true;
      ok = false;
   }

   if( showVersion )
   {
      printHandler.WriteVersion( );
   }
   else if( showHelp )
   {
      printHandler.WriteHelp( );
   }
   else if( !ok )
   {
      printHandler.WriteHelp( );
   }
   return( ok );
}

//===========================================================================//
// Method         : ExtractArgString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TCL_CommandLine_c::ExtractArgString( 
      string* psrArgString ) const
{
   if( psrArgString )
   {   
      *psrArgString = "";

      int argc = this->GetArgc( );
      if( argc > 1 )
      {
         char** pargv = this->GetArgv( );

         for( int i = 1; i < argc; ++i )
         {
            *psrArgString += pargv[i] ;
            *psrArgString += ( i < argc - 1 ? " " : "" );
         }
      }
   }
}

//===========================================================================//
// Method         : ParseBool_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TCL_CommandLine_c::ParseBool_( 
      const char* pszBool,
            bool* pbool )
{
   bool ok = true;

   if( pszBool )
   {
      if(( TC_stricmp( pszBool, "on" ) == 0 ) ||
         ( TC_stricmp( pszBool, "yes" ) == 0 ) ||
	 ( TC_stricmp( pszBool, "true" ) == 0 ))
      {
         *pbool = true;
      }
      else if(( TC_stricmp( pszBool, "off" ) == 0 ) ||
              ( TC_stricmp( pszBool, "no" ) == 0 ) ||
              ( TC_stricmp( pszBool, "false" ) == 0 ))
      {
         *pbool = false;
      } 
      else
      {
	 ok = false;
      } 
   }
   return( ok );
}

//===========================================================================//
// Method         : ParseExecuteToolMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TCL_CommandLine_c::ParseExecuteToolMode_( 
      const char* pszMode,
            int*  pmode )
{
   bool ok = true;

   if( pszMode )
   {
      if( TC_stricmp( pszMode, "all" ) == 0 )
      {
         *pmode = TOS_EXECUTE_TOOL_ALL;
      }
      else if( TC_stricmp( pszMode, "none" ) == 0 )
      {
         *pmode = TOS_EXECUTE_TOOL_NONE;
      }
      else if( TC_stricmp( pszMode, "pack" ) == 0 )
      {
         *pmode |= TOS_EXECUTE_TOOL_PACK;
      }
      else if( TC_stricmp( pszMode, "place" ) == 0 )
      {
         *pmode |= TOS_EXECUTE_TOOL_PACK;
      } 
      else if( TC_stricmp( pszMode, "route" ) == 0 )
      {
         *pmode |= TOS_EXECUTE_TOOL_PACK;
      } 
      else
      {
	 ok = false;
      } 
   }
   return( ok );
}

//===========================================================================//
// Method         : ParsePlaceCostMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TCL_CommandLine_c::ParsePlaceCostMode_( 
      const char*                pszMode,
            TOS_PlaceCostMode_t* pmode )
{
   bool ok = true;

   if( pszMode )
   {
      if( TC_stricmp( pszMode, "bounding_box" ) == 0 )
      {
	 *pmode = TOS_PLACE_COST_ROUTABILITY_DRIVEN;
      }
      else if( TC_stricmp( pszMode, "path_timing_driven" ) == 0 )
      {
	 *pmode = TOS_PLACE_COST_PATH_TIMING_DRIVEN;
      }
      else if( TC_stricmp( pszMode, "net_timing_driven" ) == 0 )
      {
	 *pmode = TOS_PLACE_COST_NET_TIMING_DRIVEN;
      }
      else
      {
	 ok = false;
      } 
   }
   return( ok );
}

//===========================================================================//
// Method         : ParseRouteAbstractMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TCL_CommandLine_c::ParseRouteAbstractMode_( 
      const char*                    pszMode,
            TOS_RouteAbstractMode_t* pmode )
{
   bool ok = true;

   if( pszMode )
   {
      if( TC_stricmp( pszMode, "global" ) == 0 )
      {
	 *pmode = TOS_ROUTE_ABSTRACT_GLOBAL;
      }
      else if( TC_stricmp( pszMode, "detailed" ) == 0 )
      {
	 *pmode = TOS_ROUTE_ABSTRACT_DETAILED;
      }
      else
      {
	 ok = false;
      } 
   }
   return( ok );
}

//===========================================================================//
// Method         : ParseRouteResourceMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TCL_CommandLine_c::ParseRouteResourceMode_( 
      const char*                    pszMode,
            TOS_RouteResourceMode_t* pmode )
{
   bool ok = true;

   if( pszMode )
   {
      if( TC_stricmp( pszMode, "demand_only" ) == 0 )
      {
	 *pmode = TOS_ROUTE_RESOURCE_DEMAND_ONLY;
      }
      else if( TC_stricmp( pszMode, "delay_normalized" ) == 0 )
      {
	 *pmode = TOS_ROUTE_RESOURCE_DELAY_NORMALIZED;
      }
      else
      {
	 ok = false;
      } 
   }
   return( ok );
}

//===========================================================================//
// Method         : ParseRouteCostMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TCL_CommandLine_c::ParseRouteCostMode_( 
      const char*                pszMode,
            TOS_RouteCostMode_t* pmode )
{
   bool ok = true;

   if( pszMode )
   {
      if( TC_stricmp( pszMode, "breadth_first" ) == 0 )
      {
	 *pmode = TOS_ROUTE_COST_BREADTH_FIRST;
      }
      else if( TC_stricmp( pszMode, "timing_driven" ) == 0 )
      {
	 *pmode = TOS_ROUTE_COST_TIMING_DRIVEN;
      }
      else
      {
	 ok = false;
      } 
   }
   return( ok );
}
