//===========================================================================//
// Purpose : Enums, typedefs, and defines for TLO_LogicalObjects classes.
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

#ifndef TLO_TYPEDEFS_H
#define TLO_TYPEDEFS_H

#include "TCT_OrderedVector.h"
#include "TCT_SortedNameDynamicVector.h"

//---------------------------------------------------------------------------//
// Define cell constants and typedefs
//---------------------------------------------------------------------------//

class TLO_Cell_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedNameDynamicVector_c< TLO_Cell_c > TLO_CellList_t;

enum TLO_CellSource_e
{
   TLO_CELL_SOURCE_UNDEFINED = 0,
   TLO_CELL_SOURCE_AUTO_DEFINED,
   TLO_CELL_SOURCE_USER_DEFINED,

   TLO_CELL_SOURCE_STANDARD = TLO_CELL_SOURCE_AUTO_DEFINED,
   TLO_CELL_SOURCE_CUSTOM = TLO_CELL_SOURCE_USER_DEFINED
};
typedef enum TLO_CellSource_e TLO_CellSource_t;

//---------------------------------------------------------------------------//
// Define port constants and typedefs
//---------------------------------------------------------------------------//

class TLO_Port_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedNameDynamicVector_c< TLO_Port_c > TLO_PortList_t;

//---------------------------------------------------------------------------//
// Define power constants and typedefs
//---------------------------------------------------------------------------//

class TLO_Power_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TLO_Power_c > TLO_PowerList_t;

enum TLO_PowerType_e
{
   TLO_POWER_TYPE_UNDEFINED = 0,
   TLO_POWER_TYPE_IGNORED,
   TLO_POWER_TYPE_NONE,
   TLO_POWER_TYPE_AUTO,
   TLO_POWER_TYPE_CAP,
   TLO_POWER_TYPE_RELATIVE_LENGTH,
   TLO_POWER_TYPE_ABSOLUTE_LENGTH,
   TLO_POWER_TYPE_ABSOLUTE_SIZE
};
typedef TLO_PowerType_e TLO_PowerType_t;

//---------------------------------------------------------------------------//
// Define pin constants and typedefs
//---------------------------------------------------------------------------//

class TC_NameType_c; // Forward declaration for subsequent class typedefs
typedef TC_NameType_c TLO_Pin_t;
typedef TCT_SortedNameDynamicVector_c< TLO_Pin_t > TLO_PinList_t;

#endif
