//===========================================================================//
// Purpose : Method definitions for the TCH_RegionPlaceHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - Configure
//           - Set, Reset
//           - UpdatePlaceRegions
//           - IsValid
//
//           Protected methods include:
//           - TCH_RegionPlaceHandler_c, ~TCH_RegionPlaceHandler_c
//
//           Private methods include:
//           - LoadHierInstMapList_
//           - UpdatePlacementRegionLists_
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

#include <cstdio>
#include <cstring>
#include <string>
using namespace std;

#include "TGS_ArrayGrid.h"

#include "TPO_Inst.h"

#include "TCH_RegionPlaceHandler.h"

// Initialize the region place handler "singleton" class, as needed
TCH_RegionPlaceHandler_c* TCH_RegionPlaceHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TCH_RegionPlaceHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
TCH_RegionPlaceHandler_c::TCH_RegionPlaceHandler_c( 
      void )
      :
      placeRegionsList_( TCH_PLACE_REGIONS_LIST_DEF_CAPACITY )
{
   pinstance_ = 0;
}

//===========================================================================//
// Method         : ~TCH_RegionPlaceHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
TCH_RegionPlaceHandler_c::~TCH_RegionPlaceHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
void TCH_RegionPlaceHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TCH_RegionPlaceHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
void TCH_RegionPlaceHandler_c::DeleteInstance( 
      void )
{
   if( pinstance_ )
   {
      delete pinstance_;
      pinstance_ = 0;
   }
}

//===========================================================================//
// Method         : GetInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
TCH_RegionPlaceHandler_c& TCH_RegionPlaceHandler_c::GetInstance(
      bool newInstance )
{
   if( !pinstance_ )
   {
      if( newInstance )
      {
         NewInstance( );
      }
   }
   return( *pinstance_ );
}

//===========================================================================//
// Method         : HasInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
bool TCH_RegionPlaceHandler_c::HasInstance(
      void )
{
   return( pinstance_ ? true : false );
}

//===========================================================================//
// Method         : Configure
// Description    : Configure a region place handler (singleton) based on
//                  Toro's place regions constraints.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
bool TCH_RegionPlaceHandler_c::Configure(
      const TPO_InstList_t&          toro_blockList,
      const TPO_PlaceRegionsList_t&  toro_placeRegionsList )
{
   TPO_PlaceRegionsList_t toro_blockPlaceRegionsList( toro_blockList.GetLength( ));
   for( size_t i = 0; i < toro_blockList.GetLength( ); ++i )
   {
      const TPO_Inst_c& toro_block = *toro_blockList[i];
      const char* pszName = toro_block.GetName( );
      const TGS_RegionList_t& regionList = toro_block.GetPlaceRegionList( );
      if( !regionList.IsValid( ))
         continue;

      TPO_PlaceRegions_c placeRegions;
      placeRegions.AddName( pszName );

      for( size_t j = 0; j < regionList.GetLength( ); ++j )
      {
         TGS_Region_c region = *regionList[j];

         TGS_ArrayGrid_c placeArrayGrid( 1.0, 1.0 );
         placeArrayGrid.SnapToGrid( region, &region );
         TGO_Region_c placeRegion( static_cast< int >( region.x1 ),
                                   static_cast< int >( region.y1 ),
                                   static_cast< int >( region.x2 ),
                                   static_cast< int >( region.y2 ));

         placeRegions.AddRegion( placeRegion );
      }
      toro_blockPlaceRegionsList.Add( placeRegions );
   }

   this->placeRegionsList_.Clear( );
   this->placeRegionsList_.SetCapacity( toro_blockPlaceRegionsList.GetLength( ) +
                                        toro_placeRegionsList.GetLength( ));

   this->placeRegionsList_.Add( toro_blockPlaceRegionsList );
   this->placeRegionsList_.Add( toro_placeRegionsList );

   return( this->placeRegionsList_.IsValid( ));
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
void TCH_RegionPlaceHandler_c::Set(
            t_block*         vpr_blockArray,
            int              vpr_blockCount,
      const t_logical_block* vpr_logicalBlockArray,
            int              vpr_logicalBlockCount )
{
   // Set local reference to VPR's grid array
   this->vpr_data_.Init( vpr_blockArray, vpr_blockCount,
                         vpr_logicalBlockArray, vpr_logicalBlockCount );
}

//===========================================================================//
// Method         : Reset
// Description    : Resets the region place handler in order to clear any
//                  existing placement information in VPR's grid array, the
//                  local region macro list, and local region block list.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
void TCH_RegionPlaceHandler_c::Reset(
      void )
{
   // Reset local VPR's data placement information
   this->vpr_data_.Reset( );

   // Reset placement and type information for region macro and block lists
   this->placeRegionsList_.Clear( );
}

//===========================================================================//
// Method         : UpdatePlaceRegions
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
void TCH_RegionPlaceHandler_c::UpdatePlaceRegions(
            t_block*         vpr_blockArray,
            int              vpr_blockCount,
      const t_logical_block* vpr_logicalBlockArray,
            int              vpr_logicalBlockCount )
{
   // Initialize local VPR grid array and block list structures
   this->Set( vpr_blockArray, vpr_blockCount,
              vpr_logicalBlockArray, vpr_logicalBlockCount );

   // Allocate a local list used to map hierachy names to top-level block names (and asso. block index)
   TPO_HierInstMapList_t hierInstMapList( vpr_logicalBlockCount );

   // Build a list that maps all hierarchical (packed) names to top-level block names
   // (including asso. index into VPR's block array)
   this->LoadHierInstMapList_( &hierInstMapList );

   // Update VPR's block array placement region lists based on hierarchy-to-instance mapping
   this->UpdatePlacementRegionLists_( hierInstMapList );
}

//===========================================================================//
// Method         : IsValid
// Description    : Returns TRUE if at least one placement region exists
//                  (ie. at least one placement constraint has been defined).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
bool TCH_RegionPlaceHandler_c::IsValid(
      void ) const
{
   return( this->placeRegionsList_.IsValid( ) ? true : false );
}

//===========================================================================//
// Method         : LoadHierInstMapList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
void TCH_RegionPlaceHandler_c::LoadHierInstMapList_(
            TPO_HierInstMapList_t* phierInstMapList ) const
{
   const t_block* vpr_blockArray = this->vpr_data_.vpr_blockArray;
   int vpr_blockCount = this->vpr_data_.vpr_blockCount;

   for( int blockIndex = 0; blockIndex < vpr_blockCount; ++blockIndex )
   {
      // Find and add next block's (top-level) name to hierarchy-to-instance mapping
      const t_block& vpr_block = vpr_blockArray[blockIndex];
      const char* pszBlockName = vpr_block.name;
   
      TPO_HierInstMap_c hierInstMap( pszBlockName, pszBlockName, blockIndex );
      phierInstMapList->Add( hierInstMap );

      // Apply recursion to find and add block's packed names to hierarchy-to-instance mapping
      if( vpr_block.pb )
      {
         this->LoadHierInstMapList_( *vpr_block.pb, 
                                     pszBlockName, blockIndex,
                                     phierInstMapList );
      }
   }
}

//===========================================================================//
void TCH_RegionPlaceHandler_c::LoadHierInstMapList_(
      const t_pb&                  vpr_pb,
      const char*                  pszBlockName,
            int                    blockIndex,
            TPO_HierInstMapList_t* phierInstMapList ) const
{
   if( vpr_pb.name )
   {
      if( vpr_pb.child_pbs && 
          vpr_pb.pb_graph_node && 
          vpr_pb.pb_graph_node->pb_type )
      {
         const t_mode& vpr_mode = vpr_pb.pb_graph_node->pb_type->modes[vpr_pb.mode];
         for( int typeIndex = 0; typeIndex < vpr_mode.num_pb_type_children; ++typeIndex ) 
         {
            for( int instIndex = 0; instIndex < vpr_mode.pb_type_children[typeIndex].num_pb; ++instIndex ) 
            {
               if( vpr_pb.child_pbs && 
                   vpr_pb.child_pbs[typeIndex] &&
                   vpr_pb.child_pbs[typeIndex][instIndex].name )
               {
                  const t_pb& vpr_child_pb = vpr_pb.child_pbs[typeIndex][instIndex];
                  this->LoadHierInstMapList_( vpr_child_pb, 
                                               pszBlockName, blockIndex, 
                                               phierInstMapList );
               }
            }
         }
      }
      else if( vpr_pb.logical_block != OPEN )
      {
         const t_logical_block* vpr_logicalBlockArray = this->vpr_data_.vpr_logicalBlockArray;
         const char* pszHierName = vpr_logicalBlockArray[vpr_pb.logical_block].name;

         TPO_HierInstMap_c hierInstMap( pszHierName, pszBlockName, blockIndex );
         phierInstMapList->Add( hierInstMap );
      }
   }
}

//===========================================================================//
// Method         : UpdatePlacementRegionLists_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/23/13 jeffr : Original
//===========================================================================//
void TCH_RegionPlaceHandler_c::UpdatePlacementRegionLists_(
      const TPO_HierInstMapList_t& hierInstMapList ) const
{
   // Generate a local list of all hierarchical instance names
   // (used to facilitate regular expression pattern matching)
   TPO_NameList_t hierNameList( hierInstMapList.GetLength( ));
   for( size_t i = 0; i < hierInstMapList.GetLength( ); ++i )
   {
      const TPO_HierInstMap_c& hierInstMap = *hierInstMapList[i];
      hierNameList.Add( hierInstMap.GetName( ));
   }

   // Iterate for each placement region defined in the input circuit design
   const TPO_PlaceRegionsList_t placeRegionsList = this->placeRegionsList_;
   for( size_t i = 0; i < placeRegionsList.GetLength( ); ++i )
   {
      const TPO_PlaceRegions_c& placeRegions = *placeRegionsList[i];
      const TGO_RegionList_t& regionList = placeRegions.GetRegionList( );
      const TPO_NameList_t& nameList = placeRegions.GetNameList( );

      // Iterate for each name asso. with the current placement region constraints
      // Note: The given name may be embedded within a packed block hierarchy
      // Note: The given name may be a regular expression that needs pattern matching
      for( size_t j = 0; j < nameList.GetLength( ); ++j )
      {
         const TC_Name_c& name = *nameList[j];
         const char* pszName = name.GetName( );

         TPO_NameList_t expandedNameList( hierNameList.GetLength( ));
         expandedNameList.Add( pszName );
          
         hierNameList.ApplyRegExp( &expandedNameList );
         for( size_t k = 0; k < expandedNameList.GetLength( ); ++k )
         {
            const TC_Name_c& expandedName = *expandedNameList[k];
            const char* pszExpandedName = expandedName.GetName( );
            TPO_HierInstMap_c hierInstMap( pszExpandedName );
            if( hierInstMapList.Find( hierInstMap, &hierInstMap ))
            {
               t_block* vpr_blockArray = this->vpr_data_.vpr_blockArray;
               int blockIndex = static_cast< int >( hierInstMap.GetInstIndex( ));
               t_block* pvpr_block = &vpr_blockArray[blockIndex];
               pvpr_block->placement_region_list.Add( regionList );
            }
         }
      }
   }
}
