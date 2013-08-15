//===========================================================================//
// Purpose : Enums, typedefs, and defines for TFH_FabricHandlers class.
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

#ifndef TFH_TYPEDEFS_H
#define TFH_TYPEDEFS_H

#include "TC_IndexCount.h"
#include "TCT_OrderedVector.h"
#include "TCT_SortedVector.h"

//---------------------------------------------------------------------------//
// Define fabric grid block constants and typedefs
//---------------------------------------------------------------------------//

class TFH_GridBlock_c; // Forward declaration for subsequent class
typedef TCT_SortedVector_c< TFH_GridBlock_c > TFH_GridBlockList_t;

enum TFH_BlockType_e
{
   TFH_BLOCK_UNDEFINED = 0,
   TFH_BLOCK_PHYSICAL_BLOCK,
   TFH_BLOCK_INPUT_OUTPUT,
   TFH_BLOCK_SWITCH_BOX
};
typedef enum TFH_BlockType_e TFH_BlockType_t;

//---------------------------------------------------------------------------//
// Define fabric switch box constants and typedefs
//---------------------------------------------------------------------------//

class TFH_SwitchBox_c; // Forward declaration for subsequent class
typedef TCT_SortedVector_c< TFH_SwitchBox_c > TFH_SwitchBoxList_t;

//---------------------------------------------------------------------------//
// Define fabric connection block constants and typedefs
//---------------------------------------------------------------------------//

class TFH_ConnectionBlock_c; // Forward declaration for subsequent class
typedef TCT_SortedVector_c< TFH_ConnectionBlock_c > TFH_ConnectionBlockList_t;

class TFH_ConnectionMap_c; // Forward declaration for subsequent class
typedef TCT_SortedVector_c< TFH_ConnectionMap_c > TFH_ConnectionMapList_t;

class TC_Bit_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TC_Bit_c > TFH_BitPattern_t;

//---------------------------------------------------------------------------//
// Define fabric channel width constants and typedefs
//---------------------------------------------------------------------------//

typedef TC_IndexCount_c TFH_ChannelWidth_t;
typedef TCT_OrderedVector_c< TFH_ChannelWidth_t > TFH_ChannelWidthList_t;

enum TFH_SelectChannelMode_e
{
   TFH_SELECT_CHANNEL_UNDEFINED = 0,
   TFH_SELECT_CHANNEL_X,
   TFH_SELECT_CHANNEL_Y
};
typedef enum TFH_SelectChannelMode_e TFH_SelectChannelMode_t;

#endif
