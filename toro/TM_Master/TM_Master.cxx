//===========================================================================//
// Purpose : Method definitions for the TM_Master class.
//
//           Public methods include:
//           - TM_Master_c, ~TM_Master_c
//           - Apply
//
//           Private methods include:
//           - NewHandlerSingletons_
//           - DeleteHandlerSingletons_
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

#include "TIO_PrintHandler.h"

#include "TOS_OptionsStore.h"
#include "TFS_FloorplanStore.h"

#if defined( TM_MASTER_NEW_DELETE_HANDLERS )
   #include "TC_MinGrid.h"
#endif

#include "TI_Input.h"
#include "TP_Process.h"
#include "TO_Output.h"

#include "TM_Master.h"

//===========================================================================//
// Method         : TM_Master_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TM_Master_c::TM_Master_c( 
      void )
{
}

//===========================================================================//
TM_Master_c::TM_Master_c( 
      const TCL_CommandLine_c& commandLine,
            bool*              pok )
{
   bool ok = this->Apply( commandLine );
   if( pok )
   {
      *pok = ok;
   }
}

//===========================================================================//
// Method         : ~TM_Master_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TM_Master_c::~TM_Master_c( 
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
bool TM_Master_c::Apply( 
      const TCL_CommandLine_c& commandLine )
{
   bool ok = true;

   // Initialize various data handler singletons prior to doing any useful work
   this->NewHandlerSingletons_( );

   // And allocate an options store and floorplan (architecture + circuit) store
   TOS_OptionsStore_c optionsStore;
   TFS_FloorplanStore_c floorplanStore;

   // Inform user that "Elvis has entered the building..."
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.WriteBanner( );

   // Time to do get serious and do some input-process-output work
   if( ok )
   {
      TI_Input_c input;
      ok = input.Apply( commandLine, &optionsStore, &floorplanStore );
   }
   if( ok )
   {
      TP_Process_c process;
      ok = process.Apply( optionsStore, &floorplanStore );
   }
   if( ok )
   {
      TO_Output_c output;
      ok = output.Apply( commandLine, optionsStore, floorplanStore );
   }
   if( ok )
   {
      ok = ( printHandler.GetErrorCount( ) == 0 &&
             printHandler.GetInternalCount( ) == 0 ?
             true : false );
   }

   // Destroy various data handler singletons after completing our masterpiece
   this->DeleteHandlerSingletons_( );

   return( ok );
} 

//===========================================================================//
// Method         : NewHandlerSingletons_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TM_Master_c::NewHandlerSingletons_(
      void ) const
{
   #if defined( TM_MASTER_NEW_DELETE_HANDLERS )

      // Initialize program data handler 'singletons' (one or more)
      TC_MinGrid_c::NewInstance( );

   #endif
}

//===========================================================================//
// Method         : DeleteHandlerSingletons_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TM_Master_c::DeleteHandlerSingletons_(
      void ) const
{
   #if defined( TM_MASTER_NEW_DELETE_HANDLERS )

      // Don't forget, 'Leave No Trace' (mostly to clean-up memory leak warnings)
      TC_MinGrid_c::DeleteInstance( );

   #endif
}
