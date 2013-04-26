//===========================================================================//
// Purpose : Method definitions for the TI_Input pre-processor class.
//
//           Public methods include:
//           - TI_Input_c, ~TI_Input_c
//           - Init
//           - Apply
//           - IsValid
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

#include "TI_Input.h"

//===========================================================================//
// Method         : TI_Input_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TI_Input_c::TI_Input_c( 
      void )
      :
      pcommandLine_( 0 ),
      poptionsStore_( 0 ),
      pcontrolSwitches_( 0 ),
      prulesSwitches_( 0 ),
      parchitectureSpec_( 0 ),
      pfabricModel_( 0 ),
      pcircuitDesign_( 0 )
{
}

//===========================================================================//
// Method         : ~TI_Input_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TI_Input_c::~TI_Input_c( 
      void )
{
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::Init( 
      const TCL_CommandLine_c&    commandLine,
            TOS_OptionsStore_c*   poptionsStore,
            TFS_FloorplanStore_c* pfloorplanStore )
{
   this->pcommandLine_ = &commandLine;

   if( poptionsStore )
   {
      poptionsStore->Init( );

      this->poptionsStore_ = poptionsStore;
      this->pcontrolSwitches_ = &poptionsStore->controlSwitches;
      this->prulesSwitches_ = &poptionsStore->rulesSwitches;
   }
   if( pfloorplanStore )
   {
      this->pfloorplanStore_ = pfloorplanStore;
      this->parchitectureSpec_ = &pfloorplanStore->architectureSpec;
      this->pfabricModel_ = &pfloorplanStore->fabricModel;
      this->pcircuitDesign_ = &pfloorplanStore->circuitDesign;
   }
   return( this->IsValid( ));
}

//===========================================================================//
// Method         : Apply
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::Apply( 
      const TCL_CommandLine_c&    commandLine,
            TOS_OptionsStore_c*   poptionsStore,
            TFS_FloorplanStore_c* pfloorplanStore )
{
   bool ok = this->Init( commandLine, poptionsStore, pfloorplanStore );
   if( ok )
   {
      ok = this->Apply( );
   }
   return( ok );
}

//===========================================================================//
// Method         : Apply
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::Apply( 
      void )
{
   bool ok = true;

   if( this->IsValid( ))
   {
      if( ok )
      {
         // First, read and process the input options file(s)
         ok = this->LoadOptionsFiles_( );
         if( ok )
         {
            // And, handle any input option file post-processing 
            this->HandleOptionsStore_( );
         }
      }

      if( ok )
      {
         // Next, display interesting program execution status
         this->DisplayProgramOwner_( );
         this->DisplayProgramCommand_( );

         TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
         printHandler.Info( "Pre-processing...\n" );
      }

      const TOS_InputOptions_c& inputOptions = this->pcontrolSwitches_->inputOptions;
      if( ok && inputOptions.xmlFileEnable )
      {
         // Next, read the input VPR XML file
         ok = this->ReadFileProcess_( TI_Input_c::TI_FILE_XML,
                                      TIO_SZ_INPUT_XML_DEF_TYPE );
      }
      if( ok && inputOptions.architectureFileEnable )
      {
         // Next, read the input architecture specs file
         ok = this->ReadFileProcess_( TI_Input_c::TI_FILE_ARCHITECTURE,
                                      TIO_SZ_INPUT_ARCHITECTURE_DEF_TYPE );
      }
      if( ok && ( inputOptions.xmlFileEnable || inputOptions.architectureFileEnable ))
      {
         // And, handle any input architecture specs file post-processing 
         ok = this->HandleArchitectureSpec_( );
      }

      if( ok && inputOptions.fabricFileEnable )
      {
         // Next, read the input fabric file 
         ok = this->ReadFileProcess_( TI_Input_c::TI_FILE_FABRIC, 
                                      TIO_SZ_INPUT_FABRIC_DEF_TYPE );
      }
      if( ok && inputOptions.fabricFileEnable )
      {
         // And, handle any input fabric file post-processing 
         this->HandleFabricModel_( );
      }

      if( ok && inputOptions.blifFileEnable )
      {
         // Next, read the input VPR BLIF file 
         ok = this->ReadFileProcess_( TI_Input_c::TI_FILE_BLIF, 
                                      TIO_SZ_INPUT_BLIF_DEF_TYPE );
      }
      if( ok && inputOptions.circuitFileEnable )
      {
         // Next, read the input circuit design file
         ok = this->ReadFileProcess_( TI_Input_c::TI_FILE_CIRCUIT, 
                                      TIO_SZ_INPUT_CIRCUIT_DEF_TYPE );
      }
      if( ok && ( inputOptions.blifFileEnable || inputOptions.circuitFileEnable ))
      {
         // And, handle any input circuit design file post-processing 
         ok = this->HandleCircuitDesign_( );
      }

      if( ok )
      {
         // Ready to validate all input data at this time
         if( ok )
         {
            ok = this->ValidateOptionsStore_( );
         }
         if( ok )
         {
            ok = this->ValidateArchitectureSpec_( );
         }
         if( ok )
         {
            ok = this->ValidateFabricModel_( );
         }
         if( ok )
         {
            ok = this->ValidateCircuitDesign_( );
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::IsValid( 
      void ) const
{
   return( this->pcommandLine_ &&
           this->poptionsStore_ &&
           this->pfloorplanStore_ ?
           true : false );
}
