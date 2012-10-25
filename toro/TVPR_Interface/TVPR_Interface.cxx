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
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#include "TVPR_Interface.h"

#include "TVPR_OptionsStore.h"
#include "TVPR_ArchitectureSpec.h"
#include "TVPR_FabricModel.h"
#include "TVPR_CircuitDesign.h"

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
   pinstance_ = 0;
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
      ok = this->Execute( );
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

      TVPR_FabricModel_c vpr_fabricModel;
      ok = vpr_fabricModel.Export( fabricModel );
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
   if( ok && circuitDesign.IsValid( ))
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
   if( ok && !circuitDesign.IsValid( ))
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
      void ) const
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
         vpr_init_pre_place_and_route( this->vpr_.setup, this->vpr_.arch );
         vpr_place_and_route( this->vpr_.setup, this->vpr_.arch );
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
