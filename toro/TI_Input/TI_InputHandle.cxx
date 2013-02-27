//===========================================================================//
// Purpose : Supporting methods for the TI_Input pre-processor class.  
//           These methods support handling input options and data files.
//
//           Private methods include:
//           - HandleOptionsStore_
//           - HandleArchitectureSpec_
//           - HandleFabricModel_
//           - HandleCircuitDesign_
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

#include "TI_Input.h"

//===========================================================================//
// Method         : HandleOptionsStore_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TI_Input_c::HandleOptionsStore_( 
      void )
{
   // Apply options file name (sans ".options") to define a "base name" string
   // (used to define subsequent default file names, as needed)
   const TOS_ControlSwitches_c& controlSwitches = this->poptionsStore_->controlSwitches;
   this->ApplyDefaultBaseName_( controlSwitches.srDefaultBaseName );

   const TOS_OutputOptions_c& outputOptions = controlSwitches.outputOptions;
   const TOS_MessageOptions_c& messageOptions = controlSwitches.messageOptions;
   const TOS_ExecuteOptions_c& executeOptions = controlSwitches.executeOptions;

   // Apply options store to handle essential message options (e.g. logging)
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   if( !printHandler.IsLogFileOutputEnabled( ))
   {
      if( outputOptions.logFileEnable && 
          outputOptions.srLogFileName.length( ))
      {
         const string srLogFileName = outputOptions.srLogFileName;
         printHandler.SetLogFileOutput( srLogFileName.data( ));

	 printHandler.DisableOutput( TIO_PRINT_OUTPUT_STDIO | TIO_PRINT_OUTPUT_CUSTOM );
	 printHandler.WriteBanner( );
	 printHandler.EnableOutput( TIO_PRINT_OUTPUT_STDIO | TIO_PRINT_OUTPUT_CUSTOM );
      }
   }

   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   MinGrid.SetGrid( messageOptions.minGridPrecision );

   printHandler.SetTimeStampsEnabled( messageOptions.timeStampsEnable );

   for( size_t i = 0; i < messageOptions.info.acceptList.GetLength( ); ++i )
   {
      printHandler.AddInfoAcceptRegExp( messageOptions.info.acceptList[i]->GetName( ));
   }
   for( size_t i = 0; i < messageOptions.info.rejectList.GetLength( ); ++i )
   {
      printHandler.AddInfoRejectRegExp( messageOptions.info.rejectList[i]->GetName( ));
   }
   for( size_t i = 0; i < messageOptions.warning.acceptList.GetLength( ); ++i )
   {
      printHandler.AddWarningAcceptRegExp( messageOptions.warning.acceptList[i]->GetName( ));
   }
   for( size_t i = 0; i < messageOptions.warning.rejectList.GetLength( ); ++i )
   {
      printHandler.AddWarningRejectRegExp( messageOptions.warning.rejectList[i]->GetName( ));
   }
   for( size_t i = 0; i < messageOptions.error.acceptList.GetLength( ); ++i )
   {
      printHandler.AddErrorAcceptRegExp( messageOptions.error.acceptList[i]->GetName( ));
   }
   for( size_t i = 0; i < messageOptions.error.rejectList.GetLength( ); ++i )
   {
      printHandler.AddErrorRejectRegExp( messageOptions.error.rejectList[i]->GetName( ));
   }
   for( size_t i = 0; i < messageOptions.trace.acceptList.GetLength( ); ++i )
   {
      printHandler.AddTraceAcceptRegExp( messageOptions.trace.acceptList[i]->GetName( ));
   }
   for( size_t i = 0; i < messageOptions.trace.rejectList.GetLength( ); ++i )
   {
      printHandler.AddTraceRejectRegExp( messageOptions.trace.rejectList[i]->GetName( ));
   }

   printHandler.SetMaxWarningCount( executeOptions.maxWarningCount );
   printHandler.SetMaxErrorCount( executeOptions.maxErrorCount );
}

//===========================================================================//
// Method         : HandleArchitectureSpec_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::HandleArchitectureSpec_( 
      void )
{
   bool ok = this->parchitectureSpec_->InitDefaults( );

   return( ok );
}

//===========================================================================//
// Method         : HandleFabricModel_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TI_Input_c::HandleFabricModel_( 
      void )
{
   // This is only a stub... (currently looking for intereseting work to do)
}

//===========================================================================//
// Method         : HandleCircuitDesign_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::HandleCircuitDesign_( 
      void )
{
   const string& srDefaultBaseName = pcontrolSwitches_->srDefaultBaseName;
   bool ok = this->pcircuitDesign_->InitDefaults( srDefaultBaseName );
   return( ok );
}
