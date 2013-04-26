//===========================================================================//
// Purpose : Method definitions for the TFV_FabricPin class.
//
//           Public methods include:
//           - TFV_FabricPin_c, ~TFV_FabricPin_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
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

#include "TC_MinGrid.h"
#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TFV_FabricPin.h"

//===========================================================================//
// Method         : TFV_FabricPin_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
TFV_FabricPin_c::TFV_FabricPin_c( 
      void )
      :
      type_( TC_TYPE_UNDEFINED ),
      side_( TC_SIDE_UNDEFINED ),
      offset_( 0 ),
      delta_( 0.0 ),
      width_( 0.0 ),
      slice_( 0 ),
      channelCount_( 0 ),
      connectionList_( TFV_CONNECTION_LIST_DEF_CAPACITY )
{
} 

//===========================================================================//
TFV_FabricPin_c::TFV_FabricPin_c( 
      const string&       srName,
            TC_TypeMode_t type,
            TC_SideMode_t side,
            int           offset,
            double        delta,
            double        width,
            unsigned int  slice )
      :
      srName_( srName ),
      type_( type ),
      side_( side ),
      offset_( offset ),
      delta_( delta ),
      width_( width ),
      slice_( slice ),
      channelCount_( 0 ),
      connectionList_( TFV_CONNECTION_LIST_DEF_CAPACITY )
{
} 

//===========================================================================//
TFV_FabricPin_c::TFV_FabricPin_c( 
      const char*         pszName,
            TC_TypeMode_t type,
            TC_SideMode_t side,
            int           offset,
            double        delta,
            double        width,
            unsigned int  slice )
      :
      srName_( TIO_PSZ_STR( pszName )),
      type_( type ),
      side_( side ),
      offset_( offset ),
      delta_( delta ),
      width_( width ),
      slice_( slice ),
      channelCount_( 0 ),
      connectionList_( TFV_CONNECTION_LIST_DEF_CAPACITY )
{
} 

//===========================================================================//
TFV_FabricPin_c::TFV_FabricPin_c( 
      const string&       srName,
            unsigned int  slice,
            TC_SideMode_t side )
      :
      srName_( srName ),
      type_( TC_TYPE_UNDEFINED ),
      side_( side ),
      offset_( 0 ),
      delta_( 0.0 ),
      width_( 0.0 ),
      slice_( slice ),
      channelCount_( 0 ),
      connectionList_( TFV_CONNECTION_LIST_DEF_CAPACITY )
{
} 

//===========================================================================//
TFV_FabricPin_c::TFV_FabricPin_c( 
      const char*         pszName,
            unsigned int  slice,
            TC_SideMode_t side )
      :
      srName_( TIO_PSZ_STR( pszName )),
      type_( TC_TYPE_UNDEFINED ),
      side_( side ),
      offset_( 0 ),
      delta_( 0.0 ),
      width_( 0.0 ),
      slice_( slice ),
      channelCount_( 0 ),
      connectionList_( TFV_CONNECTION_LIST_DEF_CAPACITY )
{
} 

//===========================================================================//
TFV_FabricPin_c::TFV_FabricPin_c( 
      const TFV_FabricPin_c& fabricPin )
      :
      srName_( fabricPin.srName_ ),
      type_( fabricPin.type_ ),
      side_( fabricPin.side_ ),
      offset_( fabricPin.offset_ ),
      delta_( fabricPin.delta_ ),
      width_( fabricPin.width_ ),
      slice_( fabricPin.slice_ ),
      channelCount_( fabricPin.channelCount_ ),
      connectionList_( fabricPin.connectionList_ )
{
} 

//===========================================================================//
// Method         : ~TFV_FabricPin_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
TFV_FabricPin_c::~TFV_FabricPin_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
TFV_FabricPin_c& TFV_FabricPin_c::operator=( 
      const TFV_FabricPin_c& fabricPin )
{
   if( &fabricPin != this )
   {
      this->srName_ = fabricPin.srName_;
      this->type_ = fabricPin.type_;
      this->side_ = fabricPin.side_;
      this->offset_ = fabricPin.offset_;
      this->delta_ = fabricPin.delta_;
      this->width_ = fabricPin.width_;
      this->slice_ = fabricPin.slice_;
      this->channelCount_ = fabricPin.channelCount_;
      this->connectionList_ = fabricPin.connectionList_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
bool TFV_FabricPin_c::operator<( 
      const TFV_FabricPin_c& fabricPin ) const
{
   bool isLessThan = false;

   if( TC_CompareStrings( this->srName_, fabricPin.srName_ ) < 0 )
   {
      isLessThan = true;
   }
   else if( TC_CompareStrings( this->srName_, fabricPin.srName_ ) == 0 )
   {
      if( this->slice_ < fabricPin.slice_ )
      {
         isLessThan = true;
      }
      else if( this->slice_ == fabricPin.slice_ )
      {
         if( this->side_ < fabricPin.side_ )
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
// 08/30/12 jeffr : Original
//===========================================================================//
bool TFV_FabricPin_c::operator==( 
      const TFV_FabricPin_c& fabricPin ) const
{
   return(( this->srName_ == fabricPin.srName_ ) &&
          ( this->slice_ == fabricPin.slice_ ) &&
          ( this->side_ == fabricPin.side_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
bool TFV_FabricPin_c::operator!=( 
      const TFV_FabricPin_c& fabricPin ) const
{
   return( !this->operator==( fabricPin ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TFV_FabricPin_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   string srType;
   TC_ExtractStringTypeMode( this->type_, &srType );

   string srSide;
   TC_ExtractStringSideMode( this->side_, &srSide );

   printHandler.Write( pfile, spaceLen, "\"%s\" %s [%s %d %0.*f %0.*f %u]",
                                        TIO_SR_STR( this->srName_ ),
                                        TIO_SR_STR( srType ),
                                        TIO_SR_STR( srSide ),
                                        this->offset_,
                                        precision, this->delta_,
                                        precision, this->width_,
                                        this->slice_ );
   if( this->connectionList_.IsValid( ))
   {
      printHandler.Write( pfile, 0, " ->" );
      for( size_t i = 0; i < this->connectionList_.GetLength( ); ++i )
      {
         const TFV_Connection_t& connection = *this->connectionList_[i];

         const char* pszSide = "?";
         switch( connection.GetSide( ))
         {
         case TC_SIDE_LEFT:  pszSide = "L"; break;
         case TC_SIDE_RIGHT: pszSide = "R"; break;
         case TC_SIDE_LOWER: pszSide = "B"; break;
         case TC_SIDE_UPPER: pszSide = "T"; break;
         default:                           break;
         }
         printHandler.Write( pfile, 0, " (%s %d)",
                                       TIO_PSZ_STR( pszSide ),
                                       connection.GetIndex( ));
      }
   }
   printHandler.Write( pfile, 0, "\n" );
}
