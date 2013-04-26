//===========================================================================//
// Purpose : Method definitions for the TCH_PreRoutedHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - Configure
//           - Set
//           - GetOrderMode
//           - GetNetNameList
//           - GetNetList
//           - ValidatePreRoutes
//           - IsMemberPreRouteNet
//           - IsMemberPreRoutePath
//           - IsValid
//
//           Protected methods include:
//           - TCH_PreRoutedHandler_c, ~TCH_PreRoutedHandler_c
//
//           Private methods include:
//           - InitNetList_
//           - LoadNetList_
//           - AddNetRoutePath_
//           - ValidateNetList_
//           - ValidateNetListInstPins_
//           - ValidateNetListRoutePaths_
//           - ValidateNetInstPins_
//           - ValidateNetRoutePathInstPins_
//           - UpdateNetListInstPins_
//           - UpdateNetListInstPoint_
//           - UpdateNetListRoutePaths_
//           - ExistsRoutePathInstPin_
//           - LoadVPR_GraphNodeList_
//           - FindVPR_GraphNode_
//           - BuildInstList_
//           - FindNet_
//           - FindNetRoutePath_
//           - ShowDuplicateRoutePathWarning_
//           - ShowNetNotFoundWarning_
//           - ShowPinNotFoundWarning_
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
#include <string>
using namespace std;

#include "TIO_PrintHandler.h"

#include "TCH_PreRoutedHandler.h"

// Initialize the pre-routed handler "singleton" class, as needed
TCH_PreRoutedHandler_c* TCH_PreRoutedHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TCH_PreRoutedHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_PreRoutedHandler_c::TCH_PreRoutedHandler_c( 
      void )
      :
      orderMode_( TCH_ROUTE_ORDER_UNDEFINED ),
      netNameList_( TCH_PREROUTED_NET_NAME_LIST_DEF_CAPACITY ),
      netList_( TCH_PREROUTED_NET_LIST_DEF_CAPACITY ),
      cachedNetName_( "" ),
      cachedNetIndex_( TNO_NET_INDEX_INVALID )
{
   pinstance_ = 0;
}

//===========================================================================//
// Method         : ~TCH_PreRoutedHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_PreRoutedHandler_c::~TCH_PreRoutedHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_PreRoutedHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TCH_PreRoutedHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_PreRoutedHandler_c::DeleteInstance( 
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
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_PreRoutedHandler_c& TCH_PreRoutedHandler_c::GetInstance(
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
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::HasInstance(
      void )
{
   return( pinstance_ ? true : false );
}

//===========================================================================//
// Method         : Configure
// Description    : Configure a pre-routed handler (singleton) based on Toro's
//                  net list and any associated pre-routed options.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::Configure(
      const TNO_NetList_c&       netList, // Defined by Toro's net list
      const TNO_NameList_t&      netOrderList,
            TCH_RouteOrderMode_t orderMode ) 
{
   bool ok = true;

   this->orderMode_ = orderMode;

   // Initialize local net name and net lists based on user-defined net list
   this->InitNetList_( netList, netOrderList,
                       &this->netNameList_, &this->netList_ );

   // Load local net list based on user-defined net list route paths
   ok = this->LoadNetList_( netList );

   return( ok );
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_PreRoutedHandler_c::Set(
            t_grid_tile**    vpr_gridArray,
            int              vpr_nx,
            int              vpr_ny,
      const t_block*         vpr_blockArray,
            int              vpr_blockCount,
      const t_net*           vpr_netArray,
            int              vpr_netCount,
      const t_rr_node*       vpr_rrNodeArray,
            int              vpr_rrNodeCount )
{
   // Set local reference to VPR's net list and asso. global data structures
   this->vpr_data_.Init( vpr_gridArray, vpr_nx, vpr_ny,
                         vpr_blockArray, vpr_blockCount,
                         vpr_netArray, vpr_netCount,
                         vpr_rrNodeArray, vpr_rrNodeCount );
}

//===========================================================================//
// Method         : GetOrderMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_RouteOrderMode_t TCH_PreRoutedHandler_c::GetOrderMode( 
      void ) const
{
   return( this->orderMode_ );
}

//===========================================================================//
// Method         : GetNetNameList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const TCH_NetNameList_t& TCH_PreRoutedHandler_c::GetNetNameList( 
      void ) const
{
   return( this->netNameList_ );
}

//===========================================================================//
// Method         : GetNetList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const TCH_NetList_t& TCH_PreRoutedHandler_c::GetNetList( 
      void ) const
{
   return( this->netList_ );
}

//===========================================================================//
// Method         : ValidatePreRoutes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::ValidatePreRoutes(
      void )
{
   bool ok = true;

   // Validate that given net name list is consistent with VPR's net list
   if( ok )
   {
      ok = this->ValidateNetList_( this->vpr_data_.vpr_netArray, 
                                   this->vpr_data_.vpr_netCount, 
                                   this->netNameList_, &this->netList_ );
   }

   // Validate that given net source/sink pins are consistent with VPR's net list
   if( ok )
   {
      ok = this->ValidateNetListInstPins_( this->vpr_data_.vpr_blockArray, 
                                           this->vpr_data_.vpr_netArray, 
                                           this->vpr_data_.vpr_netCount,
                                           &this->netList_ );
   }

   // Update given net source/sink pins based on VPR's current placement
   if( ok )
   {
      this->UpdateNetListInstPins_( this->vpr_data_.vpr_blockArray, 
                                    this->vpr_data_.vpr_blockCount,
                                    &this->netList_ );
   }

   // Update given net route paths based on VPR's current "rr_graph"
   if( ok )
   {
      this->UpdateNetListRoutePaths_( this->vpr_data_.vpr_rrNodeArray, 
                                      this->vpr_data_.vpr_rrNodeCount,
                                      this->vpr_data_.vpr_gridArray,
                                      &this->netList_ );
   }

   // Validate that given net route paths are consistent with VPR's "rr_graph"
   if( ok )
   {
      ok = this->ValidateNetListRoutePaths_( this->vpr_data_.vpr_rrNodeArray,
                                             &this->netList_ );
   }
   return( ok );
}

//===========================================================================//
// Method         : IsMemberPreRouteNet
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::IsMemberPreRouteNet(
      const char* pszNetName,
            int   vpr_rrSrcIndex,
            int   vpr_rrSinkIndex ) const
{
   bool isMember = false;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Trace( "PreRoutedHandler::IsMemberPreRouteNet: net \"%s\", src-sink %d-%d\n",
                       TIO_PSZ_STR( pszNetName ), 
                       vpr_rrSrcIndex, vpr_rrSinkIndex );

   const TCH_Net_c* pnet = this->FindNet_( pszNetName );
   if( pnet && pnet->IsLegal( ))
   {
      // Find net's route path asso. with given source/sink "rr_graph" node indices
      const TCH_RoutePath_c* proutePath = 0;
      proutePath = this->FindNetRoutePath_( *pnet, 
                                            vpr_rrSrcIndex, vpr_rrSinkIndex );
      if( proutePath && proutePath->IsLegal( ))
      {
         isMember = true;
      }
   }
   return( isMember );
}

//===========================================================================//
// Method         : IsMemberPreRoutePath
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::IsMemberPreRoutePath(
      const char* pszNetName,
            int   vpr_rrSrcIndex,
            int   vpr_rrSinkIndex,
            int   vpr_rrFromIndex,
            int   vpr_rrToIndex ) const
{
   bool isMember = false;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Trace( "PreRoutedHandler::IsMemberPreRoutePath: net \"%s\" src-sink %d-%d from-to %d-%d\n",
                       TIO_PSZ_STR( pszNetName ), 
                       vpr_rrSrcIndex, vpr_rrSinkIndex,
                       vpr_rrFromIndex, vpr_rrToIndex );

   const TCH_Net_c* pnet = this->FindNet_( pszNetName );
   if( pnet && pnet->IsLegal( ))
   {
      // Find net's route path asso. with given source/sink "rr_graph" node indices
      const TCH_RoutePath_c* proutePath = 0;
      proutePath = this->FindNetRoutePath_( *pnet, 
                                            vpr_rrSrcIndex, vpr_rrSinkIndex );
      if( proutePath && proutePath->IsLegal( ))
      {
         isMember = this->IsMemberPreRoutePath( *proutePath,
                                                vpr_rrFromIndex, vpr_rrToIndex );
      }
   }
   return( isMember );
}

//===========================================================================//
bool TCH_PreRoutedHandler_c::IsMemberPreRoutePath(
      const TCH_RoutePath_c& routePath,
            int              vpr_rrFromIndex,
            int              vpr_rrToIndex ) const
{
   bool isMember = false;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Trace( "PreRoutedHandler::IsMemberPreRoutePath: from-to %d-%d\n",
                       vpr_rrFromIndex, vpr_rrToIndex );

   // Decide VPR "rr_graph" node membership based on local graph->route mapping
   const TCH_RouteNode_c* pfromRouteNode = 0;
   pfromRouteNode = routePath.FindVPR_GraphToRoute( vpr_rrFromIndex );

   // Find 'to' "rr_graph" node membership based on graphToRouteMap lookup
   const TCH_RouteNode_c* ptoRouteNode = 0;
   ptoRouteNode = routePath.FindVPR_GraphToRoute( vpr_rrToIndex );

   // Finally, test 'from' and 'to' are consecutive nodes in VPR's "rr_graph"
   if(( pfromRouteNode && pfromRouteNode->IsLegal( )) &&
      ( ptoRouteNode && ptoRouteNode->IsLegal( )))
   {
      const t_rr_node* vpr_rrNodeArray = this->vpr_data_.vpr_rrNodeArray;
      const t_rr_node& vpr_rrFromNode = vpr_rrNodeArray[vpr_rrFromIndex];

      // Iterate 'from' node edges, searching for 'to' node membership
      int vpr_edgeCount = vpr_rrFromNode.num_edges;
      for( int vpr_edgeIndex = 0; vpr_edgeIndex < vpr_edgeCount; ++vpr_edgeIndex )
      {
         int vpr_rrChildIndex = vpr_rrFromNode.edges[vpr_edgeIndex];
         if( vpr_rrChildIndex == vpr_rrToIndex )
         {
            isMember = true;
            break;
         }
      }
   }

   if( !isMember )
   {
      if( pfromRouteNode->GetVPR_Type( ) == IPIN )
      {
         isMember = true;
      }
   }
   return( isMember );
}

//===========================================================================//
// Method         : IsValid
// Description    : Returns TRUE if at least one pre-routed net exists
//                  (ie. at least one routing constraint has been defined).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::IsValid(
      void ) const
{
   return( this->netNameList_.IsValid( ) &&
           this->netList_.IsValid( ) ?
           true : false );
}

//===========================================================================//
// Method         : InitNetList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_PreRoutedHandler_c::InitNetList_(
      const TNO_NetList_c&     netList,
      const TNO_NameList_t&    netOrderList,
            TCH_NetNameList_t* pnetNameList,
            TCH_NetList_t*     pnetList ) const
{
   pnetNameList->SetCapacity( netList.GetLength( ));
   pnetList->SetCapacity( netList.GetLength( ));

   for( size_t i = 0; i < netOrderList.GetLength( ); ++i )
   {
      const char* pszNetName = netOrderList[i]->GetName( );

      if( !netList.IsMember( pszNetName ))
         continue;

      TCH_NetName_t netName( pszNetName, pnetList->GetLength( ));
      pnetNameList->Add( netName );

      TCH_Net_c net( pszNetName );
      pnetList->Add( net );
   }
}

//===========================================================================//
// Method         : LoadNetList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::LoadNetList_(
      const TNO_NetList_c& netList ) const
{
   bool ok = true;
   
   for( size_t i = 0; i < netList.GetLength( ); ++i )
   {
      const TNO_Net_c& net = *netList[i];
      const char* pszNetName = net.GetName( );
      TCH_Net_c* pnet = this->FindNet_( pszNetName );

      // Load local route path based on given net's route list
      if( !net.HasRouteList( ))
         continue;

      const TNO_RouteList_t& routeList = net.GetRouteList( );
      ok = this->AddNetRoutePath_( routeList, pnet );
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : AddNetRoutePath_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::AddNetRoutePath_(
      const TNO_RouteList_t& routeList,
            TCH_Net_c*       pnet ) const
{
   bool ok = true;

   for( size_t i = 0; i < routeList.GetLength( ); ++i )
   {
      const TNO_Route_t& route = *routeList[i];
      TCH_RoutePath_c routePath( route );

      if( !pnet->IsMemberRoutePath( routePath ))
      {
         pnet->AddRoutePath( routePath );
      }
      else
      {
         const char* pszNetName = pnet->GetName( );
         const char* pszSrcInstName = routePath.FindSourceInstName( );
         const char* pszSrcPortName = routePath.FindSourcePortName( );
         const char* pszSinkInstName = routePath.FindSinkInstName( );
         const char* pszSinkPortName = routePath.FindSinkPortName( );
         ok = this->ShowDuplicateRoutePathWarning_( pszNetName,
                                                    pszSrcInstName, pszSrcPortName,
                                                    pszSinkInstName, pszSinkPortName );
         if( !ok )
            break;
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : ValidateNetList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::ValidateNetList_(
      const t_net*             vpr_netArray,
            int                vpr_netCount,
      const TCH_NetNameList_t& netNameList,
            TCH_NetList_t*     pnetList ) const
{
   bool ok = true;

   // First, iterate local net list to reset each net's "isLegal" flag
   for( size_t i = 0; i < pnetList->GetLength( ); ++i )
   {
      TCH_Net_c* pnet = (*pnetList)[i];
      pnet->Reset( );
   }

   // Next, iterate VPR's net list and set local net's "isLegal", as possible
   for( int vpr_netIndex = 0; vpr_netIndex < vpr_netCount; ++vpr_netIndex )
   {
      const t_net& vpr_net = vpr_netArray[vpr_netIndex];
      const char* pszNetName = vpr_net.name;

      TCH_NetName_t netName( pszNetName );
      if( !netNameList.IsMember( netName ))
         continue;

      TCH_Net_c* pnet = this->FindNet_( pszNetName );
      pnet->SetVPR_NetIndex( vpr_netIndex );
      pnet->SetLegal( true );
   }

   // Finally, iterate local net list to report any invalid net names
   for( size_t i = 0; i < pnetList->GetLength( ); ++i )
   {
      const TCH_Net_c& net = *(*pnetList)[i];
      if( net.IsLegal( ))
         continue;

      const char* pszNetName = net.GetName( );

      ok = this->ShowNetNotFoundWarning_( pszNetName );
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : ValidateNetListInstPins_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::ValidateNetListInstPins_(
      const t_block*       vpr_blockArray,
      const t_net*         vpr_netArray,
            int            vpr_netCount,
            TCH_NetList_t* pnetList ) const
{
   bool ok = true;

   // First, iterate local net list to reset each route path's "isLegal" flag
   for( size_t i = 0; i < pnetList->GetLength( ); ++i )
   {
      TCH_Net_c* pnet = (*pnetList)[i];
      pnet->ResetRoutePathList( );
   }

   // Next, iterate local net and set local route path "isLegal", as possible
   for( size_t i = 0; i < pnetList->GetLength( ); ++i )
   {
      TCH_Net_c* pnet = (*pnetList)[i];
      if( !pnet->IsLegal( ))
         continue;

      ok = this->ValidateNetInstPins_( vpr_blockArray,
                                       vpr_netArray, vpr_netCount, pnet );
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : ValidateNetListRoutePaths_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::ValidateNetListRoutePaths_(
      const t_rr_node*     vpr_rrNodeArray,
            TCH_NetList_t* pnetList ) const
{
   bool ok = true;

   // Iterate local net list to reset each route node's "isLegal" flag
   for( size_t i = 0; i < pnetList->GetLength( ); ++i )
   {
      TCH_Net_c* pnet = (*pnetList)[i];
      pnet->ResetRoutePathNodeList( );
   }

   // Iterate each valid local net and set local route node "isLegal", as possible
   for( size_t i = 0; i < pnetList->GetLength( ); ++i )
   {
      TCH_Net_c* pnet = (*pnetList)[i];
      if( !pnet->IsLegal( ))
         continue;

      const TCH_RoutePathList_t* proutePathList = pnet->GetRoutePathList( );
      for( size_t j = 0; j < proutePathList->GetLength( ); ++j )
      {
         TCH_RoutePath_c* proutePath = (*proutePathList)[j];
         if( !proutePath->IsLegal( ))
            continue;

         // Validate route path from/to nodes 'consecutive' w.r.t VPR's "rr_graph"
         TCH_RouteNodeList_t* prouteNodeList = proutePath->GetRouteNodeList( );
         const char* pszNetName = pnet->GetName( );
         ok = proutePath->ValidateRouteNodeList( vpr_rrNodeArray, 
                                                 prouteNodeList, pszNetName );
         if( !ok )
            break;
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : ValidateNetInstPins_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::ValidateNetInstPins_(
      const t_block*   vpr_blockArray,
      const t_net*     vpr_netArray,
            int        vpr_netCount,
            TCH_Net_c* pnet ) const
{
   bool ok = true;

   const char* pszNetName = pnet->GetName( );
   int vpr_netIndex = pnet->GetVPR_NetIndex( );

   const TCH_RoutePathList_t* proutePathList = pnet->GetRoutePathList( );
   for( size_t i = 0; i < proutePathList->GetLength( ); ++i )
   {
      TCH_RoutePath_c* proutePath = (*proutePathList)[i];
      if( !proutePath->HasSourceNode( ) || !proutePath->HasSinkNode( ))
         continue;

      ok = this->ValidateNetRoutePathInstPins_( vpr_blockArray,
                                                vpr_netArray,
                                                vpr_netCount,
                                                vpr_netIndex,
                                                pszNetName,
                                                proutePath );
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : ValidateNetRoutePathInstPins_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::ValidateNetRoutePathInstPins_(
      const t_block*         vpr_blockArray,
      const t_net*           vpr_netArray,
            int              vpr_netCount,
            int              vpr_netIndex,
      const char*            pszNetName,
            TCH_RoutePath_c* proutePath ) const
{
   bool ok = true;

   // Search for source/sink inst-pins based on VPR's net list
   const char* pszSrcInstName = proutePath->FindSourceInstName( );
   const char* pszSrcPinName = proutePath->FindSourcePinName( );
   const char* pszSinkInstName = proutePath->FindSinkInstName( );
   const char* pszSinkPinName = proutePath->FindSinkPinName( );

   if( this->ExistsRoutePathInstPin_( vpr_blockArray, 
                                      vpr_netArray, vpr_netCount, vpr_netIndex, 
                                      pszSrcInstName, pszSrcPinName ) &&
       this->ExistsRoutePathInstPin_( vpr_blockArray, 
                                      vpr_netArray, vpr_netCount, vpr_netIndex, 
                                      pszSinkInstName, pszSinkPinName ))
   {
      // Update route path's "isLegal" flag based on valid source/sink inst-pins
      proutePath->SetLegal( true );
   }
   else
   {
      if( ok && 
          !this->ExistsRoutePathInstPin_( vpr_blockArray, 
                                          vpr_netArray, vpr_netCount, vpr_netIndex, 
                                          pszSrcInstName, pszSrcPinName ))
      {
         const char* pszSrcInstPin = proutePath->FindSourceName( );
         ok = this->ShowPinNotFoundWarning_( pszNetName, pszSrcInstPin, "Source" );
      }
      if( ok && 
          !this->ExistsRoutePathInstPin_( vpr_blockArray, 
                                          vpr_netArray, vpr_netCount, vpr_netIndex, 
                                          pszSinkInstName, pszSinkPinName ))
      {
         const char* pszSinkInstPin = proutePath->FindSinkName( );
         ok = this->ShowPinNotFoundWarning_( pszNetName, pszSinkInstPin, "Sink" );
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : UpdateNetListInstPins_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_PreRoutedHandler_c::UpdateNetListInstPins_(
      const t_block*       vpr_blockArray,
            int            vpr_blockCount,
            TCH_NetList_t* pnetList ) const
{
   // Iterate VPR's block list to generate a local sorted instance list
   TPO_InstList_t instList;
   this->BuildInstList_( vpr_blockArray, vpr_blockCount, &instList );

   // Iterate local net list to update route path's source/sink inst-pin 
   // coordinates using the local sorted instance list
   for( size_t i = 0; i < pnetList->GetLength( ); ++i )
   {
      TCH_Net_c* pnet = (*pnetList)[i];
      if( !pnet->IsLegal( ))
         continue;

      const TCH_RoutePathList_t* proutePathList = pnet->GetRoutePathList( );
      for( size_t j = 0; j < proutePathList->GetLength( ); ++j )
      {
         const TCH_RoutePath_c* proutePath = (*proutePathList)[j];
         if( !proutePath->IsLegal( ))
            continue;

         TCH_RouteNode_c* psrcRouteNode = proutePath->FindSourceNode( );
         this->UpdateNetListInstPoint_( instList, psrcRouteNode );

         TCH_RouteNode_c* popinRouteNode = proutePath->FindSourceNode( 1 );
         this->UpdateNetListInstPoint_( instList, popinRouteNode );

         TCH_RouteNode_c* pipinRouteNode = proutePath->FindSinkNode( -1 );
         this->UpdateNetListInstPoint_( instList, pipinRouteNode );

         TCH_RouteNode_c* psinkRouteNode = proutePath->FindSinkNode( );
         this->UpdateNetListInstPoint_( instList, psinkRouteNode );
      }
   }
}

//===========================================================================//
// Method         : UpdateNetListInstPoint_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_PreRoutedHandler_c::UpdateNetListInstPoint_(
      const TPO_InstList_t&  instList,
            TCH_RouteNode_c* prouteNode ) const
{
   const char* pszInstName = prouteNode->GetInstName( );
   const TPO_Inst_c* pinst = instList.Find( pszInstName );
   if( pinst )
   {
      const TGO_Point_c& gridPoint = pinst->GetPlaceOrigin( );
      prouteNode->SetVPR_GridPoint( gridPoint );
   }
}

//===========================================================================//
// Method         : UpdateNetListRoutePaths_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_PreRoutedHandler_c::UpdateNetListRoutePaths_(
      const t_rr_node*     vpr_rrNodeArray,
            int            vpr_rrNodeCount,
            t_grid_tile**  vpr_gridArray,
            TCH_NetList_t* pnetList ) const
{
   // Make a sorted "rr_graph" node list based on VPR's "rr_graph" nodes
   TCH_VPR_GraphNodeList_t vpr_graphNodeList;
   this->LoadVPR_GraphNodeList_( vpr_rrNodeArray, vpr_rrNodeCount,
                                 vpr_gridArray, &vpr_graphNodeList );

   // Iterate local net list to update route path's "rr_graph" node indices
   for( size_t i = 0; i < pnetList->GetLength( ); ++i )
   {
      TCH_Net_c* pnet = (*pnetList)[i];
      if( !pnet->IsLegal( ))
         continue;

      const TCH_RoutePathList_t* proutePathList = pnet->GetRoutePathList( );
      for( size_t j = 0; j < proutePathList->GetLength( ); ++j )
      {
         TCH_RoutePath_c* proutePath = (*proutePathList)[j];
         if( !proutePath->IsLegal( ))
            continue;

         // Update route path "rr_graph" node indices
         const TCH_RouteNodeList_t* prouteNodeList = proutePath->GetRouteNodeList( );
         for( size_t k = 0; k < prouteNodeList->GetLength( ); ++k )
         {
            TCH_RouteNode_c* prouteNode = (*prouteNodeList)[k];

            const TCH_VPR_GraphNode_c* pvpr_graphNode = 0;
            pvpr_graphNode = this->FindVPR_GraphNode_( vpr_graphNodeList, 
                                                       *prouteNode );
            if( pvpr_graphNode )
            {
               prouteNode->SetVPR_NodeIndex( pvpr_graphNode->GetVPR_NodeIndex( ));

               if( prouteNode->GetVPR_Type( ) == IPIN )
               {
                  int ipinIndex = prouteNode->GetVPR_NodeIndex( );
                  int sinkIndex = vpr_rrNodeArray[ipinIndex].edges[0];

                  TCH_RouteNode_c* psinkNode = (*prouteNodeList)[k+1];
                  psinkNode->SetVPR_NodeIndex( sinkIndex );
               }
            }
         }

         // Also, update "rr_graph" node -> route path index mapping
         proutePath->InitVPR_GraphToRouteMap( );
      }
   }
}

//===========================================================================//
// Method         : ExistsRoutePathInstPin_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::ExistsRoutePathInstPin_(
      const t_block* vpr_blockArray,
      const t_net*   vpr_netArray,
            int      vpr_netCount,
            int      vpr_netIndex,
      const char*    pszInstName,
      const char*    pszPinName ) const
{
   bool exists = false;

   if( vpr_netIndex < vpr_netCount )
   {
      const t_net& vpr_net = vpr_netArray[vpr_netIndex];

      // Iterate for every pin in the given VPR net...
      int nodeCount = 1 + vpr_net.num_sinks; // [VPR] Assume [0] for output pin
      for( int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex )
      {
         // Extract pin's instance (ie. block) name and port/pin name
         int blockIndex = vpr_net.node_block[nodeIndex];
         const char* pszBlockName = vpr_blockArray[blockIndex].name;
         if( strcmp( pszBlockName, pszInstName ) != 0 )
            continue;

         const t_pb_graph_pin* pvpr_graphPin = 0;
         pvpr_graphPin = this->vpr_data_.FindGraphPin( vpr_net, nodeIndex );

         if( pvpr_graphPin )
         {
            const char* pszPortName = pvpr_graphPin->port->name;
            if( strcmp( pszPortName, pszPinName ) != 0 )
               continue;

            exists = true;
            break;
         }
      }
   }
   return( exists );
}

//===========================================================================//
// Method         : LoadVPR_GraphNodeList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_PreRoutedHandler_c::LoadVPR_GraphNodeList_(
      const t_rr_node*               vpr_rrNodeArray,
            int                      vpr_rrNodeCount,
            t_grid_tile**            vpr_gridArray,
            TCH_VPR_GraphNodeList_t* pvpr_graphNodeList ) const
{
   pvpr_graphNodeList->Clear( );
   pvpr_graphNodeList->SetCapacity( vpr_rrNodeCount );

   // Iterate VPR's "rr_graph" to generate a sorted "rr_graph" node list
   for( int vpr_rrNodeIndex = 0; vpr_rrNodeIndex < vpr_rrNodeCount; ++vpr_rrNodeIndex )
   {
      const t_rr_node& vpr_rrNode = vpr_rrNodeArray[vpr_rrNodeIndex];

      t_rr_type vpr_type = vpr_rrNode.type;
      int ptcNum = vpr_rrNode.ptc_num;
      int xlow = vpr_rrNode.xlow;
      int ylow = vpr_rrNode.ylow;

      int numPins = vpr_gridArray[xlow][ylow].type->num_pins;
      int capacity = vpr_gridArray[xlow][ylow].type->capacity;

      int vpr_ptcNum = -1;
      TGO_Point_c vpr_gridPoint;
      switch( vpr_type )
      {
      case SOURCE:
      case SINK:
      case IPIN:
      case OPIN:
         vpr_ptcNum = ptcNum % ( numPins / capacity );
         vpr_gridPoint.x = xlow;
         vpr_gridPoint.y = ylow;
         vpr_gridPoint.z = ptcNum / ( numPins / capacity );
         break;
      case CHANX:
      case CHANY:
         vpr_ptcNum = ptcNum;
         vpr_gridPoint.x = xlow;
         vpr_gridPoint.y = ylow;
         vpr_gridPoint.z = 0;
         break;
      default:
         break;
      }
      TCH_VPR_GraphNode_c vpr_graphNode( vpr_type, vpr_ptcNum, vpr_gridPoint,
                                         vpr_rrNodeIndex );

      pvpr_graphNodeList->Add( vpr_graphNode );
   }
}

//===========================================================================//
// Method         : FindVPR_GraphNode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const TCH_VPR_GraphNode_c* TCH_PreRoutedHandler_c::FindVPR_GraphNode_(
      const TCH_VPR_GraphNodeList_t& vpr_graphNodeList,
      const TCH_RouteNode_c&         routeNode ) const
{
   // Find graph node based on route node's VPR type, grid point, and ptc_num
   t_rr_type vpr_type = routeNode.GetVPR_Type( );

   int vpr_ptcNum = -1;
   TGO_Point_c vpr_gridPoint;
   switch( vpr_type )
   {
   case SOURCE:
   case SINK:
   case IPIN:
   case OPIN:
      vpr_ptcNum = routeNode.GetVPR_Pin( );
      vpr_gridPoint.x = routeNode.GetVPR_GridPoint( ).x;
      vpr_gridPoint.y = routeNode.GetVPR_GridPoint( ).y;
      vpr_gridPoint.z = routeNode.GetVPR_GridPoint( ).z;
      break;
   case CHANX:
   case CHANY:
      vpr_ptcNum = routeNode.GetVPR_Track( );
      vpr_gridPoint.x = routeNode.GetVPR_Channel( ).x1;
      vpr_gridPoint.y = routeNode.GetVPR_Channel( ).y1;
      vpr_gridPoint.z = 0;
      break;
   default:
      break;
   }

   TCH_VPR_GraphNode_c vpr_graphNode( vpr_type, vpr_ptcNum, vpr_gridPoint );
   return( vpr_graphNodeList[vpr_graphNode] );
}

//===========================================================================//
// Method         : BuildInstList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_PreRoutedHandler_c::BuildInstList_(
      const t_block*        vpr_blockArray,
            int             vpr_blockCount,
            TPO_InstList_t* pinstList ) const
{
   pinstList->Clear( );
   pinstList->SetCapacity( vpr_blockCount );

   // Iterate VPR's block list to generate a local sorted instance list
   for( int vpr_blockIndex = 0; vpr_blockIndex < vpr_blockCount; ++vpr_blockIndex )
   {
      const t_block& vpr_block = vpr_blockArray[vpr_blockIndex];

      const char* pszBlockName = vpr_block.name;
      TGO_Point_c gridPoint( vpr_block.x, vpr_block.y, vpr_block.z );

      TPO_Inst_c inst( pszBlockName );
      inst.SetPlaceOrigin( gridPoint );

      pinstList->Add( inst );
   }
}

//===========================================================================//
// Method         : FindNet_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_Net_c* TCH_PreRoutedHandler_c::FindNet_(
      const char* pszNetName ) const
{
   size_t netIndex = TNO_NET_INDEX_INVALID;

   if( this->cachedNetName_ == pszNetName )
   {
      netIndex = this->cachedNetIndex_;
   }
   else
   {
      const TCH_NetName_t* pnetName = this->netNameList_.Find( pszNetName );
      netIndex = ( pnetName ? pnetName->GetIndex( ) : TNO_NET_INDEX_INVALID );
   }

   TCH_PreRoutedHandler_c* ppreRoutedHandler = const_cast< TCH_PreRoutedHandler_c* >( this );
   ppreRoutedHandler->cachedNetName_ = pszNetName;
   ppreRoutedHandler->cachedNetIndex_ = netIndex;

   return( this->netList_[netIndex] );
}

//===========================================================================//
// Method         : FindNetRoutePath_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
const TCH_RoutePath_c* TCH_PreRoutedHandler_c::FindNetRoutePath_(
      const TCH_Net_c& net,
            int        vpr_srcNodeIndex,
            int        vpr_sinkNodeIndex ) const
{
   const TCH_RoutePath_c* proutePath = 0;

   const TCH_RoutePathList_t& routePathList = *net.GetRoutePathList( );
   for( size_t i = 0; i < routePathList.GetLength( ); ++i )
   {
      proutePath = routePathList[i];
      const TCH_VPR_GraphToRouteMap_t& graphToRouteMap = *proutePath->GetVPR_GraphToRouteMap( );

      TCH_VPR_GraphToRoute_c srcGraphToRoute( vpr_srcNodeIndex );
      const TCH_VPR_GraphToRoute_c* psrcGraphToRoute = graphToRouteMap[srcGraphToRoute];

      TCH_VPR_GraphToRoute_c sinkGraphToRoute( vpr_sinkNodeIndex );
      const TCH_VPR_GraphToRoute_c* psinkGraphToRoute = graphToRouteMap[sinkGraphToRoute];

      if( psrcGraphToRoute && psinkGraphToRoute )
      {
         size_t srcNodeIndex = psrcGraphToRoute->GetRouteNodeIndex( );
         size_t sinkNodeIndex = psinkGraphToRoute->GetRouteNodeIndex( );
         if(( srcNodeIndex == proutePath->FindSourceIndex( )) &&
            ( sinkNodeIndex == proutePath->FindSinkIndex( )))
         {
            break;
         }
      }
      proutePath = 0;
   }
   return( proutePath );
}

//===========================================================================//
// Method         : ShowDuplicateRoutePathWarning_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::ShowDuplicateRoutePathWarning_(
      const char* pszNetName,
      const char* pszSrcInstName,
      const char* pszSrcPortName,
      const char* pszSinkInstName,
      const char* pszSinkPortName ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   bool ok = printHandler.Warning( "Ignoring pre-route for net \"%s\" from source \"%s|%s\" to sink \"%s|%s\".\n",
                                   TIO_PSZ_STR( pszNetName ),
                                   TIO_PSZ_STR( pszSrcInstName ),
                                   TIO_PSZ_STR( pszSrcPortName ),
                                   TIO_PSZ_STR( pszSinkInstName ),
                                   TIO_PSZ_STR( pszSinkPortName ));
   printHandler.Info( "%sDuplicate route paths detected in the input pre-routes.\n",
                      TIO_PREFIX_WARNING_SPACE );
   return( ok );
}

//===========================================================================//
// Method         : ShowNetNotFoundWarning_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::ShowNetNotFoundWarning_(
      const char* pszNetName ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   bool ok = printHandler.Warning( "Ignoring pre-route for net \"%s\".\n",
                                   TIO_PSZ_STR( pszNetName ));
   printHandler.Info( "%sNet name was not found in VPR's net list.\n",
                      TIO_PREFIX_WARNING_SPACE );
   return( ok );
}

//===========================================================================//
// Method         : ShowPinNotFoundWarning_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_PreRoutedHandler_c::ShowPinNotFoundWarning_(
      const char* pszNetName,
      const char* pszInstPin,
      const char* pszType ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   bool ok = printHandler.Warning( "Ignoring pre-route for net \"%s\" due to pin \"%s\".\n",
                                   TIO_PSZ_STR( pszNetName ),
                                   TIO_PSZ_STR( pszInstPin ));
   printHandler.Info( "%s%s pin was not found in VPR's net list.\n",
                      TIO_PREFIX_WARNING_SPACE,
                      TIO_PSZ_STR( pszType ));
   return( ok );
}
