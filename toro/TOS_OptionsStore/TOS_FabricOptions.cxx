//===========================================================================//
// Purpose : Method definitions for the TOS_FabricOptions class.
//
//           Public methods include:
//           - TOS_FabricOptions_c, ~TOS_FabricOptions_c
//           - Print
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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
#include "TOS_FabricOptions.h"

//===========================================================================//
// Method         : TOS_FabricOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/12/13 jeffr : Original
//===========================================================================//
TOS_FabricOptions_c::TOS_FabricOptions_c( 
      void )
{
   this->blocks.override = false;
   this->switchBoxes.override = false;
   this->connectionBlocks.override = false;
   this->channels.override = false;
}

//===========================================================================//
TOS_FabricOptions_c::TOS_FabricOptions_c( 
      bool blocks_override_,
      bool switchBoxes_override_,
      bool connectionBlocks_override_,
      bool channels_override_ )
{
   this->blocks.override = blocks_override_;
   this->switchBoxes.override = switchBoxes_override_;
   this->connectionBlocks.override = connectionBlocks_override_;
   this->channels.override = channels_override_;
}

//===========================================================================//
// Method         : ~TOS_FabricOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/12/13 jeffr : Original
//===========================================================================//
TOS_FabricOptions_c::~TOS_FabricOptions_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/12/13 jeffr : Original
//===========================================================================//
void TOS_FabricOptions_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "FABRIC_BLOCK_ENABLE        = %s\n", TIO_BOOL_STR( this->blocks.override ));
   printHandler.Write( pfile, spaceLen, "FABRIC_SB_ENABLE           = %s\n", TIO_BOOL_STR( this->connectionBlocks.override ));
   printHandler.Write( pfile, spaceLen, "FABRIC_CB_ENABLE           = %s\n", TIO_BOOL_STR( this->connectionBlocks.override ));
   printHandler.Write( pfile, spaceLen, "FABRIC_CHANNEL_ENABLE      = %s\n", TIO_BOOL_STR( this->channels.override ));
}
