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
