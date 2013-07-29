//===========================================================================//
// Purpose : Declaration and inline definitions for the universal 
//           TCH_RegionPlaceHandler singleton class. This class supports
//           region placement constraints for the VPR placement tool
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

#ifndef TCH_REGION_PLACE_HANDLER_H
#define TCH_REGION_PLACE_HANDLER_H

#include <cstdio>
using namespace std;

#include "TPO_Typedefs.h"
#include "TPO_PlaceRegions.h"
#include "TPO_HierInstMap.h"

#include "TCH_VPR_Data.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
class TCH_RegionPlaceHandler_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TCH_RegionPlaceHandler_c& GetInstance( bool newInstance = true );
   static bool HasInstance( void );

   bool Configure( const TPO_InstList_t& toro_blockList,
                   const TPO_PlaceRegionsList_t& toro_placeRegionsList );

   void Set( t_block* vpr_blockArray,
             int vpr_blockCount,
             const t_logical_block* vpr_logicalBlockArray,
             int vpr_logicalBlockCount );
   void Reset( void );

   void UpdatePlaceRegions( t_block* vpr_blockArray,
                            int vpr_blockCount,
                            const t_logical_block* vpr_logicalBlockArray,
                            int vpr_logicalBlockCount );

   bool IsValid( void ) const;

protected:

   TCH_RegionPlaceHandler_c( void );
   ~TCH_RegionPlaceHandler_c( void );

private:

   void LoadHierInstMapList_( TPO_HierInstMapList_t* phierInstMapList ) const;
   void LoadHierInstMapList_( const t_pb& vpr_pb,
                              const char* pszBlockName,
                              int blockIndex,
                              TPO_HierInstMapList_t* phierInstMapList ) const;
   void UpdatePlacementRegionLists_( const TPO_HierInstMapList_t& hierInstMapList ) const;

private:

   TPO_PlaceRegionsList_t placeRegionsList_;
                                // Ordered list of region placement constraints

   TCH_VPR_Data_c vpr_data_;    // Local copies of VPR's block structures

   static TCH_RegionPlaceHandler_c* pinstance_;  // Define ptr for singleton instance

private:

   enum TCH_DefCapacity_e 
   { 
      TCH_PLACE_REGIONS_LIST_DEF_CAPACITY = 1
   };
};

#endif
