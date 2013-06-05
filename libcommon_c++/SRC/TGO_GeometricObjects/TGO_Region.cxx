//===========================================================================//
// Purpose : Method definitions for the TGO_Region class.
//
//           Public methods include:
//           - TGO_Region_c, ~TGO_Region_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set, Reset
//           - FindMin, FindMax
//           - FindLowerLeft, FindLowerRight
//           - FindUpperLeft, FindUpperRight
//           - FindOrient
//           - FindWidth, FindLength
//           - FindDistance
//           - FindNearest
//           - ApplyScale
//           - ApplyUnion
//           - ApplyIntersect
//           - IsIntersecting
//           - IsOverlapping
//           - IsWithin
//           - IsWide, IsTall
//           - IsSquare
//           - IsLegal
//           - IsValid
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

#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TGO_Box.h"
#include "TGO_Region.h"

//===========================================================================//
// Method         : TGO_Region_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_Region_c::TGO_Region_c( 
      void )
      :
      x1( INT_MIN ),
      y1( INT_MIN ),
      x2( INT_MIN ),
      y2( INT_MIN )
{
}

//===========================================================================//
TGO_Region_c::TGO_Region_c( 
      int x1_, 
      int y1_,
      int x2_, 
      int y2_ )
{
   this->Set( x1_, y1_, x2_, y2_ );
}

//===========================================================================//
TGO_Region_c::TGO_Region_c(
      const TGO_Point_c& pointA,
      const TGO_Point_c& pointB )
{
   this->Set( pointA, pointB );
}

//===========================================================================//
TGO_Region_c::TGO_Region_c( 
      const TGO_Region_c& region )
{
   this->Set( region );
}

//===========================================================================//
// Method         : ~TGO_Region_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_Region_c::~TGO_Region_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_Region_c& TGO_Region_c::operator=( 
      const TGO_Region_c& region )
{
   if( &region != this )
   {
      this->x1 = region.x1;
      this->y1 = region.y1;
      this->x2 = region.x2;
      this->y2 = region.y2;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Region_c::operator<( 
      const TGO_Region_c& region ) const
{
   return(( this->x1 >= region.x1 ) &&
          ( this->y1 >= region.y1 ) &&
          ( this->x2 <= region.x2 ) &&
          ( this->y2 <= region.y2 ) ?
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Region_c::operator==( 
      const TGO_Region_c& region ) const
{
   return(( this->x1 == region.x1 ) &&
          ( this->y1 == region.y1 ) &&
          ( this->x2 == region.x2 ) &&
          ( this->y2 == region.y2 ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Region_c::operator!=( 
      const TGO_Region_c& region ) const
{
   return(( !this->operator==( region )) ? 
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Region_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srRegion;
   this->ExtractString( &srRegion );

   printHandler.Write( pfile, spaceLen, "[region] %s\n", TIO_SR_STR( srRegion ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Region_c::ExtractString( 
      string* psrRegion ) const
{
   if( psrRegion )
   {
      char szRegion[TIO_FORMAT_STRING_LEN_REGION];

      if( this->IsValid( ))
      {
         sprintf( szRegion, "%d %d %d %d", 
                            this->x1, this->y1, this->x2, this->y2 );
      }
      else
      {
         sprintf( szRegion, "? ? ? ?" );
      }
      *psrRegion = szRegion;
   }
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Region_c::Set( 
      int x1_, 
      int y1_,
      int x2_, 
      int y2_ )
{
   this->x1 = TCT_Min( x1_, x2_ );
   this->y1 = TCT_Min( y1_, y2_ );
   this->x2 = TCT_Max( x1_, x2_ );
   this->y2 = TCT_Max( y1_, y2_ );
}

//===========================================================================//
void TGO_Region_c::Set( 
      const TGO_Point_c& pointA,
      const TGO_Point_c& pointB )
{
   this->Set( pointA.x, pointA.y, pointB.x, pointB.y );
}

//===========================================================================//
void TGO_Region_c::Set( 
      const TGO_Region_c& region )
{
   this->Set( region.x1, region.y1, region.x2, region.y2 );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Region_c::Reset( 
      void )
{
   this->Set( INT_MIN, INT_MIN, INT_MIN, INT_MIN );
}

//===========================================================================//
// Method         : FindMin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
int TGO_Region_c::FindMin( 
      void ) const
{
   int dx = this->GetDx( );
   int dy = this->GetDy( );

   return( TCT_Min( dx, dy ));
}

//===========================================================================//
// Method         : FindMax
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
int TGO_Region_c::FindMax( 
      void ) const
{
   int dx = this->GetDx( );
   int dy = this->GetDy( );

   return( TCT_Max( dx, dy ));
}

//===========================================================================//
// Method         : FindLowerLeft
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_Point_c TGO_Region_c::FindLowerLeft( 
      void ) const
{
   TGO_Point_c lowerLeft( this->x1, this->y1 );

   return( lowerLeft );
}


//===========================================================================//
// Method         : FindLowerRight
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_Point_c TGO_Region_c::FindLowerRight( 
      void ) const
{
   TGO_Point_c lowerRight( this->x2, this->y1 );

   return( lowerRight );
}


//===========================================================================//
// Method         : FindUpperLeft
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_Point_c TGO_Region_c::FindUpperLeft( 
      void ) const
{
   TGO_Point_c upperLeft( this->x1, this->y2 );

   return( upperLeft );
}


//===========================================================================//
// Method         : FindUpperRight
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_Point_c TGO_Region_c::FindUpperRight( 
      void ) const
{
   TGO_Point_c upperRight( this->x2, this->y2 );

   return( upperRight );
}

//===========================================================================//
// Method         : FindOrient
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_OrientMode_t TGO_Region_c::FindOrient( 
      TGO_OrientMode_t orient ) const
{
   TGO_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   return( thisBox.FindOrient( orient ));
}

//===========================================================================//
// Method         : FindWidth
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
int TGO_Region_c::FindWidth( 
      TGO_OrientMode_t orient ) const
{
   TGO_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   return( thisBox.FindWidth( orient ));
}

//===========================================================================//
// Method         : FindLength
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
int TGO_Region_c::FindLength( 
      TGO_OrientMode_t orient ) const
{
   TGO_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   return( thisBox.FindLength( orient ));
}

//===========================================================================//
// Method         : FindDistance
// Purpose        : Compute and return the "flightline" distance between
//                  the nearest point on this region and the given point or 
//                  region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
double TGO_Region_c::FindDistance( 
      const TGO_Region_c& refRegion ) const
{
   TGO_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );
   TGO_Box_c refBox( refRegion.x1, refRegion.y1, refRegion.x2, refRegion.y2 );

   return( thisBox.FindDistance( refBox ));
}

//===========================================================================//
double TGO_Region_c::FindDistance( 
      const TGO_Point_c& refPoint ) const
{
   TGO_Region_c refRegion( refPoint, refPoint );

   return( this->FindDistance( refRegion ));
}

//===========================================================================//
// Method         : FindNearest
// Purpose        : Return the nearest points between this region and the 
//                  given point or region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGO_Region_c::FindNearest(
      const TGO_Point_c& refPoint, 
            TGO_Point_c* pthisNearestPoint ) const
{
   TGO_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   thisBox.FindNearest( refPoint, pthisNearestPoint );
}

//===========================================================================//
void TGO_Region_c::FindNearest(
      const TGO_Region_c& refRegion,
            TGO_Point_c*  prefNearestPoint,
            TGO_Point_c*  pthisNearestPoint ) const
{
   TGO_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );
   TGO_Box_c refBox( refRegion.x1, refRegion.y1, refRegion.x2, refRegion.y2 );

   thisBox.FindNearest( refBox, prefNearestPoint, pthisNearestPoint );
}

//===========================================================================//
// Method         : ApplyScale
// Purpose        : Scale this region, expanding or shrinking it based on the
//                  given value(s).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Region_c::ApplyScale( 
      int dx,
      int dy )
{
   TGO_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   thisBox.ApplyScale( dx, dy, 0 );

   this->Set( thisBox.lowerLeft.x, thisBox.lowerLeft.y, 
              thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGO_Region_c::ApplyScale( 
      int scale )
{
   this->ApplyScale( scale, scale );
}

//===========================================================================//
// Method         : ApplyUnion
// Purpose        : Set this region's dimensions to the union of the given 
//                  region or regions.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Region_c::ApplyUnion( 
      const TGO_Region_c& regionA, 
      const TGO_Region_c& regionB )
{
   TGO_Box_c thisBox;
   TGO_Box_c regionBoxA( regionA.x1, regionA.y1, regionA.x2, regionA.y2 );
   TGO_Box_c regionBoxB( regionB.x1, regionB.y1, regionB.x2, regionB.y2 );

   thisBox.ApplyUnion( regionBoxA, regionBoxB );

   this->Set( thisBox.lowerLeft.x, thisBox.lowerLeft.y, 
              thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGO_Region_c::ApplyUnion( 
      const TGO_Region_c& region )
{
   this->ApplyUnion( *this, region );
}

//===========================================================================//
void TGO_Region_c::ApplyUnion( 
      const TGO_Point_c& point )
{
   TGO_Region_c region( point, point );
   this->ApplyUnion( region );
}

//===========================================================================//
// Method         : ApplyIntersect
// Purpose        : Set this region's dimensions to the intersection of the 
//                  given region or regions.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Region_c::ApplyIntersect( 
      const TGO_Region_c& regionA, 
      const TGO_Region_c& regionB )
{
   TGO_Box_c thisBox;
   TGO_Box_c regionBoxA( regionA.x1, regionA.y1, regionA.x2, regionA.y2 );
   TGO_Box_c regionBoxB( regionB.x1, regionB.y1, regionB.x2, regionB.y2 );

   thisBox.ApplyIntersect( regionBoxA, regionBoxB );

   this->Set( thisBox.lowerLeft.x, thisBox.lowerLeft.y, 
              thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGO_Region_c::ApplyIntersect( 
      const TGO_Region_c& region )
{
   this->ApplyIntersect( *this, region );
}

//===========================================================================//
// Method         : IsIntersecting
// Purpose        : Return true if this region is intersecting with the given
//                  region.  Intersecting implies intersecting x/y region
//                  coordinates.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Region_c::IsIntersecting( 
      const TGO_Region_c& region ) const
{
   bool isIntersecting = true;

   if(( region.x1 < this->x1 ) && ( region.x2 < this->x1 ))
   {
      isIntersecting = false;
   }
   else if(( region.x1 > this->x2 ) && ( region.x2 > this->x2 )) 
   {
      isIntersecting = false;
   }
   else if(( region.y1 < this->y1 ) && ( region.y2 < this->y1 ))
   {
      isIntersecting = false;
   }
   else if(( region.y1 > this->y2 ) && ( region.y2 > this->y2 ))
   {
      isIntersecting = false;
   }
   return( isIntersecting );
}

//===========================================================================//
// Method         : IsOverlapping
// Purpose        : Return true if this region is overlapping with the given
//                  region.  Overlapping implies overlapping x/y region
//                  coordinates.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Region_c::IsOverlapping( 
      const TGO_Region_c& region ) const
{
   bool isOverlapping = false;

   if(( region.x1 <= this->x1 ) && ( region.x2 >= this->x2 ))
   {
      isOverlapping = true;
   }
   else if(( region.x1 >= this->x1 ) && ( region.x1 <= this->x2 ))
   {
      isOverlapping = true;
   }
   else if(( region.x2 >= this->x1 ) && ( region.x2 <= this->x2 ))
   {
      isOverlapping = true;
   }

   if(( region.y1 <= this->y1 ) && ( region.y2 >= this->y2 ))
   {
      isOverlapping = true;
   }
   else if(( region.y1 >= this->y1 ) && ( region.y1 <= this->y2 ))
   {
      isOverlapping = true;
   }
   else if(( region.y2 >= this->y1 ) && ( region.y2 <= this->y2 ))
   {
      isOverlapping = true;
   }
   return( isOverlapping );
}

//===========================================================================//
// Method         : IsWithin
// Purpose        : Return true if the given point or region is within (ie.
//                  completely contained by) this region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Region_c::IsWithin( 
      const TGO_Region_c& region ) const
{
   bool isWithin = false;

   if(( region.x1 >= this->x1 ) && ( region.x2 <= this->x2 ) &&
      ( region.y1 >= this->y1 ) && ( region.y2 <= this->y2 ))
   {
      isWithin = true;
   }
   return( isWithin );
}

//===========================================================================//
bool TGO_Region_c::IsWithin( 
      const TGO_Point_c& point ) const
{
   bool isWithin = false;

   if(( point.x >= this->x1 ) && ( point.x <= this->x2 ) &&
      ( point.y >= this->y1 ) && ( point.y <= this->y2 ))
   {
      isWithin = true;
   }
   return( isWithin );
}

//===========================================================================//
// Method         : IsWide
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Region_c::IsWide( 
      void ) const
{
   int dx = this->GetDx( );
   int dy = this->GetDy( );

  return( dx >= dy ? true : false );
}

//===========================================================================//
// Method         : IsTall
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Region_c::IsTall( 
      void ) const
{
   int dx = this->GetDx( );
   int dy = this->GetDy( );

  return( dy > dx ? true : false );
}

//===========================================================================//
// Method         : IsSquare
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Region_c::IsSquare( 
      void ) const
{
   int dx = this->GetDx( );
   int dy = this->GetDy( );

   return( dy == dx ? true : false );
}

//===========================================================================//
// Method         : IsLegal
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Region_c::IsLegal( 
      void ) const
{
   TGO_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   return( thisBox.IsLegal( ));
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Region_c::IsValid( 
      void ) const
{
   return(( this->x1 != INT_MIN ) && ( this->x1 != INT_MAX ) &&
          ( this->y1 != INT_MIN ) && ( this->y1 != INT_MAX ) &&
          ( this->x2 != INT_MIN ) && ( this->x2 != INT_MAX ) &&
          ( this->y2 != INT_MIN ) && ( this->y2 != INT_MAX ) ?
          true : false );
}
