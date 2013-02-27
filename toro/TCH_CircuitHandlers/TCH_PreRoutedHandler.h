//===========================================================================//
// Purpose : Declaration and inline definitions for the universal 
//           TCH_PreRoutedHandler singleton class. This class supports
//           pre-routed constraints for the VPR routing tool
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

#ifndef TCH_PREROUTED_ROUTE_HANDLER_H
#define TCH_PREROUTED_ROUTE_HANDLER_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Typedefs.h"
#include "TC_NameIndex.h"

#include "TPO_Typedefs.h"
#include "TPO_Inst.h"

#include "TNO_Typedefs.h"
#include "TNO_NetList.h"

#include "TCH_Typedefs.h"
#include "TCH_Net.h"
#include "TCH_VPR_GraphNode.h"
#include "TCH_VPR_Data.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
class TCH_PreRoutedHandler_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TCH_PreRoutedHandler_c& GetInstance( bool newInstance = true );
   static bool HasInstance( void );

   bool Configure( const TNO_NetList_c& netList,
		   const TNO_NameList_t& netOrderList,
                   TCH_RouteOrderMode_t orderMode );

   void Set( t_grid_tile** vpr_gridArray,
             int vpr_nx,
             int vpr_ny,
             const t_block* vpr_blockArray,
             int vpr_blockCount,
             const t_net* vpr_netArray,
             int vpr_netCount,
             const t_rr_node* vpr_rrNodeArray,
             int vpr_rrNodeCount );

   TCH_RouteOrderMode_t GetOrderMode( void ) const;
   const TCH_NetNameList_t& GetNetNameList( void ) const;
   const TCH_NetList_t& GetNetList( void ) const;

   bool ValidatePreRoutes( void );

   bool IsMemberPreRouteNet( const char* pszNetName,
                             int vpr_rrSrcIndex,
                             int vpr_rrSinkIndex ) const;
   bool IsMemberPreRoutePath( const char* pszNetName,
                              int vpr_rrSrcIndex,
                              int vpr_rrSinkIndex,
                              int vpr_rrFromIndex,
                              int vpr_rrToIndex ) const;
   bool IsMemberPreRoutePath( const TCH_RoutePath_c& routePath,
                              int vpr_rrFromIndex,
                              int vpr_rrToIndex ) const;

   bool IsValid( void ) const;

protected:

   TCH_PreRoutedHandler_c( void );
   ~TCH_PreRoutedHandler_c( void );

private:

   void InitNetList_( const TNO_NetList_c& netList,
                      const TNO_NameList_t& netOrderList,
                      TCH_NetNameList_t* pnetNameList,
                      TCH_NetList_t* pnetList ) const;
   bool LoadNetList_( const TNO_NetList_c& netList ) const;
   bool AddNetRoutePath_( const TNO_RouteList_t& routeList,
                          TCH_Net_c* pnet ) const;

   bool ValidateNetList_( const t_net* vpr_netArray,
                          int vpr_netCount,
                          const TCH_NetNameList_t& netNameList,
                          TCH_NetList_t* pnetList ) const;
   bool ValidateNetListInstPins_( const t_block* vpr_blockArray,
                                  const t_net* vpr_netArray,
                                  int vpr_netCount,
                                  TCH_NetList_t* pnetList ) const;
   bool ValidateNetListRoutePaths_( const t_rr_node* vpr_rrNodeArray,
                                    TCH_NetList_t* pnetList ) const;
   bool ValidateNetInstPins_( const t_block* vpr_blockArray,
                              const t_net* vpr_netArray,
                              int vpr_netCount,
                              TCH_Net_c* pnet ) const;
   bool ValidateNetRoutePathInstPins_( const t_block* vpr_blockArray,
                                       const t_net* vpr_netArray,
                                       int vpr_netCount,
                                       int vpr_netIndex,
                                       const char* pszNetName,
                                       TCH_RoutePath_c* proutePath ) const;

   void UpdateNetListInstPins_( const t_block* vpr_blockArray,
                                int vpr_blockCount,
                                TCH_NetList_t* pnetList ) const;
   void UpdateNetListInstPoint_( const TPO_InstList_t& instList,
                                 TCH_RouteNode_c* prouteNode ) const;
   void UpdateNetListRoutePaths_( const t_rr_node* vpr_rrNodeArray,
                                  int vpr_rrNodeCount,
                                  t_grid_tile** vpr_gridArray,
                                  const t_block* vpr_blockArray,
                                  TCH_NetList_t* pnetList ) const;

   bool ExistsRoutePathInstPin_( const t_block* vpr_blockArray,
                                 const t_net* vpr_netArray,
                                 int vpr_netCount,
                                 int vpr_netIndex,
                                 const char* pszInstName,
                                 const char* pszPinName ) const;

   void LoadVPR_GraphNodeList_( const t_rr_node* vpr_rrNodeArray,
                                int vpr_rrNodeCount,
                                t_grid_tile** vpr_gridArray,
                                const t_block* vpr_blockArray,
                                TCH_VPR_GraphNodeList_t* pvpr_graphNodeList ) const;
   const TCH_VPR_GraphNode_c* FindVPR_GraphNode_( const TCH_VPR_GraphNodeList_t& vpr_graphNodeList,
                                                  const TCH_RouteNode_c& routeNode ) const;

   void BuildInstList_( const t_block* vpr_blockArray,
                        int vpr_blockCount,
                        TPO_InstList_t* pinstList ) const;

   TCH_Net_c* FindNet_( const char* pszNetName ) const;
   const TCH_RoutePath_c* FindNetRoutePath_( const TCH_Net_c& net,
                                             int vpr_srcNodeIndex,
                                             int vpr_sinkNodeIndex ) const;

   bool ShowDuplicateRoutePathWarning_( const char* pszNetName,
                                        const char* pszSrcInstName,
                                        const char* pszSrcPortName,
                                        const char* pszSinkInstName,
                                        const char* pszSinkPortName ) const;
   bool ShowNetNotFoundWarning_( const char* pszNetName ) const;
   bool ShowPinNotFoundWarning_( const char* pszNetName,
                                 const char* pszInstPin,
                                 const char* pszType ) const;
private:

   TCH_RouteOrderMode_t orderMode_; 
                             // Selects pre-route routing order: FIRST|AUTO

   TCH_NetNameList_t netNameList_; 
                             // Sorted list of pre-route net names
   TCH_NetList_t netList_;   // Ordered list of pre-route nets

                             // Note: The pre-route net list is *not* sorted in
                             // order to preserve the initial input pre-route 
                             // net ordering. However, the pre-route net name 
                             // list is sorted to order to support quick and 
                             // efficient access into the net list.

   string cachedNetName_;    // Used to improve efficiency when testing for 
                             // membership based on a net name
   size_t cachedNetIndex_;   // "

   TCH_VPR_Data_c vpr_data_; // Local copies of VPR's net and block structures

   static TCH_PreRoutedHandler_c* pinstance_;  // Define ptr for singleton instance

private:

   enum TCH_DefCapacity_e 
   { 
      TCH_PREROUTED_NET_NAME_LIST_DEF_CAPACITY = 1,
      TCH_PREROUTED_NET_LIST_DEF_CAPACITY = 1
   };
};

#endif
