//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TGS_ExtractStringOrientMode
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

#include "TGS_StringUtils.h"

//===========================================================================//
// Function       : TGS_ExtractStringOrientMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_ExtractStringOrientMode(
      TGS_OrientMode_t orientMode,
      string*          psrOrientMode )
{
   if( psrOrientMode )
   {
      *psrOrientMode = "";

      switch( orientMode )
      {
      case TGS_ORIENT_HORIZONTAL: *psrOrientMode = "horz"; break;
      case TGS_ORIENT_VERTICAL:   *psrOrientMode = "vert"; break;
      default:                    *psrOrientMode = "?";    break;
      }
   }
}
