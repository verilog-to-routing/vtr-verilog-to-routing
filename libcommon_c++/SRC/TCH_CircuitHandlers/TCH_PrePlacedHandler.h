//===========================================================================//
// Purpose : Declaration and inline definitions for the universal 
//           TCH_PrePlacedHandler singleton class. This class supports
//           pre-placed constraints for the VPR placement tool
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

#ifndef TCH_PREPLACED_PLACE_HANDLER_H
#define TCH_PREPLACED_PLACE_HANDLER_H

#include <cstdio>
using namespace std;

#include "TPO_Typedefs.h"
#include "TPO_Inst.h"

#include "TCH_Typedefs.h"
#include "TCH_PrePlacedBlock.h"
#include "TCH_VPR_Data.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
class TCH_PrePlacedHandler_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TCH_PrePlacedHandler_c& GetInstance( bool newInstance = true );
   static bool HasInstance( void );

   void Configure( const TPO_InstList_t& toro_circuitBlockList );

   void Set( t_grid_tile** vpr_gridArray,
             int vpr_nx,
             int vpr_ny,
             t_block* vpr_blockArray,
             int vpr_blockCount,
             const t_type_descriptor* vpr_typeArray,
             int vpr_typeCount,
             int* vpr_freeLocationArray,
             t_legal_pos** vpr_legalPosArray );

   bool Reset( void );

   bool InitialPlace( t_grid_tile** vpr_gridArray,
                      int vpr_nx,
                      int vpr_ny,
                      t_block* vpr_blockArray,
                      int vpr_blockCount,
                      const t_type_descriptor* vpr_typeArray,
                      int vpr_typeCount,
                      int* vpr_freeLocationArray,
                      t_legal_pos** vpr_legalPosArray );

   bool IsValid( void ) const;

protected:

   TCH_PrePlacedHandler_c( void );
   ~TCH_PrePlacedHandler_c( void );

private:

   bool ResetPrePlacedBlockList_( t_block* vpr_blockArray,
                                  int vpr_blockCount,
                                  TCH_PrePlacedBlockList_t* pprePlacedBlockList );

   bool InitialPlaceBlockList_( bool* pplacedAllBlocks );
   bool InitialPlaceBlock_( const TCH_PrePlacedBlock_c& prePlacedBlock,
                            bool* pvalidPlacement,
                            bool* pfoundPlacement );

   bool ShowIllegalBlockWarning_( const char* pszBlockName,
                                  const TGO_Point_c& origin ) const;
   bool ShowMissingBlockNameError_( const char* pszBlockName ) const;

private:

   TCH_PrePlacedBlockList_t prePlacedBlockList_;
                                // Sorted list of VPR's block names and placements

   TCH_VPR_Data_c vpr_data_;    // Local copies of VPR's grid and block structures

   static TCH_PrePlacedHandler_c* pinstance_;  // Define ptr for singleton instance

private:

   enum TCH_DefCapacity_e 
   { 
      TCH_PREPLACED_BLOCK_LIST_DEF_CAPACITY = 1
   };
};

#endif
