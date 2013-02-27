//===========================================================================//
// Purpose : Declaration and inline definitions for the TCH_VPR_GraphNode 
//           class.
//
//           Inline methods include:
//           - GetVPR_NodeIndex
//           - IsValid
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

#ifndef TCH_VPR_GRAPH_NODE_H
#define TCH_VPR_GRAPH_NODE_H

#include <cstdio>
using namespace std;

#include "TGO_Point.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
class TCH_VPR_GraphNode_c
{
public:

   TCH_VPR_GraphNode_c( void );
   TCH_VPR_GraphNode_c( t_rr_type vpr_type,
                        int vpr_ptcNum,
                        const TGO_Point_c& vpr_gridPoint,
                        int vpr_rrNodeIndex = -1 );
   TCH_VPR_GraphNode_c( const TCH_VPR_GraphNode_c& vpr_graphNode );
   ~TCH_VPR_GraphNode_c( void );

   TCH_VPR_GraphNode_c& operator=( const TCH_VPR_GraphNode_c& vpr_graphNode );
   bool operator<( const TCH_VPR_GraphNode_c& vpr_graphNode ) const;
   bool operator==( const TCH_VPR_GraphNode_c& vpr_graphNode ) const;
   bool operator!=( const TCH_VPR_GraphNode_c& vpr_graphNode ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   int GetVPR_NodeIndex( void ) const;

   bool IsValid( void ) const;

private:

   class TCH_VPR_c
   {
   public:

      t_rr_type type;  // Defined by VPR's t_rr_type 
                       // (eg. SOURCE, SINK, IPIN, OPIN, CHANX, CHANY)
      int ptcNum;      // Defined by VPR's "rr_graph" pin, track, or class 
                       // number (depends on VPR's t_rr_type)
      TGO_Point_c gridPoint;
                       // Defined by VPR's grid coordinate

      int rrNodeIndex; // Index to asso. VPR "rr_graph" node
   } vpr_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
inline int TCH_VPR_GraphNode_c::GetVPR_NodeIndex(
      void ) const
{
   return( this->vpr_.rrNodeIndex );
}

//===========================================================================//
inline bool TCH_VPR_GraphNode_c::IsValid( 
      void ) const
{
   return( this->vpr_.rrNodeIndex != -1 ? true : false );
}

#endif
