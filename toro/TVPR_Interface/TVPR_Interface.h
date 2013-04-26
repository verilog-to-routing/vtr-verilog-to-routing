//===========================================================================//
// Purpose : Declaration and inline definitions for the TVPR_Interface 
//           singleton class.  This class is responsible for applying VPR 
//           via VPR's API.
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

#ifndef TVPR_INTERFACE_H
#define TVPR_INTERFACE_H

#include "TOS_OptionsStore.h"
#include "TAS_ArchitectureSpec.h"
#include "TFM_FabricModel.h"
#include "TCD_CircuitDesign.h"

#include "vpr_api.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
class TVPR_Interface_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TVPR_Interface_c& GetInstance( void );

   bool Apply( const TOS_OptionsStore_c& optionsStore,
               const TAS_ArchitectureSpec_c& architectureSpec,
               TFM_FabricModel_c* pfabricModel,
               TCD_CircuitDesign_c* pcircuitDesign );
   bool Open( const TOS_OptionsStore_c& optionsStore,
              const TAS_ArchitectureSpec_c& architectureSpec,
              const TFM_FabricModel_c& fabricModel,
              const TCD_CircuitDesign_c& circuitDesign );
   bool Execute( const TOS_OptionsStore_c& optionsStore,
                 const TCD_CircuitDesign_c& circuitDesign ) const;
   bool Close( TFM_FabricModel_c* pfabricModel = 0,
               TCD_CircuitDesign_c* pcircuitDesign = 0 );

protected:

   TVPR_Interface_c( void );
   ~TVPR_Interface_c( void );

private:

   class TVPR_DataStructures_c
   {
   public:

      t_options    options;   // Define VPR's API data structures
      t_arch       arch;      // "
      t_vpr_setup  setup;     // "
      t_power_opts powerOpts; // "
      boolean      success;   // Define VPR's API return status
   } vpr_;

   bool isAlive_;   // TRUE => interface is active (ie. is initialized)

   static TVPR_Interface_c* pinstance_; // Define ptr for singleton instance
};

#endif 
