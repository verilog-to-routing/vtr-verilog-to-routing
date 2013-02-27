//===========================================================================//
// Purpose : Method definitions for the TCH_VPR_Data class.
//
//           Public methods include:
//           - TCH_VPR_Data_c, ~TCH_VPR_Data_c
//           - operator=
//           - operator==, operator!=
//           - Init, Clear
//           - Set, Reset
//           - FindGraphPin
//           - FindGridPointBlockName
//           - FindGridPointBlockIndex
//           - FindGridPointType
//           - IsOpenGridPoint
//           - IsEmptyGridPoint
//           - IsWithinGridArray
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
using namespace std;

#include "TC_Typedefs.h"
#include "TC_MemoryUtils.h"

#include "TCH_VPR_Data.h"

//===========================================================================//
// Method         : TCH_VPR_Data_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_VPR_Data_c::TCH_VPR_Data_c( 
      void )
      :
      vpr_gridArray( 0 ),
      vpr_nx( 0 ),
      vpr_ny( 0 ),
      vpr_blockArray( 0 ),
      vpr_blockCount( 0 ),
      vpr_typeArray( 0 ),
      vpr_typeCount( 0 ),
      vpr_netArray( 0 ),
      vpr_netCount( 0 ),
      vpr_rrNodeArray( 0 ),
      vpr_rrNodeCount( 0 ),
      vpr_freeLocationArray( 0 ),
      vpr_legalPosArray( 0 ),
      pvpr_freeLocationArray_( 0 ),
      pvpr_legalPosArray_( 0 )
{
}

//===========================================================================//
TCH_VPR_Data_c::TCH_VPR_Data_c( 
            t_grid_tile**      vpr_gridArray_,
            int                vpr_nx_,
            int                vpr_ny_,
            t_block*           vpr_blockArray_,
            int                vpr_blockCount_,
      const t_type_descriptor* vpr_typeArray_,
            int                vpr_typeCount_,
            int*               vpr_freeLocationArray_,
            t_legal_pos**      vpr_legalPosArray_ )
      :
      vpr_gridArray( 0 ),
      vpr_nx( 0 ),
      vpr_ny( 0 ),
      vpr_blockArray( 0 ),
      vpr_blockCount( 0 ),
      vpr_typeArray( 0 ),
      vpr_typeCount( 0 ),
      vpr_netArray( 0 ),
      vpr_netCount( 0 ),
      vpr_rrNodeArray( 0 ),
      vpr_rrNodeCount( 0 ),
      vpr_freeLocationArray( 0 ),
      vpr_legalPosArray( 0 ),
      pvpr_freeLocationArray_( 0 ),
      pvpr_legalPosArray_( 0 )
{
   this->Init( vpr_gridArray_, vpr_nx_, vpr_ny_,
               vpr_blockArray_, vpr_blockCount_,
               vpr_typeArray_, vpr_typeCount_,
               vpr_freeLocationArray_, vpr_legalPosArray_ );
}

//===========================================================================//
TCH_VPR_Data_c::TCH_VPR_Data_c( 
            t_grid_tile**    vpr_gridArray_,
            int              vpr_nx_,
            int              vpr_ny_,
      const t_block*         vpr_blockArray_,
            int              vpr_blockCount_,
      const t_net*           vpr_netArray_,
            int              vpr_netCount_,
      const t_rr_node*       vpr_rrNodeArray_,
            int              vpr_rrNodeCount_ )
{
   this->Init( vpr_gridArray_, vpr_nx_, vpr_ny_,
               vpr_blockArray_, vpr_blockCount_,
               vpr_netArray_, vpr_netCount_,
               vpr_rrNodeArray_, vpr_rrNodeCount_ );
}

//===========================================================================//
TCH_VPR_Data_c::TCH_VPR_Data_c( 
      const TCH_VPR_Data_c& vpr_data )
      :
      vpr_gridArray( 0 ),
      vpr_nx( 0 ),
      vpr_ny( 0 ),
      vpr_blockArray( 0 ),
      vpr_blockCount( 0 ),
      vpr_typeArray( 0 ),
      vpr_typeCount( 0 ),
      vpr_netArray( 0 ),
      vpr_netCount( 0 ),
      vpr_rrNodeArray( 0 ),
      vpr_rrNodeCount( 0 ),
      vpr_freeLocationArray( 0 ),
      vpr_legalPosArray( 0 ),
      pvpr_freeLocationArray_( 0 ),
      pvpr_legalPosArray_( 0 )
{
   this->Init( vpr_data.vpr_gridArray, vpr_data.vpr_nx, vpr_data.vpr_ny,
               vpr_data.vpr_blockArray, vpr_data.vpr_blockCount,
               vpr_data.vpr_typeArray, vpr_data.vpr_typeCount,
               vpr_data.vpr_netArray, vpr_data.vpr_netCount,
               vpr_data.vpr_rrNodeArray, vpr_data.vpr_rrNodeCount,
               vpr_data.vpr_freeLocationArray, vpr_data.vpr_legalPosArray );
}

//===========================================================================//
// Method         : ~TCH_VPR_Data_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_VPR_Data_c::~TCH_VPR_Data_c( 
      void )
{
   this->Clear( );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_VPR_Data_c& TCH_VPR_Data_c::operator=( 
      const TCH_VPR_Data_c& vpr_data )
{
   if( &vpr_data != this )
   {
      this->Clear( );
      this->Init( vpr_data.vpr_gridArray, vpr_data.vpr_nx, vpr_data.vpr_ny,
                  vpr_data.vpr_blockArray, vpr_data.vpr_blockCount,
                  vpr_data.vpr_typeArray, vpr_data.vpr_typeCount,
                  vpr_data.vpr_netArray, vpr_data.vpr_netCount,
                  vpr_data.vpr_rrNodeArray, vpr_data.vpr_rrNodeCount,
                  vpr_data.vpr_freeLocationArray, vpr_data.vpr_legalPosArray );
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_VPR_Data_c::operator==( 
      const TCH_VPR_Data_c& vpr_data ) const
{
   return(( this->vpr_gridArray == vpr_data.vpr_gridArray ) &&
          ( this->vpr_nx == vpr_data.vpr_nx ) &&
          ( this->vpr_ny == vpr_data.vpr_ny ) &&
          ( this->vpr_blockArray == vpr_data.vpr_blockArray ) &&
          ( this->vpr_blockCount == vpr_data.vpr_blockCount ) &&
          ( this->vpr_typeArray == vpr_data.vpr_typeArray ) &&
          ( this->vpr_typeCount == vpr_data.vpr_typeCount ) &&
          ( this->vpr_netArray == vpr_data.vpr_netArray ) &&
          ( this->vpr_netCount == vpr_data.vpr_netCount ) &&
          ( this->vpr_rrNodeArray == vpr_data.vpr_rrNodeArray ) &&
          ( this->vpr_rrNodeCount == vpr_data.vpr_rrNodeCount ) &&
          ( this->vpr_freeLocationArray == vpr_data.vpr_freeLocationArray ) &&
	  ( this->vpr_legalPosArray == vpr_data.vpr_legalPosArray ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_VPR_Data_c::operator!=( 
      const TCH_VPR_Data_c& vpr_data ) const
{
   return( !this->operator==( vpr_data ) ? true : false );
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_VPR_Data_c::Init( 
            t_grid_tile**      vpr_gridArray_,
            int                vpr_nx_,
            int                vpr_ny_,
            t_block*           vpr_blockArray_,
            int                vpr_blockCount_,
      const t_type_descriptor* vpr_typeArray_,
            int                vpr_typeCount_,
            int*               vpr_freeLocationArray_,
            t_legal_pos**      vpr_legalPosArray_ )
{
   this->vpr_gridArray = vpr_gridArray_;
   this->vpr_nx = vpr_nx_;
   this->vpr_ny = vpr_ny_;

   this->vpr_blockArray = vpr_blockArray_;
   this->vpr_blockCount = vpr_blockCount_;
   this->vpr_typeArray = const_cast< t_type_descriptor* >( vpr_typeArray_ );
   this->vpr_typeCount = vpr_typeCount_;

   this->vpr_freeLocationArray = vpr_freeLocationArray_;
   this->vpr_legalPosArray = vpr_legalPosArray_;

   free( this->pvpr_freeLocationArray_ );
   this->pvpr_freeLocationArray_ = static_cast< int* >( TC_calloc( this->vpr_typeCount, sizeof(int)));

   for( int typeIndex = 0; typeIndex <= this->vpr_typeCount; ++typeIndex )
   {
      this->pvpr_freeLocationArray_[typeIndex] = this->vpr_freeLocationArray[typeIndex];
   }

   free( this->pvpr_legalPosArray_ );
   this->pvpr_legalPosArray_ = static_cast< t_legal_pos** >( TC_calloc( this->vpr_typeCount, sizeof(t_legal_pos*)));

   for( int typeIndex = 0; typeIndex < this->vpr_typeCount; ++typeIndex )
   {
      int freeCount = this->vpr_freeLocationArray[typeIndex];
      this->pvpr_legalPosArray_[typeIndex] = static_cast< t_legal_pos* >( TC_calloc( freeCount, sizeof(t_legal_pos)));

      for( int freeIndex = 0; freeIndex < this->vpr_freeLocationArray[typeIndex]; ++freeIndex )
      {
         this->pvpr_legalPosArray_[typeIndex][freeIndex].x = this->vpr_legalPosArray[typeIndex][freeIndex].x;
         this->pvpr_legalPosArray_[typeIndex][freeIndex].y = this->vpr_legalPosArray[typeIndex][freeIndex].y;
         this->pvpr_legalPosArray_[typeIndex][freeIndex].z = this->vpr_legalPosArray[typeIndex][freeIndex].z;
      }
   }
}

//===========================================================================//
void TCH_VPR_Data_c::Init(
            t_grid_tile**    vpr_gridArray_,
            int              vpr_nx_,
            int              vpr_ny_,
      const t_block*         vpr_blockArray_,
            int              vpr_blockCount_,
      const t_net*           vpr_netArray_,
            int              vpr_netCount_,
      const t_rr_node*       vpr_rrNodeArray_,
            int              vpr_rrNodeCount_ )
{
   this->vpr_gridArray = const_cast< t_grid_tile** >( vpr_gridArray_ );
   this->vpr_nx = vpr_nx_;
   this->vpr_ny = vpr_ny_;

   this->vpr_blockArray = const_cast< t_block* >( vpr_blockArray_ );
   this->vpr_blockCount = vpr_blockCount_;

   this->vpr_netArray = const_cast< t_net* >( vpr_netArray_ );
   this->vpr_netCount = vpr_netCount_;

   this->vpr_rrNodeArray = const_cast< t_rr_node* >( vpr_rrNodeArray_ );
   this->vpr_rrNodeCount = vpr_rrNodeCount_;
}

//===========================================================================//
void TCH_VPR_Data_c::Init( 
            t_grid_tile**      vpr_gridArray_,
            int                vpr_nx_,
            int                vpr_ny_,
            t_block*           vpr_blockArray_,
            int                vpr_blockCount_,
      const t_type_descriptor* vpr_typeArray_,
            int                vpr_typeCount_,
      const t_net*             vpr_netArray_,
            int                vpr_netCount_,
      const t_rr_node*         vpr_rrNodeArray_,
            int                vpr_rrNodeCount_,
            int*               vpr_freeLocationArray_,
            t_legal_pos**      vpr_legalPosArray_ )
{
   this->Init( vpr_gridArray_, vpr_nx_, vpr_ny_,
               vpr_blockArray_, vpr_blockCount_,
               vpr_typeArray_, vpr_typeCount_,
               vpr_freeLocationArray_, vpr_legalPosArray_ );
   this->Init( vpr_gridArray_, vpr_nx_, vpr_ny_,
               vpr_blockArray_, vpr_blockCount_,
               vpr_netArray_, vpr_netCount_,
               vpr_rrNodeArray_, vpr_rrNodeCount_ );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_VPR_Data_c::Clear( 
      void )
{
   for( int typeIndex = 0; typeIndex < this->vpr_typeCount; ++typeIndex )
   {
      free( this->pvpr_legalPosArray_[typeIndex] );
      this->pvpr_legalPosArray_[typeIndex] = 0;
   }
   free( this->pvpr_legalPosArray_ );
   this->pvpr_legalPosArray_ = 0;

   free( this->pvpr_freeLocationArray_ );
   this->pvpr_freeLocationArray_ = 0;

   this->vpr_legalPosArray = 0;
   this->vpr_freeLocationArray = 0;

   this->vpr_rrNodeCount = 0;
   this->vpr_rrNodeArray = 0;

   this->vpr_netCount = 0;
   this->vpr_netArray = 0;

   this->vpr_typeCount = 0;
   this->vpr_typeArray = 0;
   this->vpr_blockCount = 0;
   this->vpr_blockArray = 0;

   this->vpr_ny = 0;
   this->vpr_nx = 0;
   this->vpr_gridArray = 0;
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_VPR_Data_c::Set( 
      const TGO_Point_c&          point,
            int                   blockIndex,
            TCH_PlaceStatusMode_t blockStatus )
{
   // For reference, see place.c - initial_placement() code snippet...
   int x = point.x;
   int y = point.y;
   int z = point.z;

   this->vpr_gridArray[x][y].blocks[z] = blockIndex;
   this->vpr_gridArray[x][y].usage++;

   this->vpr_blockArray[blockIndex].x = x;
   this->vpr_blockArray[blockIndex].y = y;
   this->vpr_blockArray[blockIndex].z = z;

   this->vpr_blockArray[blockIndex].isFixed = ( blockStatus == TCH_PLACE_STATUS_FIXED ? 
                                                TRUE : FALSE );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_VPR_Data_c::Reset( 
      void )
{
   // For reference, see place.c - initial_placement() code snippet...
   for( int x = 0; x <= this->vpr_nx + 1; ++x )
   {
      for( int y = 0; y <= this->vpr_ny + 1; ++y )
      {
         if( !this->vpr_gridArray[x][y].type )
            continue;

         for( int z = 0; z < this->vpr_gridArray[x][y].type->capacity; ++z )
         {
            if( this->vpr_gridArray[x][y].blocks[z] == INVALID )
               continue;

            this->vpr_gridArray[x][y].blocks[z] = EMPTY;
         }
         this->vpr_gridArray[x][y].usage = 0;
      }
   }

   for( int blockIndex = 0; blockIndex < this->vpr_blockCount; ++blockIndex )
   {
      this->vpr_blockArray[blockIndex].x = EMPTY;
      this->vpr_blockArray[blockIndex].y = EMPTY;
      this->vpr_blockArray[blockIndex].z = EMPTY;
   }

   for( int typeIndex = 0; typeIndex <= this->vpr_typeCount; ++typeIndex )
   {
      this->vpr_freeLocationArray[typeIndex] = this->pvpr_freeLocationArray_[typeIndex];
   }

   for( int typeIndex = 0; typeIndex <= this->vpr_typeCount; ++typeIndex )
   { 
      for( int freeIndex = 0; freeIndex < this->vpr_freeLocationArray[typeIndex]; ++freeIndex )
      {
         this->vpr_legalPosArray[typeIndex][freeIndex].x = this->pvpr_legalPosArray_[typeIndex][freeIndex].x;
         this->vpr_legalPosArray[typeIndex][freeIndex].y = this->pvpr_legalPosArray_[typeIndex][freeIndex].y;
         this->vpr_legalPosArray[typeIndex][freeIndex].z = this->pvpr_legalPosArray_[typeIndex][freeIndex].z;
      }
   }
}

//===========================================================================//
// Method         : FindGraphPin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const t_pb_graph_pin* TCH_VPR_Data_c::FindGraphPin(
      int netIndex,
      int pinIndex ) const
{
   const t_net& vpr_net = this->vpr_netArray[netIndex];

   return( this->FindGraphPin( vpr_net, pinIndex ));
}

//===========================================================================//
const t_pb_graph_pin* TCH_VPR_Data_c::FindGraphPin(
      const t_net& vpr_net,
            int    nodeIndex ) const
{
   int pinIndex = vpr_net.node_block_pin[nodeIndex];
   int blockIndex = vpr_net.node_block[nodeIndex];
   const t_block& vpr_block = this->vpr_blockArray[blockIndex];

   return( this->FindGraphPin( vpr_block, pinIndex ));
}

//===========================================================================//
const t_pb_graph_pin* TCH_VPR_Data_c::FindGraphPin(
      const t_block& vpr_block,
            int      pinIndex ) const
{
   const t_pb_graph_pin* pvpr_graph_pin = 0;

   if( vpr_block.pb &&
       vpr_block.pb->pb_graph_node &&
       vpr_block.pb->pb_graph_node->pb_type )
   {
      const t_pb_graph_node& vpr_pb_graph_node = *vpr_block.pb->pb_graph_node;
      const t_pb_type& vpr_pb_type = *vpr_block.pb->pb_graph_node->pb_type;

      // VPR: If this is post-placed, then the pin index may have been 
      //      shuffled up by the z * num_pins, bring it back down to 
      //      0..num_pins-1 range for easier analysis

      pinIndex %= ( vpr_pb_type.num_input_pins + 
                    vpr_pb_type.num_output_pins + 
                    vpr_pb_type.num_clock_pins );
		
      if( pinIndex < vpr_pb_type.num_input_pins ) 
      {
         for( int portIndex = 0; portIndex < vpr_pb_graph_node.num_input_ports; ++portIndex ) 
         {
            if( pinIndex - vpr_pb_graph_node.num_input_pins[portIndex] >= 0 ) 
            {
               pinIndex -= vpr_pb_graph_node.num_input_pins[portIndex];
            }
            else
            {
               pvpr_graph_pin = &vpr_pb_graph_node.input_pins[portIndex][pinIndex];
	       break;
            }
         }
      } 
      else if( pinIndex < ( vpr_pb_type.num_input_pins + vpr_pb_type.num_output_pins ))
      {
	 pinIndex -= vpr_pb_type.num_input_pins;
         for( int portIndex = 0; portIndex < vpr_pb_graph_node.num_output_ports; ++portIndex )
         {
            if( pinIndex - vpr_pb_graph_node.num_output_pins[portIndex] >= 0 )
            {
               pinIndex -= vpr_pb_graph_node.num_output_pins[portIndex];
            }
            else
            {
               pvpr_graph_pin = &vpr_pb_graph_node.output_pins[portIndex][pinIndex];
	       break;
            }
         }
      } 
      else 
      {
	 pinIndex -= ( vpr_pb_type.num_input_pins + vpr_pb_type.num_output_pins );
         for( int portIndex = 0; portIndex < vpr_pb_graph_node.num_clock_ports; ++portIndex )
         {
            if( pinIndex - vpr_pb_graph_node.num_clock_pins[portIndex] >= 0 )
            {
               pinIndex -= vpr_pb_graph_node.num_clock_pins[portIndex];
            }
            else
            {
               pvpr_graph_pin = &vpr_pb_graph_node.clock_pins[portIndex][pinIndex];
	       break;
            }
         }
      }
   }
   return( pvpr_graph_pin );
}

//===========================================================================//
// Method         : FindGridPointBlockName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
const char* TCH_VPR_Data_c::FindGridPointBlockName(
      const TGO_Point_c& point ) const
{
   const char* pszBlockName = 0;

   int blockIndex = this->FindGridPointBlockIndex( point );
   if(( blockIndex != EMPTY ) && ( blockIndex != INVALID ))
   {
      pszBlockName = this->vpr_blockArray[blockIndex].name;
   }
   return( pszBlockName );
}

//===========================================================================//
// Method         : FindGridPointBlockIndex
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
int TCH_VPR_Data_c::FindGridPointBlockIndex(
      const TGO_Point_c& point ) const
{
   int blockIndex = EMPTY;

   if( this->IsWithinGridArray( point ))
   {
      int x = point.x;
      int y = point.y;
      int z = point.z;
      blockIndex = this->vpr_gridArray[x][y].blocks[z];
   }
   return( blockIndex );
}

//===========================================================================//
// Method         : FindGridPointType
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
const t_type_descriptor* TCH_VPR_Data_c::FindGridPointType(
      const TGO_Point_c& point ) const
{
   const t_type_descriptor* vpr_type = 0;

   if( this->IsWithinGridArray( point ))
   {
      int x = point.x;
      int y = point.y;
      vpr_type = this->vpr_gridArray[x][y].type;
   }
   return( vpr_type );
}

//===========================================================================//
// Method         : IsOpenGridPoint
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_VPR_Data_c::IsOpenGridPoint(
      const TGO_Point_c&       point,
      const t_type_descriptor* vpr_type ) const
{ 
   bool isOpen = false;

   if(( this->IsEmptyGridPoint( point )) &&
      ( this->FindGridPointType( point ) == vpr_type ))
   {
      isOpen = true;
   }
   return( isOpen );
}

//===========================================================================//
// Method         : IsEmptyGridPoint
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_VPR_Data_c::IsEmptyGridPoint(
      const TGO_Point_c& point ) const
{
   bool isEmpty = false;

   if( this->IsWithinGridArray( point ))
   {
      int x = point.x;
      int y = point.y;
      int z = point.z;
      isEmpty = ( this->vpr_gridArray[x][y].blocks[z] == EMPTY ?
                  true : false );
   }
   return( isEmpty );
}

//===========================================================================//
// Method         : IsWithinGridArray
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_VPR_Data_c::IsWithinGridArray(
      const TGO_Point_c& point ) const
{
   bool isWithin = (( point.x >= 0 ) && ( point.x <= ( this->vpr_nx + 1 )) &&
                    ( point.y >= 0 ) && ( point.y <= ( this->vpr_ny + 1 )) ?
                    true : false );
   return( isWithin );
}
