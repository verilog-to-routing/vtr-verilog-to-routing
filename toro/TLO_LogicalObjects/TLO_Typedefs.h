//===========================================================================//
// Purpose : Enums, typedefs, and defines for TLO_LogicalObjects classes.
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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
// Define pin constants and typedefs
//---------------------------------------------------------------------------//

class TC_NameType_c; // Forward declaration for subsequent class typedefs
typedef TC_NameType_c TLO_Pin_t;
typedef TCT_SortedNameDynamicVector_c< TLO_Pin_t > TLO_PinList_t;

#endif
