//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TGO_ExtractStringRotateMode
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

#include "TGO_StringUtils.h"

//===========================================================================//
// Function       : TGO_ExtractStringRotateMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TGO_ExtractStringRotateMode(
      TGO_RotateMode_t rotateMode,
      string*          psrRotateMode )
{
   if( psrRotateMode )
   {
      *psrRotateMode = "";

      switch( rotateMode )
      {
      case TGO_ROTATE_R0:    *psrRotateMode = "R0";    break;
      case TGO_ROTATE_R90:   *psrRotateMode = "R90";   break;
      case TGO_ROTATE_R180:  *psrRotateMode = "R180";  break;
      case TGO_ROTATE_R270:  *psrRotateMode = "R270";  break;
      case TGO_ROTATE_MX:    *psrRotateMode = "MX";    break;
      case TGO_ROTATE_MXR90: *psrRotateMode = "MXR90"; break;
      case TGO_ROTATE_MY:    *psrRotateMode = "MY";    break;
      case TGO_ROTATE_MYR90: *psrRotateMode = "MYR90"; break;
      default:               *psrRotateMode = "?";     break;
      }
   }
}
