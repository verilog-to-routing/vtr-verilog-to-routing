//===========================================================================//
// Purpose : Declaration and inline definitions for the universal 
//           TFH_FabricConnectionBlockHandler singleton class. This class
//           supports connection block overrides for the VPR place and route 
//           tool.
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

#ifndef TFH_FABRIC_CONNECTION_BLOCK_HANDLER_H
#define TFH_FABRIC_CONNECTION_BLOCK_HANDLER_H

#include <cstdio>
using namespace std;

#include "TFH_ConnectionBlock.h"
#include "TFH_ConnectionMap.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
class TFH_FabricConnectionBlockHandler_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TFH_FabricConnectionBlockHandler_c& GetInstance( bool newInstance = true );
   static bool HasInstance( void );

   size_t GetLength( void ) const;
   const TFH_ConnectionBlockList_t& GetConnectionBlockList( void ) const;
   const TFH_ConnectionMapList_t& GetConnectionMapList( void ) const;

   bool UpdateGraphConnections( void* vpr_rrNodeArray,
                                int vpr_rrNodeCount );

   void Clear( void );

   void Add( const TGO_Point_c& vpr_blockOrigin,
             int vpr_pinIndex,
             const string& srPinName,
             TC_SideMode_t pinSide,
             const TFH_BitPattern_t& connectionPattern );
   void Add( const TGO_Point_c& vpr_blockOrigin,
             int vpr_pinIndex,
             const char* pszPinName,
             TC_SideMode_t pinSide,
             const TFH_BitPattern_t& connectionPattern );
   void Add( const TFH_ConnectionBlock_c& connectionBlock );

   bool IsMember( const TGO_Point_c& vpr_blockOrigin,
                  int vpr_pinIndex ) const;
   bool IsMember( const TFH_ConnectionBlock_c& connectionBlock ) const;

   bool IsValid( void ) const;

protected:

   TFH_FabricConnectionBlockHandler_c( void );
   ~TFH_FabricConnectionBlockHandler_c( void );

private:

   void BuildConnectionMapList_( const void* vpr_rrNodeArray,
                                 int vpr_rrNodeCount,
                                 const TFH_ConnectionBlockList_t& connectionBlockList,
                                 TFH_ConnectionMapList_t* pconnectionMapList ) const;
   void LoadConnectionMapList_( const TFH_ConnectionBlockList_t& connectionBlockList,
                                TFH_ConnectionMapList_t* pconnectionMapList ) const;
   void UpdateConnectionMapList_( const void* vpr_rrNodeArray,
                                  int vpr_rrNodeCount,
                                  TFH_ConnectionMapList_t* pconnectionMapList ) const;

   bool OverrideGraphOutputPinsToTracks_( void* vpr_rrNodeArray,
                                          int vpr_rrNodeCount,
                                          const TFH_ConnectionBlockList_t& connectionBlockList,
                                          const TFH_ConnectionMapList_t& connectionMapList ) const;
   bool OverrideGraphTracksToInputPins_( void* vpr_rrNodeArray,
                                         int vpr_rrNodeCount,
                                         const TFH_ConnectionBlockList_t& connectionBlockList,
                                         const TFH_ConnectionMapList_t& connectionMapList ) const;

   bool ConnectGraphOutputPins_( void* vpr_rrNodeArray,
                                 int vpr_rrNodeCount,
                                 const TFH_ConnectionBlockList_t& connectionBlockList,
                                 const TFH_ConnectionMapList_t& connectionMapList ) const;
   void DisconnectGraphOutputPins_( void* vpr_rrNodeArray,
                                    int vpr_rrNodeCount,
                                    const TFH_ConnectionBlockList_t& connectionBlockList ) const;

   bool ConnectGraphInputPins_( void* vpr_rrNodeArray,
                                int vpr_rrNodeCount,
                                const TFH_ConnectionBlockList_t& connectionBlockList,
                                const TFH_ConnectionMapList_t& connectionMapList ) const;
   void DisconnectGraphInputPins_( void* vpr_rrNodeArray,
                                   int vpr_rrNodeCount,
                                   const TFH_ConnectionBlockList_t& connectionBlockList ) const;

   bool AppendGraphNodeEdge_( void* pvpr_rrNode,
                              int vpr_rrIndex,
                              short ioPinSwitch ) const;
   bool DeleteGraphNodeEdge_( void* pvpr_rrNode,
                              int edgeIndex,
                              short* pioPinSwitch ) const;

   void FindChannelOrigin_( const TGO_Point_c& vpr_blockOrigin,
                            TC_SideMode_t pinSide,
                            TGO_Point_c* pvpr_channelOrigin,
                            TGO_OrientMode_t* pchannelOrient = 0 ) const;

private:

   TFH_ConnectionBlockList_t connectionBlockList_;
   TFH_ConnectionMapList_t connectionMapList_;

   short inputPinSwitch_;
   short outputPinSwitch_;

   static TFH_FabricConnectionBlockHandler_c* pinstance_; // Define ptr for singleton instance

private:

   enum TFH_DefCapacity_e 
   { 
      TFH_CONNECTION_BLOCK_LIST_DEF_CAPACITY = 64,
      TFH_CONNECTION_MAP_LIST_DEF_CAPACITY = 64
   };
};

#endif
