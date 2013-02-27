//===========================================================================//
// Purpose : Declaration and inline definitions for the universal 
//           TFH_FabricChannelHandler singleton class. This class supports
//           channel width overrides for the VPR placement tool
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

#ifndef TFH_FABRIC_CHANNEL_HANDLER_H
#define TFH_FABRIC_CHANNEL_HANDLER_H

#include <cstdio>
using namespace std;

#include "TC_IndexCount.h"

#include "TFH_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TFH_FabricChannelHandler_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TFH_FabricChannelHandler_c& GetInstance( bool newInstance = true );
   static bool HasInstance( void );

   size_t GetLength( TFH_SelectChannelMode_t selectChannel ) const;
   const TFH_ChannelWidthList_t* GetChannelWidthList( TFH_SelectChannelMode_t selectChannel ) const;

   const TFH_ChannelWidth_t* At( TFH_SelectChannelMode_t selectChannel,
                                 size_t i ) const;

   void Clear( TFH_SelectChannelMode_t selectChannel = TFH_SELECT_CHANNEL_UNDEFINED );

   void Add( TFH_SelectChannelMode_t selectChannel,
             int index, 
             size_t count );
   void Add( TFH_SelectChannelMode_t selectChannel,
             const TFH_ChannelWidth_t& channelWidth );

   bool IsValid( void ) const;

protected:

   TFH_FabricChannelHandler_c( void );
   ~TFH_FabricChannelHandler_c( void );

private:

   TFH_ChannelWidthList_t xChannelWidthList_;
   TFH_ChannelWidthList_t yChannelWidthList_;

   static TFH_FabricChannelHandler_c* pinstance_; // Define ptr for singleton instance

private:

   enum TFH_DefCapacity_e 
   { 
      TFH_CHANNEL_WIDTH_LIST_DEF_CAPACITY = 1
   };
};

#endif
