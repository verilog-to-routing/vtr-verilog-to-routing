//===========================================================================//
// Purpose : Method definitions for the TP_Process_c class.
//
//           Public methods include:
//           - TP_Process_c, ~TP_Process_c
//           - Apply
//
//===========================================================================//

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
