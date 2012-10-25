//===========================================================================//
//Purpose : Method definitions for the TNO_Segment class.
//
//           Public methods include:
//           - TNO_Segment_c, ~TNO_Segment_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Clear
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#include "TIO_PrintHandler.h"

#include "TC_StringUtils.h"

#include "TNO_Segment.h"

//===========================================================================//
// Method         : TNO_Segment_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_Segment_c::TNO_Segment_c( 
      void )
      :
      track_( 0 )
{
} 

//===========================================================================//
TNO_Segment_c::TNO_Segment_c( 
      const string& srName )
      :
      srName_( srName ),
      track_( 0 )
{
} 

//===========================================================================//
TNO_Segment_c::TNO_Segment_c( 
      const char* pszName )
      :
      srName_( TIO_PSZ_STR( pszName )),
      track_( 0 )
{
} 

//===========================================================================//
TNO_Segment_c::TNO_Segment_c( 
      const string&       srName,
      const TGS_Region_c& channel,
            unsigned int  track )
      :
      srName_( srName ),
      channel_( channel ),
      track_( track )
{
} 

//===========================================================================//
TNO_Segment_c::TNO_Segment_c( 
      const char*         pszName,
      const TGS_Region_c& channel,
            unsigned int  track )
      :
      srName_( TIO_PSZ_STR( pszName )),
      channel_( channel ),
      track_( track )
{
} 

//===========================================================================//
TNO_Segment_c::TNO_Segment_c( 
      const TNO_Segment_c& segment )
      :
      srName_( segment.srName_ ),
      channel_( segment.channel_ ),
      track_( segment.track_ )
{
} 

//===========================================================================//
// Method         : ~TNO_Segment_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_Segment_c::~TNO_Segment_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_Segment_c& TNO_Segment_c::operator=( 
      const TNO_Segment_c& segment )
{
   if( &segment != this )
   {
      this->srName_ = segment.srName_;
      this->channel_ = segment.channel_;
      this->track_ = segment.track_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_Segment_c::operator<( 
      const TNO_Segment_c& segment ) const
{
   return(( TC_CompareStrings( this->srName_, segment.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_Segment_c::operator==( 
      const TNO_Segment_c& segment ) const
{
   return(( this->srName_ == segment.srName_ ) &&
          ( this->channel_ == segment.channel_ ) &&
          ( this->track_ == segment.track_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_Segment_c::operator!=( 
      const TNO_Segment_c& segment ) const
{
   return( !this->operator==( segment ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Segment_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srSegment;
   this->ExtractString( &srSegment );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srSegment ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Segment_c::ExtractString( 
      string* psrSegment ) const
{
   if( psrSegment )
   {
      if( this->IsValid( ))
      {
         string srChannel;
         this->channel_.ExtractString( &srChannel );

         char szTrack[TIO_FORMAT_STRING_LEN_VALUE];
         sprintf( szTrack, "%u", this->track_ );

         *psrSegment = "\"";
         *psrSegment += this->srName_;
         *psrSegment += "\" ";
         *psrSegment += srChannel;
         *psrSegment += " ";
         *psrSegment += szTrack;
      }
      else
      {
         *psrSegment = "?";
      }
   }
} 

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Segment_c::Clear( 
      void )
{
   this->srName_ = "";
   this->channel_.Reset( );
   this->track_ = 0;
}
