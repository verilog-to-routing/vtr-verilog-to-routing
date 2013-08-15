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
