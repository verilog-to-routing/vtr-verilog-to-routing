//===========================================================================//
// Purpose : Declaration and inline definitions for the universal 
//           TCH_RegionPlaceHandler singleton class. This class supports
//           region placement constraints for the VPR placement tool
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
