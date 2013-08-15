//===========================================================================//
// Purpose : Declaration and inline definitions for the TCH_RoutePath class.
//
//           Inline methods include:
//           - GetRouteNodeList
//           - GetVPR_GraphToRouteMap
//           - SetLegal
//           - IsLegal
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

#ifndef TCH_ROUTE_PATH_H
#define TCH_ROUTE_PATH_H

#include <cstdio>
using namespace std;

#include "TCH_Typedefs.h"
#include "TCH_RouteNode.h"
#include "TCH_VPR_GraphToRoute.h"

#include "vpr_api.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
class TCH_RoutePath_c
{
public:

   TCH_RoutePath_c( void );
   TCH_RoutePath_c( size_t capacity );
   TCH_RoutePath_c( const TNO_Route_t& route );
   TCH_RoutePath_c( const TCH_RoutePath_c& routePath );
   ~TCH_RoutePath_c( void );

   TCH_RoutePath_c& operator=( const TCH_RoutePath_c& routePath );
   bool operator<( const TCH_RoutePath_c& routePath ) const;
   bool operator==( const TCH_RoutePath_c& routePath ) const;
   bool operator!=( const TCH_RoutePath_c& routePath ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   TCH_RouteNodeList_t* GetRouteNodeList( void );
   const TCH_VPR_GraphToRouteMap_t* GetVPR_GraphToRouteMap( void ) const;

   void SetRoute( const TNO_Route_t& route );
   void AddRouteInstPin( const TNO_Node_c& node );
   void AddRouteSegment( const TNO_Node_c& node );

   bool ValidateRouteNodeList( const t_rr_node* vpr_rrNodeArray,
                               TCH_RouteNodeList_t* prouteNodeList,
                               const char* pszNetName );

   void InitVPR_GraphToRouteMap( void );
   const TCH_RouteNode_c* FindVPR_GraphToRoute( int vpr_rrNodeIndex ) const;

   size_t FindSourceIndex( int offset = 0 ) const;
   size_t FindSinkIndex( int offset = 0 ) const;

   const char* FindSourceName( int offset = 0 ) const;
   const char* FindSinkName( int offset = 0 ) const;

   const char* FindSourceInstName( int offset = 0 ) const;
   const char* FindSinkInstName( int offset = 0 ) const;

   const char* FindSourcePortName( int offset = 0 ) const;
   const char* FindSinkPortName( int offset = 0 ) const;

   const char* FindSourcePinName( int offset = 0 ) const;
   const char* FindSinkPinName( int offset = 0 ) const;

   TCH_RouteNode_c* FindSourceNode( int offset = 0 ) const;
   TCH_RouteNode_c* FindSinkNode( int offset = 0 ) const;

   bool HasSourceNode( int offset = 0 ) const;
   bool HasSinkNode( int offset = 0 ) const;

   void SetLegal( bool isLegal );

   bool IsLegal( void ) const;
   bool IsValid( void ) const;

private:

   TCH_RouteNodeList_t routeNodeList_; // List of route nodes (ie. route path)
   TCH_VPR_GraphToRouteMap_t vpr_graphToRouteMap_; 
                                       // Defines mapping from a VPR "rr_graph"
                                       // node to the local route node list

   bool isLegal_; // TRUE => source/sink nodes have been validated w.r.t a VPR net
                  // (insures that source and sink actually exists within a VPR net)
private:

   enum TCH_DefCapacity_e 
   { 
      TCH_ROUTE_NODE_LIST_DEF_CAPACITY = 1,
      TCH_GRAPH_TO_ROUTE_MAP_DEF_CAPACITY = 1
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
inline TCH_RouteNodeList_t* TCH_RoutePath_c::GetRouteNodeList( 
      void )
{
   return( &this->routeNodeList_ );
} 

//===========================================================================//
inline const TCH_VPR_GraphToRouteMap_t* TCH_RoutePath_c::GetVPR_GraphToRouteMap( 
      void ) const
{
   return( &this->vpr_graphToRouteMap_ );
} 

//===========================================================================//
inline void TCH_RoutePath_c::SetLegal( 
      bool isLegal )
{
   this->isLegal_ = isLegal;
} 

//===========================================================================//
inline bool TCH_RoutePath_c::IsLegal( 
      void ) const
{
   return( this->isLegal_ ? true : false );
}

//===========================================================================//
inline bool TCH_RoutePath_c::IsValid( 
      void ) const
{
   return( this->HasSourceNode( ) && this->HasSinkNode( ) ? true : false );
}

#endif
