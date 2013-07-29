//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TLO_ExtractStringCellSource
//           - TLO_ExtractStringPowerType
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
      TLO_CellSource_t cellSource,
      string*          psrCellSource )
{
   if( psrCellSource )
   {
      *psrCellSource = "";

      switch( cellSource )
      {
      case TLO_CELL_SOURCE_STANDARD: *psrCellSource = "standard"; break;
      case TLO_CELL_SOURCE_CUSTOM:   *psrCellSource = "custom";   break;
      default:                       *psrCellSource = "?";        break;
      }
   }
}

//===========================================================================//
// Function       : TLO_ExtractStringPowerType
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TLO_ExtractStringPowerType(
      TLO_PowerType_t cellSource,
      string*         psrPowerType )
{
   if( psrPowerType )
   {
      *psrPowerType = "";

      switch( cellSource )
      {
      case TLO_POWER_TYPE_IGNORED:         *psrPowerType = "ignored";         break;
      case TLO_POWER_TYPE_NONE:            *psrPowerType = "none";            break;
      case TLO_POWER_TYPE_AUTO:            *psrPowerType = "auto";            break;
      case TLO_POWER_TYPE_CAP:             *psrPowerType = "cap";             break;
      case TLO_POWER_TYPE_RELATIVE_LENGTH: *psrPowerType = "relative_length"; break;
      case TLO_POWER_TYPE_ABSOLUTE_LENGTH: *psrPowerType = "absoute_length";  break;
      case TLO_POWER_TYPE_ABSOLUTE_SIZE:   *psrPowerType = "absolute_size";   break; 
      default:                             *psrPowerType = "?";               break;
      }
   }
}
