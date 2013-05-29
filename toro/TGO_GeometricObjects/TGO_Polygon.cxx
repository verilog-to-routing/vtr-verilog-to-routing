//===========================================================================//
// Purpose : Method definitions for the TGO_Polygon class.
//
//           Public methods include:
//           - TGO_Polygon_c, ~TGO_Polygon_c
//           - operator=
//           - operator==, operator!=
//           - operator[]
//           - Print
//           - ExtractString
//           - Set, Reset
//           - Add
//           - Delete
//           - Replace
//           - FindArea
//           - FindNearest
//           - FindSide
//           - IsEdge
//           - IsWithin
//           - IsIntersecting
//           - IsCorner
//           - IsConvexCorner
//           - IsConcaveCorner
//           - IsOrthogonal
//           - IsRectilinear
//
//           Private methods include:
//           - IsLastLinear_
//           - IsLastConnected_
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

#include "TIO_PrintHandler.h"

#include "TGO_Polygon.h"

//===========================================================================//
// Method         : TGO_Polygon
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
TGO_Polygon_c::TGO_Polygon_c( 
      void )
      :
      pointList_( TGO_POINT_LIST_DEF_CAPACITY ),
      region_( INT_MAX, INT_MAX, INT_MIN, INT_MIN )
{
}

//===========================================================================//
TGO_Polygon_c::TGO_Polygon_c( 
      const TGO_PointList_t& pointList )
      :
      region_( INT_MAX, INT_MAX, INT_MIN, INT_MIN )
{
   this->Set( pointList );
}

//===========================================================================//
TGO_Polygon_c::TGO_Polygon_c( 
      const TGO_Region_c& region )
      :
      region_( INT_MAX, INT_MAX, INT_MIN, INT_MIN )
{
   this->Set( region );
}

//===========================================================================//
TGO_Polygon_c::TGO_Polygon_c( 
      const TGO_Polygon_c& polygon )
      :
      region_( INT_MAX, INT_MAX, INT_MIN, INT_MIN )
{
   this->Set( polygon.pointList_ );
}

//===========================================================================//
// Method         : ~TGO_Polygon
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
TGO_Polygon_c::~TGO_Polygon_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
TGO_Polygon_c& TGO_Polygon_c::operator=( 
      const TGO_Polygon_c& polygon )
{
   if( &polygon != this )
   {
      this->pointList_ = polygon.pointList_;
      this->region_ = polygon.region_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
bool TGO_Polygon_c::operator==( 
      const TGO_Polygon_c& polygon ) const
{
   return(( this->pointList_ == polygon.pointList_ ) &&
          ( this->region_ == polygon.region_ ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
bool TGO_Polygon_c::operator!=( 
      const TGO_Polygon_c& polygon ) const
{
   return(( !this->operator==( polygon )) ? true : false );
}

//===========================================================================//
// Method         : operator[]
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
TGO_Point_c* TGO_Polygon_c::operator[]( 
      size_t i )
{
   return( this->pointList_.operator[]( i ));
}

//===========================================================================//
const TGO_Point_c* TGO_Polygon_c::operator[]( 
      size_t i ) const
{
   return( this->pointList_.operator[]( i ));
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
void TGO_Polygon_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srPolygon;
   this->ExtractString( &srPolygon );

   printHandler.Write( pfile, spaceLen, "[polygon] %s\n", TIO_SR_STR( srPolygon ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
void TGO_Polygon_c::ExtractString( 
      string* psrPolygon ) const
{
   if( psrPolygon )
   {
      if( this->pointList_.IsValid( ))
      {
         *psrPolygon = "";
         for( size_t i = 0; i < this->pointList_.GetLength( ); ++i )
         {
            const TGO_Point_c& point = *this->pointList_[i];

            string srPoint;
            point.ExtractString( &srPoint );

            *psrPolygon += ( i == 0 ? "" : " " );
            *psrPolygon += srPoint;
         }
      }
      else
      {
         *psrPolygon = "?";
      }
   }
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
void TGO_Polygon_c::Set( 
      const TGO_PointList_t& pointList )
{
   this->pointList_.SetCapacity( pointList.GetLength( ) + 1 );
   this->Add( pointList );
}

//===========================================================================//
void TGO_Polygon_c::Set( 
      const TGO_Region_c& region )
{
   this->pointList_.SetCapacity( 4 );
   this->Add( region.x1, region.y1 );
   this->Add( region.x2, region.y1 );
   this->Add( region.x2, region.y2 );
   this->Add( region.x1, region.y2 );
}

//===========================================================================//
void TGO_Polygon_c::Set( 
      const TGO_Polygon_c& polygon )
{
   this->Set( polygon.pointList_ );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
void TGO_Polygon_c::Reset(       
      void )
{
   this->pointList_.Clear( );
   this->region_.Reset( );
}

//===========================================================================//
// Method         : Add
// Purpose        : Add the given point(s) to this polygon's point list.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
void TGO_Polygon_c::Add( 
      const TGO_PointList_t& pointList )
{
   for( size_t i = 0; i < pointList.GetLength( ); ++i )
   {
      const TGO_Point_c& point = *pointList[i];
      this->Add( point );
   }
}

//===========================================================================//
void TGO_Polygon_c::Add( 
      const TGO_Point_c& point )
{
   this->pointList_.Add( point );
   this->region_.ApplyUnion( point );
}

//===========================================================================//
void TGO_Polygon_c::Add( 
      const TGO_Point_c& pointA,
      const TGO_Point_c& pointB )
{
   this->Add( pointA );
   this->Add( pointB );
}

//===========================================================================//
void TGO_Polygon_c::Add( 
      const TGO_Line_c& edge )
{
   TGO_Point_c beginPoint( edge.x1, edge.y1 );
   TGO_Point_c endPoint( edge.x2, edge.y2 );

   if( this->IsLastLinear_( edge ))
   {
      size_t len = this->pointList_.GetLength( );
      const TGO_Point_c& prevPoint = *this->pointList_[len-1];
      this->Delete( prevPoint );
      this->Add( endPoint );
   }
   else if( this->IsLastConnected_( edge ))
   {
      this->Add( endPoint );
   }
   else 
   {
      this->Add( beginPoint );
      this->Add( endPoint );
   }
}

//===========================================================================//
void TGO_Polygon_c::Add( 
      int x, 
      int y )
{
   TGO_Point_c point( x, y );
   this->Add( point );
}

//===========================================================================//
// Method         : Delete
// Purpose        : Delete the given point from this polygon's point list.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
void TGO_Polygon_c::Delete( 
      const TGO_Point_c& point )
{
   this->pointList_.Delete( point );
}

//===========================================================================//
// Method         : Replace
// Purpose        : Replace the given point in this polygon's point list with
//                  given point data.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
void TGO_Polygon_c::Replace( 
      const TGO_Point_c& point,
      const TGO_Point_c& pointPrime )
{
   for( size_t i = 0; i < this->pointList_.GetLength( ); ++i )
   {
      if( TCTF_IsEQ( this->pointList_[i]->x, point.x ) &&
          TCTF_IsEQ( this->pointList_[i]->y, point.y ))
      {
         this->pointList_[i]->x = pointPrime.x;
         this->pointList_[i]->y = pointPrime.y;
      }
   }
}

//===========================================================================//
// Method         : FindArea
// Purpose        : The methods computes and returns the area of the given
//                  polygon using Planimeters and Green's Theorem. This
//                  theorem finds area by summing trapezoids formed by an 
//                  enclosed polygon. The formula used it:
//                  
//                     1/2 * sum( xp[i] * yp[i+1] - xp[i+1] * yp[i] )
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
size_t TGO_Polygon_c::FindArea( 
      void ) const
{
   size_t area = 0;

   size_t len = this->pointList_.GetLength( );
   if( *this->pointList_[0] == *this->pointList_[len-1] )
   {
      --len;
   }

   for( size_t i = 0; i < len; ++i )
   {
      int j = ( i < len - 1 ? i + 1 : 0 );
      const TGO_Point_c& pointA = *this->pointList_[i];
      const TGO_Point_c& pointB = *this->pointList_[j];

      area += pointA.x * pointB.y; 
      area -= pointB.x * pointA.y;
   }
   area /= 2;

   return( area );
}

//===========================================================================//
// Method         : FindNearest
// Purpose        : Find and return the nearest point or line edge in this 
//                  polygon based on the given point.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
void TGO_Polygon_c::FindNearest( 
      const TGO_Point_c& refPoint,
            TGO_Point_c* pthisNearestPoint ) const
{
   if( pthisNearestPoint )
   {
      pthisNearestPoint->Reset( );

      if( this->pointList_.IsValid( ))
      {
         TGO_Line_c thisNearestEdge;
         this->FindNearest( refPoint, &thisNearestEdge );
         thisNearestEdge.FindNearest( refPoint, pthisNearestPoint );
      }
   }
}

//===========================================================================//
void TGO_Polygon_c::FindNearest( 
      const TGO_Point_c& refPoint,
            TGO_Line_c*  pthisNearestEdge ) const
{
   if( pthisNearestEdge )
   {
      pthisNearestEdge->Reset( );

      if( this->pointList_.IsValid( ) &&
          this->pointList_.GetLength( ) >= 2 )
      {
         double minDist = TC_FLT_MAX;

         size_t len = this->pointList_.GetLength( );
         for( size_t i = 0; i < len; ++i )
         {
            size_t j = ( i < len - 1 ? i + 1 : 0 );
            TGO_Line_c pointEdge( *this->pointList_[i], *this->pointList_[j]);

            double pointDist = pointEdge.FindDistance( refPoint );
            if( TCTF_IsLT( pointDist, minDist ))
            {
               minDist = pointDist;
               *pthisNearestEdge = pointEdge;
            }
         }
      }
   }
}

//===========================================================================//
// Method         : FindSide
//                : Find and return a polygon side based on the given line
//                  edge.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/27/04 erullan : Original
//===========================================================================//
TC_SideMode_t TGO_Polygon_c::FindSide(
      const TGO_Line_c& edge ) const
{
   TC_SideMode_t side = TC_SIDE_UNDEFINED;

   TGO_Point_c centerPoint = edge.FindCenter( );

   if( edge.IsHorizontal( )) 
   {
      TGO_Point_c bottomPoint( centerPoint.x, centerPoint.y - 1 );
      TGO_Point_c topPoint( centerPoint.x, centerPoint.y + 1 );

      if( this->IsWithin( bottomPoint ) && !this->IsWithin( topPoint )) 
      {
         side = TC_SIDE_TOP;
      }
      else if( !this->IsWithin( bottomPoint ) && this->IsWithin( topPoint )) 
      {
         side = TC_SIDE_BOTTOM;
      }
   }
   else if( edge.IsVertical( )) 
   {
      TGO_Point_c leftPoint( centerPoint.x - 1, centerPoint.y );
      TGO_Point_c rightPoint( centerPoint.x + 1, centerPoint.y );

      if( this->IsWithin( leftPoint ) && !this->IsWithin( rightPoint )) 
      {
         side = TC_SIDE_RIGHT;
      }
      else if( !this->IsWithin( leftPoint ) && this->IsWithin( rightPoint )) 
      {
         side = TC_SIDE_LEFT;
      }
   }
   return( side );
}

//===========================================================================//
// Method         : IsEdge
// Purpose        : Return true if the given point or line edge is on this
//                  polygon's edge line.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
bool TGO_Polygon_c::IsEdge( 
      const TGO_PointList_t& pointList ) const
{
   bool isEdge = false;

   for( size_t i = 0; i < pointList.GetLength( ); ++i )
   {
      isEdge = true;

      const TGO_Point_c& point = *pointList[i];
      if( !this->IsEdge( point ))
      {
         isEdge = false;
         break;
      }
   }
   return( isEdge );
}

//===========================================================================//
bool TGO_Polygon_c::IsEdge( 
      const TGO_Point_c& point ) const
{
   bool isEdge = false;

   size_t len = this->pointList_.GetLength( );
   for( size_t i = 0; i < len; ++i )
   {
      size_t j = ( i < len - 1 ? i + 1 : 0 );
      TGO_Line_c lineEdge( *this->pointList_[i], *this->pointList_[j] );
      if( lineEdge.IsIntersecting( point ))
      {
         isEdge = true;
         break;
      }
   }
   return( isEdge );
}

//===========================================================================//
bool TGO_Polygon_c::IsEdge( 
      const TGO_Line_c& edge ) const
{
   bool isEdge = false;

   TGO_Point_c beginPoint( edge.x1, edge.y1 );
   TGO_Point_c endPoint( edge.x2, edge.y2 );

   size_t len = this->pointList_.GetLength( );
   for( size_t i = 0; i < len; ++i )
   {
      int j = ( i < len - 1 ? i + 1 : 0 );
      TGO_Line_c lineEdge( *this->pointList_[i ], *this->pointList_[j] );
      if( lineEdge.IsIntersecting( beginPoint ) &&
          lineEdge.IsIntersecting( endPoint ))
      {
         isEdge = true;
         break;
      }
   }
   return( isEdge );
}

//===========================================================================//
bool TGO_Polygon_c::IsEdge( 
      int x, 
      int y ) const
{
   TGO_Point_c point( x, y );
   return( this->IsEdge( point ));
}

//===========================================================================//
// Method         : IsWithin
// Refernce       : This code is based on the "PNPOLY - Point Inclusion in
//                  Polygon Test" algorithm as described by Wm Randolph 
//                  Franklin at his web page:
//                  
//                  http://www.ecse.rpi.edu/Homepages/wrf/geom/pnpoly.html
//                  
//                  A pseudo-code description is:
//                  
//                  int pnpoly(int npol, float *xp, float *yp, float x, float y)
//                  {
//                    int i, j, c = 0;
//                    for (i = 0, j = npol-1; i < npol; j = i++) {
//                      if ((((yp[i]<=y) && (y<yp[j])) ||
//                           ((yp[j]<=y) && (y<yp[i]))) &&
//                          (x < (xp[j] - xp[i]) * (y - yp[i]) / 
//                          (yp[j] - yp[i]) + xp[i]))
//                        c = !c;
//                    }
//                    return c;
//                  }
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
bool TGO_Polygon_c::IsWithin( 
      const TGO_PointList_t& pointList ) const
{
   bool isWithin = true;

   for( size_t i = 0; i < pointList.GetLength( ); ++i )
   {
      const TGO_Point_c& point = *pointList[i];
      if( !this->IsWithin( point ))
      {
         isWithin = false;
         break;
      }
   }
   return( isWithin );
}

//===========================================================================//
bool TGO_Polygon_c::IsWithin( 
      const TGO_Point_c& point ) const
{
   bool isWithin = false;

   int c = 0;
   size_t i, j;
   size_t len = this->pointList_.GetLength( );
   for( i = 0, j = len - 1; i < len; j = i++ ) 
   {
      const TGO_Point_c& pointA = *this->pointList_[i];
      const TGO_Point_c& pointB = *this->pointList_[j];

      if((( pointA.y <= point.y ) && ( point.y < pointB.y )) ||
         (( pointB.y <= point.y ) && ( point.y < pointA.y )))
      {
         double d = static_cast< double >( pointB.x - pointA.x ) * 
                    static_cast< double >( point.y - pointA.y ) / 
                    static_cast< double >( pointB.y - pointA.y ) + 
                    static_cast< double >( pointA.x );
         if( TCTF_IsLT( static_cast< double >( point.x ), d ))
         {
            c = !c;
         }
      }
   }

   isWithin = ( c ? true : false );
   if( isWithin )
   {
      isWithin = ( !this->IsEdge( point ) ? true : false );
   }
   return( isWithin );
}

//===========================================================================//
bool TGO_Polygon_c::IsWithin( 
      int x, 
      int y ) const
{
   TGO_Point_c point( x, y );
   return( this->IsWithin( point ));
}

//===========================================================================//
// Method         : IsIntersecting
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
bool TGO_Polygon_c::IsIntersecting( 
      const TGO_Line_c& edge,
            TGO_Line_c* pedge ) const
{
   bool isIntersecting = false;

   size_t len = this->pointList_.GetLength( );
   if( len >= 2 )
   {
      for( size_t i = 0; i < len; ++i )
      {
         size_t j = ( i < len - 1 ? i + 1 : 0 );
         TGO_Line_c pointEdge( *this->pointList_[i], *this->pointList_[j]);
         if( pointEdge.IsIntersecting( edge ))
         {
            isIntersecting = true;

            if( pedge )
            {
               *pedge = pointEdge;
            }
            break;
         }
      }
   }
   return( isIntersecting );
}

//===========================================================================//
bool TGO_Polygon_c::IsIntersecting( 
      const TGO_Line_c&  edge,
            TGO_Point_c* ppoint ) const
{
   bool isIntersecting = false;

   size_t len = this->pointList_.GetLength( );
   if( len >= 2 )
   {
      for( size_t i = 0; i < len; ++i )
      {
         size_t j = ( i < len - 1 ? i + 1 : 0 );
         TGO_Line_c pointEdge( *this->pointList_[i], *this->pointList_[j] );
         if( pointEdge.IsIntersecting( edge, ppoint ))
         {
            isIntersecting = true;
            break;
         }
      }
   }
   return( isIntersecting );
}

//===========================================================================//
// Method         : IsCorner
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
bool TGO_Polygon_c::IsCorner( 
      const TGO_Point_c& point ) const
{
   bool isCorner = false;

   TGO_Line_c edge;
   this->FindNearest( point, &edge );
   if( edge.IsValid( ))
   {
      TGO_Point_c beginPoint = edge.GetBeginPoint( );
      TGO_Point_c endPoint = edge.GetEndPoint( );

      if (( point == beginPoint ) || ( point == endPoint ))
      {
         size_t len = this->pointList_.GetLength( );
         size_t i = this->pointList_.FindIndex( point );
         size_t p = ( i > 0 ? i - 1 : len - 1 );
         size_t n = ( i < len - 1 ? i + 1 : 0 );
         const TGO_Point_c& prevPoint = *this->pointList_[p];
         const TGO_Point_c& nextPoint = *this->pointList_[n];

         TGO_Line_c prevEdge( prevPoint, point );
         TGO_Line_c nextEdge( point, nextPoint );
         isCorner = ( !prevEdge.IsParallel( nextEdge ) ? true : false );
      }
   }
   return( isCorner );
}

//===========================================================================//
bool TGO_Polygon_c::IsCorner( 
      int x, 
      int y ) const
{
   TGO_Point_c point( x, y );
   return( this->IsCorner( point ));
}

//===========================================================================//
// Method         : IsConvexCorner
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
bool TGO_Polygon_c::IsConvexCorner( 
      const TGO_Point_c& point ) const
{
   bool isConvexCorner = false;

   if( this->IsCorner( point ))
   {
      // Build region based on bounding box formed from two edges that make corner
      size_t len = this->pointList_.GetLength( );
      size_t i = this->pointList_.FindIndex( point );
      size_t p = ( i > 0 ? i - 1 : len - 1 );
      size_t n = ( i < len - 1 ? i + 1 : 0 );
      const TGO_Point_c& prevPoint = *this->pointList_[p];
      const TGO_Point_c& nextPoint = *this->pointList_[n];
      TGO_Region_c edgeRegion( point, point );
      edgeRegion.ApplyUnion( prevPoint );
      edgeRegion.ApplyUnion( nextPoint );

      // Build a 1x1 region based on original point intersecting with corner region
      TGO_Region_c region( point, point );
      region.ApplyScale( 1 );
      region.ApplyIntersect( edgeRegion );

      // Test for 1x1 region is within polygon area (convex) or not (concave)
      if( this->IsWithin( region.x1, region.y1 ) ||
          this->IsWithin( region.x1, region.y2 ) ||
          this->IsWithin( region.x2, region.y2 ) ||
          this->IsWithin( region.x2, region.y1 ))
      {
         isConvexCorner = true;
      }
   }
   return( isConvexCorner );
}

//===========================================================================//
bool TGO_Polygon_c::IsConvexCorner( 
      int x, 
      int y ) const
{
   TGO_Point_c point( x, y );
   return( this->IsConvexCorner( point ));
}

//===========================================================================//
// Method         : IsConcaveCorner
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
bool TGO_Polygon_c::IsConcaveCorner( 
      const TGO_Point_c& point ) const
{
   bool isConcaveCorner = false;

   if( this->IsCorner( point ) &&
       !this->IsConvexCorner( point ))
   {
      isConcaveCorner = true;
   }
   return( isConcaveCorner );
}

//===========================================================================//
bool TGO_Polygon_c::IsConcaveCorner( 
      int x, 
      int y ) const
{
   TGO_Point_c point( x, y );
   return( this->IsConcaveCorner( point ));
}

//===========================================================================//
// Method         : IsOrthogonal
// Purpose        : Return true if the current polygon point list is fully
//                  orthogonal. In other words, every pair of points is 
//                  horizontally or vertically aligned. This method also 
//                  returns the first pair of non-orthogonal points, if any.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
bool TGO_Polygon_c::IsOrthogonal( 
      TGO_Point_c* ppointA, 
      TGO_Point_c* ppointB ) const 
{
   bool isOrthogonal = true;

   size_t len = this->pointList_.GetLength( );
   for( size_t i = 0; i < len; ++i )
   {
      int j = ( i < len - 1 ? i + 1 : 0 );
      const TGO_Point_c& pointA = *this->pointList_[i];
      const TGO_Point_c& pointB = *this->pointList_[j];

      if( !pointA.IsOrthogonal( pointB ))
      {
         isOrthogonal = false;

         if( ppointA )
         {
            *ppointA = pointA;
         }
         if( ppointB )
         {
            *ppointB = pointB;
         }
         break;
      }
   }
   return( isOrthogonal );
}

//===========================================================================//
// Method         : IsRectilinear
// Purpose        : This method determines if the polygon is rectilinear in 
//                  shape by testing the points of the polygon to see if they
//                  fall along the edges of the containing region formed by
//                  the points of the polygon.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/18/03 erullan : Original
//===========================================================================//
bool TGO_Polygon_c::IsRectilinear( 
      void ) const
{
   bool isRectilinear = false;

   const TGO_Region_c& region = this->region_;
   TGO_Line_c leftEdge( region.x1, region.y1, region.x1, region.y2 );
   TGO_Line_c rightEdge( region.x2, region.y1, region.x2, region.y2 );
   TGO_Line_c bottomEdge( region.x1, region.y1, region.x2, region.y1 );
   TGO_Line_c topEdge( region.x1, region.y2, region.x2, region.y2 );

   size_t len = this->pointList_.GetLength( );
   for( size_t i = 0; i < len; i++ )
   {
      const TGO_Point_c& point = *this->pointList_[i];

      if(( !leftEdge.IsWithin( point )) &&
         ( !rightEdge.IsWithin( point )) &&
         ( !bottomEdge.IsWithin( point )) &&
         ( !topEdge.IsWithin( point ))) 
      {
         isRectilinear = true;
         break;
      }
   }
   return( isRectilinear );
}

//===========================================================================//
// Method         : IsLastLinear_
// Purpose        : Return true if the given line edge is linear with respect
//                  to this polygon's last (ie. most recently added) points.
//                  A linear edge implies the given edge is linear with the
//                  (ie. is connected to and is parallel with) polygon's
//                  last two points.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
bool TGO_Polygon_c::IsLastLinear_( 
      const TGO_Line_c& edge ) const
{
   bool isLastLinear = false;

   size_t len = this->pointList_.GetLength( );
   if( len >= 2 )
   {
      TGO_Point_c beginPoint( edge.x1, edge.y1 );
      if( *this->pointList_[len-1] == beginPoint )
      {
         TGO_Line_c prevLine( *this->pointList_[len-2],
                              *this->pointList_[len-1] );
         if( prevLine.IsParallel( edge ))
         {
            isLastLinear = true;
         }
      }
   }
   return( isLastLinear );
}

//===========================================================================//
// Method         : IsLastConneced_
// Purpose        : Return true if the given line edge is connected with
//                  respect to this polygon's last (ie. most recently added) 
//                  point. A connected line implies the given edge's begin 
//                  point is equal to the polygon's last point.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/13 jeffr : Original
//===========================================================================//
bool TGO_Polygon_c::IsLastConnected_(
      const TGO_Line_c& edge ) const
{
   bool isLastConnected = false;

   size_t len = this->pointList_.GetLength( );
   if( len >= 1 )
   {
      TGO_Point_c beginPoint( edge.x1, edge.y1 );
      if( *this->pointList_[len-1] == beginPoint )
      {
         isLastConnected = true;
      }
   }
   return( isLastConnected );
}
