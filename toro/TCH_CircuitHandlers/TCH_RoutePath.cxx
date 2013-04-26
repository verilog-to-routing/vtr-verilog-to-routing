//===========================================================================//
// Purpose : Method definitions for the TCH_RoutePath class.
//
//           Public methods include:
//           - TCH_RoutePath_c, ~TCH_RoutePath_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - SetRoute
//           - AddRouteInstPin
//           - AddRouteSegment
//           - ValidateRouteNodeList
//           - InitVPR_GraphToRouteMap
//           - FindVPR_GraphToRoute
//           - FindSourceIndex, FindSinkIndex
//           - FindSourceName, FindSinkName
//           - FindSourceInstName, FindSinkInstName
//           - FindSourcePortName, FindSinkPortName
//           - FindSourcePinName, FindSinkPinName
//           - FindSourceNode, FindSinkNode
//           - HasSourceNode, HasSinkNode
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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TCH_RoutePath.h"

//===========================================================================//
// Method         : TCH_RoutePath_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_RoutePath_c::TCH_RoutePath_c( 
      void )
      :
      routeNodeList_( TCH_ROUTE_NODE_LIST_DEF_CAPACITY ),
      vpr_graphToRouteMap_( TCH_GRAPH_TO_ROUTE_MAP_DEF_CAPACITY ),
      isLegal_( false )
{
} 

//===========================================================================//
TCH_RoutePath_c::TCH_RoutePath_c( 
      size_t capacity )
      :
      routeNodeList_( capacity ),
      vpr_graphToRouteMap_( TCH_GRAPH_TO_ROUTE_MAP_DEF_CAPACITY ),
      isLegal_( false )
{
} 

//===========================================================================//
TCH_RoutePath_c::TCH_RoutePath_c( 
      const TNO_Route_t& route )
      :
      routeNodeList_( TCH_ROUTE_NODE_LIST_DEF_CAPACITY ),
      vpr_graphToRouteMap_( TCH_GRAPH_TO_ROUTE_MAP_DEF_CAPACITY ),
      isLegal_( false )
{
   this->SetRoute( route );
} 

//===========================================================================//
TCH_RoutePath_c::TCH_RoutePath_c( 
      const TCH_RoutePath_c& routePath )
      :
      routeNodeList_( routePath.routeNodeList_ ),
      vpr_graphToRouteMap_( routePath.vpr_graphToRouteMap_ ),
      isLegal_( routePath.isLegal_ )
{
} 

//===========================================================================//
// Method         : ~TCH_RoutePath_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_RoutePath_c::~TCH_RoutePath_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_RoutePath_c& TCH_RoutePath_c::operator=( 
      const TCH_RoutePath_c& routePath )
{
   if( &routePath != this )
   {
      this->routeNodeList_ = routePath.routeNodeList_;
      this->vpr_graphToRouteMap_ = routePath.vpr_graphToRouteMap_;
      this->isLegal_ = routePath.isLegal_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_RoutePath_c::operator<( 
      const TCH_RoutePath_c& routePath ) const
{
   bool isLessThan = false;

   if( this->HasSourceNode( ) && this->HasSinkNode( ) &&
       routePath.HasSourceNode( ) && routePath.HasSinkNode( ))
   {
      const char* pszThisSourceName = this->FindSourceName( );
      const char* pszThisSinkName = this->FindSinkName( );

      const char* pszRoutePathSourceName = routePath.FindSourceName( );
      const char* pszRoutePathSinkName = routePath.FindSinkName( );

      if( TC_CompareStrings( pszThisSourceName, pszRoutePathSourceName ) < 0 )
      {
         isLessThan = true;
      }
      else if(( TC_CompareStrings( pszThisSourceName, pszRoutePathSourceName ) == 0 ) &&
              ( TC_CompareStrings( pszThisSinkName, pszRoutePathSinkName ) < 0 ))
      {
         isLessThan = true;
      }
   }
   return( isLessThan );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_RoutePath_c::operator==( 
      const TCH_RoutePath_c& routePath ) const
{
   bool isEqual = false;

   if( this->HasSourceNode( ) && this->HasSinkNode( ) &&
       routePath.HasSourceNode( ) && routePath.HasSinkNode( ))
   {
      const TCH_RouteNode_c& thisSourceNode = *this->FindSourceNode( );
      const TCH_RouteNode_c& thisSinkNode = *this->FindSinkNode( );

      const TCH_RouteNode_c& routePathSourceNode = *routePath.FindSourceNode( );
      const TCH_RouteNode_c& routePathSinkNode = *routePath.FindSinkNode( );

      if(( thisSourceNode == routePathSourceNode ) &&
         ( thisSinkNode == routePathSinkNode ))
      {
         isEqual = true;
      }
   }
   return( isEqual );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_RoutePath_c::operator!=( 
      const TCH_RoutePath_c& routePath ) const
{
   return( !this->operator==( routePath ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_RoutePath_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<routePath" );
   printHandler.Write( pfile, 0, " isLegal=%s", TIO_BOOL_VAL( this->isLegal_ ));
   printHandler.Write( pfile, 0, " routeNodeList=[%d]", this->routeNodeList_.GetLength( ));
   printHandler.Write( pfile, 0, " <vpr" );
   printHandler.Write( pfile, 0, " graphToRouteMap=[%d]", this->vpr_graphToRouteMap_.GetLength( ));
   printHandler.Write( pfile, 0, ">" );
   printHandler.Write( pfile, 0, ">\n" );

   this->routeNodeList_.Print( pfile, spaceLen + 3 );
   this->vpr_graphToRouteMap_.Print( pfile, spaceLen + 3 );
} 

//===========================================================================//
// Method         : SetRoute
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_RoutePath_c::SetRoute(
      const TNO_Route_t& route )
{
   this->routeNodeList_.Clear( );
   this->routeNodeList_.SetCapacity( route.GetLength( ));

   if( route.GetLength( ) > 1 )
   {
      for( size_t i = 1; i < route.GetLength( ); ++i )
      {
         const TNO_Node_c& priorNode = *route[i-1];
         const TNO_Node_c& thisNode = *route[i];

         if( priorNode.IsInstPin( ) && thisNode.IsSegment( ))
         {
            this->AddRouteInstPin( priorNode );
            this->AddRouteSegment( thisNode );
         }
         else if( priorNode.IsSegment( ) && thisNode.IsInstPin( ))
         {
            this->AddRouteInstPin( thisNode );
         }
         else if( thisNode.IsSegment( ))
         {
            this->AddRouteSegment( thisNode );
         }
      }
   }
}

//===========================================================================//
// Method         : AddRouteInstPin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_RoutePath_c::AddRouteInstPin(
      const TNO_Node_c& node )
{
   if( node.IsInstPin( ))
   {
      size_t pinIndex = node.GetInstPin( ).GetPinIndex( );

      size_t len = this->routeNodeList_.GetLength( );
      if( len == 0 )
      {
         TCH_RouteNode_c* psrcNode = this->routeNodeList_.Add( node );
         TCH_RouteNode_c* popinNode = this->routeNodeList_.Add( node );

         psrcNode->SetVPR_Type( SOURCE );
         psrcNode->SetVPR_Pin( static_cast< int >( pinIndex ));

         popinNode->SetVPR_Type( OPIN );
         popinNode->SetVPR_Pin( static_cast< int >( pinIndex ));
      }
      else
      {
         TCH_RouteNode_c* pipinNode = this->routeNodeList_.Add( node );
         TCH_RouteNode_c* psinkNode = this->routeNodeList_.Add( node );

         pipinNode->SetVPR_Type( IPIN );
         pipinNode->SetVPR_Pin( static_cast< int >( pinIndex ));

         psinkNode->SetVPR_Type( SINK );
         psinkNode->SetVPR_Pin( static_cast< int >( pinIndex ));
      }
   }
}

//===========================================================================//
// Method         : AddRouteSegment
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_RoutePath_c::AddRouteSegment(
      const TNO_Node_c& node )
{
   if( node.IsSegment( ))
   {
      TCH_RouteNode_c* pchanNode = this->routeNodeList_.Add( node );

      TGS_OrientMode_t orient = node.GetSegment( ).GetOrient( );
      t_rr_type vpr_type = ( orient == TGS_ORIENT_HORIZONTAL ?
                             CHANX : CHANY );

      const TGS_Region_c& channel = node.GetSegment( ).GetChannel( );
      TGO_Region_c vpr_channel( static_cast< int >( channel.x1 ),
                                static_cast< int >( channel.y1 ),
                                static_cast< int >( channel.x2 ),
                                static_cast< int >( channel.y2 ));

      pchanNode->SetVPR_Type( vpr_type );
      pchanNode->SetVPR_Track( node.GetSegment( ).GetTrack( ));
      pchanNode->SetVPR_Channel( vpr_channel );
   }
}

//===========================================================================//
// Method         : ValidateRouteNodeList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_RoutePath_c::ValidateRouteNodeList(
      const t_rr_node*           vpr_rrNodeArray,
            TCH_RouteNodeList_t* prouteNodeList,
      const char*                pszNetName )
{
   bool ok = true;

   // Iterate this route path's list of route nodes to validate that all
   // from/to nodes are defined 'consecutive' w.r.t. VPR's "rr_graph"
   for( size_t i = 0; i < prouteNodeList->GetLength( )-1; ++i )
   {
      TCH_RouteNode_c* pfromRouteNode = (*prouteNodeList)[i];
      TCH_RouteNode_c* ptoRouteNode = (*prouteNodeList)[i+1];

      // Find VPR's "rr_graph" node based on route node's "rr_node" index
      int vpr_rrFromIndex = pfromRouteNode->GetVPR_NodeIndex( );
      if( vpr_rrFromIndex == -1 )
         continue;

      int vpr_rrToIndex = ptoRouteNode->GetVPR_NodeIndex( );

      // Validate from/to nodes are cosecutive nodes w.r.t. VPR's "rr_graph"
      bool validFromToNode = false;
      const t_rr_node& vpr_rrFromNode = vpr_rrNodeArray[vpr_rrFromIndex];
      int vpr_edgeCount = vpr_rrFromNode.num_edges;
      for( int vpr_edgeIndex = 0; vpr_edgeIndex < vpr_edgeCount; ++vpr_edgeIndex )
      {
         int vpr_rrChildIndex = vpr_rrFromNode.edges[vpr_edgeIndex];
         if( vpr_rrChildIndex == vpr_rrToIndex )
         {
            validFromToNode = true;
            break;
         }
      }

      if( !validFromToNode )
      {
         if( pfromRouteNode->GetVPR_Type( ) == IPIN )
         {
            validFromToNode = true;
         }
      }

      if( validFromToNode )
      {
         pfromRouteNode->SetLegal( true );
         if( ptoRouteNode->GetVPR_Type( ) == SINK )
         {
            ptoRouteNode->SetLegal( true );
         }
      }
      else
      {
         const char* pszSrcInstPin = this->FindSourceName( );
         const char* pszSinkInstPin = this->FindSinkName( );

         TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
         ok = printHandler.Warning( "Ignoring pre-route for net \"%s\" from source \"%s\" to sink \"%s\".\n",
                                    TIO_PSZ_STR( pszNetName ),
                                    TIO_PSZ_STR( pszSrcInstPin ),
                                    TIO_PSZ_STR( pszSinkInstPin ));
         printHandler.Info( "%sRoute path is not legal based on VPR's \"rr_graph\" (from %d to %d).\n",
                            TIO_PREFIX_WARNING_SPACE,
                            vpr_rrFromIndex, vpr_rrToIndex );

         this->SetLegal( false );
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : InitVPR_GraphToRouteMap
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_RoutePath_c::InitVPR_GraphToRouteMap(
      void )
{
   this->vpr_graphToRouteMap_.Clear( );
   this->vpr_graphToRouteMap_.SetCapacity( this->routeNodeList_.GetLength( ));

   for( size_t i = 0; i < this->routeNodeList_.GetLength( ); ++i )
   {
      const TCH_RouteNode_c& routeNode = *this->routeNodeList_[i];

      int vpr_rrNodeIndex = routeNode.GetVPR_NodeIndex( );
      if( vpr_rrNodeIndex == -1 )
         continue;

      TCH_VPR_GraphToRoute_c vpr_graphToRoute( vpr_rrNodeIndex, i );
      this->vpr_graphToRouteMap_.Add( vpr_graphToRoute );
   }
}

//===========================================================================//
// Method         : FindVPR_GraphToRoute
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const TCH_RouteNode_c* TCH_RoutePath_c::FindVPR_GraphToRoute(
      int vpr_rrNodeIndex ) const
{
   const TCH_RouteNode_c* prouteNode = 0;

   // Decide VPR "rr_graph" node membership based on local graph->route mapping
   TCH_VPR_GraphToRoute_c vpr_graphToRoute( vpr_rrNodeIndex );
   const TCH_VPR_GraphToRoute_c* pvpr_graphToRoute = 0;
   pvpr_graphToRoute = this->vpr_graphToRouteMap_[vpr_graphToRoute];
   if( pvpr_graphToRoute )
   {
      size_t routeNodeIndex = pvpr_graphToRoute->GetRouteNodeIndex( );
      prouteNode = this->routeNodeList_[routeNodeIndex];
   }
   return( prouteNode );
}

//===========================================================================//
// Method         : FindSourceIndex
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
size_t TCH_RoutePath_c::FindSourceIndex( 
      int offset ) const
{
   size_t sourceIndex = TCH_ROUTE_NODE_INDEX_INVALID;
   
   if( this->HasSourceNode( offset ))
   {
      sourceIndex = 0 + offset;
   }
   return( sourceIndex );
}

//===========================================================================//
// Method         : FindSinkIndex
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
size_t TCH_RoutePath_c::FindSinkIndex( 
      int offset ) const
{
   size_t sinkIndex = TCH_ROUTE_NODE_INDEX_INVALID;
   
   if( this->HasSinkNode( offset ))
   {
      size_t i = this->routeNodeList_.GetLength( );
      sinkIndex = i - 1 + offset;
   }
   return( sinkIndex );
}

//===========================================================================//
// Method         : FindSourceName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const char* TCH_RoutePath_c::FindSourceName( 
      int offset ) const
{
   const TCH_RouteNode_c* psourceNode = this->FindSourceNode( offset );
   return( psourceNode ? psourceNode->GetName( ) : 0 );
}

//===========================================================================//
// Method         : FindSinkName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const char* TCH_RoutePath_c::FindSinkName( 
      int offset ) const
{
   const TCH_RouteNode_c* psinkNode = this->FindSinkNode( offset );
   return( psinkNode ? psinkNode->GetName( ) : 0 );
}

//===========================================================================//
// Method         : FindSourceInstName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const char* TCH_RoutePath_c::FindSourceInstName( 
      int offset ) const
{
   const TCH_RouteNode_c* psourceNode = this->FindSourceNode( offset );
   return( psourceNode ? psourceNode->GetInstName( ) : 0 );
}

//===========================================================================//
// Method         : FindSinkInstName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const char* TCH_RoutePath_c::FindSinkInstName( 
      int offset ) const
{
   const TCH_RouteNode_c* psinkNode = this->FindSinkNode( offset );
   return( psinkNode ? psinkNode->GetInstName( ) : 0 );
}

//===========================================================================//
// Method         : FindSourcePortName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const char* TCH_RoutePath_c::FindSourcePortName( 
      int offset ) const
{
   const TCH_RouteNode_c* psourceNode = this->FindSourceNode( offset );
   return( psourceNode ? psourceNode->GetPortName( ) : 0 );
}

//===========================================================================//
// Method         : FindSinkPortName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const char* TCH_RoutePath_c::FindSinkPortName( 
      int offset ) const
{
   const TCH_RouteNode_c* psinkNode = this->FindSinkNode( offset );
   return( psinkNode ? psinkNode->GetPortName( ) : 0 );
}

//===========================================================================//
// Method         : FindSourcePinName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const char* TCH_RoutePath_c::FindSourcePinName( 
      int offset ) const
{
   const TCH_RouteNode_c* psourceNode = this->FindSourceNode( offset );
   return( psourceNode ? psourceNode->GetPinName( ) : 0 );
}

//===========================================================================//
// Method         : FindSinkPinName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const char* TCH_RoutePath_c::FindSinkPinName( 
      int offset ) const
{
   const TCH_RouteNode_c* psinkNode = this->FindSinkNode( offset );
   return( psinkNode ? psinkNode->GetPinName( ) : 0 );
}

//===========================================================================//
// Method         : FindSourceNode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_RouteNode_c* TCH_RoutePath_c::FindSourceNode( 
      int offset ) const
{
   size_t sourceIndex = this->FindSourceIndex( offset );
   return( sourceIndex != TCH_ROUTE_NODE_INDEX_INVALID ?
           this->routeNodeList_[sourceIndex] : 0 );
}

//===========================================================================//
// Method         : FindSinkNode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_RouteNode_c* TCH_RoutePath_c::FindSinkNode( 
      int offset ) const
{
   size_t sinkIndex = this->FindSinkIndex( offset );
   return( sinkIndex != TCH_ROUTE_NODE_INDEX_INVALID ?
           this->routeNodeList_[sinkIndex] : 0 );
}

//===========================================================================//
// Method         : HasSourceNode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_RoutePath_c::HasSourceNode( 
      int offset ) const
{
   bool hasSourceNode = false;

   if( this->routeNodeList_.GetLength( ) >= static_cast< size_t >( 1 + offset ))
   {
      const TCH_RouteNode_c& sourceNode = *this->routeNodeList_[0 + offset];
      if( sourceNode.GetType( ) == TNO_NODE_INST_PIN )
      {
         hasSourceNode = true;
      }
   }
   return( hasSourceNode );
}

//===========================================================================//
// Method         : HasSinkNode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_RoutePath_c::HasSinkNode( 
      int offset ) const
{
   bool hasSinkNode = false;

   if( this->routeNodeList_.GetLength( ) >= static_cast< size_t >( 2 + offset ))
   {
      size_t i = this->routeNodeList_.GetLength( );
      const TCH_RouteNode_c& sinkNode = *this->routeNodeList_[i - 1 + offset];
      if( sinkNode.GetType( ) == TNO_NODE_INST_PIN )
      {
         hasSinkNode = true;
      }
   }
   return( hasSinkNode );
}
