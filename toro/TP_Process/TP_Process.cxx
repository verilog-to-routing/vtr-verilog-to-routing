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
