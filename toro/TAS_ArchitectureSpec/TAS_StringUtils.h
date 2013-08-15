//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful string functions.
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

#ifndef TAS_STRING_UTILS_H
#define TAS_STRING_UTILS_H

#include <string>
using namespace std;

#include "TAS_Typedefs.h"

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//

void TAS_ExtractStringClassType( TAS_ClassType_t type, string* psrType );

void TAS_ExtractStringPhysicalBlockModelType( TAS_PhysicalBlockModelType_t type, string* psrType );
void TAS_ExtractStringInterconnectMapType( TAS_InterconnectMapType_t type, string* psrType );
void TAS_ExtractStringConnectionBoxType( TAS_ConnectionBoxType_t type, string* psrType );

void TAS_ExtractStringPinAssignPatternType( TAS_PinAssignPatternType_t type, string* psrType );
void TAS_ExtractStringGridAssignDistrMode( TAS_GridAssignDistrMode_t mode, string* psrMode );
void TAS_ExtractStringGridAssignOrientMode( TAS_GridAssignOrientMode_t mode, string* psrMode );

void TAS_ExtractStringTimingMode( TAS_TimingMode_t mode, string* psrMode );
void TAS_ExtractStringTimingType( TAS_TimingType_t type, string* psrType );

void TAS_ExtractStringArraySizeMode( TAS_ArraySizeMode_t mode, string* psrMode );

void TAS_ExtractStringSwitchBoxType( TAS_SwitchBoxType_t type, string* psrType );
void TAS_ExtractStringSwitchBoxModelType( TAS_SwitchBoxModelType_t type, string* psrType );

void TAS_ExtractStringSegmentDirType( TAS_SegmentDirType_t type, string* psrType );

void TAS_ExtractStringChannelUsageMode( TAS_ChannelUsageMode_t mode, string* psrMode );
void TAS_ExtractStringChannelDistrMode( TAS_ChannelDistrMode_t mode, string* psrMode );

void TAS_ExtractStringPowerMethodMode( TAS_PowerMethodMode_t mode, string* psrMode );

#endif
