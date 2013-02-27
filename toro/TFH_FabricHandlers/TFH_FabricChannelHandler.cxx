//===========================================================================//
// Purpose : Method definitions for the TFH_FabricChannelHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - GetLength
//           - GetChannelWidthList
//           - At
//           - Clear
//           - Add
//           - IsValid
//
//           Protected methods include:
//           - TFH_FabricChannelHandler_c, ~TFH_FabricChannelHandler_c
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

#include "TFH_Typedefs.h"
#include "TFH_FabricChannelHandler.h"

// Initialize the channel widths handler "singleton" class, as needed
TFH_FabricChannelHandler_c* TFH_FabricChannelHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TFH_FabricChannelHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricChannelHandler_c::TFH_FabricChannelHandler_c( 
      void )
      :
      xChannelWidthList_( TFH_CHANNEL_WIDTH_LIST_DEF_CAPACITY ),
      yChannelWidthList_( TFH_CHANNEL_WIDTH_LIST_DEF_CAPACITY )
{
   pinstance_ = 0;
}

//===========================================================================//
// Method         : ~TFH_FabricChannelHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricChannelHandler_c::~TFH_FabricChannelHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricChannelHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TFH_FabricChannelHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricChannelHandler_c::DeleteInstance( 
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
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricChannelHandler_c& TFH_FabricChannelHandler_c::GetInstance(
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
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_FabricChannelHandler_c::HasInstance(
      void )
{
   return( pinstance_ ? true : false );
}

//===========================================================================//
// Method         : GetLength
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
size_t TFH_FabricChannelHandler_c::GetLength(
      TFH_SelectChannelMode_t selectChannel ) const
{
   size_t length = 0;

   switch( selectChannel )
   {
   case TFH_SELECT_CHANNEL_X: 
      length = this->xChannelWidthList_.GetLength( ); 
      break;
   case TFH_SELECT_CHANNEL_Y:
      length = this->yChannelWidthList_.GetLength( ); 
      break;
   default:
      break;
   }
   return( length );
}

//===========================================================================//
// Method         : GetChannelWidthList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
const TFH_ChannelWidthList_t* TFH_FabricChannelHandler_c::GetChannelWidthList(
      TFH_SelectChannelMode_t selectChannel ) const
{
   const TFH_ChannelWidthList_t* pchannelWidthList = 0;

   switch( selectChannel )
   {
   case TFH_SELECT_CHANNEL_X: 
      pchannelWidthList = &this->xChannelWidthList_;
      break;
   case TFH_SELECT_CHANNEL_Y:
      pchannelWidthList = &this->yChannelWidthList_;
      break;
   default:
      break;
   }
   return( pchannelWidthList );
}

//===========================================================================//
// Method         : At
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
const TFH_ChannelWidth_t* TFH_FabricChannelHandler_c::At( 
      TFH_SelectChannelMode_t selectChannel,
      size_t                  i ) const
{
   const TFH_ChannelWidth_t* pchannelWidth = 0;

   switch( selectChannel )
   {
   case TFH_SELECT_CHANNEL_X: 
      pchannelWidth = this->xChannelWidthList_.At( i );
      break;
   case TFH_SELECT_CHANNEL_Y:
      pchannelWidth = this->yChannelWidthList_.At( i );
      break;
   default:
      break;
   }
   return( pchannelWidth );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricChannelHandler_c::Clear( 
      TFH_SelectChannelMode_t selectChannel )
{
   switch( selectChannel )
   {
   case TFH_SELECT_CHANNEL_X: 
      this->xChannelWidthList_.Clear( );
      break;
   case TFH_SELECT_CHANNEL_Y:
      this->yChannelWidthList_.Clear( );
      break;
   default:
      this->xChannelWidthList_.Clear( );
      this->yChannelWidthList_.Clear( );
      break;
   }
}

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricChannelHandler_c::Add( 
      TFH_SelectChannelMode_t selectChannel,
      int                     index, 
      size_t                  count )
{
   TFH_ChannelWidth_t channelWidth( index, count );
   this->Add( selectChannel, channelWidth );
}

//===========================================================================//
void TFH_FabricChannelHandler_c::Add( 
            TFH_SelectChannelMode_t selectChannel,
      const TFH_ChannelWidth_t&     channelWidth )
{
   switch( selectChannel )
   {
   case TFH_SELECT_CHANNEL_X: 
      this->xChannelWidthList_.Add( channelWidth );
      break;
   case TFH_SELECT_CHANNEL_Y:
      this->yChannelWidthList_.Add( channelWidth );
      break;
   default:
      break;
   }
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_FabricChannelHandler_c::IsValid(
      void ) const
{
   return( this->xChannelWidthList_.IsValid( ) && 
           this->yChannelWidthList_.IsValid( ) ?
           true : false );
}
