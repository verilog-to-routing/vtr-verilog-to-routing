//===========================================================================//
// Purpose : Declaration and inline definitions for a universal TCH_VPR_Data 
//           utility class.
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

#ifndef TCH_VPR_DATA_H
#define TCH_VPR_DATA_H

#include <cstdio>
using namespace std;

#include "TGO_Point.h"

#include "TCH_Typedefs.h"

#include "vpr_api.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TCH_VPR_Data_c
{
public:

   TCH_VPR_Data_c( void );
   TCH_VPR_Data_c( t_block* vpr_blockArray,
                   int vpr_blockCount,
                   const t_logical_block* vpr_logicalBlockArray,
                   int vpr_logicalBlockCount );
   TCH_VPR_Data_c( t_grid_tile** vpr_gridArray,
                   int vpr_nx,
                   int vpr_ny,
                   t_block* vpr_blockArray,
                   int vpr_blockCount,
                   const t_type_descriptor* vpr_typeArray,
                   int vpr_typeCount,
                   int* vpr_freeLocationArray,
                   t_legal_pos** vpr_legalPosArray );
   TCH_VPR_Data_c( t_grid_tile** vpr_gridArray,
                   int vpr_nx,
                   int vpr_ny,
                   const t_block* vpr_blockArray,
                   int vpr_blockCount,
                   const t_net* vpr_netArray,
                   int vpr_netCount,
                   const t_rr_node* vpr_rrNodeArray,
                   int vpr_rrNodeCount );
   TCH_VPR_Data_c( const TCH_VPR_Data_c& vpr_data );
   ~TCH_VPR_Data_c( void );

   TCH_VPR_Data_c& operator=( const TCH_VPR_Data_c& vpr_data );
   bool operator==( const TCH_VPR_Data_c& vpr_data ) const;
   bool operator!=( const TCH_VPR_Data_c& vpr_data ) const;

   void Init( t_block* vpr_blockArray,
              int vpr_blockCount,
              const t_logical_block* vpr_logicalBlockArray,
              int vpr_logicalBlockCount );
   void Init( t_grid_tile** vpr_gridArray,
              int vpr_nx,
              int vpr_ny,
              t_block* vpr_blockArray,
              int vpr_blockCount,
              const t_type_descriptor* vpr_typeArray,
              int vpr_typeCount,
              int* vpr_freeLocationArray,
              t_legal_pos** vpr_legalPosArray );
   void Init( t_grid_tile** vpr_gridArray,
              int vpr_nx,
              int vpr_ny,
              const t_block* vpr_blockArray,
              int vpr_blockCount,
              const t_net* vpr_netArray,
              int vpr_netCount,
              const t_rr_node* vpr_rrNodeArray,
              int vpr_rrNodeCount );
   void Init( t_grid_tile** vpr_gridArray,
              int vpr_nx,
              int vpr_ny,
              t_block* vpr_blockArray,
              int vpr_blockCount,
              const t_type_descriptor* vpr_typeArray,
              int vpr_typeCount,
              const t_net* vpr_netArray,
              int vpr_netCount,
              const t_rr_node* vpr_rrNodeArray,
              int vpr_rrNodeCount,
              int* vpr_freeLocationArray,
              t_legal_pos** vpr_legalPosArray );

   void Clear( void );

   void Set( const TGO_Point_c& point,
             int blockIndex,
             TCH_PlaceStatusMode_t blockStatus = TCH_PLACE_STATUS_UNDEFINED );
   void Reset( void );

   const t_pb_graph_pin* FindGraphPin( int netIndex,
                                       int pinIndex ) const;
   const t_pb_graph_pin* FindGraphPin( const t_net& vpr_net,
                                       int pinIndex ) const;
   const t_pb_graph_pin* FindGraphPin( const t_block& vpr_block,
                                       int pinIndex ) const;

   const char* FindGridPointBlockName( const TGO_Point_c& point ) const;
   int FindGridPointBlockIndex( const TGO_Point_c& point ) const;
   const t_type_descriptor* FindGridPointType( const TGO_Point_c& point ) const;

   bool IsOpenGridPoint( const TGO_Point_c& point,
                         const t_type_descriptor* vpr_type ) const;
   bool IsEmptyGridPoint( const TGO_Point_c& point ) const;
   bool IsWithinGridArray( const TGO_Point_c& point ) const;

public:

   t_grid_tile**      vpr_gridArray;        // Local pointer to VPR's grid array structure
   int                vpr_nx;               // "
   int                vpr_ny;               // "

   t_block*           vpr_blockArray;       // Local pointers to VPR's block array structure
   int                vpr_blockCount;       // "
   t_logical_block*   vpr_logicalBlockArray;// "
   int                vpr_logicalBlockCount;// "
   t_type_descriptor* vpr_typeArray;        // "
   int                vpr_typeCount;        // "

   t_net*        vpr_netArray;              // Local pointer to VPR's net array structure
   int           vpr_netCount;              // "

   t_rr_node*    vpr_rrNodeArray;           // Local pointer to VPR's "rr_graph" structure
   int           vpr_rrNodeCount;           // "

   int*          vpr_freeLocationArray;     // Local pointers to VPR's placement structures
   t_legal_pos** vpr_legalPosArray;         // "

private:

   int*          pvpr_freeLocationArray_;   // Local copies of VPR's placement structures
   t_legal_pos** pvpr_legalPosArray_;       // (used to restore original placement structures)
};

#endif
