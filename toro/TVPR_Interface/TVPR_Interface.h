//===========================================================================//
// Purpose : Declaration and inline definitions for the TVPR_Interface 
//           singleton class.  This class is responsible for applying VPR 
//           via VPR's API.
//
//===========================================================================//

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
   bool Execute( void ) const;
   bool Close( TFM_FabricModel_c* pfabricModel = 0,
               TCD_CircuitDesign_c* pcircuitDesign = 0 );

protected:

   TVPR_Interface_c( void );
   ~TVPR_Interface_c( void );

private:

   class TVPR_DataStructures_c
   {
   public:

      t_options   options;   // Define VPR's API data structures
      t_arch      arch;      // "
      t_vpr_setup setup;     // "
   } vpr_;

   bool isAlive_;   // TRUE => interface is active (ie. is initialized)

   static TVPR_Interface_c* pinstance_; // Define ptr for singleton instance
};

#endif 
