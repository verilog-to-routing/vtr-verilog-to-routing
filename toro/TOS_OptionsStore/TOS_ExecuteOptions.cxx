//===========================================================================//
// Purpose : Method definitions for the TOS_ExecuteOptions class.
//
//           Public methods include:
//           - TOS_ExecuteOptions_c, ~TOS_ExecuteOptions_c
//           - Print
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

#include "TOS_StringUtils.h"
#include "TOS_ExecuteOptions.h"

//===========================================================================//
// Method         : TOS_ExecuteOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_ExecuteOptions_c::TOS_ExecuteOptions_c( 
      void )
      :
      maxWarningCount( 0 ),
      maxErrorCount( 0 ),
      toolMask( 0 )
{
}

//===========================================================================//
TOS_ExecuteOptions_c::TOS_ExecuteOptions_c( 
      unsigned long maxWarningCount_,
      unsigned long maxErrorCount_,
      int           toolMask_ )
      :
      maxWarningCount( maxWarningCount_ ),
      maxErrorCount( maxErrorCount_ ),
      toolMask( toolMask_ )
{
}

//===========================================================================//
// Method         : ~TOS_ExecuteOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_ExecuteOptions_c::~TOS_ExecuteOptions_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExecuteOptions_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srToolMask;
   TOS_ExtractStringExecuteToolMask( this->toolMask, &srToolMask );

   printHandler.Write( pfile, spaceLen, "HALT_MAX_WARNINGS          = %lu\n", this->maxWarningCount );
   printHandler.Write( pfile, spaceLen, "HALT_MAX_ERRORS            = %lu\n", this->maxErrorCount );

   printHandler.Write( pfile, spaceLen, "EXECUTE_MODE               = { %s}\n", TIO_SR_STR( srToolMask ));
}
