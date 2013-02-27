//===========================================================================//
// Purpose : Enums, typedefs, and defines for TFM_FabricModel classes.
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

#ifndef TFM_TYPEDEFS_H
#define TFM_TYPEDEFS_H

#include "TCT_SortedNameDynamicVector.h"
#include "TCT_SortedVector.h"
#include "TCT_OrderedVector.h"

//---------------------------------------------------------------------------//
// Define fabric data constants and typedefs
//---------------------------------------------------------------------------//

enum TFM_BlockType_e
{
   TFM_BLOCK_UNDEFINED = 0,
   TFM_BLOCK_PHYSICAL_BLOCK,
   TFM_BLOCK_INPUT_OUTPUT,
   TFM_BLOCK_SWITCH_BOX
};
typedef enum TFM_BlockType_e TFM_BlockType_t;

class TFM_Block_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedNameDynamicVector_c< TFM_Block_c > TFM_BlockList_t;

//---------------------------------------------------------------------------//
// Define physical block constants and typedefs
//---------------------------------------------------------------------------//

typedef TFM_Block_c TFM_PhysicalBlock_t;
typedef TCT_SortedNameDynamicVector_c< TFM_PhysicalBlock_t > TFM_PhysicalBlockList_t;

//---------------------------------------------------------------------------//
// Define input/output constants and typedefs
//---------------------------------------------------------------------------//

typedef TFM_Block_c TFM_InputOutput_t;
typedef TCT_SortedNameDynamicVector_c< TFM_InputOutput_t > TFM_InputOutputList_t;

//---------------------------------------------------------------------------//
// Define switch box constants and typedefs
//---------------------------------------------------------------------------//

typedef TFM_Block_c TFM_SwitchBox_t;
typedef TCT_SortedNameDynamicVector_c< TFM_SwitchBox_t > TFM_SwitchBoxList_t;

//---------------------------------------------------------------------------//
// Define channel constants and typedefs
//---------------------------------------------------------------------------//

class TFM_Channel_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedNameDynamicVector_c< TFM_Channel_c > TFM_ChannelList_t;

//---------------------------------------------------------------------------//
// Define segment constants and typedefs
//---------------------------------------------------------------------------//

class TFM_Segment_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedNameDynamicVector_c< TFM_Segment_c > TFM_SegmentList_t;

//---------------------------------------------------------------------------//
// Define pin constants and typedefs
//---------------------------------------------------------------------------//

class TFM_Pin_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedVector_c< TFM_Pin_c > TFM_PinList_t;

class TC_Bit_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TC_Bit_c > TFM_BitPattern_t;

#endif
