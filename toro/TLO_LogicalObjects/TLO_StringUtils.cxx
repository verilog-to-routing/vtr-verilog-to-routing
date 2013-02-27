//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TLO_ExtractStringCellSource
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

#include "TLO_StringUtils.h"

//===========================================================================//
// Function       : TLO_ExtractStringCellSource
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TLO_ExtractStringCellSource(
      TLO_CellSource_t source,
      string*          psrSource )
{
   if( psrSource )
   {
      *psrSource = "";

      switch( source )
      {
      case TLO_CELL_SOURCE_STANDARD: *psrSource = "standard"; break;
      case TLO_CELL_SOURCE_CUSTOM:   *psrSource = "custom";   break;
      default:                       *psrSource = "?";        break;
      }
   }
}
