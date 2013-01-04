//===========================================================================//
// Purpose : Enums, typedefs, and defines for TRP_RelativePlace classes.
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TRP_TYPEDEFS_H
#define TRP_TYPEDEFS_H

#include "TCT_OrderedVector.h"
#include "TCT_SortedNameDynamicVector.h"
#include "TCT_Hash.h"

//---------------------------------------------------------------------------//
// Define relative place handler constants and typedefs
//---------------------------------------------------------------------------//

enum TRP_PlaceMacroRotateMode_e 
{ 
   TRP_PLACE_MACRO_ROTATE_UNDEFINED,
   TRP_PLACE_MACRO_ROTATE_FROM,
   TRP_PLACE_MACRO_ROTATE_TO
};
typedef enum TRP_PlaceMacroRotateMode_e TRP_PlaceMacroRotateMode_t;

// Define maximum number of attempts made while trying to find a random,
// yet valid (legal) origin point (ie. placement) position.
#define TRP_FIND_RANDOM_ORIGIN_POINT_MAX_ATTEMPTS 64

//---------------------------------------------------------------------------//
// Define relative macro constants and typedefs
//---------------------------------------------------------------------------//

class TRP_RelativeMacro_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TRP_RelativeMacro_c > TRP_RelativeMacroList_t;

#define TRP_RELATIVE_MACRO_UNDEFINED SIZE_MAX
 
//---------------------------------------------------------------------------//
// Define relative node constants and typedefs
//---------------------------------------------------------------------------//

class TRP_RelativeNode_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TRP_RelativeNode_c > TRP_RelativeNodeList_t;

#define TRP_RELATIVE_NODE_UNDEFINED SIZE_MAX

//---------------------------------------------------------------------------//
// Define relative block constants and typedefs
//---------------------------------------------------------------------------//

class TRP_RelativeBlock_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedNameDynamicVector_c< TRP_RelativeBlock_c > TRP_RelativeBlockList_t;

//---------------------------------------------------------------------------//
// Define relative block constants and typedefs
//---------------------------------------------------------------------------//

class TRP_RelativeMove_c; // Forward declaration for subsequent class typedefs
typedef TCT_OrderedVector_c< TRP_RelativeMove_c > TRP_RelativeMoveList_t;

//---------------------------------------------------------------------------//
// Define rotate mask hash constants and typedefs
//---------------------------------------------------------------------------//

class TRP_RotateMaskKey_c; // Forward declaration for subsequent class typedefs
class TRP_RotateMaskValue_c; // Forward declaration for subsequent class typedefs
typedef TCT_Hash_c< TRP_RotateMaskKey_c, TRP_RotateMaskValue_c > TRP_RotateMaskHash_t;

//---------------------------------------------------------------------------//
// Define VPR place.c structures (temporary solution until place.c updated...)
//---------------------------------------------------------------------------//
// ???
#ifndef PLACE_MACRO_H
   typedef struct s_pl_moved_block 
   {
   	int block_num;
   	int xold;
   	int xnew;
   	int yold;
   	int ynew;
   	int zold;
   	int znew;
   	int swapped_to_was_empty;
   	int swapped_from_is_empty;
   } t_pl_moved_block;

   typedef struct s_pl_blocks_to_be_moved 
   {
   	int num_moved_blocks;
   	t_pl_moved_block * moved_blocks;
   } t_pl_blocks_to_be_moved;

   typedef struct s_legal_pos
   {
	int x;
	int y;
	int z;
   } t_legal_pos;
#endif

#endif
