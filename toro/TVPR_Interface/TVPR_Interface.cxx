//===========================================================================//
// Purpose : Method definitions for the TVPR_Interface_c class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance
//           - Apply
//           - Open
//           - Execute
//           - Close
//
//           Protected methods include:
//           - TIO_PrintHandler_c, ~TIO_PrintHandler_c
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// This program is free software; you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation; version 3 of the License, or any later version. //
//                                                                           //
// This program is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY //
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   //
// for more details.                                                         //
//                                                                           //
// You should have received a copy of the GNU General Public License along   //
// with this program; if not, see <http://www.gnu.org/licenses>.             //
//---------------------------------------------------------------------------//

#include "TIO_StringText.h"
#include "TIO_PrintHandler.h"

#include "TFH_FabricGridHandler.h"
#include "TFH_FabricChannelHandler.h"
#include "TFH_FabricSwitchBoxHandler.h"
#include "TFH_FabricConnectionBlockHandler.h"
#include "TCH_RelativePlaceHandler.h"
#include "TCH_PrePlacedHandler.h"
#include "TCH_PreRoutedHandler.h"

#include "TVPR_OptionsStore.h"
#include "TVPR_ArchitectureSpec.h"
#include "TVPR_FabricModel.h"
#include "TVPR_CircuitDesign.h"

#include "TVPR_Interface.h"

// Initialize the VPR interface "singleton" class, as needed
TVPR_Interface_c* TVPR_Interface_c::pinstance_ = 0;

//===========================================================================//
// Method         : TVPR_Interface_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
TVPR_Interface_c::TVPR_Interface_c( 
      void )
      :
      isAlive_( false )
{
   this->vpr_.success = FALSE;

   pinstance_ = 0;

   // Initialize grid handler 'singleton' prior to placement
   // (in order to handle fabric grid overrides, if any)
   TFH_FabricGridHandler_c::NewInstance( );

   // Initialize channel widths handler 'singleton' prior to placement
   // (in order to handle fabric channel width overrides, if any)
   TFH_FabricChannelHandler_c::NewInstance( );

   // Initialize switch box handler 'singleton' prior to routing
   // (in order to handle fabric switch box overrides, if any)
   TFH_FabricSwitchBoxHandler_c::NewInstance( );

   // Initialize connection block handler 'singleton' prior to routing
   // (in order to handle fabric connection block overrides, if any)
   TFH_FabricConnectionBlockHandler_c::NewInstance( );

   // Initialize relative placement handler 'singleton' prior to placement
   // (in order to handle relative placement constraints, if any)
   TCH_RelativePlaceHandler_c::NewInstance( );

   // Initialize pre-placed placement handler 'singleton' prior to placement
   // (in order to handle pre-placed placement constraints, if any)
   TCH_PrePlacedHandler_c::NewInstance( );

   // Initialize pre-routed route handler 'singleton' prior to routing
   // (in order to handle pre-routed route constraints, if any)
   TCH_PreRoutedHandler_c::NewInstance( );
}

//===========================================================================//
// Method         : ~TVPR_Interface_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
TVPR_Interface_c::~TVPR_Interface_c( 
      void )
{
   TCH_PreRoutedHandler_c::DeleteInstance( );

   TCH_PrePlacedHandler_c::DeleteInstance( );

   TCH_RelativePlaceHandler_c::DeleteInstance( );

   TFH_FabricConnectionBlockHandler_c::DeleteInstance( );

   TFH_FabricSwitchBoxHandler_c::DeleteInstance( );

   TFH_FabricChannelHandler_c::DeleteInstance( );

   TFH_FabricGridHandler_c::DeleteInstance( );

   this->Close( );
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_Interface_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TVPR_Interface_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_Interface_c::DeleteInstance( 
      void )
{
   if( pinstance_ )
   {
      delete pinstance_;
      pinstance_ = 0;
   }
}

//===========================================================================//
// Method         : GetInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
TVPR_Interface_c& TVPR_Interface_c::GetInstance(
      void )
{
   if( !pinstance_ )
   {
      NewInstance( );
   }
   return( *pinstance_ );
}

//===========================================================================//
// Method         : Apply
// Reference      : See VPR's "main.c" processing source code file
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TVPR_Interface_c::Apply(
      const TOS_OptionsStore_c&     optionsStore,
      const TAS_ArchitectureSpec_c& architectureSpec,
            TFM_FabricModel_c*      pfabricModel,
            TCD_CircuitDesign_c*    pcircuitDesign )
{
   bool ok = true;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   if( ok )
   {
      printHandler.Info( "Opening VPR interface...\n" );
      ok = this->Open( optionsStore,
                       architectureSpec,
                       *pfabricModel,
                       *pcircuitDesign );
   }
   if( ok )
   {
      printHandler.Info( "Executing VPR interface...\n" );
      ok = this->Execute( optionsStore,
                          *pcircuitDesign );
   }
   if( ok )
   {
      printHandler.Info( "Closing VPR interface...\n" );
      this->Close( pfabricModel,
                   pcircuitDesign );
   }
   return( ok );
}

//===========================================================================//
// Method         : Open
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TVPR_Interface_c::Open( 
      const TOS_OptionsStore_c&     optionsStore,
      const TAS_ArchitectureSpec_c& architectureSpec,
      const TFM_FabricModel_c&      fabricModel,
      const TCD_CircuitDesign_c&    circuitDesign )
{
   bool ok = true;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   this->Close( );

   // VPR snippet copied from VPR's vpr_init function...
   memset( &this->vpr_.options, 0, sizeof( t_options ));
   memset( &this->vpr_.arch, 0, sizeof( t_arch ));
   memset( &this->vpr_.setup, 0, sizeof( t_vpr_setup ));
   memset( &this->vpr_.powerOpts, 0, sizeof( t_power_opts ));

   // Toro snippet specific to TVPR_Interface_c's initialization
   TVPR_OptionsStore_c vpr_optionsStore;
   ok = vpr_optionsStore.Export( optionsStore, 
                                 &this->vpr_.options );

   // VPR snippet copied from VPR's vpr_init function...
   this->vpr_.setup.TimingEnabled = this->vpr_.options.TimingAnalysis;
   this->vpr_.setup.constant_net_delay = this->vpr_.options.constant_net_delay;

   // Toro snippet specific to TVPR_Interface_c's initialization
   if( ok && architectureSpec.IsValid( ))
   {
      printHandler.Info( "Exporting architecture spec to VPR...\n" );

      bool isTimingEnabled = ( this->vpr_.setup.TimingEnabled ? true : false );
      TVPR_ArchitectureSpec_c vpr_architectureSpec;
      ok = vpr_architectureSpec.Export( architectureSpec, 
                                        &this->vpr_.arch,
                                        &type_descriptors, // [VPR] global variable
                                        &num_types,        // [VPR] global variable
                                        isTimingEnabled );
   }
   if( ok && fabricModel.IsValid( ))
   {
      printHandler.Info( "Exporting fabric model to VPR...\n" );

      const TOS_FabricOptions_c& fabricOptions = optionsStore.GetFabricOptions( );
      bool overrideBlocks = fabricOptions.blocks.override;
      bool overrideSwitchBoxes = fabricOptions.switchBoxes.override;
      bool overrideConnectionBlocks = fabricOptions.connectionBlocks.override;
      bool overrideChannels = fabricOptions.channels.override;

      TVPR_FabricModel_c vpr_fabricModel;
      ok = vpr_fabricModel.Export( fabricModel,
                                   &this->vpr_.arch,
                                   type_descriptors, // [VPR] global variable
                                   num_types,        // [VPR] global variable
                                   overrideBlocks,
                                   overrideSwitchBoxes,
                                   overrideConnectionBlocks,
                                   overrideChannels );
   }

   // VPR snippet copied from VPR's vpr_init function...
   if( ok )
   {
      printHandler.SetPrefix( TIO_SZ_VPR_PREFIX );

      boolean readArchFile = static_cast< boolean >( !architectureSpec.IsValid( ));
      vpr_setup_vpr( &this->vpr_.options, 
                     this->vpr_.setup.TimingEnabled, 
                     readArchFile,
                     &this->vpr_.setup.FileNameOpts, 
                     &this->vpr_.arch, 
                     &this->vpr_.setup.Operation,
                     &this->vpr_.setup.user_models, 
                     &this->vpr_.setup.library_models, 
                     &this->vpr_.setup.PackerOpts, 
                     &this->vpr_.setup.PlacerOpts,
                     &this->vpr_.setup.AnnealSched, 
                     &this->vpr_.setup.RouterOpts, 
                     &this->vpr_.setup.RoutingArch, 
                     &this->vpr_.setup.Segments, 
                     &this->vpr_.setup.Timing,
                     &this->vpr_.setup.ShowGraphics, 
                     &this->vpr_.setup.GraphPause,
                     &this->vpr_.powerOpts );

      // VPR snippet copied from VPR's vpr_init function...
      vpr_check_options( this->vpr_.options, 
                         this->vpr_.setup.TimingEnabled );
      vpr_check_arch( this->vpr_.arch, 
                      this->vpr_.setup.TimingEnabled );
      vpr_check_setup( this->vpr_.setup.Operation, 
                       this->vpr_.setup.PlacerOpts, 
                       this->vpr_.setup.AnnealSched, 
                       this->vpr_.setup.RouterOpts, 
                       this->vpr_.setup.RoutingArch,
                       this->vpr_.setup.Segments, 
                       this->vpr_.setup.Timing, 
                       this->vpr_.arch.Chans );

      printHandler.ClearPrefix( );
   }

   // Toro snippet specific to TVPR_Interface_c's initialization
   if( ok && circuitDesign.IsValid( ) && circuitDesign.IsLegal( ))
   {
      printHandler.Info( "Exporting circuit design to VPR...\n" );

      bool deleteInvalidData = ( this->vpr_.setup.PackerOpts.sweep_hanging_nets_and_inputs ? true : false );
      TVPR_CircuitDesign_c vpr_circuitDesign;
      ok = vpr_circuitDesign.Export( circuitDesign,
                                     this->vpr_.setup.library_models, 
                                     this->vpr_.setup.user_models, 
                                     &vpack_net,
                                     &num_logical_nets,
                                     &logical_block,
                                     &num_logical_blocks,
                                     &num_p_inputs,
                                     &num_p_outputs,
                                     deleteInvalidData );
   }

   // VPR snippet copied from VPR's vpr_init function...
   if(( ok && !circuitDesign.IsValid( )) ||
      ( ok && circuitDesign.IsValid( ) && !circuitDesign.IsLegal( )))
   {
      printHandler.SetPrefix( TIO_SZ_VPR_PREFIX );

      boolean readActivityFile = FALSE;
      char* pszActivityFileName = 0;
      vpr_read_and_process_blif( this->vpr_.setup.PackerOpts.blif_file_name, 
                                 this->vpr_.setup.PackerOpts.sweep_hanging_nets_and_inputs, 
                                 this->vpr_.setup.user_models, 
                                 this->vpr_.setup.library_models,
                                 readActivityFile, pszActivityFileName );
      printHandler.ClearPrefix( );
   }

   if( optionsStore.controlSwitches.messageOptions.trace.vpr.showSetup )
   {
      printHandler.SetPrefix( TIO_SZ_VPR_PREFIX );

      vpr_show_setup( this->vpr_.options, this->vpr_.setup );

      printHandler.ClearPrefix( );
   }

   this->isAlive_ = ok;
   return( this->isAlive_ );
}

//===========================================================================//
// Method         : Execute
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TVPR_Interface_c::Execute(
      const TOS_OptionsStore_c&  optionsStore,
      const TCD_CircuitDesign_c& circuitDesign ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   if( this->isAlive_ )
   {
      printHandler.SetPrefix( TIO_SZ_VPR_PREFIX );

      if( this->vpr_.setup.PackerOpts.doPacking ) 
      {
         vpr_pack( this->vpr_.setup, this->vpr_.arch );
      }
      if( this->vpr_.setup.PlacerOpts.doPlacement || 
          this->vpr_.setup.RouterOpts.doRouting ) 
      {
         const TOS_PlaceOptions_c& placeOptions = optionsStore.GetPlaceOptions( );
         if( placeOptions.relativePlace.enable )
         {
            TCH_RelativePlaceHandler_c& relativePlaceHandler = TCH_RelativePlaceHandler_c::GetInstance( );
            relativePlaceHandler.Configure( placeOptions.relativePlace.rotateEnable,
                                            placeOptions.relativePlace.maxPlaceRetryCt,
                                            placeOptions.relativePlace.maxMacroRetryCt,
                                            circuitDesign.blockList );
         }
         if( placeOptions.prePlaced.enable )
         {
            TCH_PrePlacedHandler_c& prePlacedHandler = TCH_PrePlacedHandler_c::GetInstance( );
            prePlacedHandler.Configure( circuitDesign.blockList );
         }

         const TOS_RouteOptions_c& routeOptions = optionsStore.GetRouteOptions( );
         if( routeOptions.preRouted.enable )
         {
            TOS_RouteOrderMode_t preRoutedOrder = routeOptions.preRouted.orderMode;
            TCH_RouteOrderMode_t preRoutedOrder_ = ( preRoutedOrder == TOS_ROUTE_ORDER_FIRST ?
                                                     TCH_ROUTE_ORDER_FIRST : TCH_ROUTE_ORDER_AUTO );

            TCH_PreRoutedHandler_c& preRoutedHandler = TCH_PreRoutedHandler_c::GetInstance( );
            preRoutedHandler.Configure( circuitDesign.netList, 
                                        circuitDesign.netOrderList,
                                        preRoutedOrder_ );
         }

         vpr_init_pre_place_and_route( this->vpr_.setup, this->vpr_.arch );
         boolean success = vpr_place_and_route( this->vpr_.setup, 
                                                this->vpr_.arch );

         TVPR_Interface_c* pinterface = const_cast< TVPR_Interface_c* >( this );
         pinterface->vpr_.success = success;
      }
      printHandler.ClearPrefix( );
   }
   return( this->isAlive_ );
}

//===========================================================================//
// Method         : Close
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TVPR_Interface_c::Close( 
      TFM_FabricModel_c*   pfabricModel,
      TCD_CircuitDesign_c* pcircuitDesign )
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   if( this->isAlive_ )
   {
      if( pfabricModel )
      {
         printHandler.Info( "Importing fabric model from VPR...\n" );

         // Extract fabric model from VPR's internal data structures
         // (based on VPR's global "grid", "nx", and "ny")
         // (and, based on VPR's global "rr_node" and "num_rr_nodes")
         TVPR_FabricModel_c vpr_fabricModel;
         vpr_fabricModel.Import( grid, nx, ny,
                                 rr_node, num_rr_nodes,
                                 chan_width_x, chan_width_y,
                                 pfabricModel );
      }
      if( pcircuitDesign )
      {
         printHandler.Info( "Importing circuit design from VPR...\n" );

         // Extract circuit design from VPR's internal data structures
         // (based on VPR's global "block" and "num_blocks")
         TVPR_CircuitDesign_c vpr_circuitDesign;
         vpr_circuitDesign.Import( &this->vpr_.arch,
                                   vpack_net, num_logical_nets,
                                   block, num_blocks,
                                   logical_block, 
                                   rr_node,
                                   pcircuitDesign );
      }

      printHandler.SetPrefix( TIO_SZ_VPR_PREFIX );

      vpr_free_vpr_data_structures( this->vpr_.arch, 
                                    this->vpr_.options, 
                                    this->vpr_.setup );
      printHandler.ClearPrefix( );

      this->isAlive_ = false;
   }
   return( this->isAlive_ );
}
