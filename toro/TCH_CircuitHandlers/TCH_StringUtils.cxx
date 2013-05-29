//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TCH_ExtractStringStatusMode
//           - TCH_ExtractStringVPR_Type
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

#include "TCH_StringUtils.h"

//===========================================================================//
// Function       : TCH_ExtractStringStatusMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_ExtractStringStatusMode(
      TCH_PlaceStatusMode_t mode,
      string*               psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TCH_PLACE_STATUS_FLOAT:  *psrMode = "float";  break;
      case TCH_PLACE_STATUS_FIXED:  *psrMode = "fixed";  break;
      case TCH_PLACE_STATUS_PLACED: *psrMode = "placed"; break;
      default:                      *psrMode = "?";      break;
      }
   }
}

//===========================================================================//
void TCH_ExtractStringStatusMode(
      TCH_RouteStatusMode_t mode,
      string*               psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TCH_ROUTE_STATUS_FLOAT:  *psrMode = "float";  break;
      case TCH_ROUTE_STATUS_FIXED:  *psrMode = "fixed";  break;
      case TCH_ROUTE_STATUS_ROUTED: *psrMode = "routed"; break;
      default:                      *psrMode = "?";      break;
      }
   }
}

//===========================================================================//
// Function       : TCH_ExtractStringVPR_Type
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_ExtractStringVPR_Type(
      t_rr_type type,
      string*   psrType )
{
   if( psrType )
   {
      *psrType = "";

      switch( type )
      {
      case SOURCE: *psrType = "source"; break;
      case SINK:   *psrType = "sink";   break;
      case IPIN:   *psrType = "ipin";   break;
      case OPIN:   *psrType = "opin";   break;
      case CHANX:  *psrType = "chanx";  break;
      case CHANY:  *psrType = "chany";  break;
      default:     *psrType = "?";      break;
      }
   }
}
