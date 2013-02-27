//===========================================================================//
// Purpose : Method definitions for the TP_Process_c class.
//
//           Public methods include:
//           - TP_Process_c, ~TP_Process_c
//           - Apply
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

#include "TIO_PrintHandler.h"

#include "TVPR_Interface.h"

#include "TP_Process.h"

//===========================================================================//
// Method         : TP_Process_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TP_Process_c::TP_Process_c( 
      void )
{
}

//===========================================================================//
// Method         : ~TP_Process_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TP_Process_c::~TP_Process_c( 
      void )
{
}

//===========================================================================//
// Method         : Apply
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TP_Process_c::Apply(
      const TOS_OptionsStore_c&   optionsStore,
            TFS_FloorplanStore_c* pfloorplanStore )
{
   bool ok = true;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Info( "Processing...\n" );

   // Initialize a VPR interface 'singleton'
   TVPR_Interface_c::NewInstance( );
   TVPR_Interface_c& vprInterface = TVPR_Interface_c::GetInstance( );

   const TAS_ArchitectureSpec_c& architectureSpec = pfloorplanStore->architectureSpec;
   TFM_FabricModel_c* pfabricModel = &pfloorplanStore->fabricModel;
   TCD_CircuitDesign_c* pcircuitDesign = &pfloorplanStore->circuitDesign;
   ok = vprInterface.Apply( optionsStore,
                            architectureSpec,
			    pfabricModel,
                            pcircuitDesign );

   TVPR_Interface_c::DeleteInstance( );

   return( ok );
}
