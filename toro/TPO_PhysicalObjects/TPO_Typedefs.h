//===========================================================================//
// Purpose : Enums, typedefs, and defines for TPO_PhysicalObjects classes.
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

#ifndef TPO_TYPEDEFS_H
#define TPO_TYPEDEFS_H

#include "TCT_OrderedQueue.h"
#include "TCT_OrderedVector.h"
#include "TCT_SortedNameDynamicVector.h"

#include "TC_Name.h"
#include "TCT_NameList.h"

//---------------------------------------------------------------------------//
// Define instance constants and typedefs
//---------------------------------------------------------------------------//

class TPO_Inst_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedNameDynamicVector_c< TPO_Inst_c > TPO_InstList_t;

typedef TPO_Inst_c TPO_Port_t; 
typedef TCT_SortedNameDynamicVector_c< TPO_Port_t > TPO_PortList_t;

enum TPO_InstSource_e
{
   TPO_INST_SOURCE_UNDEFINED = 0,
   TPO_INST_SOURCE_INPUT,
   TPO_INST_SOURCE_OUTPUT,
   TPO_INST_SOURCE_NAMES,
   TPO_INST_SOURCE_LATCH,
   TPO_INST_SOURCE_SUBCKT
};
typedef enum TPO_InstSource_e TPO_InstSource_t;

enum TPO_LatchType_e
{
   TPO_LATCH_TYPE_UNDEFINED = 0,
   TPO_LATCH_TYPE_FALLING_EDGE, // corresponds to BLIF type 'FE'
   TPO_LATCH_TYPE_RISING_EDGE,  // corresponds to BLIF type 'RE'
   TPO_LATCH_TYPE_ACTIVE_HIGH,  // corresponds to BLIF type 'AH'
   TPO_LATCH_TYPE_ACTIVE_LOW,   // corresponds to BLIF type 'AL'
   TPO_LATCH_TYPE_ASYNCHRONOUS  // corresponds to BLIF type 'AS'
};
typedef enum TPO_LatchType_e TPO_LatchType_t;

enum TPO_LatchState_e
{
   TPO_LATCH_STATE_UNDEFINED = 0,
   TPO_LATCH_STATE_TRUE,        // corresponds to BLIF init-val '1'
   TPO_LATCH_STATE_FALSE,       // corresponds to BLIF init-val '0'
   TPO_LATCH_STATE_DONT_CARE,   // corresponds to BLIF init-val '2'
   TPO_LATCH_STATE_UNKNOWN      // corresponds to BLIF init-val '3'
};
typedef enum TPO_LatchState_e TPO_LatchState_t;

class TC_Bit_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedQueue_c< TC_Bit_c > TPO_LogicBits_t;
typedef TCT_OrderedVector_c< TPO_LogicBits_t > TPO_LogicBitsList_t;

enum TPO_StatusMode_e
{
   TPO_STATUS_UNDEFINED = 0,
   TPO_STATUS_FLOAT,
   TPO_STATUS_FIXED,
   TPO_STATUS_PLACED
};
typedef enum TPO_StatusMode_e TPO_StatusMode_t;

typedef TCT_NameList_c< TC_Name_c > TPO_HierNameList_t;

class TPO_HierMap_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TPO_HierMap_c > TPO_HierMapList_t;

class TC_SideName_c; // Forward declaration for subsequent class typedefs
typedef TC_SideName_c TPO_Relative_t;
typedef TCT_SortedNameDynamicVector_c< TPO_Relative_t > TPO_RelativeList_t;

//---------------------------------------------------------------------------//
// Define pin constants and typedefs
//---------------------------------------------------------------------------//

class TC_NameType_c; // Forward declaration for subsequent class typedefs
typedef TC_NameType_c TPO_Pin_t;
typedef TCT_SortedNameDynamicVector_c< TPO_Pin_t > TPO_PinList_t;

//---------------------------------------------------------------------------//
// Define instance pin constants and typedefs
//---------------------------------------------------------------------------//

class TPO_PinMap_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TPO_PinMap_c > TPO_PinMapList_t;

#endif
