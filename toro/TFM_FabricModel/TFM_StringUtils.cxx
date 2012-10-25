//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TFM_ExtractStringBlockType
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

#include "TFM_StringUtils.h"

//===========================================================================//
// Function       : TFM_ExtractStringBlockType
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TFM_ExtractStringBlockType(
      TFM_BlockType_t type,
      string*         psrType )
{
   if( psrType )
   {
      *psrType = "";

      switch( type )
      {
      case TFM_BLOCK_PHYSICAL_BLOCK: *psrType = "PB"; break;
      case TFM_BLOCK_INPUT_OUTPUT:   *psrType = "IO"; break;
      case TFM_BLOCK_SWITCH_BOX:     *psrType = "SB"; break;
      default:                       *psrType = "?";  break;
      }
   }
}
