//===========================================================================//
// Purpose : Method definitions for a TFH_FabricConnectionBlockHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - GetLength
//           - GetConnectionBlockList
//           - GetConnectionMapList
//           - UpdateGraphConnections
//           - Clear
//           - Add
//           - IsMember
//           - IsValid
//
//           Protected methods include:
//           - TFH_FabricConnectionBlockHandler_c
//           - ~TFH_FabricConnectionBlockHandler_c
//
//           Private methods include:
//           - BuildConnectionMapList
//           - LoadConnectionMapList_
//           - UpdateConnectionMapList_
//           - OverrideGraphOutputPinsToTracks_
//           - OverrideGraphTracksToInputPins_
//           - ConnectGraphOutputPins_
//           - DisconnectGraphOutputPins_
//           - ConnectGraphInputPins_
//           - DisconnectGraphInputPins_
//           - AppendGraphNodeEdge_
//           - DeleteGraphNodeEdge_
//           - FindChannelOrigin_
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

#include "TC_MemoryUtils.h"

#include "TIO_PrintHandler.h"

#include "TFH_Typedefs.h"
#include "TFH_FabricConnectionBlockHandler.h"

// Initialize the connection block handler "singleton" class, as needed
TFH_FabricConnectionBlockHandler_c* TFH_FabricConnectionBlockHandler_c::pinstance_ = 0;

#include "vpr_api.h"

//===========================================================================//
// Method         : TFH_FabricConnectionBlockHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
TFH_FabricConnectionBlockHandler_c::TFH_FabricConnectionBlockHandler_c( 
      void )
      :
      connectionBlockList_( TFH_CONNECTION_BLOCK_LIST_DEF_CAPACITY ),
      connectionMapList_( TFH_CONNECTION_MAP_LIST_DEF_CAPACITY ),
      inputPinSwitch_( 0 ),
      outputPinSwitch_( 0 )
{
   pinstance_ = 0;
}

//===========================================================================//
// Method         : ~TFH_FabricConnectionBlockHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
TFH_FabricConnectionBlockHandler_c::~TFH_FabricConnectionBlockHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
void TFH_FabricConnectionBlockHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TFH_FabricConnectionBlockHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
void TFH_FabricConnectionBlockHandler_c::DeleteInstance( 
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
// 04/24/13 jeffr : Original
//===========================================================================//
TFH_FabricConnectionBlockHandler_c& TFH_FabricConnectionBlockHandler_c::GetInstance(
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
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_FabricConnectionBlockHandler_c::HasInstance(
      void )
{
   return( pinstance_ ? true : false );
}

//===========================================================================//
// Method         : GetLength
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
size_t TFH_FabricConnectionBlockHandler_c::GetLength(
      void ) const
{
   return( this->connectionBlockList_.GetLength( )); 
}

//===========================================================================//
// Method         : GetConnectionBlockList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
const TFH_ConnectionBlockList_t& TFH_FabricConnectionBlockHandler_c::GetConnectionBlockList(
      void ) const
{
   return( this->connectionBlockList_ );
}

//===========================================================================//
// Method         : GetConnectionMapList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
const TFH_ConnectionMapList_t& TFH_FabricConnectionBlockHandler_c::GetConnectionMapList(
      void ) const
{
   return( this->connectionMapList_ );
}

//===========================================================================//
// Method         : UpdateGraphConnections
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_FabricConnectionBlockHandler_c::UpdateGraphConnections(
      void* vpr_rrNodeArray,
      int   vpr_rrNodeCount )
{
   bool ok = true;

   // First, build a local connection map based on given connection blocks
   // (ie. map the given connections from pins->tracks to tracks->pins)
   this->BuildConnectionMapList_( vpr_rrNodeArray, vpr_rrNodeCount,
                                  this->connectionBlockList_,
                                  &this->connectionMapList_ );

   // Next, update all "rr_graph" nodes from OPINs to CHANX|CHANY, as possible
   if( ok )
   {
      ok = this->OverrideGraphOutputPinsToTracks_( vpr_rrNodeArray, vpr_rrNodeCount,
                                                   this->connectionBlockList_,
                                                   this->connectionMapList_ );
   }

   // Then, update all "rr_graph" nodes from CHANX|CHANY to IPINs, as possible
   if( ok )
   {
      ok = this->OverrideGraphTracksToInputPins_( vpr_rrNodeArray, vpr_rrNodeCount,
                                                  this->connectionBlockList_,
                                                  this->connectionMapList_ );
   }
   return( ok );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
void TFH_FabricConnectionBlockHandler_c::Clear( 
      void )
{
   this->connectionBlockList_.Clear( );
}

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
void TFH_FabricConnectionBlockHandler_c::Add( 
      const TGO_Point_c&      vpr_blockOrigin,
            int               vpr_pinIndex,
      const string&           srPinName,
            TC_SideMode_t     pinSide,
      const TFH_BitPattern_t& connectionPattern )
{
   TFH_ConnectionBlock_c connectionBlock( vpr_blockOrigin, vpr_pinIndex, 
                                          srPinName, pinSide,
                                          connectionPattern );
   this->Add( connectionBlock );
}

//===========================================================================//
void TFH_FabricConnectionBlockHandler_c::Add( 
      const TGO_Point_c&      vpr_blockOrigin,
            int               vpr_pinIndex,
      const char*             pszPinName,
            TC_SideMode_t     pinSide,
      const TFH_BitPattern_t& connectionPattern )
{
   TFH_ConnectionBlock_c connectionBlock( vpr_blockOrigin, vpr_pinIndex,
                                          pszPinName, pinSide,
                                          connectionPattern );
   this->Add( connectionBlock );
}

//===========================================================================//
void TFH_FabricConnectionBlockHandler_c::Add( 
      const TFH_ConnectionBlock_c& connectionBlock )
{
   this->connectionBlockList_.Add( connectionBlock );
}

//===========================================================================//
// Method         : IsMember
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_FabricConnectionBlockHandler_c::IsMember( 
      const TGO_Point_c& vpr_blockOrigin,
            int          vpr_pinIndex ) const
{
   TFH_ConnectionBlock_c connectionBlock( vpr_blockOrigin, vpr_pinIndex );
   return( this->IsMember( connectionBlock ));
}

//===========================================================================//
bool TFH_FabricConnectionBlockHandler_c::IsMember( 
      const TFH_ConnectionBlock_c& connectionBlock ) const
{
   return( this->connectionBlockList_.IsMember( connectionBlock ));
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_FabricConnectionBlockHandler_c::IsValid(
      void ) const
{
   return( this->connectionBlockList_.IsValid( ) ? true : false );
}

//===========================================================================//
// Method         : BuildConnectionMapList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
void TFH_FabricConnectionBlockHandler_c::BuildConnectionMapList_(
      const void*                      vpr_rrNodeArray,
            int                        vpr_rrNodeCount,
      const TFH_ConnectionBlockList_t& connectionBlockList,
            TFH_ConnectionMapList_t*   pconnectionMapList ) const
{
   // Load connection map (tracks->pins) per given connection blocks (pins->tracks)
   this->LoadConnectionMapList_( connectionBlockList,
                                 pconnectionMapList );

   // Update connection map based on "rr_graph" node indices (CHANX|CHANY only)
   this->UpdateConnectionMapList_( vpr_rrNodeArray, vpr_rrNodeCount,
                                   pconnectionMapList );
}

//===========================================================================//
// Method         : LoadConnectionMapList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
void TFH_FabricConnectionBlockHandler_c::LoadConnectionMapList_(
      const TFH_ConnectionBlockList_t& connectionBlockList,
            TFH_ConnectionMapList_t*   pconnectionMapList ) const
{
   pconnectionMapList->SetCapacity( connectionBlockList.GetLength( ));

   for( size_t i = 0; i < connectionBlockList.GetLength( ); ++i )
   {
      const TFH_ConnectionBlock_c& connectionBlock = *connectionBlockList[i];

      const TGO_Point_c& vpr_blockOrigin = connectionBlock.GetBlockOrigin( );
      int vpr_pinIndex = connectionBlock.GetPinIndex( );
      TC_SideMode_t pinSide = connectionBlock.GetPinSide( );
      const TFH_BitPattern_t& connectionPattern = connectionBlock.GetConnectionPattern( );

      TGO_Point_c vpr_channelOrigin;
      TGO_OrientMode_t channelOrient = TGO_ORIENT_UNDEFINED;
      this->FindChannelOrigin_( vpr_blockOrigin, pinSide, 
                                &vpr_channelOrigin, &channelOrient );

      for( size_t j = 0; j < connectionPattern.GetLength(); ++j ) 
      {
         const TC_Bit_c& connectionBit = *connectionPattern[j];
         if( !connectionBit.IsTrue( ))
            continue;

         int vpr_trackIndex = static_cast< int >( j );
         TFH_ConnectionMap_c connectionMap( vpr_channelOrigin, channelOrient,
                                            vpr_trackIndex, vpr_blockOrigin,
                                            vpr_pinIndex );

         pconnectionMapList->Add( connectionMap );
      }
   }
}

//===========================================================================//
// Method         : UpdateConnectionMapList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
void TFH_FabricConnectionBlockHandler_c::UpdateConnectionMapList_(
      const void*                    vpr_rrNodeArray,
            int                      vpr_rrNodeCount,
            TFH_ConnectionMapList_t* pconnectionMapList ) const
{
   for( int rrIndex = 0; rrIndex < vpr_rrNodeCount; ++rrIndex )
   {
      const t_rr_node* pvpr_rrNodeArray = static_cast< const t_rr_node* >( vpr_rrNodeArray );
      const t_rr_node& vpr_rrNode = pvpr_rrNodeArray[rrIndex];
      if(( vpr_rrNode.type != CHANX ) && ( vpr_rrNode.type != CHANY ))
         continue;

      TGO_Point_c vpr_channelOrigin( vpr_rrNode.xlow, vpr_rrNode.ylow );
      TGO_OrientMode_t channelOrient = ( vpr_rrNode.type == CHANX ?
                                         TGO_ORIENT_HORIZONTAL : TGO_ORIENT_VERTICAL );
      int vpr_trackIndex = vpr_rrNode.ptc_num;

      TFH_ConnectionMap_c connectionMap( vpr_channelOrigin, channelOrient, vpr_trackIndex );
      if( pconnectionMapList->IsMember( connectionMap ))
      {
         size_t i = pconnectionMapList->FindIndex( connectionMap );
         (*pconnectionMapList)[i]->SetGraphIndex( rrIndex );
      }
   }
}

//===========================================================================//
// Method         : OverrideGraphOutputPinsToTracks_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_FabricConnectionBlockHandler_c::OverrideGraphOutputPinsToTracks_(
            void*                      vpr_rrNodeArray,
            int                        vpr_rrNodeCount,
      const TFH_ConnectionBlockList_t& connectionBlockList,
      const TFH_ConnectionMapList_t&   connectionMapList ) const
{
   bool ok = true;

   // First, disconnect all OPIN->CHANX|CHANY edges per connection blocks
   this->DisconnectGraphOutputPins_( vpr_rrNodeArray, vpr_rrNodeCount,
                                     connectionBlockList );

   // Then, connect all OPIN->CHANX|CHANY edges per connection blocks and maps
   ok = this->ConnectGraphOutputPins_( vpr_rrNodeArray, vpr_rrNodeCount,
                                       connectionBlockList, connectionMapList );
   return( ok );
}

//===========================================================================//
// Method         : OverrideGraphTracksToInputPins_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_FabricConnectionBlockHandler_c::OverrideGraphTracksToInputPins_(
            void*                      vpr_rrNodeArray,
            int                        vpr_rrNodeCount,
      const TFH_ConnectionBlockList_t& connectionBlockList,
      const TFH_ConnectionMapList_t&   connectionMapList ) const
{
   bool ok = true;

   // First, disconnect all CHANX|CHANY->IPIN edges per given connection maps
   this->DisconnectGraphInputPins_( vpr_rrNodeArray, vpr_rrNodeCount,
                                    connectionBlockList );

   // Then, connect all CHANX|CHANY->IPIN edges per given connection blocks
   ok = this->ConnectGraphInputPins_( vpr_rrNodeArray, vpr_rrNodeCount,
                                      connectionBlockList, connectionMapList );
   return( ok );
}

//===========================================================================//
// Method         : ConnectGraphOutputPins_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_FabricConnectionBlockHandler_c::ConnectGraphOutputPins_(
            void*                      vpr_rrNodeArray,
            int                        vpr_rrNodeCount,
      const TFH_ConnectionBlockList_t& connectionBlockList,
      const TFH_ConnectionMapList_t&   connectionMapList ) const
{
   bool ok= true;

   for( int from_rrIndex = 0; from_rrIndex < vpr_rrNodeCount; ++from_rrIndex )
   {
      t_rr_node* pvpr_rrNodeArray = static_cast< t_rr_node* >( vpr_rrNodeArray );
      t_rr_node* pfrom_rrNode = &pvpr_rrNodeArray[from_rrIndex];
      if( pfrom_rrNode->type != OPIN )
         continue;

      TGO_Point_c vpr_blockOrigin( pfrom_rrNode->xlow, pfrom_rrNode->ylow, pfrom_rrNode->z );
      int vpr_pinIndex = pfrom_rrNode->ptc_num;

      TFH_ConnectionBlock_c connectionBlock( vpr_blockOrigin, vpr_pinIndex );
      if( connectionBlockList.IsMember( connectionBlock ))
      {
         // Found matching OPIN in the given connection block list
         // now override "rr_graph" by adding new OPIN->CHANX|CHANY connections
         size_t i = connectionBlockList.FindIndex( connectionBlock );
         const TFH_ConnectionBlock_c* pconnectionBlock = connectionBlockList[i];

         vpr_blockOrigin = pconnectionBlock->GetBlockOrigin( );
         TC_SideMode_t pinSide = pconnectionBlock->GetPinSide( );

         TGO_Point_c vpr_channelOrigin;
         TGO_OrientMode_t channelOrient = TGO_ORIENT_UNDEFINED;
         this->FindChannelOrigin_( vpr_blockOrigin, pinSide, 
                                   &vpr_channelOrigin, &channelOrient );

         const TFH_BitPattern_t& connectionPattern = pconnectionBlock->GetConnectionPattern( );
         for( size_t j = 0; j < connectionPattern.GetLength(); ++j ) 
         {
            const TC_Bit_c& connectionBit = *connectionPattern[j];
            if( !connectionBit.IsTrue( ))
               continue;

            int vpr_trackIndex = static_cast< int >( j );
            TFH_ConnectionMap_c connectionMap( vpr_channelOrigin, channelOrient, vpr_trackIndex );

            size_t k = connectionMapList.FindIndex( connectionMap );
            const TFH_ConnectionMap_c* pconnectionMap = connectionMapList[k];
            int to_rrIndex = pconnectionMap->GetGraphIndex( );

            ok = this->AppendGraphNodeEdge_( pfrom_rrNode, to_rrIndex,
                                             this->outputPinSwitch_ );
            if( !ok )
               break;
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : DisconnectGraphOutputPins_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
void TFH_FabricConnectionBlockHandler_c::DisconnectGraphOutputPins_(
            void*                      vpr_rrNodeArray,
            int                        vpr_rrNodeCount,
      const TFH_ConnectionBlockList_t& connectionBlockList ) const
{
   for( int from_rrIndex = 0; from_rrIndex < vpr_rrNodeCount; ++from_rrIndex )
   {
      t_rr_node* pvpr_rrNodeArray = static_cast< t_rr_node* >( vpr_rrNodeArray );
      t_rr_node* pfrom_rrNode = &pvpr_rrNodeArray[from_rrIndex];
      if( pfrom_rrNode->type != OPIN )
         continue;

      TGO_Point_c vpr_blockOrigin( pfrom_rrNode->xlow, pfrom_rrNode->ylow, pfrom_rrNode->z );
      int vpr_pinIndex = pfrom_rrNode->ptc_num;

      TFH_ConnectionBlock_c connectionBlock( vpr_blockOrigin, vpr_pinIndex );
      if( connectionBlockList.IsMember( connectionBlock ))
      {
         // Found matching OPIN pin in given connection block list
         // now override "rr_graph" by deleting existing connections
         for( int j = 0; j < pfrom_rrNode->num_edges; ++j )
         {
            short* poutputPinSwitch = const_cast< short* >( &this->outputPinSwitch_ );
            bool deletedNode = this->DeleteGraphNodeEdge_( pfrom_rrNode, j,
                                                           poutputPinSwitch );
            if( deletedNode )
            {
               --j;
            }
         }
      }
   }
}

//===========================================================================//
// Method         : ConnectGraphInputPins_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_FabricConnectionBlockHandler_c::ConnectGraphInputPins_(
            void*                      vpr_rrNodeArray,
            int                        vpr_rrNodeCount,
      const TFH_ConnectionBlockList_t& connectionBlockList,
      const TFH_ConnectionMapList_t&   connectionMapList ) const
{
   bool ok = true;

   for( int to_rrIndex = 0; to_rrIndex < vpr_rrNodeCount; ++to_rrIndex )
   {
      t_rr_node* pvpr_rrNodeArray = static_cast< t_rr_node* >( vpr_rrNodeArray );
      t_rr_node* pto_rrNode = &pvpr_rrNodeArray[to_rrIndex];
      if( pto_rrNode->type != IPIN )
         continue;

      TGO_Point_c vpr_blockOrigin( pto_rrNode->xlow, pto_rrNode->ylow, pto_rrNode->z );
      int vpr_pinIndex = pto_rrNode->ptc_num;

      TFH_ConnectionBlock_c connectionBlock( vpr_blockOrigin, vpr_pinIndex );
      if( connectionBlockList.IsMember( connectionBlock ))
      {
         // Found matching IPIN in the given connection block list
         // now override "rr_graph" by adding new CHANX\CHANY->IPIN connections
         size_t i = connectionBlockList.FindIndex( connectionBlock );
         const TFH_ConnectionBlock_c* pconnectionBlock = connectionBlockList[i];

         vpr_blockOrigin = pconnectionBlock->GetBlockOrigin( );
         TC_SideMode_t pinSide = pconnectionBlock->GetPinSide( );

         TGO_Point_c vpr_channelOrigin;
         TGO_OrientMode_t channelOrient = TGO_ORIENT_UNDEFINED;
         this->FindChannelOrigin_( vpr_blockOrigin, pinSide, 
                                   &vpr_channelOrigin, &channelOrient );

         const TFH_BitPattern_t& connectionPattern = pconnectionBlock->GetConnectionPattern( );
         for( size_t j = 0; j < connectionPattern.GetLength(); ++j ) 
         {
            const TC_Bit_c& connectionBit = *connectionPattern[j];
            if( !connectionBit.IsTrue( ))
               continue;

            int vpr_trackIndex = static_cast< int >( j );
            TFH_ConnectionMap_c connectionMap( vpr_channelOrigin, channelOrient, vpr_trackIndex );

            size_t k = connectionMapList.FindIndex( connectionMap );
            const TFH_ConnectionMap_c* pconnectionMap = connectionMapList[k];
            int from_rrIndex = pconnectionMap->GetGraphIndex( );
            t_rr_node* pfrom_rrNode = &pvpr_rrNodeArray[from_rrIndex];

            ok = this->AppendGraphNodeEdge_( pfrom_rrNode, to_rrIndex,
                                             this->inputPinSwitch_ );
            if( !ok )
               break;
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : DisconnectGraphInputPins_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
void TFH_FabricConnectionBlockHandler_c::DisconnectGraphInputPins_(
            void*                      vpr_rrNodeArray,
            int                        vpr_rrNodeCount,
      const TFH_ConnectionBlockList_t& connectionBlockList ) const
{
   for( int from_rrIndex = 0; from_rrIndex < vpr_rrNodeCount; ++from_rrIndex )
   {
      t_rr_node* pvpr_rrNodeArray = static_cast< t_rr_node* >( vpr_rrNodeArray );
      t_rr_node* pfrom_rrNode = &pvpr_rrNodeArray[from_rrIndex];
      if(( pfrom_rrNode->type != CHANX ) && ( pfrom_rrNode->type != CHANY ))
         continue;

      for( int i = 0; i < pfrom_rrNode->num_edges; ++i )
      {
         int to_rrIndex = pfrom_rrNode->edges[i];
         if( to_rrIndex == -1 )
            continue;

         const t_rr_node& to_rrNode = pvpr_rrNodeArray[to_rrIndex];
         if( to_rrNode.type != IPIN )
            continue;

         TGO_Point_c vpr_blockOrigin( to_rrNode.xlow, to_rrNode.ylow, to_rrNode.z );
         int vpr_pinIndex = to_rrNode.ptc_num;

         TFH_ConnectionBlock_c connectionBlock( vpr_blockOrigin, vpr_pinIndex );
         if( connectionBlockList.IsMember( connectionBlock ))
         {
            short* pinputPinSwitch = const_cast< short* >( &this->inputPinSwitch_ );
            this->DeleteGraphNodeEdge_( pfrom_rrNode, i,
                                        pinputPinSwitch );
            --i; // Need to re-iterate this edge following a disconnect
         }
      }
   }
}

//===========================================================================//
// Method         : AppendGraphNodeEdge_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_FabricConnectionBlockHandler_c::AppendGraphNodeEdge_(
      void* vpr_rrNode,
      int   vpr_rrIndex,
      short ioPinSwitch ) const
{ 
   bool ok = true;

   t_rr_node* pvpr_rrNode = static_cast< t_rr_node* >( vpr_rrNode );
   pvpr_rrNode->num_edges += 1;

   int len = pvpr_rrNode->num_edges;

   pvpr_rrNode->edges = static_cast< int* >( TC_realloc( pvpr_rrNode->edges, len, sizeof( int )));
   if( !pvpr_rrNode->edges )
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.IsValidNew( pvpr_rrNode->edges, len * sizeof( int ), 
                                    "TFH_FabricConnectionBlockHandler_c::AppendGraphNodeEdge_" );
   }

   pvpr_rrNode->switches = static_cast< short* >( TC_realloc( pvpr_rrNode->switches, len, sizeof( short )));
   if( !pvpr_rrNode->switches )
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.IsValidNew( pvpr_rrNode->switches, len * sizeof( short ), 
                                    "TFH_FabricConnectionBlockHandler_c::AppendGraphNodeEdge_" );
   }

   if( ok )
   {
      pvpr_rrNode->edges[len-1] = vpr_rrIndex;
      pvpr_rrNode->switches[len-1] = ioPinSwitch;
   }
   return( ok );
}

//===========================================================================//
// Method         : DeleteGraphNodeEdge_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_FabricConnectionBlockHandler_c::DeleteGraphNodeEdge_(
      void*  vpr_rrNode,
      int    edgeIndex,
      short* pioPinSwitch ) const
{
   bool deletedNode = false;

   // Disconnect graph edge by deleting it from the node's edge list
   // (after copying last edge into this position to shrink allocated list)
   t_rr_node* pvpr_rrNode = static_cast< t_rr_node* >( vpr_rrNode );
   if( edgeIndex < pvpr_rrNode->num_edges )
   {
      int i = edgeIndex;
      int len = pvpr_rrNode->num_edges;

      if( pioPinSwitch )
      {
         *pioPinSwitch = pvpr_rrNode->switches[i];
      }

      pvpr_rrNode->num_edges -= 1;

      pvpr_rrNode->edges[i] = pvpr_rrNode->edges[len-1];
      pvpr_rrNode->edges[len-1] = -1;

      pvpr_rrNode->switches[i] = pvpr_rrNode->switches[len-1];
      pvpr_rrNode->switches[len-1] = -1;

      deletedNode = true;
   }
   return( deletedNode );
}

//===========================================================================//
// Method         : FindChannelOrigin_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
void TFH_FabricConnectionBlockHandler_c::FindChannelOrigin_(
      const TGO_Point_c&      vpr_blockOrigin,
            TC_SideMode_t     pinSide,
            TGO_Point_c*      pvpr_channelOrigin,
            TGO_OrientMode_t* pchannelOrient ) const
{
   *pvpr_channelOrigin = vpr_blockOrigin;

   switch( pinSide )
   {
   case TC_SIDE_LEFT:  pvpr_channelOrigin->Set( vpr_blockOrigin.x-1, vpr_blockOrigin.y ); break; 
   case TC_SIDE_RIGHT: pvpr_channelOrigin->Set( vpr_blockOrigin.x, vpr_blockOrigin.y );   break; 
   case TC_SIDE_LOWER: pvpr_channelOrigin->Set( vpr_blockOrigin.x, vpr_blockOrigin.y-1 ); break; 
   case TC_SIDE_UPPER: pvpr_channelOrigin->Set( vpr_blockOrigin.x, vpr_blockOrigin.y );   break; 
   default:                                                                               break;
   }

   if( pchannelOrient )
   {
   switch( pinSide )
   {
      case TC_SIDE_LEFT:
      case TC_SIDE_RIGHT: *pchannelOrient = TGO_ORIENT_VERTICAL;   break; 
      case TC_SIDE_LOWER: 
      case TC_SIDE_UPPER: *pchannelOrient = TGO_ORIENT_HORIZONTAL; break; 
      default:                                                     break;
      }
   }
}
