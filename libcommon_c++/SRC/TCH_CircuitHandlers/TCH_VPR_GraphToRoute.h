//===========================================================================//
// Purpose : Declaration and inline definitions for the TCH_VPR_GraphToRoute 
//           class.
//
//           Inline methods include:
//           - GetVPR_NodeIndex
//           - GetRouteNodeIndex
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

#ifndef TCH_VPR_GRAPH_TO_ROUTE_H
#define TCH_VPR_GRAPH_TO_ROUTE_H

#include <cstdio>
using namespace std;

#include "TCH_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
class TCH_VPR_GraphToRoute_c
{
public:

   TCH_VPR_GraphToRoute_c( void );
   TCH_VPR_GraphToRoute_c( int vpr_rrNodeIndex,
                           size_t routeNodeIndex = TCH_ROUTE_NODE_INDEX_INVALID );
   TCH_VPR_GraphToRoute_c( const TCH_VPR_GraphToRoute_c& vpr_graphToRoute );
   ~TCH_VPR_GraphToRoute_c( void );

   TCH_VPR_GraphToRoute_c& operator=( const TCH_VPR_GraphToRoute_c& vpr_graphToRoute );
   bool operator<( const TCH_VPR_GraphToRoute_c& vpr_graphToRoute ) const;
   bool operator==( const TCH_VPR_GraphToRoute_c& vpr_graphToRoute ) const;
   bool operator!=( const TCH_VPR_GraphToRoute_c& vpr_graphToRoute ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   int GetVPR_NodeIndex( void ) const;
   size_t GetRouteNodeIndex( void ) const;

   bool IsValid( void ) const;

private:

   class TCH_VPR_c
   {
   public:

      int rrNodeIndex;     // Defines VPR "rr_graph" node asso. with this node
   } vpr_;

   size_t routeNodeIndex_; // Defines mapping from a VPR "rr_graph" node to a 
                           // local route path node index
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
inline int TCH_VPR_GraphToRoute_c::GetVPR_NodeIndex(
      void ) const
{
   return( this->vpr_.rrNodeIndex );
}

//===========================================================================//
inline size_t TCH_VPR_GraphToRoute_c::GetRouteNodeIndex(
      void ) const
{
   return( this->routeNodeIndex_ );
}

//===========================================================================//
inline bool TCH_VPR_GraphToRoute_c::IsValid( 
      void ) const
{
   return( this->vpr_.rrNodeIndex != -1 ? true : false );
}

#endif
