//===========================================================================//
// Purpose : Method definitions for the TGS_Rect class.
//
//           Public methods include:
//           - TGS_Rect_c, ~TGS_Rect_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set, Reset
//           - FindCenter
//           - FindLowerLeft
//           - FindLowerRight
//           - FindUpperLeft
//           - FindUpperRight
//           - FindPoint
//           - FindRect
//           - FindSide
//           - FindDistance
//           - FindNearest
//           - FindFarthest
//           - FindDifference
//           - ApplyScale
//           - ApplyUnion
//           - ApplyIntersect
//           - ApplyOverlap
//           - ApplyAdjacent
//           - ApplyDifference
//           - IsConnected
//           - IsAdjacent
//           - IsIntersecting
//           - IsOverlapping
//           - IsWithin
//           - IsWithinDx, IsWithinDy
//           - IsLegal
//           - IsValid
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

#if defined( SUN8 ) || defined( SUN10 )
   #include <math.h>
#endif

#include "TC_MinGrid.h"
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TGS_Box.h"
#include "TGS_Rect.h"

//===========================================================================//
// Method         : TGS_Rect_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TGS_Rect_c::TGS_Rect_c( 
      TGS_Layer_t    layer_,
      double         x1, 
      double         y1,
      double         x2, 
      double         y2,
      TGS_SnapMode_t snap )
      :
      layer( layer_ ),
      region( x1, y1, x2, y2, snap )
{
}

//===========================================================================//
TGS_Rect_c::TGS_Rect_c( 
            TGS_Layer_t    layer_,
      const TGS_Region_c&  region_,
            TGS_SnapMode_t snap )
      :
      layer( layer_ ),
      region( region_.x1, region_.y1, region_.x2, region_.y2, snap )
{
}

//===========================================================================//
TGS_Rect_c::TGS_Rect_c( 
            TGS_Layer_t    layer_,
      const TGS_Point_c&   pointA,
      const TGS_Point_c&   pointB,
            TGS_SnapMode_t snap )
      :
      layer( layer_ ),
      region( pointA.x, pointA.y, pointB.x, pointB.y, snap )
{
}

//===========================================================================//
TGS_Rect_c::TGS_Rect_c( 
      const TGS_Point_c&   pointA,
      const TGS_Point_c&   pointB,
            TGS_SnapMode_t snap )
      :
      layer( pointA.z ),
      region( pointA.x, pointA.y, pointB.x, pointB.y, snap )
{
}

//===========================================================================//
TGS_Rect_c::TGS_Rect_c( 
      const TGS_Rect_c&    rect,
            TGS_SnapMode_t snap )
      :
      layer( rect.layer ),
      region( rect.region.x1, rect.region.y1, rect.region.x2, rect.region.y2, snap )
{
}

//===========================================================================//
TGS_Rect_c::TGS_Rect_c( 
      const TGS_Rect_c&    rect,
            double         scale,
            TGS_SnapMode_t snap )
      :
      layer( rect.layer ),
      region( rect.region.x1, rect.region.y1, rect.region.x2, rect.region.y2, snap )
{
   this->ApplyScale( scale, scale, snap );
}

//===========================================================================//
TGS_Rect_c::TGS_Rect_c( 
      const TGS_Rect_c& rectA,
      const TGS_Rect_c& rectB )
{
   this->ApplyUnion( rectA, rectB );
}

//===========================================================================//
// Method         : ~TGS_Rect_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TGS_Rect_c::~TGS_Rect_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TGS_Rect_c& TGS_Rect_c::operator=( 
      const TGS_Rect_c& rect )
{
   if( &rect != this )
   {
      this->layer = rect.layer;
      this->region = rect.region;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TGS_Rect_c::operator<( 
      const TGS_Rect_c& rect ) const
{
   bool isLessThan = false;
   
   if(( this->layer < rect.layer ))
   {
      isLessThan = true;
   }
   else if(( this->layer == rect.layer ) &&
           ( this->region != rect.region ) &&
           ( this->region < rect.region ))
   {
      isLessThan = true;
   }
   return( isLessThan );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TGS_Rect_c::operator==( 
      const TGS_Rect_c& rect ) const
{
   return(( this->layer == rect.layer ) &&
          ( this->region == rect.region ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TGS_Rect_c::operator!=( 
      const TGS_Rect_c& rect ) const
{
   return(( !this->operator==( rect )) ? 
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srRect;
   this->ExtractString( &srRect );

   printHandler.Write( pfile, spaceLen, "[rect] %s\n", TIO_SR_STR( srRect ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::ExtractString( 
      string* psrRect,
      size_t  precision ) const
{
   if( psrRect )
   {
      char szRect[TIO_FORMAT_STRING_LEN_RECT];

      if( precision == SIZE_MAX )
      {
         precision = TC_MinGrid_c::GetInstance( ).GetPrecision( );
      }

      if( this->IsValid( ))
      {
         if( this->layer > 0 )
         {
            sprintf( szRect, "%d %0.*f %0.*f %0.*f %0.*f",
                             this->layer,
                             static_cast< int >( precision ), this->region.x1,
                             static_cast< int >( precision ), this->region.y1,
                             static_cast< int >( precision ), this->region.x2,
                             static_cast< int >( precision ), this->region.y2 );
         }
         else
         {
            sprintf( szRect, "%0.*f %0.*f %0.*f %0.*f",
                             static_cast< int >( precision ), this->region.x1,
                             static_cast< int >( precision ), this->region.y1,
                             static_cast< int >( precision ), this->region.x2,
                             static_cast< int >( precision ), this->region.y2 );
         }
      }
      else
      {
         sprintf( szRect, "? ? ? ?" );
      }
      *psrRect = szRect;
   }
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::Set( 
      TGS_Layer_t    layer_,
      double         x1, 
      double         y1,
      double         x2, 
      double         y2,
      TGS_SnapMode_t snap )
{
   this->layer = layer_;
   this->region.Set( x1, y1, x2, y2, snap );
}

//===========================================================================//
void TGS_Rect_c::Set(
            TGS_Layer_t    layer_,
      const TGS_Region_c&  region_,
            TGS_SnapMode_t snap )
{
   this->Set( layer_, region_.x1, region_.y1, region_.x2, region_.y2, snap );
}

//===========================================================================//
void TGS_Rect_c::Set(
            TGS_Layer_t    layer_,
      const TGS_Point_c&   pointA,
      const TGS_Point_c&   pointB,
            TGS_SnapMode_t snap )
{
   this->Set( layer_, pointA.x, pointA.y, pointB.x, pointB.y, snap );
}

//===========================================================================//
void TGS_Rect_c::Set(
      const TGS_Point_c&   pointA,
      const TGS_Point_c&   pointB,
            TGS_SnapMode_t snap )
{
   TGS_Layer_t layer_ = pointA.z;
   this->Set( layer_, pointA.x, pointA.y, pointB.x, pointB.y, snap );
}

//===========================================================================//
void TGS_Rect_c::Set( 
      const TGS_Rect_c&    rect,
            TGS_SnapMode_t snap )
{
   TGS_Layer_t layer_ = rect.layer;
   const TGS_Region_c& region_ = rect.region;
   this->Set( layer_, region_.x1, region_.y1, region_.x2, region_.y2, snap );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::Reset( 
      void )
{
   this->Set( TGS_LAYER_UNDEFINED, FLT_MIN, FLT_MIN, FLT_MIN, FLT_MIN );
}

//===========================================================================//
// Method         : FindCenter
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Rect_c::FindCenter( 
      TGS_SnapMode_t snap ) const
{
   TGS_Point_c center( this->region.FindCenter( snap ));
   
   center.z = this->layer;
   return( center );
}

//===========================================================================//
// Method         : FindLowerLeft
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Rect_c::FindLowerLeft( 
      void ) const
{
   TGS_Point_c lowerRight( this->region.x1, this->region.y1, this->layer );

   return( lowerRight );
}

//===========================================================================//
// Method         : FindLowerRight
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Rect_c::FindLowerRight( 
      void ) const
{
   TGS_Point_c lowerRight( this->region.x2, this->region.y1, this->layer );

   return( lowerRight );
}

//===========================================================================//
// Method         : FindUpperLeft
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Rect_c::FindUpperLeft( 
      void ) const
{
   TGS_Point_c upperLeft( this->region.x1, this->region.y2, this->layer );

   return( upperLeft );
}

//===========================================================================//
// Method         : FindUpperRight
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Rect_c::FindUpperRight( 
      void ) const
{
   TGS_Point_c upperRight( this->region.x2, this->region.y2, this->layer );

   return( upperRight );
}

//===========================================================================//
// Method         : FindPoint
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Rect_c::FindPoint(
      TGS_CornerMode_t corner ) const
{
   TGS_Point_c point = this->region.FindPoint( corner );
   point.z = this->layer;

   return( point );
}

//===========================================================================//
// Method         : FindRect
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::FindRect( 
      TC_SideMode_t  side,
      double         minDistance,
      TGS_Rect_c*    pthisSideRect ) const
{
   if( pthisSideRect )
   {
      pthisSideRect->Reset( );

      switch( side )
      { 
      case TC_SIDE_PREV:
         if( this->IsWide( ) && !this->IsTall( ))
	 {
	    side = TC_SIDE_LEFT;
	 }
         if( this->IsTall( ) && !this->IsWide( ))
	 {
	    side = TC_SIDE_LOWER;
	 }
	 break;

      case TC_SIDE_NEXT:
         if( this->IsWide( ) && !this->IsTall( ))
	 {
	    side = TC_SIDE_RIGHT;
	 }
         if( this->IsTall( ) && !this->IsWide( ))
	 {
	    side = TC_SIDE_UPPER;
	 }
	 break;

      default:
	 break;
      }

      switch( side )
      {
      case TC_SIDE_LEFT:
         pthisSideRect->Set( this->layer, 
                             this->region.x1 - minDistance, this->region.y1, 
                             this->region.x1 - minDistance, this->region.y2 );
         break;

      case TC_SIDE_RIGHT:
         pthisSideRect->Set( this->layer, 
                             this->region.x2 + minDistance, this->region.y1, 
                             this->region.x2 + minDistance, this->region.y2 );
         break;

      case TC_SIDE_LOWER:
         pthisSideRect->Set( this->layer, 
                             this->region.x1, this->region.y1 - minDistance,
                             this->region.x2, this->region.y1 - minDistance );
         break;

      case TC_SIDE_UPPER:
         pthisSideRect->Set( this->layer, 
                             this->region.x1, this->region.y2 + minDistance, 
                             this->region.x2, this->region.y2 + minDistance );
         break;

      case TC_SIDE_UNDEFINED:
         break;

      default:
	 break;
      }
   }
}

//===========================================================================//
// Method         : FindSide
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TC_SideMode_t TGS_Rect_c::FindSide(
      const TGS_Rect_c& refRect ) const
{
   TGS_Box_c thisBox( this->region.x1, this->region.y1, this->layer,
                      this->region.x2, this->region.y2, this->layer );
   TGS_Box_c refBox( refRect.region.x1, refRect.region.y1, refRect.layer, 
                     refRect.region.x2, refRect.region.y2, refRect.layer );

   return( thisBox.FindSide( refBox ));
}

//===========================================================================//
// Method         : FindDistance
// Purpose        : Compute and return the "flightline" distance between
//                  the nearest point on this rectangle and the given layer, 
//                  point, or rectangle.
// Note           : When measuring to determine the "flightline" distance,
//                  the layer-to-layer transition distance is assumed to be
//                  a single unit value.  This is equivalent to the current
//                  minimum grid unit value.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
double TGS_Rect_c::FindDistance( 
      TGS_Layer_t refLayer ) const
{
   TGS_Rect_c refRect( refLayer, this->region );

   return( this->FindDistance( refRect ));
}

//===========================================================================//
double TGS_Rect_c::FindDistance( 
      const TGS_Region_c& refRegion ) const
{
   TGS_Rect_c refRect( this->layer, refRegion );

   return( this->FindDistance( refRect ));
}

//===========================================================================//
double TGS_Rect_c::FindDistance( 
      const TGS_Point_c& refPoint ) const
{
   TGS_Rect_c refRect( refPoint, refPoint );

   return( this->FindDistance( refRect ));
}

//===========================================================================//
double TGS_Rect_c::FindDistance( 
      const TGS_Rect_c& refRect ) const
{
   TGS_Point_c refPoint, thisPoint;
   this->FindNearest( refRect, &refPoint, &thisPoint );

   return( thisPoint.FindDistance( refPoint ));
}

//===========================================================================//
// Method         : FindNearest
// Purpose        : Return the nearest points between this rectangle and the 
//                  given point or rectangle.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::FindNearest(
      const TGS_Point_c& refPoint, 
            TGS_Point_c* pthisNearestPoint ) const
{
   TGS_Box_c thisBox( this->region.x1, this->region.y1, this->layer,
                      this->region.x2, this->region.y2, this->layer );

   thisBox.FindNearest( refPoint, pthisNearestPoint );
}

//===========================================================================//
void TGS_Rect_c::FindNearest(
      const TGS_Rect_c&  refRect,
            TGS_Point_c* prefNearestPoint,
            TGS_Point_c* pthisNearestPoint ) const
{
   TGS_Box_c thisBox( this->region.x1, this->region.y1, this->layer,
                      this->region.x2, this->region.y2, this->layer );
   TGS_Box_c refBox( refRect.region.x1, refRect.region.y1, refRect.layer,
                     refRect.region.x2, refRect.region.y2, refRect.layer );

   thisBox.FindNearest( refBox, prefNearestPoint, pthisNearestPoint );
}

//===========================================================================//
// Method         : FindFarthest
// Purpose        : Return the farthest points between this rectangle and the 
//                  point or rectangle.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::FindFarthest(
      const TGS_Point_c& refPoint, 
            TGS_Point_c* pthisFarthestPoint ) const
{
   TGS_Box_c thisBox( this->region.x1, this->region.y1, this->layer,
                      this->region.x2, this->region.y2, this->layer );

   thisBox.FindFarthest( refPoint, pthisFarthestPoint );
}

//===========================================================================//
void TGS_Rect_c::FindFarthest(
      const TGS_Rect_c&  refRect,
            TGS_Point_c* prefFarthestPoint,
            TGS_Point_c* pthisFarthestPoint ) const
{
   TGS_Box_c thisBox( this->region.x1, this->region.y1, this->layer,
                      this->region.x2, this->region.y2, this->layer );
   TGS_Box_c refBox( refRect.region.x1, refRect.region.y1, refRect.layer,
                     refRect.region.x2, refRect.region.y2, refRect.layer );

   thisBox.FindFarthest( refBox, prefFarthestPoint, pthisFarthestPoint );
}

//===========================================================================//
// Method         : FindDifference
// Purpose        : Find the "difference" of the given two rectangles, 
//                  returning a list of difference sub-rectangles that result
//                  after removing the second rectangle from the first 
//                  rectangle.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::FindDifference( 
      const TGS_Rect_c&      rectA, 
      const TGS_Rect_c&      rectB,
            TGS_OrientMode_t orient,
            TGS_RectList_t*  pdifferenceList,
            double           minSpacing,
            double           minWidth,
            double           minArea ) const
{
   rectA.FindDifference( rectB, orient, 
                         pdifferenceList,
                         minSpacing, minWidth, minArea );
}

//===========================================================================//
void TGS_Rect_c::FindDifference( 
      const TGS_Rect_c&      rect,
            TGS_OrientMode_t orient,
            TGS_RectList_t*  pdifferenceList,
            double           minSpacing,
            double           minWidth,
            double           minArea ) const
{
   const TGS_Region_c& thisRegion = this->region;
   const TGS_Region_c& rectRegion = rect.region;

   TGS_RegionList_t differenceList;
   thisRegion.FindDifference( rectRegion, orient, 
                              &differenceList, 
                              minSpacing, minWidth, minArea );
   if( pdifferenceList )
   {
      pdifferenceList->Clear( );

      for( size_t i = 0; i < differenceList.GetLength( ); ++i )
      {
         const TGS_Region_c& differenceRegion = *differenceList[ i ];

         TGS_Rect_c differenceRect( this->layer, differenceRegion );
         pdifferenceList->Add( differenceRect );
      }
   }
}

//===========================================================================//
// Method         : ApplyScale
// Purpose        : Set this rectangle's dimensions based on the given scale
//                  factors.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::ApplyScale( 
      double         dx,
      double         dy,
      TGS_SnapMode_t snap )
{
   TGS_Box_c thisBox( this->region.x1, this->region.y1, this->layer,
                      this->region.x2, this->region.y2, this->layer );

   thisBox.ApplyScale( dx, dy, 0, snap );

   this->Set( thisBox.lowerLeft.z, 
	      thisBox.lowerLeft.x, thisBox.lowerLeft.y,
	      thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGS_Rect_c::ApplyScale( 
      double         scale,
      TGS_SnapMode_t snap )
{
   this->ApplyScale( scale, scale, snap );
}

//===========================================================================//
void TGS_Rect_c::ApplyScale( 
      double           scale,
      TGS_OrientMode_t orient,
      TGS_SnapMode_t   snap )
{
   TGS_Box_c thisBox( this->region.x1, this->region.y1, this->layer,
                      this->region.x2, this->region.y2, this->layer );

   thisBox.ApplyScale( scale, orient, snap );

   this->Set( thisBox.lowerLeft.z, 
	      thisBox.lowerLeft.x, thisBox.lowerLeft.y,
	      thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGS_Rect_c::ApplyScale( 
      double         scale,
      TC_SideMode_t  side,
      TGS_SnapMode_t snap )
{
   TGS_Box_c thisBox( this->region.x1, this->region.y1, this->layer,
                      this->region.x2, this->region.y2, this->layer );

   thisBox.ApplyScale( scale, side, snap );

   this->Set( thisBox.lowerLeft.z, 
	      thisBox.lowerLeft.x, thisBox.lowerLeft.y,
	      thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
// Method         : ApplyUnion
// Purpose        : Set this rectangle dimensions to the union of the given 
//                  rectangle or rectangles.  The given rectangle or
//                  rectangles may or may not be intersecting.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::ApplyUnion( 
      const TGS_Rect_c& rectA, 
      const TGS_Rect_c& rectB )
{
   TGS_Box_c rectBoxA( rectA.region.x1, rectA.region.y1, rectA.layer,
                       rectA.region.x2, rectA.region.y2, rectA.layer );
   TGS_Box_c rectBoxB( rectB.region.x1, rectB.region.y1, rectB.layer,
                       rectB.region.x2, rectB.region.y2, rectB.layer );
   TGS_Box_c thisBox;
   thisBox.ApplyUnion( rectBoxA, rectBoxB );

   this->Set( thisBox.lowerLeft.z, 
	      thisBox.lowerLeft.x, thisBox.lowerLeft.y,
	      thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGS_Rect_c::ApplyUnion( 
      const TGS_Rect_c& rect )
{
   this->ApplyUnion( *this, rect );
}

//===========================================================================//
// Method         : ApplyIntersect
// Purpose        : Set this rectangle dimensions to the intersection of the 
//                  given rectangle or rectangles.  The given rectangle
//                  or rectangles must be intersecting.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::ApplyIntersect( 
      const TGS_Rect_c& rectA, 
      const TGS_Rect_c& rectB )
{
   TGS_Box_c rectBoxA( rectA.region.x1, rectA.region.y1, rectA.layer,
                       rectA.region.x2, rectA.region.y2, rectA.layer );
   TGS_Box_c rectBoxB( rectB.region.x1, rectB.region.y1, rectB.layer,
                       rectB.region.x2, rectB.region.y2, rectB.layer );
   TGS_Box_c thisBox;
   thisBox.ApplyIntersect( rectBoxA, rectBoxB );

   this->Set( thisBox.lowerLeft.z, 
	      thisBox.lowerLeft.x, thisBox.lowerLeft.y,
	      thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGS_Rect_c::ApplyIntersect( 
      const TGS_Rect_c& rect )
{
   this->ApplyIntersect( *this, rect );
}

//===========================================================================//
// Method         : ApplyOverlap
// Purpose        : Set this rectangle dimensions based on the overlap of the 
//                  given rectangle or rectangles.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::ApplyOverlap( 
      const TGS_Rect_c& rectA, 
      const TGS_Rect_c& rectB )
{
   TGS_Box_c rectBoxA( rectA.region.x1, rectA.region.y1, rectA.layer,
                       rectA.region.x2, rectA.region.y2, rectA.layer );
   TGS_Box_c rectBoxB( rectB.region.x1, rectB.region.y1, rectB.layer,
                       rectB.region.x2, rectB.region.y2, rectB.layer );
   TGS_Box_c thisBox;
   thisBox.ApplyOverlap( rectBoxA, rectBoxB );

   this->Set( thisBox.lowerLeft.z, 
	      thisBox.lowerLeft.x, thisBox.lowerLeft.y,
	      thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGS_Rect_c::ApplyOverlap( 
      const TGS_Rect_c& rect )
{
   this->ApplyOverlap( *this, rect );
}

//===========================================================================//
// Method         : ApplyAdjacent
// Purpose        : Set this rectangle dimensions based on the adjacency of 
//                  the given rectangle or rectangles.  The given rectangle
//                  or rectangles must be adjacent.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::ApplyAdjacent( 
      const TGS_Rect_c& rectA, 
      const TGS_Rect_c& rectB,
            double      minDistance )
{
   const TGS_Region_c& rectRegionA = rectA.region;
   const TGS_Region_c& rectRegionB = rectB.region;

   TGS_Layer_t thisLayer = TCT_Min( rectA.layer, rectB.layer );
   TGS_Region_c thisRegion;
   thisRegion.ApplyAdjacent( rectRegionA, rectRegionB, minDistance );

   this->Set( thisLayer, thisRegion );
}

//===========================================================================//
void TGS_Rect_c::ApplyAdjacent( 
      const TGS_Rect_c& rect, 
            double      minDistance )
{
   this->ApplyAdjacent( *this, rect, minDistance );
}

//===========================================================================//
// Method         : ApplyDifference
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TGS_Rect_c::ApplyDifference( 
      const TGS_Rect_c& rectA, 
      const TGS_Rect_c& rectB )
{
   const TGS_Region_c& rectRegionA = rectA.region;
   const TGS_Region_c& rectRegionB = rectB.region;

   TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED;

   if( orient == TGS_ORIENT_UNDEFINED )
   {
      if( rectRegionA.IsWithinDy( rectRegionB ) ||
          rectRegionB.IsWithinDy( rectRegionA ))
      {
         orient = TGS_ORIENT_HORIZONTAL;
      }
      if( rectRegionA.IsWithinDx( rectRegionB ) ||
          rectRegionB.IsWithinDx( rectRegionA ))
      {
         orient = TGS_ORIENT_VERTICAL;
      }
   }

   if( orient == TGS_ORIENT_UNDEFINED )
   {
      TGS_Point_c rectCenterA = rectRegionA.FindCenter( );
      TGS_Point_c rectCenterB = rectRegionB.FindCenter( );

      double rectCenterDx = rectCenterA.GetDx( rectCenterB );
      double rectCenterDy = rectCenterA.GetDy( rectCenterB );
      if( TCTF_IsGE( rectCenterDx, rectCenterDy ))
      {
         orient = TGS_ORIENT_HORIZONTAL;
      }
      else // if( TCTF_IsGE( rectCenterDy, rectCenterDx ))
      {
         orient = TGS_ORIENT_VERTICAL;
      }
   }

   TGS_Layer_t thisLayer = TCT_Min( rectA.layer, rectB.layer );
   TGS_Region_c thisRegion;
   thisRegion.ApplyDifference( rectRegionA, rectRegionB, orient );

   this->Set( thisLayer, thisRegion );
}

//===========================================================================//
void TGS_Rect_c::ApplyDifference( 
      const TGS_Rect_c& rect )
{
   this->ApplyDifference( *this, rect );
}

//===========================================================================//
// Method         : IsConnected
// Purpose        : Return true if this rectangle is connected (ie. is 
//                  intersecting with, is adjacent to, or is overlapping
//                  with) the given rectangle.
//
//                  Note: This method only needs to test for intersecting
//                        and overlapping conditions.  When a rectangle is
//                        intersecting with, it is by definition also 
//                        adjacent to.
//
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TGS_Rect_c::IsConnected( 
      const TGS_Rect_c& rect ) const
{
   return(( this->IsIntersecting( rect )) ||  
          ( this->IsOverlapping( rect )) ?
          true : false );
}

//===========================================================================//
// Method         : IsAdjacent
// Purpose        : Return true if this rectangle is adjacent to the given 
//                  rectangle.  Adjacent implies both rectangles have same
//                  layer and non-intersecting x/y region coordinates that
//                  share a common side.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TGS_Rect_c::IsAdjacent( 
      const TGS_Rect_c& rect ) const
{
   bool isAdjacent = (( rect.layer == this->layer ) &&
                      ( rect.region.IsAdjacent( this->region )) ?
		      true : false );
   return( isAdjacent );
}

//===========================================================================//
// Method         : IsIntersecting
// Purpose        : Return true if this rectangle is intersecting with the
//                  given rectangle.  Intersection implies both rectangles
//                  are same layer and have intersecting x/y region 
//                  coordinates.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TGS_Rect_c::IsIntersecting( 
      const TGS_Rect_c& rect ) const
{
   bool isIntersecting = (( rect.layer == this->layer ) &&
                          ( rect.region.IsIntersecting( this->region )) ?
	    	          true : false );
   return( isIntersecting );
}

//===========================================================================//
// Method         : IsOverlapping
// Purpose        : Return true if this rectangle is overlapping with the
//                  given rectangle.  Overlapping implies both rectangles
//                  are on adjacent layers and have intersecting x/y region 
//                  coordinates.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TGS_Rect_c::IsOverlapping( 
      const TGS_Rect_c& rect ) const
{
   bool isOverlapping = (( abs( rect.layer - this->layer ) == 1 ) &&
                         ( rect.region.IsIntersecting( this->region )) ?
                         true : false );
   return( isOverlapping );
}

//===========================================================================//
// Method         : IsWithin
// Purpose        : Return true if the given point or rectangle is within
//                  (ie. completely contained by) this rectangle.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TGS_Rect_c::IsWithin( 
      const TGS_Point_c& point ) const
{
   TGS_Rect_c pointRect( point, point );

   return( this->IsWithin( pointRect ));
}

//===========================================================================//
bool TGS_Rect_c::IsWithin( 
      const TGS_Rect_c& rect ) const
{
   TGS_Box_c thisBox( this->region.x1, this->region.y1, this->layer,
                      this->region.x2, this->region.y2, this->layer );
   TGS_Box_c rectBox( rect.region.x1, rect.region.y1, rect.layer,
                      rect.region.x2, rect.region.y2, rect.layer );

   return( thisBox.IsWithin( rectBox ));
}

//===========================================================================//
// Method         : IsWithinDx
// Purpose        : Return true if the given rectangle is within this 
//                  rectangle region's width (ie. dx) dimensions.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TGS_Rect_c::IsWithinDx( 
      const TGS_Rect_c& rect ) const
{
   const TGS_Region_c& thisRegion = this->region;
   const TGS_Region_c& rectRegion = rect.region;

   return( thisRegion.IsWithinDx( rectRegion ) ? true : false );
}

//===========================================================================//
// Method         : IsWithinDy
// Purpose        : Return true if the given rectangle is within this 
//                  rectangle region's height (ie. dy) dimensions.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TGS_Rect_c::IsWithinDy( 
      const TGS_Rect_c& rect ) const
{
   const TGS_Region_c& thisRegion = this->region;
   const TGS_Region_c& rectRegion = rect.region;

   return( thisRegion.IsWithinDy( rectRegion ) ? true : false );
}

//===========================================================================//
// Method         : IsLegal
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TGS_Rect_c::IsLegal( 
      void ) const
{
   bool isLegal = this->IsValid( );
   if( isLegal )
   {
      isLegal = this->region.IsLegal( );
   }
   return( isLegal );
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TGS_Rect_c::IsValid( 
      void ) const
{
   return(( this->layer != TGS_LAYER_UNDEFINED ) &&
          ( this->region.IsValid( )) ?
          true : false );
}
