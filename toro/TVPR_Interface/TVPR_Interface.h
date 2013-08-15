//===========================================================================//
// Purpose : Declaration and inline definitions for the TVPR_Interface 
//           singleton class.  This class is responsible for applying VPR 
//           via VPR's API.
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
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
   bool Close( const TOS_OptionsStore_c& optionsStore,
               TFM_FabricModel_c* pfabricModel,
               TCD_CircuitDesign_c* pcircuitDesign );
   bool Close( bool freeDataStructures = true );

protected:

   TVPR_Interface_c( void );
   ~TVPR_Interface_c( void );

private:

   bool ShowInternalError_( t_vpr_error* vpr_error ) const;

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
