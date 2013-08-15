//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TPO_ExtractStringInstSource
//           - TPO_ExtractStringLatchType
//           - TPO_ExtractStringLatchState
//           - TPO_ExtractStringStatusMode
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

#include "TPO_StringUtils.h"

//===========================================================================//
// Function       : TPO_ExtractStringInstSource
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_ExtractStringInstSource(
      TPO_InstSource_t source,
      string*          psrSource )
{
   if( psrSource )
   {
      *psrSource = "";

      switch( source )
      {
      case TPO_INST_SOURCE_INPUT:  *psrSource = "input";  break;
      case TPO_INST_SOURCE_OUTPUT: *psrSource = "output"; break;
      case TPO_INST_SOURCE_NAMES:  *psrSource = "names";  break;
      case TPO_INST_SOURCE_LATCH:  *psrSource = "latch";  break;
      case TPO_INST_SOURCE_SUBCKT: *psrSource = "subckt"; break;
      default:                     *psrSource = "?";      break;
      }
   }
}

//===========================================================================//
// Function       : TPO_ExtractStringLatchType
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_ExtractStringLatchType(
      TPO_LatchType_t type,
      string*         psrType )
{
   if( psrType )
   {
      *psrType = "";

      switch( type )
      {
      case TPO_LATCH_TYPE_FALLING_EDGE: *psrType = "fe"; break;
      case TPO_LATCH_TYPE_RISING_EDGE:  *psrType = "re"; break;
      case TPO_LATCH_TYPE_ACTIVE_HIGH:  *psrType = "ah"; break;
      case TPO_LATCH_TYPE_ACTIVE_LOW:   *psrType = "al"; break;
      case TPO_LATCH_TYPE_ASYNCHRONOUS: *psrType = "as"; break;
      default:                          *psrType = "?";  break;
      }
   }
}

//===========================================================================//
// Function       : TPO_ExtractStringLatchState
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_ExtractStringLatchState(
      TPO_LatchState_t state,
      string*          psrState )
{
   if( psrState )
   {
      *psrState = "";

      switch( state )
      {
      case TPO_LATCH_STATE_TRUE:      *psrState = "1"; break;
      case TPO_LATCH_STATE_FALSE:     *psrState = "0"; break;
      case TPO_LATCH_STATE_DONT_CARE: *psrState = "2"; break;
      case TPO_LATCH_STATE_UNKNOWN:   *psrState = "3"; break;
      default:                        *psrState = "?"; break;
      }
   }
}

//===========================================================================//
// Function       : TPO_ExtractStringStatusMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_ExtractStringStatusMode(
      TPO_StatusMode_t status,
      string*          psrStatus )
{
   if( psrStatus )
   {
      *psrStatus = "";

      switch( status )
      {
      case TPO_STATUS_FLOAT:  *psrStatus = "float";  break;
      case TPO_STATUS_FIXED:  *psrStatus = "fixed";  break;
      case TPO_STATUS_PLACED: *psrStatus = "placed"; break;
      default:                *psrStatus = "?";      break;
      }
   }
}
