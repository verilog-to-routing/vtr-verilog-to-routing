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
