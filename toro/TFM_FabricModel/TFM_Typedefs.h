//===========================================================================//
// Purpose : Enums, typedefs, and defines for TFM_FabricModel classes.
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
