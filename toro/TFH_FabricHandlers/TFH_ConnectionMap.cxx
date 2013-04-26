//===========================================================================//
// Purpose : Method definitions for the TFH_ConnectionMap class.
//
//           Public methods include:
//           - TFH_ConnectionMap_c, ~TFH_ConnectionMap_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//
//===========================================================================//

#include "TC_StringUtils.h"

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

#include "TFH_StringUtils.h"
#include "TFH_ConnectionMap.h"

//===========================================================================//
// Method         : TFH_ConnectionMap_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
TFH_ConnectionMap_c::TFH_ConnectionMap_c( 
      void )
      : 
      channelOrient_( TGO_ORIENT_UNDEFINED ),
      vpr_trackIndex_( -1 ),
      vpr_pinIndex_( -1 ),
      vpr_rrIndex_( -1 )
{
} 

//===========================================================================//
TFH_ConnectionMap_c::TFH_ConnectionMap_c( 
      const TGO_Point_c&  vpr_channelOrigin,
            TGO_OrientMode_t channelOrient,
            int           vpr_trackIndex )
      :
      vpr_channelOrigin_( vpr_channelOrigin ),
      channelOrient_( channelOrient ),
      vpr_trackIndex_( vpr_trackIndex ),
      vpr_pinIndex_( -1 ),
      vpr_rrIndex_( -1 )
{
} 

//===========================================================================//
TFH_ConnectionMap_c::TFH_ConnectionMap_c( 
      const TGO_Point_c&     vpr_channelOrigin,
            TGO_OrientMode_t channelOrient,
            int              vpr_trackIndex,
      const TGO_Point_c&     vpr_blockOrigin,
            int              vpr_pinIndex,
            int              vpr_rrIndex )
      :
      vpr_channelOrigin_( vpr_channelOrigin ),
      channelOrient_( channelOrient ),
      vpr_trackIndex_( vpr_trackIndex ),
      vpr_blockOrigin_( vpr_blockOrigin ),
      vpr_pinIndex_( vpr_pinIndex ),
      vpr_rrIndex_( vpr_rrIndex )
{
} 

//===========================================================================//
TFH_ConnectionMap_c::TFH_ConnectionMap_c( 
      const TFH_ConnectionMap_c& connectionMap )
      :
      vpr_channelOrigin_( connectionMap.vpr_channelOrigin_ ),
      channelOrient_( connectionMap.channelOrient_ ),
      vpr_trackIndex_( connectionMap.vpr_trackIndex_ ),
      vpr_blockOrigin_( connectionMap.vpr_blockOrigin_ ),
      vpr_pinIndex_( connectionMap.vpr_pinIndex_ ),
      vpr_rrIndex_( connectionMap.vpr_rrIndex_ )
{
} 

//===========================================================================//
// Method         : ~TFH_ConnectionMap_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
TFH_ConnectionMap_c::~TFH_ConnectionMap_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
TFH_ConnectionMap_c& TFH_ConnectionMap_c::operator=( 
      const TFH_ConnectionMap_c& connectionMap )
{
   if( &connectionMap != this )
   {
      this->vpr_channelOrigin_ = connectionMap.vpr_channelOrigin_;
      this->channelOrient_ = connectionMap.channelOrient_;
      this->vpr_trackIndex_ = connectionMap.vpr_trackIndex_;
      this->vpr_blockOrigin_ = connectionMap.vpr_blockOrigin_;
      this->vpr_pinIndex_ = connectionMap.vpr_pinIndex_;
      this->vpr_rrIndex_ = connectionMap.vpr_rrIndex_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_ConnectionMap_c::operator<( 
      const TFH_ConnectionMap_c& connectionMap ) const
{
   bool isLessThan = false;
   if( this->vpr_channelOrigin_.operator<( connectionMap.vpr_channelOrigin_ ))
   {
      isLessThan = true;
   }
   else if( this->vpr_channelOrigin_ == connectionMap.vpr_channelOrigin_ )
   {
      if( this->channelOrient_ < connectionMap.channelOrient_ )
      {
         isLessThan = true;
      }
      else if( this->channelOrient_ == connectionMap.channelOrient_ )
      {
         if( this->vpr_trackIndex_ < connectionMap.vpr_trackIndex_ )
         {
            isLessThan = true;
         }
      }
   }
   return( isLessThan );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_ConnectionMap_c::operator==( 
      const TFH_ConnectionMap_c& connectionMap ) const
{
   return(( this->vpr_channelOrigin_ == connectionMap.vpr_channelOrigin_ ) &&
          ( this->channelOrient_ == connectionMap.channelOrient_ ) &&
          ( this->vpr_trackIndex_ == connectionMap.vpr_trackIndex_ ) &&
          ( this->vpr_blockOrigin_ == connectionMap.vpr_blockOrigin_ ) &&
          ( this->vpr_pinIndex_ == connectionMap.vpr_pinIndex_ ) &&
          ( this->vpr_rrIndex_ == connectionMap.vpr_rrIndex_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_ConnectionMap_c::operator!=( 
      const TFH_ConnectionMap_c& connectionMap ) const
{
   return( !this->operator==( connectionMap ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
void TFH_ConnectionMap_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<connection_map" ); 

   if( this->vpr_channelOrigin_.IsValid( ))
   {
      string srChannelOrigin;
      this->vpr_channelOrigin_.ExtractString( &srChannelOrigin );
      printHandler.Write( pfile, 0, " channel_origin=(%s)", TIO_SR_STR( srChannelOrigin ));
   }
   if( this->channelOrient_ != TGO_ORIENT_UNDEFINED )
   {
      const char* pszChannelOrient = "?";
      switch( this->channelOrient_ )
      {
      case TGO_ORIENT_HORIZONTAL: pszChannelOrient = "H"; break;
      case TGO_ORIENT_VERTICAL:   pszChannelOrient = "V"; break;
      default:                                            break;
      }
      printHandler.Write( pfile, 0, " channel_orient=\"%s\"", TIO_PSZ_STR( pszChannelOrient ));
   }
   if( this->vpr_trackIndex_ != -1 )
   {
      printHandler.Write( pfile, 0, " track_index=\"%d\"", this->vpr_trackIndex_ );
   }
   if( this->vpr_blockOrigin_.IsValid( ))
   {
      string srBlockOrigin;
      this->vpr_blockOrigin_.ExtractString( &srBlockOrigin );
      printHandler.Write( pfile, 0, " block_origin=(%s)", TIO_SR_STR( srBlockOrigin ));
   }
   if( this->vpr_pinIndex_ != -1 )
   {
      printHandler.Write( pfile, 0, " pin_index=\"%d\"", this->vpr_pinIndex_ );
   }
   if( this->vpr_rrIndex_ != -1 )
   {
      printHandler.Write( pfile, 0, " rr_index=\"%d\"", this->vpr_rrIndex_ );
   }
   printHandler.Write( pfile, 0, ">\n" );
} 
