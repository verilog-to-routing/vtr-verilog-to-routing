//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TTP_ExtractStringTileMode
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

#include "TTP_StringUtils.h"

//===========================================================================//
// Function       : TTP_ExtractStringTileMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TTP_ExtractStringTileMode(
      TTP_TileMode_t tileMode,
      string*        psrTileMode )
{
   if( psrTileMode )
   {
      *psrTileMode = "";

      switch( tileMode )
      {
      case TTP_TILE_CLEAR: *psrTileMode = "CLEAR"; break;
      case TTP_TILE_SOLID: *psrTileMode = "SOLID"; break;
      default:             *psrTileMode = "*";     break;
      }
   }
} 
