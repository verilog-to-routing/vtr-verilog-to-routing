//===========================================================================//
// Purpose : Method definitions for the TVPR_OptionsStore_c class.
//
//           Public methods include:
//           - TVPR_OptionsStore_c, ~TVPR_OptionsStore_c
//           - Export
//
//           Private methods include:
//           - FindMessageEchoType_
//           - FindPlaceCostMode_
//           - FindRouteAbstractMode_
//           - FindRouteResourceMode_
//           - FindRouteCostMode_
//
//===========================================================================//

#include <string>
using namespace std;

#include "TCT_Generic.h"
#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TVPR_OptionsStore.h"
#include "TVPR_EchoTypeStore.h"

//===========================================================================//
// Method         : TVPR_OptionsStore_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
TVPR_OptionsStore_c::TVPR_OptionsStore_c( 
      void )
{
}

//===========================================================================//
// Method         : ~TVPR_OptionsStore_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
TVPR_OptionsStore_c::~TVPR_OptionsStore_c( 
      void )
{
}

//===========================================================================//
// Method         : Export
// Reference      : See VPR's "s_options" data structure, as defined in the 
//                  "ReadOptions.h" source code file.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TVPR_OptionsStore_c::Export(
      const TOS_OptionsStore_c& optionsStore,
            t_options*          pvpr_options ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   const TOS_ControlSwitches_c& controlSwitches = optionsStore.controlSwitches;
   const TOS_InputOptions_c& inputOptions = controlSwitches.inputOptions;
   const TOS_OutputOptions_c& outputOptions = controlSwitches.outputOptions;
   const TOS_ExecuteOptions_c& executeOptions = controlSwitches.executeOptions;
   const TOS_MessageOptions_c& messageOptions = controlSwitches.messageOptions;

   const TOS_RulesSwitches_c& rulesSwitches = optionsStore.rulesSwitches;
   const TOS_PackOptions_c& packOptions = rulesSwitches.packOptions;
   const TOS_PlaceOptions_c& placeOptions = rulesSwitches.placeOptions;
   const TOS_RouteOptions_c& routeOptions = rulesSwitches.routeOptions;

   //------------------------------------------------------------------------//
   // File options
   //------------------------------------------------------------------------//

   string srVPR_DefaultName( controlSwitches.srDefaultBaseName );
   srVPR_DefaultName += ".vpr";
   pvpr_options->CircuitName = TC_strdup( srVPR_DefaultName );

   string srVPR_SlackName( srVPR_DefaultName );
   srVPR_SlackName += ".slack";
   vpr_set_output_file_name( E_SLACK_FILE, 
                             srVPR_SlackName.data( ), srVPR_DefaultName.data( ));

   string srVPR_CriticalityName( srVPR_DefaultName );
   srVPR_CriticalityName += ".criticality";
   vpr_set_output_file_name( E_CRITICALITY_FILE, 
                             srVPR_CriticalityName.data( ), srVPR_DefaultName.data( ));

   string srVPR_CriticalPathName( srVPR_DefaultName );
   srVPR_CriticalPathName += ".critical_path";
   vpr_set_output_file_name( E_CRIT_PATH_FILE, 
                             srVPR_CriticalPathName.data( ), srVPR_DefaultName.data( ));

   if( inputOptions.srXmlFileName.length( ))
   {
      pvpr_options->ArchFile = TC_strdup( inputOptions.srXmlFileName );
   }
   if( inputOptions.srBlifFileName.length( ))
   {
      pvpr_options->BlifFile = TC_strdup( inputOptions.srBlifFileName );
      pvpr_options->Count[OT_BLIF_FILE] += 1;
   }

   //------------------------------------------------------------------------//
   // File options (unused)
   //------------------------------------------------------------------------//

   if( outputOptions.srVPR_NetFileName.length( ))
   {
      pvpr_options->NetFile = TC_strdup( outputOptions.srVPR_NetFileName );
      pvpr_options->Count[OT_NET_FILE] += 1;
   }
   if( outputOptions.srVPR_PlaceFileName.length( ))
   {
      pvpr_options->PlaceFile = TC_strdup( outputOptions.srVPR_PlaceFileName );
      pvpr_options->Count[OT_PLACE_FILE] += 1;
   }
   if( outputOptions.srVPR_RouteFileName.length( ))
   {
      pvpr_options->RouteFile = TC_strdup( outputOptions.srVPR_RouteFileName );
      pvpr_options->Count[OT_ROUTE_FILE] += 1;
   }
   if( outputOptions.srVPR_SDC_FileName.length( ))
   {
      pvpr_options->SDCFile = TC_strdup( outputOptions.srVPR_SDC_FileName );
      pvpr_options->Count[OT_SDC_FILE] += 1;
   }
   if( outputOptions.srVPR_OutputFilePrefix.length( ))
   {
      pvpr_options->out_file_prefix = TC_strdup( outputOptions.srVPR_OutputFilePrefix );
      pvpr_options->Count[OT_OUTFILE_PREFIX] += 1;
   }

   //------------------------------------------------------------------------//
   // General options
   //------------------------------------------------------------------------//

   if( packOptions.costMode == TOS_PACK_COST_TIMING_DRIVEN ||
       placeOptions.costMode == TOS_PLACE_COST_PATH_TIMING_DRIVEN ||
       routeOptions.costMode == TOS_ROUTE_COST_TIMING_DRIVEN )
   {
      pvpr_options->TimingAnalysis = static_cast< boolean >( true );
      pvpr_options->Count[OT_TIMING_ANALYSIS] += 1;
   }
   if( TCTF_IsGT( packOptions.fixedDelay, 0.0 ) ||
       TCTF_IsGT( placeOptions.fixedDelay, 0.0 ) ||
       TCTF_IsGT( routeOptions.fixedDelay, 0.0 ))
   {
      double maxFixedDelay = TCT_Max( packOptions.fixedDelay,
                                      placeOptions.fixedDelay,
                                      routeOptions.fixedDelay );
      pvpr_options->constant_net_delay = static_cast< float >( maxFixedDelay );
      pvpr_options->Count[OT_TIMING_ANALYZE_ONLY_WITH_NET_DELAY] += 1;
   }
   else if( executeOptions.toolMask == TOS_EXECUTE_TOOL_NONE )
   {
      pvpr_options->Count[OT_TIMING_ANALYZE_ONLY_WITH_NET_DELAY] += 1;
   }
   if( messageOptions.trace.vpr.echoFile ||
       messageOptions.trace.vpr.echoFileNameList.IsValid( ))
   {
      pvpr_options->CreateEchoFile = static_cast< boolean >( true );
      pvpr_options->Count[OT_CREATE_ECHO_FILE] += 1;

      setEchoEnabled( IsEchoEnabled( pvpr_options )); // [VPR] global variable
   }
   if( messageOptions.trace.vpr.echoFileNameList.IsValid( ))
   {
      setAllEchoFileEnabled( static_cast< boolean >( false ));

      for( size_t i = 0; i < messageOptions.trace.vpr.echoFileNameList.GetLength( ); ++i )
      {
         const TC_NameFile_c& echoFileName = *messageOptions.trace.vpr.echoFileNameList[i];
         enum e_echo_files echoType = this->FindMessageEchoType_( echoFileName.GetName( ));
         if( echoType == E_ECHO_END_TOKEN )
         {
            ok = printHandler.Warning( "Unknown trace VPR echo file option \"%s\".\n",
                                       TIO_PSZ_STR( echoFileName.GetName( )));
            if( !ok )
               break;
            continue;
         }
         setEchoFileEnabled( echoType, static_cast< boolean >( true ));
         if( echoFileName.GetFileName( ) && *echoFileName.GetFileName( ))
         {
            setEchoFileName( echoType, echoFileName.GetFileName( ));
         }
      }
   }

   //------------------------------------------------------------------------//
   // General options (unused)
   //------------------------------------------------------------------------//

   // GraphPause <== Support is TBD...
   // constant_net_delay <== Depreciated VPR option

   //------------------------------------------------------------------------//
   // Pack options
   //------------------------------------------------------------------------//

   if( executeOptions.toolMask & TOS_EXECUTE_TOOL_PACK )
   {
      pvpr_options->Count[OT_PACK] += 1;
   }
   if( packOptions.clusterNetsMode == TOS_PACK_CLUSTER_NETS_MIN_CONNECTIONS )
   {
      pvpr_options->connection_driven = static_cast< boolean >( true );
      pvpr_options->Count[OT_CONNECTION_DRIVEN_CLUSTERING] += 1;
   }
   if( TCTF_IsGT( packOptions.areaWeight, 0.0 ))
   {
      pvpr_options->alpha = static_cast< float >( packOptions.areaWeight );
      pvpr_options->Count[OT_ALPHA_CLUSTERING] += 1;
   }
   if( TCTF_IsGT( packOptions.netsWeight, 0.0 ))
   {
      pvpr_options->beta = static_cast< float >( packOptions.netsWeight );
      pvpr_options->Count[OT_BETA_CLUSTERING] += 1;
   }
   if( packOptions.affinityMode == TOS_PACK_AFFINITY_NONE )
   {
      pvpr_options->allow_unrelated_clustering = static_cast< boolean >( true );
      pvpr_options->Count[OT_ALLOW_UNRELATED_CLUSTERING] += 1;
   }

   //------------------------------------------------------------------------//
   // Pack options (timing-driven)
   //------------------------------------------------------------------------//

   if( packOptions.costMode == TOS_PACK_COST_TIMING_DRIVEN )
   {
      pvpr_options->timing_driven = static_cast< boolean >( true );
      pvpr_options->Count[OT_TIMING_DRIVEN_CLUSTERING] += 1;
   }

   //------------------------------------------------------------------------//
   // Pack options (unused)
   //------------------------------------------------------------------------//

   // global_clocks <== Undocumented VPR option
   // cluster_size <== Undocumented VPR option
   // inputs_per_cluster <== Undocumented VPR option
   // lut_size <== Undocumented VPR option
   // hill_climbing_flag <== Undocumented VPR option
   // sweep_hanging_nets_and_inputs <== Undocumented VPR option
   // recompute_timing_after <== Undocumented VPR option
   // block_delay <== Undocumented VPR option
   // intra_cluster_net_delay <== Undocumented VPR option
   // inter_cluster_net_delay <== Undocumented VPR option
   // skip_clustering <== Undocumented VPR option
   // allow_early_exit <== Undocumented VPR option
   // packer_algorithm <== Undocumented VPR option

   //------------------------------------------------------------------------//
   // Place options
   //------------------------------------------------------------------------//

   if( executeOptions.toolMask & TOS_EXECUTE_TOOL_PLACE )
   {
      pvpr_options->Count[OT_PLACE] += 1;

      if( placeOptions.randomSeed > 0 )
      {
         pvpr_options->Seed = placeOptions.randomSeed;
         pvpr_options->Count[OT_SEED] += 1;
      }
      if( TCTF_IsGT( placeOptions.initTemp, 0.0 ))
      {
         pvpr_options->PlaceInitT = static_cast< float >( placeOptions.initTemp );
         pvpr_options->Count[OT_INIT_T] += 1;
      }
      if( TCTF_IsGT( placeOptions.exitTemp, 0.0 ))
      {
         pvpr_options->PlaceExitT = static_cast< float >( placeOptions.exitTemp );
         pvpr_options->Count[OT_EXIT_T] += 1;
      }
      if( TCTF_IsGT( placeOptions.reduceTemp, 0.0 ))
      {
         pvpr_options->PlaceAlphaT = static_cast< float >( placeOptions.reduceTemp );
         pvpr_options->Count[OT_ALPHA_T] += 1;
      }
      if( TCTF_IsGT( placeOptions.innerNum, 0.0 ))
      {
         pvpr_options->PlaceInnerNum = static_cast< float >( placeOptions.innerNum );
         pvpr_options->Count[OT_INNER_NUM] += 1;
      }
  
      if( placeOptions.costMode != TOS_PLACE_COST_UNDEFINED )
      {
         pvpr_options->PlaceAlgorithm = this->FindPlaceCostMode_( placeOptions.costMode );
         pvpr_options->Count[OT_PLACE_ALGORITHM] += 1;
      }
   }

   //------------------------------------------------------------------------//
   // Place options (timing-driven)
   //------------------------------------------------------------------------//

   if( executeOptions.toolMask & TOS_EXECUTE_TOOL_PLACE )
   {
      if( TCTF_IsGT( placeOptions.timingCostFactor, 0.0 ))
      {
         pvpr_options->PlaceTimingTradeoff = static_cast< float >( placeOptions.timingCostFactor );
         pvpr_options->Count[OT_TIMING_TRADEOFF] += 1;
      }
      if( placeOptions.timingUpdateInt > 0 )
      {
         pvpr_options->RecomputeCritIter = placeOptions.timingUpdateInt;
         pvpr_options->Count[OT_RECOMPUTE_CRIT_ITER] += 1;
      }
      if( placeOptions.timingUpdateCount > 0 )
      {
         pvpr_options->inner_loop_recompute_divider = placeOptions.timingUpdateCount;
         pvpr_options->Count[OT_INNER_LOOP_RECOMPUTE_DIVIDER] += 1;
      }
      if( TCTF_IsGT( placeOptions.slackInitWeight, 0.0 ))
      {
         pvpr_options->place_exp_first = static_cast< float >( placeOptions.slackInitWeight );
         pvpr_options->Count[OT_TD_PLACE_EXP_FIRST] += 1;
      }
      if( TCTF_IsGT( placeOptions.slackFinalWeight, 0.0 ))
      {
         pvpr_options->place_exp_last = static_cast< float >( placeOptions.slackFinalWeight );
         pvpr_options->Count[OT_TD_PLACE_EXP_LAST] += 1;
      }
   }

   //------------------------------------------------------------------------//
   // Place options (unused)
   //------------------------------------------------------------------------//

   // PinFile <== Defined via floorplan file
   // place_cost_exp <== Undocumented VPR option

   //------------------------------------------------------------------------//
   // Route options
   //------------------------------------------------------------------------//

   if( executeOptions.toolMask & TOS_EXECUTE_TOOL_ROUTE )
   {
      pvpr_options->Count[OT_ROUTE] += 1;

      if( routeOptions.abstractMode != TOS_ROUTE_ABSTRACT_UNDEFINED )
      {
         pvpr_options->RouteType = this->FindRouteAbstractMode_( routeOptions.abstractMode );
         pvpr_options->Count[OT_ROUTE_TYPE] += 1;
      }
      if( routeOptions.windowSize > 0 )
      {
         pvpr_options->bb_factor = routeOptions.windowSize;
         pvpr_options->Count[OT_BB_FACTOR] += 1;
      }
      if( routeOptions.channelWidth > 0 )
      {
         pvpr_options->RouteChanWidth = routeOptions.channelWidth;
         pvpr_options->Count[OT_ROUTE_CHAN_WIDTH] += 1;
      }
      if( routeOptions.maxIterations > 0 )
      {
         pvpr_options->max_router_iterations = routeOptions.maxIterations;
         pvpr_options->Count[OT_MAX_ROUTER_ITERATIONS] += 1;
      }
      if( TCTF_IsGT( routeOptions.histCongestionFactor, 0.0 ))
      {
         pvpr_options->acc_fac = static_cast< float >( routeOptions.histCongestionFactor );
         pvpr_options->Count[OT_ACC_FAC] += 1;
      }
      if( TCTF_IsGT( routeOptions.initCongestionFactor, 0.0 ))
      {
         pvpr_options->initial_pres_fac = static_cast< float >( routeOptions.initCongestionFactor );
         pvpr_options->first_iter_pres_fac = static_cast< float >( routeOptions.initCongestionFactor );
         pvpr_options->Count[OT_INITIAL_PRES_FAC] += 1;
         pvpr_options->Count[OT_FIRST_ITER_PRES_FAC] += 1;
      }
      if( TCTF_IsGT( routeOptions.presentCongestionFactor, 0.0 ))
      {
         pvpr_options->pres_fac_mult = static_cast< float >( routeOptions.presentCongestionFactor );
         pvpr_options->Count[OT_PRES_FAC_MULT] += 1;
      }
      if( TCTF_IsGT( routeOptions.bendCostFactor, 0.0 ))
      {
         pvpr_options->bend_cost = static_cast< float >( routeOptions.bendCostFactor );
         pvpr_options->Count[OT_BEND_COST] += 1;
      }
      if( routeOptions.resourceMode != TOS_ROUTE_RESOURCE_UNDEFINED )
      {
         pvpr_options->base_cost_type = this->FindRouteResourceMode_( routeOptions.resourceMode );
         pvpr_options->Count[OT_BASE_COST_TYPE] += 1;
      }
      if( routeOptions.costMode != TOS_ROUTE_COST_UNDEFINED )
      {
         pvpr_options->RouterAlgorithm = this->FindRouteCostMode_( routeOptions.costMode );
         pvpr_options->Count[OT_ROUTER_ALGORITHM] += 1;
      }
   }

   //------------------------------------------------------------------------//
   // Route options (timing-driven)
   //------------------------------------------------------------------------//

   if( executeOptions.toolMask & TOS_EXECUTE_TOOL_ROUTE )
   {
      if( TCTF_IsGT( routeOptions.timingAStarFactor, 0.0 ))
      {
         pvpr_options->astar_fac = static_cast< float >( routeOptions.timingAStarFactor );
         pvpr_options->Count[OT_ASTAR_FAC] += 1;
      }
      if( TCTF_IsGT( routeOptions.timingMaxCriticality, 0.0 ))
      {
         pvpr_options->max_criticality = static_cast< float >( routeOptions.timingMaxCriticality );
         pvpr_options->Count[OT_MAX_CRITICALITY] += 1;
      }
      if( TCTF_IsGT( routeOptions.slackCriticality, 0.0 ))
      {
         pvpr_options->criticality_exp = static_cast< float >( routeOptions.slackCriticality );
         pvpr_options->Count[OT_CRITICALITY_EXP] += 1;
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : FindMessageEchoType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
enum e_echo_files TVPR_OptionsStore_c::FindMessageEchoType_( 
      const char* pszEchoType ) const
{
   TVPR_EchoTypeStore_c echoTypeStore[ E_ECHO_END_TOKEN ] =
   {
      { E_ECHO_ARCH,                               "arch" },
      { E_ECHO_BLIF_INPUT,                         "blif_input" },
      { E_ECHO_CLUSTERING_BLOCK_CRITICALITIES,     "clustering_block_criticalities" },
      { E_ECHO_CLUSTERING_TIMING_INFO,             "clustering_timing_info" },
      { E_ECHO_COMPLETE_NET_TRACE,                 "complete_net_trace" },
      { E_ECHO_CRITICALITY,                        "criticality" },
      { E_ECHO_CRITICAL_PATH,                      "critical_path" },
      { E_ECHO_END_CLB_PLACEMENT,                  "end_clb_placement" },
      { E_ECHO_FINAL_PLACEMENT_CRITICALITY,        "final_placement_criticality" },
      { E_ECHO_FINAL_PLACEMENT_SLACK,              "final_placement_slack" },
      { E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH,       "final_placement_timing_graph" },
      { E_ECHO_INITIAL_CLB_PLACEMENT,              "initial_clb_placement" },
      { E_ECHO_INITIAL_PLACEMENT_CRITICALITY,      "initial_placement_criticality" },
      { E_ECHO_INITIAL_PLACEMENT_SLACK,            "initial_placement_slack" },
      { E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH,     "initial_placement_timing_graph" },
      { E_ECHO_LUT_REMAPPING,                      "lut_remapping" },
      { E_ECHO_MEM,                                "mem" },
      { E_ECHO_NET_DELAY,                          "net_delay" },
      { E_ECHO_PB_GRAPH,                           "pb_graph" },
      { E_ECHO_PLACEMENT_CRITICAL_PATH,            "placement_critical_path" },
      { E_ECHO_PLACEMENT_CRIT_PATH,                "placement_crit_path" },
      { E_ECHO_PLACEMENT_LOGIC_SINK_DELAYS,        "placement_logic_sink_delays" },
      { E_ECHO_PLACEMENT_LOWER_BOUND_SINK_DELAYS,  "placement_lower_bound_sink_delays" },
      { E_ECHO_PLACEMENT_SINK_DELAYS,              "placement_sink_delays" },
      { E_ECHO_POST_FLOW_TIMING_GRAPH,             "post_flow_timing_graph" },
      { E_ECHO_POST_PACK_NETLIST,                  "post_pack_netlist" },
      { E_ECHO_PRE_PACKING_CRITICALITY,            "pre_packing_criticality" },
      { E_ECHO_PRE_PACKING_MOLECULES_AND_PATTERNS, "pre_packing_molecules_and_patterns" },
      { E_ECHO_PRE_PACKING_SLACK,                  "pre_packing_slack" },
      { E_ECHO_PRE_PACKING_TIMING_GRAPH,           "pre_packing_timing_graph" },
      { E_ECHO_PRE_PACKING_TIMING_GRAPH_AS_BLIF,   "pre_packing_timing_graph_as_blif" },
      { E_ECHO_ROUTING_SINK_DELAYS,                "routing_sink_delays" },
      { E_ECHO_RR_GRAPH,                           "rr_graph" },
      { E_ECHO_SLACK,                              "slack" },
      { E_ECHO_TIMING_CONSTRAINTS,                 "timing_constraints" },
      { E_ECHO_TIMING_GRAPH,                       "timing_graph" },
      { E_ECHO_SEG_DETAILS,                        "seg_details" }
   };

   enum e_echo_files echoType = E_ECHO_END_TOKEN;
   for( size_t i = 0; i < E_ECHO_END_TOKEN; ++i )
   {
      if( TC_stricmp( pszEchoType, echoTypeStore[i].pszFileType ) == 0 )
      {
	 echoType = echoTypeStore[i].echoType;
         break;
      }
   }
   return( echoType );
}

//===========================================================================//
// Method         : FindPlaceCostMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
enum e_place_algorithm TVPR_OptionsStore_c::FindPlaceCostMode_( 
      TOS_PlaceCostMode_t mode ) const
{
   enum e_place_algorithm mode_ = static_cast< enum e_place_algorithm >( 0 );
   
   switch( mode )
   {
   case TOS_PLACE_COST_ROUTABILITY_DRIVEN: mode_ = BOUNDING_BOX_PLACE;       break;
   case TOS_PLACE_COST_PATH_TIMING_DRIVEN: mode_ = PATH_TIMING_DRIVEN_PLACE; break;
   case TOS_PLACE_COST_NET_TIMING_DRIVEN:  mode_ = NET_TIMING_DRIVEN_PLACE;  break;
   case TOS_PLACE_COST_UNDEFINED:                                            break;
   }
   return( mode_ );
}

//===========================================================================//
// Method         : FindRouteAbstractMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
enum e_route_type TVPR_OptionsStore_c::FindRouteAbstractMode_( 
      TOS_RouteAbstractMode_t mode ) const
{
   enum e_route_type mode_ = static_cast< enum e_route_type >( 0 );

   switch( mode )
   {
   case TOS_ROUTE_ABSTRACT_GLOBAL:    mode_ = GLOBAL;   break;
   case TOS_ROUTE_ABSTRACT_DETAILED:  mode_ = DETAILED; break;
   case TOS_ROUTE_ABSTRACT_UNDEFINED:                   break;
   }
   return( mode_ );
}

//===========================================================================//
// Method         : FindRouteResourceMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
enum e_base_cost_type TVPR_OptionsStore_c::FindRouteResourceMode_( 
      TOS_RouteResourceMode_t mode ) const
{
   enum e_base_cost_type mode_ = static_cast< enum e_base_cost_type >( 0 );

   switch( mode )
   {
   case TOS_ROUTE_RESOURCE_DEMAND_ONLY:      mode_ = DEMAND_ONLY;      break;
   case TOS_ROUTE_RESOURCE_DELAY_NORMALIZED: mode_ = DELAY_NORMALIZED; break;
   case TOS_ROUTE_RESOURCE_UNDEFINED:                                  break;
   }
   return( mode_ );
}

//===========================================================================//
// Method         : FindRouteCostMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
enum e_router_algorithm TVPR_OptionsStore_c::FindRouteCostMode_( 
      TOS_RouteCostMode_t mode ) const
{
   enum e_router_algorithm mode_ = static_cast< e_router_algorithm >( 0 );

   switch( mode )
   {
   case TOS_ROUTE_COST_BREADTH_FIRST:   mode_ = BREADTH_FIRST;   break;
   case TOS_ROUTE_COST_TIMING_DRIVEN:   mode_ = TIMING_DRIVEN;   break;
   case TOS_ROUTE_COST_UNDEFINED:                                break;
   }
   return( mode_ );
}
