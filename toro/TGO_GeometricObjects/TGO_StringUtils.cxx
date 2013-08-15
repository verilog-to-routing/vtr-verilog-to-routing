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
