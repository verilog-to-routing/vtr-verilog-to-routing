//===========================================================================//
// Purpose : Method definitions for a TGS_ArrayGrid_c class.
//
//           Public methods include:
//           - TGS_ArrayGrid, ~TGS_ArrayGrid
//           - operator=
//           - operator==, operator!=
//           - Init
//           - GetPitch, GetOffset
//           - FoundCount
//           - SnapToGrid
//           - SnapToPitch
//
//           Private methods include:
//           - SnapDims_
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

#include "TGS_ArrayGrid.h"

//===========================================================================//
// Method         : TGS_ArrayGrid_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
TGS_ArrayGrid_c::TGS_ArrayGrid_c( 
      void )
      :
      pitchDims_( 0.0, 0.0 ),
      offsetDims_( 0.0, 0.0 )
{
}

//===========================================================================//
TGS_ArrayGrid_c::TGS_ArrayGrid_c( 
      const TGS_FloatDims_t& pitchDims )
      :
      pitchDims_( 0.0, 0.0 ),
      offsetDims_( 0.0, 0.0 )
{
   this->Init( pitchDims );
}

//===========================================================================//
TGS_ArrayGrid_c::TGS_ArrayGrid_c( 
      const TGS_FloatDims_t& pitchDims,
      const TGS_FloatDims_t& offsetDims )
      :
      pitchDims_( 0.0, 0.0 ),
      offsetDims_( 0.0, 0.0 )
{
   this->Init( pitchDims, offsetDims );
}

//===========================================================================//
TGS_ArrayGrid_c::TGS_ArrayGrid_c( 
      double xPitch, 
      double yPitch )
      :
      pitchDims_( 0.0, 0.0 ),
      offsetDims_( 0.0, 0.0 )
{
   this->Init( xPitch, yPitch );
}

//===========================================================================//
TGS_ArrayGrid_c::TGS_ArrayGrid_c( 
      double xPitch, 
      double yPitch,
      double xOffset, 
      double yOffset )
      :
      pitchDims_( 0.0, 0.0 ),
      offsetDims_( 0.0, 0.0 )
{
   this->Init( xPitch, yPitch, xOffset, yOffset );
}

//===========================================================================//
// Method         : ~TGS_ArrayGrid_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
TGS_ArrayGrid_c::~TGS_ArrayGrid_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
TGS_ArrayGrid_c& TGS_ArrayGrid_c::operator=( 
      const TGS_ArrayGrid_c& arrayGrid )
{
   if( &arrayGrid != this )
   {
      this->pitchDims_ = arrayGrid.pitchDims_;
      this->offsetDims_ = arrayGrid.offsetDims_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
bool TGS_ArrayGrid_c::operator==( 
      const TGS_ArrayGrid_c& arrayGrid ) const
{
   return(( this->pitchDims_ == arrayGrid.pitchDims_ ) &&
          ( this->offsetDims_ == arrayGrid.offsetDims_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
bool TGS_ArrayGrid_c::operator!=( 
      const TGS_ArrayGrid_c& arrayGrid ) const
{
   return(( !this->operator==( arrayGrid )) ? 
          true : false );
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
void TGS_ArrayGrid_c::Init( 
      const TGS_FloatDims_t& pitchDims )
{
  this->pitchDims_.width = pitchDims.width;
  this->pitchDims_.height = pitchDims.height;

  this->offsetDims_.width = 0.0;
  this->offsetDims_.height = 0.0;
}

//===========================================================================//
void TGS_ArrayGrid_c::Init( 
      const TGS_FloatDims_t& pitchDims,
      const TGS_FloatDims_t& offsetDims )
{
  this->pitchDims_.width = pitchDims.width;
  this->pitchDims_.height = pitchDims.height;

  this->offsetDims_.width = fmod( offsetDims.width, pitchDims.width );
  this->offsetDims_.height = fmod( offsetDims.height, pitchDims.height );
}

//===========================================================================//
void TGS_ArrayGrid_c::Init( 
      double xPitch, 
      double yPitch )
{
  this->pitchDims_.width = xPitch;
  this->pitchDims_.height = yPitch;

  this->offsetDims_.width = 0.0;
  this->offsetDims_.height = 0.0;
}

//===========================================================================//
void TGS_ArrayGrid_c::Init( 
      double xPitch, 
      double yPitch,
      double xOffset, 
      double yOffset )
{
  this->pitchDims_.width = xPitch;
  this->pitchDims_.height = yPitch;

  this->offsetDims_.width = fmod( xOffset, xPitch );
  this->offsetDims_.height = fmod( yOffset, yPitch );
}

//===========================================================================//
// Method         : GetPitch
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
void TGS_ArrayGrid_c::GetPitch( 
      TGS_FloatDims_t* ppitchDims ) const
{
   if( ppitchDims )
   {
      *ppitchDims = this->pitchDims_;
   }
}

//===========================================================================//
void TGS_ArrayGrid_c::GetPitch( 
      double* pxPitch, 
      double* pyPitch ) const
{
   if( pxPitch && pyPitch )
   {
      *pxPitch = this->pitchDims_.width;
      *pyPitch = this->pitchDims_.height;
   }
}

//===========================================================================//
// Method         : GetOffset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
void TGS_ArrayGrid_c::GetOffset( 
      TGS_FloatDims_t* poffsetDims ) const
{
   if( poffsetDims )
   {
      *poffsetDims = this->offsetDims_;
   }
}

//===========================================================================//
void TGS_ArrayGrid_c::GetOffset( 
      double* pxOffset, 
      double* pyOffset ) const
{
   if( pxOffset && pyOffset )
   {
      *pxOffset = this->offsetDims_.width;
      *pyOffset = this->offsetDims_.height;
   }
}

//===========================================================================//
// Method         : FindCount
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
void TGS_ArrayGrid_c::FindCount(
      const TGS_Region_c&       region,
            TGS_IntDims_t*      pcount,
            TGS_ArraySnapMode_t snapMode ) const
{
   TGS_IntDims_t count;
   this->FindCount( region, 
                    &count.width, 
                    &count.height,
                    snapMode );
   if( pcount )
   {
      *pcount = count;
   }
}

//===========================================================================//
void TGS_ArrayGrid_c::FindCount(
      const TGS_Region_c&       region,
            int*                pxCount,
            int*                pyCount,
            TGS_ArraySnapMode_t snapMode ) const
{
   // First, snap given region to nearest grid based on the given region
   TGS_Region_c region_;
   this->SnapToGrid( region, &region_, snapMode );

   // Second, compute counts based on region dims and pitch values
   double xCount = 0.0;
   double yCount = 0.0;

   if( region_.IsValid( ))
   {
      double xPitch = this->pitchDims_.width;
      double yPitch = this->pitchDims_.height;

      xCount = ( TCTF_IsGT( xPitch, 0.0 ) ?
                 ( region_.GetDx( ) + TC_FLT_EPSILON ) / xPitch + 1.0:
                 0.0 );
      yCount = ( TCTF_IsGT( yPitch, 0.0 ) ?
                 ( region_.GetDy( ) + TC_FLT_EPSILON ) / yPitch + 1.0:
                 0.0 );
   }

   if( pxCount && pyCount )
   {
      *pxCount = static_cast< int >( xCount );
      *pyCount = static_cast< int >( yCount );
   }
}

//===========================================================================//
// Method         : SnapToGrid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
void TGS_ArrayGrid_c::SnapToGrid( 
      const TGS_Point_c&     point,
            TGS_Point_c*     ppoint,
            TGS_CornerMode_t cornerMode ) const
{
   double x = this->SnapDims_( point.x, 
                               this->pitchDims_.width, 
                               this->offsetDims_.width );
   double y = this->SnapDims_( point.y, 
                               this->pitchDims_.height, 
                               this->offsetDims_.height );
   switch( cornerMode )
   {
   case TGS_CORNER_LOWER_LEFT: 
   case TGS_CORNER_UPPER_LEFT:
   
      if( TCTF_IsGT( x, point.x ))     // Snapped point to nearest right
         x -= this->pitchDims_.width;  // Apply pitch to snap left instead
      break;
   
   case TGS_CORNER_LOWER_RIGHT:
   case TGS_CORNER_UPPER_RIGHT:
   
      if( TCTF_IsLT( x, point.x ))     // Snapped point to nearest left
         x += this->pitchDims_.width;  // Apply pitch to snap right instead
      break;
   
   default:
      break;
   }

   switch( cornerMode )
   {
   case TGS_CORNER_LOWER_LEFT: 
   case TGS_CORNER_LOWER_RIGHT:
   
      if( TCTF_IsGT( y, point.y ))     // Snapped point to nearest upper
         y -= this->pitchDims_.height; // Apply pitch to snap lower instead
      break;
   
   case TGS_CORNER_UPPER_LEFT:
   case TGS_CORNER_UPPER_RIGHT:
   
      if( TCTF_IsLT( y, point.y ))     // Snapped point to nearest lower
         y += this->pitchDims_.height; // Apply pitch to snap upperinstead;
      break;
   
   default:
      break;
   }

   if( ppoint )
   {
      ppoint->Set( x, y );
   }
}

//===========================================================================//
void TGS_ArrayGrid_c::SnapToGrid( 
      const TGS_Region_c&       region,
            TGS_Region_c*       pregion,
            TGS_ArraySnapMode_t snapMode ) const
{
   double x1 = this->SnapDims_( region.x1, 
                                this->pitchDims_.width, 
                                this->offsetDims_.width );
   double y1 = this->SnapDims_( region.y1, 
                                this->pitchDims_.height, 
                                this->offsetDims_.height );
   double x2 = this->SnapDims_( region.x2, 
                                this->pitchDims_.width, 
                                this->offsetDims_.width );
   double y2 = this->SnapDims_( region.y2, 
                                this->pitchDims_.height, 
                                this->offsetDims_.height );

   if( snapMode == TGS_ARRAY_SNAP_WITHIN )
   {
      x1 += ( TCTF_IsLT( x1, region.x1 ) ? this->pitchDims_.width : 0.0 );
      y1 += ( TCTF_IsLT( y1, region.y1 ) ? this->pitchDims_.height : 0.0 );
      x2 -= ( TCTF_IsGT( x2, region.x2 ) ? this->pitchDims_.width : 0.0 );
      y2 -= ( TCTF_IsGT( y2, region.y2 ) ? this->pitchDims_.height : 0.0 );
   }

   if( pregion )
   {
      if( TCTF_IsLE( x1, x2 ) && 
          TCTF_IsLE( y1, y2 ))
      {
         pregion->Set( x1, y1, x2, y2 );
      }
      else
      {
         pregion->Reset( );
      }
   }
}

//===========================================================================//
void TGS_ArrayGrid_c::SnapToGrid( 
      const TGS_Region_c&       region,
      const TGS_Point_c&        point,
            TGS_Point_c*        ppoint,
            TGS_ArraySnapMode_t snapMode ) const
{
   int z = point.z;
   double x = this->SnapDims_( point.x, 
                               this->pitchDims_.width, 
                               this->offsetDims_.width );
   double y = this->SnapDims_( point.y, 
                               this->pitchDims_.height, 
                               this->offsetDims_.height );

   if( snapMode == TGS_ARRAY_SNAP_WITHIN )
   {
      x += ( TCTF_IsLT( x, region.x1 ) ? this->pitchDims_.width : 0.0 );
      y += ( TCTF_IsLT( y, region.y1 ) ? this->pitchDims_.height : 0.0 );
      x -= ( TCTF_IsGT( x, region.x2 ) ? this->pitchDims_.width : 0.0 );
      y -= ( TCTF_IsGT( y, region.y2 ) ? this->pitchDims_.height : 0.0 );
   }

   if( ppoint )
   {
      if( TCTF_IsGE( x, region.x1 ) && TCTF_IsLE( x, region.x2 ) &&
          TCTF_IsGE( y, region.y1 ) && TCTF_IsLE( y, region.y2 ))
      {
         ppoint->Set( x, y, z );
      }
      else
      {
         ppoint->Reset( );
      }
   }
}

//===========================================================================//
// Method         : SnapToPitch
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
void TGS_ArrayGrid_c::SnapToPitch( 
      const TGS_Point_c& point,
            TGS_Point_c* ppoint ) const
{
   double x = this->SnapDims_( point.x, 
                               this->pitchDims_.width );
   double y = this->SnapDims_( point.y, 
                               this->pitchDims_.height );
   if( ppoint )
   {
      ppoint->Set( x, y );
   }
}

//===========================================================================//
void TGS_ArrayGrid_c::SnapToPitch( 
      const TGS_Region_c&       region,
            TGS_Region_c*       pregion,
            TGS_ArraySnapMode_t snapMode ) const
{
   double x1 = this->SnapDims_( region.x1, 
                                this->pitchDims_.width );
   double y1 = this->SnapDims_( region.y1, 
                                this->pitchDims_.height );
   double x2 = this->SnapDims_( region.x2, 
                                this->pitchDims_.width );
   double y2 = this->SnapDims_( region.y2, 
                                this->pitchDims_.height );

   if( snapMode == TGS_ARRAY_SNAP_WITHIN )
   {
      x1 += ( TCTF_IsLT( x1, region.x1 ) ? this->pitchDims_.width : 0.0 );
      y1 += ( TCTF_IsLT( y1, region.y1 ) ? this->pitchDims_.height : 0.0 );
      x2 -= ( TCTF_IsGT( x2, region.x2 ) ? this->pitchDims_.width : 0.0 );
      y2 -= ( TCTF_IsGT( y2, region.y2 ) ? this->pitchDims_.height : 0.0 );
   }

   if( pregion )
   {
      if( TCTF_IsLE( x1, x2 ) && 
          TCTF_IsLE( y1, y2 ))
      {
         pregion->Set( x1, y1, x2, y2 );
      }
      else
      {
         pregion->Reset( );
      }
   }
}

//===========================================================================//
// Method         : SnapDims_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
double TGS_ArrayGrid_c::SnapDims_( 
      double f, 
      double pitch,
      double offset ) const
{
   if( TCTF_IsGT( pitch, 0.0 ))
   { 
      f = floor(( f - offset ) / pitch + TC_FLT_EPSILON ) * pitch + offset;
   }
   return( f );
}
