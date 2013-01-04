//===========================================================================//
// Purpose : Method definitions for the TGS_Region class.
//
//           Public methods include:
//           - TGS_Region_c, ~TGS_Region_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set, Reset
//           - FindMin, FindMax
//           - FindRatio
//           - FindArea
//           - FindCenter
//           - FindLowerLeft, FindLowerRight
//           - FindUpperLeft, FindUpperRight
//           - FindOrient
//           - FindPoint
//           - FindRegion
//           - FindSide
//           - FindWidth, FindLength
//           - FindDistance
//           - FindNearest
//           - FindDifference
//           - ApplyScale
//           - ApplyUnion
//           - ApplyIntersect
//           - ApplyAdjacent
//           - ApplyDifference
//           - ApplyShift
//           - IsIntersecting
//           - IsOverlapping
//           - IsAdjacent
//           - IsWithin
//           - IsWithinDx, IsWithinDx
//           - IsCorner
//           - IsCrossed
//           - IsWide, IsTall
//           - IsSquare
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
#include "TGS_Region.h"

//===========================================================================//
// Method         : TGS_Region_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Region_c::TGS_Region_c( 
      void )
      :
      x1( TC_FLT_MIN ),
      y1( TC_FLT_MIN ),
      x2( TC_FLT_MIN ),
      y2( TC_FLT_MIN )
{
}

//===========================================================================//
TGS_Region_c::TGS_Region_c( 
      double         x1_, 
      double         y1_,
      double         x2_, 
      double         y2_,
      TGS_SnapMode_t snap )
{
   this->Set( x1_, y1_, x2_, y2_, snap );
}

//===========================================================================//
TGS_Region_c::TGS_Region_c(
      const TGS_Point_c&   pointA,
      const TGS_Point_c&   pointB,
            TGS_SnapMode_t snap )
{
   this->Set( pointA, pointB, snap );
}

//===========================================================================//
TGS_Region_c::TGS_Region_c( 
      const TGS_Region_c&  region,
            TGS_SnapMode_t snap )
{
   this->Set( region, snap );
}

//===========================================================================//
TGS_Region_c::TGS_Region_c( 
      const TGS_Region_c&  region,
            double         dx,
            double         dy,
            TGS_SnapMode_t snap )
{
   this->Set( region, snap );
   this->ApplyScale( dx, dy, snap );
}

//===========================================================================//
TGS_Region_c::TGS_Region_c( 
      const TGS_Region_c&  region,
            double         scale,
            TGS_SnapMode_t snap )
{
   this->Set( region, snap );
   this->ApplyScale( scale, snap );
}

//===========================================================================//
TGS_Region_c::TGS_Region_c( 
      const TGS_Point_c&   pointA,
      const TGS_Point_c&   pointB,
            double         scale,
            TGS_SnapMode_t snap )
{
   this->Set( pointA, pointB, snap );
   this->ApplyScale( scale, snap );
}

//===========================================================================//
TGS_Region_c::TGS_Region_c( 
      const TGS_Region_c& regionA,
      const TGS_Region_c& regionB )
{
   this->ApplyUnion( regionA, regionB );
}

//===========================================================================//
// Method         : ~TGS_Region_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Region_c::~TGS_Region_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Region_c& TGS_Region_c::operator=( 
      const TGS_Region_c& region )
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
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::operator<( 
      const TGS_Region_c& region ) const
{
   return(( TCTF_IsGE( this->x1, region.x1 )) &&
          ( TCTF_IsGE( this->y1, region.y1 )) &&
          ( TCTF_IsLE( this->x2, region.x2 )) &&
          ( TCTF_IsLE( this->y2, region.y2 )) ?
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::operator==( 
      const TGS_Region_c& region ) const
{
   return(( TCTF_IsEQ( this->x1, region.x1 )) &&
          ( TCTF_IsEQ( this->y1, region.y1 )) &&
          ( TCTF_IsEQ( this->x2, region.x2 )) &&
          ( TCTF_IsEQ( this->y2, region.y2 )) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::operator!=( 
      const TGS_Region_c& region ) const
{
   return(( !this->operator==( region )) ? 
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Region_c::Print( 
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
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Region_c::ExtractString( 
      string* psrRegion,
      size_t  precision ) const
{
   if( psrRegion )
   {
      char szRegion[TIO_FORMAT_STRING_LEN_REGION];

      if( precision == SIZE_MAX )
      {
         precision = TC_MinGrid_c::GetInstance( ).GetPrecision( );
      }

      if( this->IsValid( ))
      {
         sprintf( szRegion, "%0.*f %0.*f %0.*f %0.*f",
                            static_cast< int >( precision ), this->x1,
                            static_cast< int >( precision ), this->y1,
                            static_cast< int >( precision ), this->x2,
                            static_cast< int >( precision ), this->y2 );
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
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Region_c::Set( 
      double         x1_, 
      double         y1_,
      double         x2_, 
      double         y2_,
      TGS_SnapMode_t snap )
{
   this->x1 = TCT_Min( x1_, x2_ );
   this->y1 = TCT_Min( y1_, y2_ );
   this->x2 = TCT_Max( x1_, x2_ );
   this->y2 = TCT_Max( y1_, y2_ );

   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      this->x1 = MinGrid.SnapToGrid( this->x1 );
      this->y1 = MinGrid.SnapToGrid( this->y1 );
      this->x2 = MinGrid.SnapToGrid( this->x2 );
      this->y2 = MinGrid.SnapToGrid( this->y2 );
   }
}

//===========================================================================//
void TGS_Region_c::Set( 
      const TGS_Point_c&   pointA,
      const TGS_Point_c&   pointB,
            TGS_SnapMode_t snap )
{
   this->Set( pointA.x, pointA.y, pointB.x, pointB.y, snap );
}

//===========================================================================//
void TGS_Region_c::Set( 
      const TGS_Region_c&  region,
            TGS_SnapMode_t snap )
{
   this->Set( region.x1, region.y1, region.x2, region.y2, snap );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Region_c::Reset( 
      void )
{
   this->Set( TC_FLT_MIN, TC_FLT_MIN, TC_FLT_MIN, TC_FLT_MIN );
}

//===========================================================================//
// Method         : FindMin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Region_c::FindMin( 
      void ) const
{
   double dx = this->GetDx( );
   double dy = this->GetDy( );

   return( TCT_Min( dx, dy ));
}

//===========================================================================//
// Method         : FindMax
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Region_c::FindMax( 
      void ) const
{
   double dx = this->GetDx( );
   double dy = this->GetDy( );

   return( TCT_Max( dx, dy ));
}

//===========================================================================//
// Method         : FindRatio
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Region_c::FindRatio( 
      void ) const
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   return( thisBox.FindRatio( ));
}

//===========================================================================//
// Method         : FindArea
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Region_c::FindArea( 
      void ) const
{
   double dx = this->GetDx( );
   double dy = this->GetDy( );

   return( dx * dy );
}

//===========================================================================//
// Method         : FindCenter
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Region_c::FindCenter( 
      TGS_SnapMode_t snap ) const
{
   double dx = this->GetDx( );
   double dy = this->GetDy( );

   TGS_Point_c center( this->x1 + ( dx / 2.0 ),
                       this->y1 + ( dy / 2.0 ));

   if( snap == TGS_SNAP_MIN_GRID )
   {
      // After divide by 2.0, may need to "snap" point to min grid coordinates
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      center.x = MinGrid.SnapToGrid( center.x );
      center.y = MinGrid.SnapToGrid( center.y );
   }
   return( center );
}

//===========================================================================//
// Method         : FindLowerLeft
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Region_c::FindLowerLeft( 
      void ) const
{
   TGS_Point_c lowerLeft( this->x1, this->y1 );

   return( lowerLeft );
}


//===========================================================================//
// Method         : FindLowerRight
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Region_c::FindLowerRight( 
      void ) const
{
   TGS_Point_c lowerRight( this->x2, this->y1 );

   return( lowerRight );
}


//===========================================================================//
// Method         : FindUpperLeft
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Region_c::FindUpperLeft( 
      void ) const
{
   TGS_Point_c upperLeft( this->x1, this->y2 );

   return( upperLeft );
}


//===========================================================================//
// Method         : FindUpperRight
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Region_c::FindUpperRight( 
      void ) const
{
   TGS_Point_c upperRight( this->x2, this->y2 );

   return( upperRight );
}

//===========================================================================//
// Method         : FindOrient
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_OrientMode_t TGS_Region_c::FindOrient( 
      TGS_OrientMode_t orient ) const
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   return( thisBox.FindOrient( orient ));
}

//===========================================================================//
// Method         : FindPoint
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Region_c::FindPoint(
      TGS_CornerMode_t corner ) const
{
   TGS_Point_c point;

   switch( corner )
   {
   case TGS_CORNER_LOWER_LEFT:
      point = this->FindLowerLeft( );
      break;

   case TGS_CORNER_LOWER_RIGHT:
      point = this->FindLowerRight( );
      break;

   case TGS_CORNER_UPPER_LEFT:
      point = this->FindUpperLeft( );
      break;

   case TGS_CORNER_UPPER_RIGHT:
      point = this->FindUpperRight( );
      break;

   case TGS_CORNER_CENTER:
      point = this->FindCenter( );
      break;

   default:
      break;
   }
   return( point );
}

//===========================================================================//
// Method         : FindRegion
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Region_c::FindRegion( 
      TC_SideMode_t side,
      TGS_Region_c* pthisSideRegion ) const
{
   double minDistance = 0.0;
   this->FindRegion( side, minDistance, pthisSideRegion );
}

//===========================================================================//
void TGS_Region_c::FindRegion( 
      TC_SideMode_t side,
      double        minDistance,
      TGS_Region_c* pthisSideRegion ) const
{
   this->FindRegion( side, minDistance, minDistance, pthisSideRegion );
}

//===========================================================================//
void TGS_Region_c::FindRegion( 
      TC_SideMode_t side,
      double        minDistance,
      double        maxDistance,
      TGS_Region_c* pthisSideRegion ) const
{
   if( pthisSideRegion )
   {
      pthisSideRegion->Reset( );

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
         pthisSideRegion->Set( this->x1 - minDistance, this->y1, 
                               this->x1 - maxDistance, this->y2 );
         break;

      case TC_SIDE_RIGHT:
         pthisSideRegion->Set( this->x2 + minDistance, this->y1, 
                               this->x2 + maxDistance, this->y2 );
         break;

      case TC_SIDE_LOWER:
         pthisSideRegion->Set( this->x1, this->y1 - minDistance,
                               this->x2, this->y1 - maxDistance );
         break;

      case TC_SIDE_UPPER:
         pthisSideRegion->Set( this->x1, this->y2 + minDistance, 
                               this->x2, this->y2 + maxDistance );
         break;

      default:
         break;
      }

      if( TCTF_IsLE( minDistance, 0.0 ) &&
          TCTF_IsLE( maxDistance, 0.0 ))
      {
         pthisSideRegion->ApplyIntersect( *this );
      }
   }
}

//===========================================================================//
// Method         : FindSide
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TC_SideMode_t TGS_Region_c::FindSide(
      const TGS_Region_c& refRegion ) const
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );
   TGS_Box_c refBox( refRegion.x1, refRegion.y1, refRegion.x2, refRegion.y2 );

   return( thisBox.FindSide( refBox ));
}

//===========================================================================//
TC_SideMode_t TGS_Region_c::FindSide(
      const TGS_Point_c& refPoint ) const
{
   TGS_Region_c refRegion( refPoint, refPoint );

   return( this->FindSide( refRegion ));
}

//===========================================================================//
// Method         : FindWidth
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Region_c::FindWidth( 
      TGS_OrientMode_t orient ) const
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   return( thisBox.FindWidth( orient ));
}

//===========================================================================//
// Method         : FindLength
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Region_c::FindLength( 
      TGS_OrientMode_t orient ) const
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

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
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Region_c::FindDistance( 
      const TGS_Region_c& refRegion ) const
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );
   TGS_Box_c refBox( refRegion.x1, refRegion.y1, refRegion.x2, refRegion.y2 );

   return( thisBox.FindDistance( refBox ));
}

//===========================================================================//
double TGS_Region_c::FindDistance( 
      const TGS_Point_c& refPoint ) const
{
   TGS_Region_c refRegion( refPoint, refPoint );

   return( this->FindDistance( refRegion ));
}

//===========================================================================//
double TGS_Region_c::FindDistance( 
      const TGS_Region_c& refRegion,
            TC_SideMode_t side ) const
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );
   TGS_Box_c refBox( refRegion.x1, refRegion.y1, refRegion.x2, refRegion.y2 );

   return( thisBox.FindDistance( refBox, side ));
}


//===========================================================================//
double TGS_Region_c::FindDistance(
      const TGS_Point_c&     refPoint,
            TGS_CornerMode_t corner ) const
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   return( thisBox.FindDistance( refPoint, corner ));
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
void TGS_Region_c::FindNearest(
      const TGS_Point_c& refPoint, 
            TGS_Point_c* pthisNearestPoint ) const
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   thisBox.FindNearest( refPoint, pthisNearestPoint );
}

//===========================================================================//
void TGS_Region_c::FindNearest(
      const TGS_Region_c& refRegion,
            TGS_Point_c*  prefNearestPoint,
            TGS_Point_c*  pthisNearestPoint ) const
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );
   TGS_Box_c refBox( refRegion.x1, refRegion.y1, refRegion.x2, refRegion.y2 );

   thisBox.FindNearest( refBox, prefNearestPoint, pthisNearestPoint );
}

//===========================================================================//
// Method         : FindDifference
// Purpose        : Find the "difference" of the given two regions, returning 
//                  a list of difference sub-regions that result after
//                  deleting the second region from the first region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Region_c::FindDifference( 
      const TGS_Region_c&     regionA, 
      const TGS_Region_c&     regionB,
            TGS_OrientMode_t  orient,
            TGS_RegionList_t* pdifferenceList,
            double            minSpacing,
            double            minWidth,
            double            minArea ) const
{
   TGS_Region_c leftRegion;
   TGS_Region_c rightRegion;
   TGS_Region_c lowerRegion;
   TGS_Region_c upperRegion;

   // Construct partial "difference" regions based on all region sides
   if( TCTF_IsLE( regionA.x1, regionB.x1 - minSpacing - minWidth ))
   {
      leftRegion.Set( regionA.x1, regionA.y1, 
                      regionB.x1 - minSpacing, regionA.y2 );
   }
   if( TCTF_IsGE( regionA.x2, regionB.x2 + minSpacing + minWidth ))
   {
      rightRegion.Set( regionB.x2 + minSpacing, regionA.y1, 
                       regionA.x2, regionA.y2 );
   }
   if( TCTF_IsLE( regionA.y1, regionB.y1 - minSpacing - minWidth ))
   {
      lowerRegion.Set( regionA.x1, regionA.y1, 
                       regionA.x2, regionB.y1 - minSpacing );
   }
   if( TCTF_IsGE( regionA.y2, regionB.y2 + minSpacing + minWidth ))
   {
      upperRegion.Set( regionA.x1, regionB.y2 + minSpacing, 
                       regionA.x2, regionA.y2 );
   }

   // Finish "difference" regions based on intersections to region sides
   if( leftRegion.IsValid( ) && lowerRegion.IsValid( ) && 
       leftRegion.IsIntersecting( lowerRegion ))
   {
      if( orient == TGS_ORIENT_HORIZONTAL )
      {
         leftRegion.y1 = lowerRegion.y2 + minSpacing;
      }
      else // if( orient == TGS_ORIENT_VERTICAL )
      {
         lowerRegion.x1 = leftRegion.x2 + minSpacing;
      }
   }
   if( leftRegion.IsValid( ) && upperRegion.IsValid( ) && 
       leftRegion.IsIntersecting( upperRegion ))
   {
      if( orient == TGS_ORIENT_HORIZONTAL )
      {
         leftRegion.y2 = upperRegion.y1 - minSpacing;
      }
      else // if( orient == TGS_ORIENT_VERTICAL )
      {
         upperRegion.x1 = leftRegion.x2 + minSpacing;
      }
   }
   if( rightRegion.IsValid( ) && lowerRegion.IsValid( ) && 
       rightRegion.IsIntersecting( lowerRegion ))
   {
      if( orient == TGS_ORIENT_HORIZONTAL )
      {
         rightRegion.y1 = lowerRegion.y2 + minSpacing;
      }
      else // if( orient == TGS_ORIENT_VERTICAL )
      {
         lowerRegion.x2 = rightRegion.x1 - minSpacing;
      }
   }
   if( rightRegion.IsValid( ) && upperRegion.IsValid( ) && 
       rightRegion.IsIntersecting( upperRegion ))
   {
      if( orient == TGS_ORIENT_HORIZONTAL )
      {
         rightRegion.y2 = upperRegion.y1 - minSpacing;
      }
      else // if( orient == TGS_ORIENT_VERTICAL )
      {
         upperRegion.x2 = rightRegion.x1 - minSpacing;
      }
   }

   // Merge "difference" regions based on intersections to region sides
   TGS_Region_c leftMerge( leftRegion );
   TGS_Region_c rightMerge( rightRegion );
   TGS_Region_c lowerMerge( lowerRegion );
   TGS_Region_c upperMerge( upperRegion );

   // Load and return the resulting "difference" operation sub-regions
   if( pdifferenceList )
   {
      pdifferenceList->Clear( );
      if( leftRegion.IsValid( ) &&
          TCTF_IsGE( leftMerge.FindArea( ), minArea ))
      {
         pdifferenceList->Add( leftRegion );
      }
      if( rightRegion.IsValid( ) &&
          TCTF_IsGE( rightMerge.FindArea( ), minArea ))
      {
         pdifferenceList->Add( rightRegion );
      }
      if( lowerRegion.IsValid( ) &&
          TCTF_IsGE( lowerMerge.FindArea( ), minArea ))
      {
         pdifferenceList->Add( lowerRegion );
      }
      if( upperRegion.IsValid( ) &&
          TCTF_IsGE( upperMerge.FindArea( ), minArea ))
      {
         pdifferenceList->Add( upperRegion );
      }
   }
}

//===========================================================================//
void TGS_Region_c::FindDifference( 
      const TGS_Region_c&     region,
            TGS_OrientMode_t  orient,
            TGS_RegionList_t* pdifferenceList,
            double            minSpacing,
            double            minWidth,
            double            minArea ) const
{
   this->FindDifference( *this, region, orient, 
                         pdifferenceList,
                         minSpacing, minWidth, minArea );
}

//===========================================================================//
// Method         : ApplyScale
// Purpose        : Scale this region, expanding or shrinking it based on the
//                  given value(s).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Region_c::ApplyScale( 
      double         dx,
      double         dy,
      TGS_SnapMode_t snap )
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   thisBox.ApplyScale( dx, dy, 0, snap );

   this->Set( thisBox.lowerLeft.x, thisBox.lowerLeft.y, 
              thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGS_Region_c::ApplyScale( 
      double         scale,
      TGS_SnapMode_t snap )
{
   this->ApplyScale( scale, scale, snap );
}

//===========================================================================//
void TGS_Region_c::ApplyScale( 
      double         scale,
      TC_SideMode_t  side,
      TGS_SnapMode_t snap )
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   thisBox.ApplyScale( scale, side, snap );

   this->Set( thisBox.lowerLeft.x, thisBox.lowerLeft.y, 
              thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGS_Region_c::ApplyScale( 
      double           scale,
      TGS_OrientMode_t orient,
      TGS_SnapMode_t   snap )
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   thisBox.ApplyScale( scale, orient, snap );

   this->Set( thisBox.lowerLeft.x, thisBox.lowerLeft.y, 
              thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGS_Region_c::ApplyScale( 
      const TGS_Region_c&  scale,
            TGS_SnapMode_t snap )
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );
   TGS_Box_c scaleBox( scale.x1, scale.y1, scale.x2, scale.y2  );

   thisBox.ApplyScale( scaleBox, snap );

   this->Set( thisBox.lowerLeft.x, thisBox.lowerLeft.y, 
              thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGS_Region_c::ApplyScale( 
      const TGS_Scale_t&   scale,      
            TGS_SnapMode_t snap )
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   thisBox.ApplyScale( scale, snap );

   this->Set( thisBox.lowerLeft.x, thisBox.lowerLeft.y, 
              thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
// Method         : ApplyUnion
// Purpose        : Set this region's dimensions to the union of the given 
//                  region or regions.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Region_c::ApplyUnion( 
      const TGS_Region_c& regionA, 
      const TGS_Region_c& regionB )
{
   TGS_Box_c thisBox;
   TGS_Box_c regionBoxA( regionA.x1, regionA.y1, regionA.x2, regionA.y2 );
   TGS_Box_c regionBoxB( regionB.x1, regionB.y1, regionB.x2, regionB.y2 );

   thisBox.ApplyUnion( regionBoxA, regionBoxB );

   this->Set( thisBox.lowerLeft.x, thisBox.lowerLeft.y, 
              thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGS_Region_c::ApplyUnion( 
      const TGS_Region_c& region )
{
   this->ApplyUnion( *this, region );
}

//===========================================================================//
// Method         : ApplyIntersect
// Purpose        : Set this region's dimensions to the intersection of the 
//                  given region or regions.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Region_c::ApplyIntersect( 
      const TGS_Region_c& regionA, 
      const TGS_Region_c& regionB )
{
   TGS_Box_c thisBox;
   TGS_Box_c regionBoxA( regionA.x1, regionA.y1, regionA.x2, regionA.y2 );
   TGS_Box_c regionBoxB( regionB.x1, regionB.y1, regionB.x2, regionB.y2 );

   thisBox.ApplyIntersect( regionBoxA, regionBoxB );

   this->Set( thisBox.lowerLeft.x, thisBox.lowerLeft.y, 
              thisBox.upperRight.x, thisBox.upperRight.y );
}

//===========================================================================//
void TGS_Region_c::ApplyIntersect( 
      const TGS_Region_c& region )
{
   this->ApplyIntersect( *this, region );
}

//===========================================================================//
// Method         : ApplyAdjacent
// Purpose        : Set this region's dimensions based on the adjacency of 
//                  the given region or regions.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Region_c::ApplyAdjacent( 
      const TGS_Region_c& regionA, 
      const TGS_Region_c& regionB,
            double        minDistance )
{
   if( TCTF_IsLE( static_cast< double>( fabs( regionA.x1 - regionB.x2 )), minDistance ) ||
       TCTF_IsLE( static_cast< double>( fabs( regionA.x2 - regionB.x1 )), minDistance ))
   {
      if( TCTF_IsLE( regionA.y1, regionB.y2 ) &&
          TCTF_IsGE( regionA.y2, regionB.y1 ))
      {
         this->Set( TCT_Min( regionA.x1, regionB.x1 ),
                    TCT_Max( regionA.y1, regionB.y1 ),
                    TCT_Max( regionA.x2, regionB.x2 ),
                    TCT_Min( regionA.y2, regionB.y2 ));
      }
      else
      {
         this->Reset( );
      }
   }
   else if( TCTF_IsLE( static_cast< double>( fabs( regionA.y1 - regionB.y2 )), minDistance ) ||
            TCTF_IsLE( static_cast< double>( fabs( regionA.y2 - regionB.y1 )), minDistance ))
   {
      if( TCTF_IsLE( regionA.x1, regionB.x2 ) &&
          TCTF_IsGE( regionA.x2, regionB.x1 ))
      {
         this->Set( TCT_Max( regionA.x1, regionB.x1 ),
                    TCT_Min( regionA.y1, regionB.y1 ),
                    TCT_Min( regionA.x2, regionB.x2 ),
                    TCT_Max( regionA.y2, regionB.y2 ));
      }
      else
      {
         this->Reset( );
      }
   }
   else
   {
      this->Reset( );
   }
}

//===========================================================================//
void TGS_Region_c::ApplyAdjacent( 
      const TGS_Region_c& region,
            double   minDistance )
{
   this->ApplyAdjacent( *this, region, minDistance );
}

//===========================================================================//
// Method         : ApplyDifference
// Purpose        : Set this region's dimensions based on the difference of 
//                  the given region or regions.
// 
//                  Note: This method uses an estimated difference based on 
//                        the orientation of the given region.  When the 
//                        difference with the given region results in more 
//                        than one region, the max region based on 
//                        orientation is applied.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Region_c::ApplyDifference( 
      const TGS_Region_c&    regionA,
      const TGS_Region_c&    regionB,
            TGS_OrientMode_t orient )
{
   if( regionA.IsWithin( regionB ))
   {
      this->Reset( );
   }
   else if( regionA.IsIntersecting( regionB ))
   {
      if( orient == TGS_ORIENT_UNDEFINED )
      {
         orient = regionB.FindOrient( );
      }

      if( orient == TGS_ORIENT_HORIZONTAL )
      {
         if( TCTF_IsGE( regionA.x1 - regionB.x1, regionB.x2 - regionA.x2 ))
         {
	    this->Set( regionB.x1, regionB.y1, regionA.x1, regionB.y2 );
         }
         else
         {
	    this->Set( regionA.x2, regionB.y1, regionB.x2, regionB.y2 );
         }
      }
      else // if( orient == TGS_ORIENT_VERTICAL )
      {
         if( TCTF_IsGE( regionA.y1 - regionB.y1, regionB.y2 - regionA.y2 ))
         {
	    this->Set( regionB.x1, regionB.y1, regionB.x2, regionA.y1 );
         }
         else
         {
	    this->Set( regionB.x1, regionA.y2, regionB.x2, regionB.y2 );
         }
      }
   }
   else
   {
      *this = regionB;
   }
}

//===========================================================================//
void TGS_Region_c::ApplyDifference( 
      const TGS_Region_c&    region,
            TGS_OrientMode_t orient )
{
   this->ApplyDifference( *this, region, orient );
}

//===========================================================================//
// Method         : ApplyShift
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Region_c::ApplyShift(
      double           dx,
      double           dy,
      TGS_OrientMode_t orient )
{
   switch( orient )
   {
   case TGS_ORIENT_HORIZONTAL:
      this->y1 += dy;
      this->y2 += dy;
      break;

   case TGS_ORIENT_VERTICAL:
      this->x1 += dx;
      this->x2 += dx;
      break;

   default:
      this->x1 += dx;
      this->y1 += dy;
      this->x2 += dx;
      this->y2 += dy;
      break;
   }
}

//===========================================================================//
void TGS_Region_c::ApplyShift(
      double           shift,
      TGS_OrientMode_t orient )
{
   this->ApplyShift( shift, shift, orient );
}

//===========================================================================//
// Method         : IsIntersecting
// Purpose        : Return true if this region is intersecting with the given
//                  region.  Intersecting implies intersecting x/y region
//                  coordinates.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsIntersecting( 
      const TGS_Region_c& region,
            double        minArea ) const
{
   bool isIntersecting = true;

   if(( TCTF_IsLT( region.x1, this->x1 )) &&
      ( TCTF_IsLT( region.x2, this->x1 )))
   {
      isIntersecting = false;
   }
   else if(( TCTF_IsGT( region.x1, this->x2 )) &&
           ( TCTF_IsGT( region.x2, this->x2 )))
   {
      isIntersecting = false;
   }
   else if(( TCTF_IsLT( region.y1, this->y1 )) &&
           ( TCTF_IsLT( region.y2, this->y1 )))
   {
      isIntersecting = false;
   }
   else if(( TCTF_IsGT( region.y1, this->y2 )) &&
           ( TCTF_IsGT( region.y2, this->y2 )))
   {
      isIntersecting = false;
   }

   if( isIntersecting )
   {
      if( TCTF_IsLT( minArea, TC_FLT_MAX ))
      {
         TGS_Region_c intersectRegion( region );
         intersectRegion.ApplyIntersect( *this );

         isIntersecting = ( TCTF_IsGE( intersectRegion.FindArea( ), minArea ) ? 
                            true : false );
      }
   }
   return( isIntersecting );
}

//===========================================================================//
bool TGS_Region_c::IsIntersecting( 
      const TGS_Region_c& region,
            TC_SideMode_t side ) const
{
   TGS_Region_c sideRegion( region );

   switch( side )
   {
   case TC_SIDE_LEFT:
      sideRegion.Set( region.x1, region.y1, region.x1, region.y2 );
      break;

   case TC_SIDE_RIGHT:
      sideRegion.Set( region.x2, region.y1, region.x2, region.y2 );
      break;

   case TC_SIDE_LOWER:
      sideRegion.Set( region.x1, region.y1, region.x2, region.y1 );
      break;

   case TC_SIDE_UPPER:
      sideRegion.Set( region.x1, region.y2, region.x2, region.y2 );
      break;

   default:
      break;
   }
   return( this->IsIntersecting( sideRegion ));
}

//===========================================================================//
// Method         : IsOverlapping
// Purpose        : Return true if this region is overlapping with the given
//                  region.  Overlapping implies overlapping x/y region
//                  coordinates.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsOverlapping( 
      const TGS_Region_c& region,
            double        minDistance ) const
{
   bool isOverlapping = false;

   bool isOverlappingX = false;
   bool isOverlappingY = false;

   if(( TCTF_IsLE( region.x1, this->x1 )) &&
      ( TCTF_IsGE( region.x2, this->x2 )))
   {
      isOverlappingX = true;
   }
   else if(( TCTF_IsGE( region.x1, this->x1 )) &&
           ( TCTF_IsLE( region.x1, this->x2 )))
   {
      isOverlappingX = true;
   }
   else if(( TCTF_IsGE( region.x2, this->x1 )) &&
           ( TCTF_IsLE( region.x2, this->x2 )))
   {
      isOverlappingX = true;
   }

   if(( TCTF_IsLE( region.y1, this->y1 )) &&
      ( TCTF_IsGE( region.y2, this->y2 )))
   {
      isOverlappingY = true;
   }
   else if(( TCTF_IsGE( region.y1, this->y1 )) &&
           ( TCTF_IsLE( region.y1, this->y2 )))
   {
      isOverlappingY = true;
   }
   else if(( TCTF_IsGE( region.y2, this->y1 )) &&
           ( TCTF_IsLE( region.y2, this->y2 )))
   {
      isOverlappingY = true;
   }

   if( isOverlappingX )
   {
      double xMin = TCT_Max( region.x1, this->x1 );
      double xMax = TCT_Min( region.x2, this->x2 );
      if( TCTF_IsGE( xMax - xMin, minDistance ))
      {
         isOverlapping = true;
      }
   }
   if( isOverlappingY )
   {
      double yMin = TCT_Max( region.y1, this->y1 );
      double yMax = TCT_Min( region.y2, this->y2 );
      if( TCTF_IsGE( yMax - yMin, minDistance ))
      {
         isOverlapping = true;
      }
   }
   return( isOverlapping );
}

//===========================================================================//
// Method         : IsAdjacent
// Purpose        : Return true if this region is adjacent to the given 
//                  region.  Adjacent implies non-intersecting x/y region
//                  coordinates that share a common side.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsAdjacent( 
      const TGS_Region_c& region,
            double        minDistance ) const
{
   bool isAdjacent = true;

   if(( TCTF_IsLT( region.x1, this->x1 - minDistance )) &&
      ( TCTF_IsLT( region.x2, this->x1 - minDistance )))
   {
      isAdjacent = false;
   }
   else if(( TCTF_IsGT( region.x1, this->x2 + minDistance )) &&
           ( TCTF_IsGT( region.x2, this->x2 + minDistance )))
   {
      isAdjacent = false;
   }
   else if(( TCTF_IsLT( region.y1, this->y1 - minDistance )) &&
           ( TCTF_IsLT( region.y2, this->y1 - minDistance )))
   {
      isAdjacent = false;
   }
   else if(( TCTF_IsGT( region.y1, this->y2 + minDistance )) &&
           ( TCTF_IsGT( region.y2, this->y2 + minDistance )))
   {
      isAdjacent = false;
   }
   else if( this->IsWithin( region ))
   {
      isAdjacent = false;
   }
   return( isAdjacent );
}

//===========================================================================//
// Method         : IsWithin
// Purpose        : Return true if the given point or region is within (ie.
//                  completely contained by) this region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsWithin( 
      const TGS_Region_c& region ) const
{
   return( this->IsWithin( region, 0.0 ));
}

//===========================================================================//
bool TGS_Region_c::IsWithin( 
      const TGS_Region_c& region,
            double        minDistance ) const
{
   bool isWithin = false;

   if(( TCTF_IsGE( region.x1, this->x1 + minDistance )) &&
      ( TCTF_IsLE( region.x2, this->x2 - minDistance )) &&
      ( TCTF_IsGE( region.y1, this->y1 + minDistance )) &&
      ( TCTF_IsLE( region.y2, this->y2 - minDistance )))
   {
      isWithin = true;
   }
   return( isWithin );
}

//===========================================================================//
bool TGS_Region_c::IsWithin( 
      const TGS_Point_c& point ) const
{
   return( this->IsWithin( point, 0.0 ));
}

//===========================================================================//
bool TGS_Region_c::IsWithin( 
      const TGS_Point_c& point,
            double       minDistance ) const
{
   bool isWithin = false;

   if(( TCTF_IsGE( point.x, this->x1 + minDistance )) &&
      ( TCTF_IsLE( point.x, this->x2 - minDistance )) &&
      ( TCTF_IsGE( point.y, this->y1 + minDistance )) &&
      ( TCTF_IsLE( point.y, this->y2 - minDistance )))
   {
      isWithin = true;
   }
   return( isWithin );
}

//===========================================================================//
// Method         : IsWithinDx
// Purpose        : Return true if the given point or region is within this 
//                  region's width (ie. dx) dimensions.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsWithinDx( 
      const TGS_Region_c& region ) const
{
   bool isWithinDx = false;

   if(( TCTF_IsGE( region.x1, this->x1 )) &&
      ( TCTF_IsLE( region.x2, this->x2 )))
   {
      isWithinDx = true;
   }
   return( isWithinDx );
}

//===========================================================================//
// Method         : IsWithinDy
// Purpose        : Return true if the given point or region is within this 
//                  region's height (ie. dy) dimensions.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsWithinDy( 
      const TGS_Region_c& region ) const
{
   bool isWithinDy = false;

   if(( TCTF_IsGE( region.y1, this->y1 )) &&
      ( TCTF_IsLE( region.y2, this->y2 )))
   {
      isWithinDy = true;
   }
   return( isWithinDy );
}

//===========================================================================//
// Method         : IsCorner
// Purpose        : Return true if the given region is within this region's
//                  lower-left, lower-right, upper-right, or upper-left 
//                  corner.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsCorner( 
      const TGS_Region_c& region ) const
{
   bool isCorner = false;

   if( TCTF_IsLT( this->x1, region.x1 ) &&
       TCTF_IsGT( this->x2, region.x1 ) &&
       TCTF_IsLT( this->x2, region.x2 ))
   {
      if( TCTF_IsLT( this->y1, region.y1 ) &&
          TCTF_IsGT( this->y2, region.y1 ) &&
          TCTF_IsLT( this->y2, region.y2 ))
      {
         // Found region relative to lower-left corner
         isCorner = true;
      }
      if( TCTF_IsGT( this->y1, region.y1 ) &&
          TCTF_IsLT( this->y1, region.y2 ) &&
          TCTF_IsGT( this->y2, region.y2 ))
      {
         // Found region relative to upper-left corner
         isCorner = true;
      }
   }

   if( TCTF_IsGT( this->x1, region.x1 ) &&
       TCTF_IsLT( this->x1, region.x2 ) &&
       TCTF_IsGT( this->x2, region.x2 ))
   {
      if( TCTF_IsLT( this->y1, region.y1 ) &&
          TCTF_IsGT( this->y2, region.y1 ) &&
          TCTF_IsLT( this->y2, region.y2 ))
      {
         // Found region relative to lower-right corner
         isCorner = true;
      }
      if( TCTF_IsGT( this->y1, region.y1 ) &&
          TCTF_IsLT( this->y1, region.y2 ) &&
          TCTF_IsGT( this->y2, region.y2 ))
      {
         // Found region relative to upper-right corner
         isCorner = true;
      }
   }
   return( isCorner );
}

//===========================================================================//
// Method         : IsCrossed
// Purpose        : Return true if the given region is crossed with respect
//                  to this regions.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsCrossed( 
      const TGS_Region_c& region,
            double        minDistance ) const
{
   bool isCrossed = false;

   if( this->IsCrossed( region, TGS_ORIENT_HORIZONTAL, minDistance ))
   {
      isCrossed = true;
   }
   if( this->IsCrossed( region, TGS_ORIENT_VERTICAL, minDistance ))
   {
      isCrossed = true;
   }
   return( isCrossed );
}

//===========================================================================//
bool TGS_Region_c::IsCrossed( 
      const TGS_Region_c&    region,
    	    TGS_OrientMode_t orient,
            double           minDistance ) const
{
   bool isCrossed = false;

   switch( orient )
   {
   case TGS_ORIENT_HORIZONTAL:

      if( TCTF_IsLE( region.x1, this->x1 - minDistance ) &&
          TCTF_IsGE( region.x2, this->x2 + minDistance ))
      {
         if( TCTF_IsGE( region.y1, this->y1 + minDistance ) &&
             TCTF_IsLE( region.y2, this->y2 - minDistance ))
         {
      	    isCrossed = true;
         }
      }
      break;

   case TGS_ORIENT_VERTICAL:

      if( TCTF_IsLE( region.y1, this->y1 - minDistance ) &&
          TCTF_IsGE( region.y2, this->y2 + minDistance ))
      {
         if( TCTF_IsGE( region.x1, this->x1 + minDistance ) &&
             TCTF_IsLE( region.x2, this->x2 - minDistance ))
         {
	    isCrossed = true;
         }
      }
      break;

   default:
      break;
   }
   return( isCrossed );
}

//===========================================================================//
// Method         : IsWide
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsWide( 
      void ) const
{
   double dx = this->GetDx( );
   double dy = this->GetDy( );

  return( TCTF_IsGE( dx, dy ) ? true : false );
}

//===========================================================================//
// Method         : IsTall
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsTall( 
      void ) const
{
   double dx = this->GetDx( );
   double dy = this->GetDy( );

  return( TCTF_IsGE( dy, dx ) ? true : false );
}

//===========================================================================//
// Method         : IsSquare
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsSquare( 
      void ) const
{
   double dx = this->GetDx( );
   double dy = this->GetDy( );

   return( TCTF_IsEQ( dy, dx ) ? true : false );
}

//===========================================================================//
// Method         : IsLegal
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsLegal( 
      void ) const
{
   TGS_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   return( thisBox.IsLegal( ));
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Region_c::IsValid( 
      void ) const
{
   return(( TCTF_IsNEQ( this->x1, TC_FLT_MIN )) &&
          ( TCTF_IsNEQ( this->x1, TC_FLT_MAX )) &&
          ( TCTF_IsNEQ( this->y1, TC_FLT_MIN )) &&
          ( TCTF_IsNEQ( this->y1, TC_FLT_MAX )) &&
          ( TCTF_IsNEQ( this->x2, TC_FLT_MIN )) &&
          ( TCTF_IsNEQ( this->x2, TC_FLT_MAX )) &&
          ( TCTF_IsNEQ( this->y2, TC_FLT_MIN )) &&
          ( TCTF_IsNEQ( this->y2, TC_FLT_MAX )) ?
          true : false );
}
