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
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
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
