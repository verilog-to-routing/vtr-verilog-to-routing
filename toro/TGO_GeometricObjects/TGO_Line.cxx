//===========================================================================//
// Purpose : Method definitions for the TGO_Line class.
//
//           Public methods include:
//           - TGO_Line_c, ~TGO_Line_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set, Reset
//           - GetBeginPoint, GetEndPoint
//           - FindLength
//           - FindCenter
//           - FindOrient
//           - FindDistance
//           - FindNearest
//           - FindIntersect
//           - CrossProduct
//           - IsConnected
//           - IsIntersecting
//           - IsWithin
//           - IsParallel
//           - IsLeft, IsRight, IsLower, IsUpper
//           - IsLowerLeft, IsLowerRight, IsUpperLeft, IsUpperRight
//           - IsOrthogonal
//           - IsHorizontal, IsVertical
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

#include "TIO_PrintHandler.h"

#include "TGO_Box.h"
#include "TGO_Region.h"
#include "TGO_Line.h"

//===========================================================================//
// Method         : TGO_Line_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGO_Line_c::TGO_Line_c( 
      void )
      :
      x1( INT_MIN ),
      y1( INT_MIN ),
      x2( INT_MIN ),
      y2( INT_MIN )
{
}

//===========================================================================//
TGO_Line_c::TGO_Line_c( 
      int x1_, 
      int y1_,
      int x2_, 
      int y2_ )
{
   this->Set( x1_, y1_, x2_, y2_ );
}

//===========================================================================//
TGO_Line_c::TGO_Line_c( 
      const TGO_Point_c& beginPoint,
      const TGO_Point_c& endPoint )
{
   this->Set( beginPoint, endPoint );
}

//===========================================================================//
TGO_Line_c::TGO_Line_c( 
      const TGO_Line_c& line )
{
   this->Set( line );
}

//===========================================================================//
// Method         : ~TGO_Line_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGO_Line_c::~TGO_Line_c( 
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
TGO_Line_c& TGO_Line_c::operator=( 
      const TGO_Line_c& line )
{
   if( &line != this )
   {
      this->x1 = line.x1;
      this->y1 = line.y1;
      this->x2 = line.x2;
      this->y2 = line.y2;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGO_Line_c::operator==( 
      const TGO_Line_c& line ) const
{
   return(( this->x1 == line.x1 ) &&
          ( this->y1 == line.y1 ) &&
          ( this->x2 == line.x2 ) &&
          ( this->y2 == line.y2 ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGO_Line_c::operator!=( 
      const TGO_Line_c& line ) const
{
   return(( !this->operator==( line )) ? 
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGO_Line_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srLine;
   this->ExtractString( &srLine );

   printHandler.Write( pfile, spaceLen, "[line] %s\n", TIO_SR_STR( srLine ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGO_Line_c::ExtractString( 
      string* psrLine ) const
{
   if( psrLine )
   {
      char szLine[TIO_FORMAT_STRING_LEN_LINE];

      if( this->IsValid( ))
      {
         sprintf( szLine, "%d %d %d %d", this->x1, this->y1, this->x2, this->y2 );
      }
      else
      {
         sprintf( szLine, "? ? ? ?" );
      }
      *psrLine = szLine;
   }
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGO_Line_c::Set( 
      int x1_, 
      int y1_,
      int x2_, 
      int y2_ )
{
   this->x1 = x1_;
   this->y1 = y1_;
   this->x2 = x2_;
   this->y2 = y2_;
}

//===========================================================================//
void TGO_Line_c::Set(
      const TGO_Point_c& beginPoint,
      const TGO_Point_c& endPoint )
{
   this->Set( beginPoint.x, beginPoint.y, endPoint.x, endPoint.y );
}

//===========================================================================//
void TGO_Line_c::Set( 
      const TGO_Line_c& line )
{
   this->Set( line.x1, line.y1, line.x2, line.y2 );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGO_Line_c::Reset( 
      void )
{
   this->Set( INT_MIN, INT_MIN, INT_MIN, INT_MIN );
}

//===========================================================================//
// Method         : GetBeginPoint
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGO_Point_c TGO_Line_c::GetBeginPoint( 
      void ) const
{
   TGO_Point_c beginPoint( this->x1, this->y1 );

   return( beginPoint );
}

//===========================================================================//
// Method         : GetEndPoint
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGO_Point_c TGO_Line_c::GetEndPoint( 
      void ) const
{
   TGO_Point_c endPoint( this->x2, this->y2 );

   return( endPoint );
}

//===========================================================================//
// Method         : FindLength
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
int TGO_Line_c::FindLength( 
      void ) const
{
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( static_cast< int >( beginPoint.FindDistance( endPoint )));
}

//===========================================================================//
// Method         : FindCenter
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGO_Point_c TGO_Line_c::FindCenter( 
      void ) const
{
   TGO_Point_c center( this->x1 + (( this->x2 - this->x1 ) / 2 ),
                       this->y1 + (( this->y2 - this->y1 ) / 2 ));
   return( center );
}

//===========================================================================//
// Method         : FindOrient
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGO_OrientMode_t TGO_Line_c::FindOrient( 
      void ) const
{
   TGO_Box_c thisBox( this->x1, this->y1, this->x2, this->y2 );

   return( thisBox.FindOrient( ));
}

//===========================================================================//
// Method         : FindDistance
// Purpose        : Compute and return the "flightline" distance between
//                  the nearest point on this line and the given line or 
//                  point.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGO_Line_c::FindDistance( 
      const TGO_Line_c& refLine ) const
{
   TGO_Point_c refPoint, thisPoint;
   this->FindNearest( refLine, &refPoint, &thisPoint );

   return( thisPoint.FindDistance( refPoint ));
}

//===========================================================================//
double TGO_Line_c::FindDistance( 
      const TGO_Point_c& refPoint ) const
{
   TGO_Point_c thisPoint;
   this->FindNearest( refPoint, &thisPoint );

   return( thisPoint.FindDistance( refPoint ));
}

//===========================================================================//
// Method         : FindNearest
// Purpose        : Find and return the nearest points between this line and
//                  the given line or point. The nearest points are found 
//                  using the following algorithm:
//                  
//                   - Let the point be 'C' and the line segment be 'AB'
//                     from point 'A' to point 'B'.
//                   - Let 'P' be the point of perpendicular projection 
//                     of 'C' onto 'AB'.
//                   - Compute 'r' as:
//                  
//                            (A.y-C.y)(A.y-B.y) - (A.x-XC)(B.x-A.x)
//                        r = --------------------------------------
//                                   (B.x-A.x)^2 + (B.y-A.y)^2
//                  
//                   - Compute 'P' as:
//                  
//                        P.x = A.x + r(B.x-A.x)
//                        P.y = A.y + r(B.y-A.y)
//
// Reference      : comp.graphics.algorithms FAQ by O'Rourke
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGO_Line_c::FindNearest( 
      const TGO_Line_c&  refLine,
            TGO_Point_c* prefNearestPoint,
            TGO_Point_c* pthisNearestPoint ) const
{
   TGO_Point_c refNearestPoint, thisNearestPoint;

   if( !this->IsIntersecting( refLine ))
   {
      double minDistance = TC_FLT_MAX;

      TGO_Point_c thisBeginPoint( this->GetBeginPoint( ));
      TGO_Point_c thisEndPoint( this->GetEndPoint( ));
      TGO_Point_c refBeginPoint( refLine.GetBeginPoint( ));
      TGO_Point_c refEndPoint( refLine.GetEndPoint( ));

      TGO_Point_c thisPoint;
      this->FindNearest( refBeginPoint, &thisPoint );
      if( TCTF_IsGT( minDistance, thisPoint.FindDistance( refBeginPoint )))
      {
         minDistance = thisPoint.FindDistance( refBeginPoint );
         thisNearestPoint = thisPoint;
         refNearestPoint = refBeginPoint;
      }

      this->FindNearest( refEndPoint, &thisPoint );
      if( TCTF_IsGT( minDistance, thisPoint.FindDistance( refEndPoint )))
      {
         minDistance = thisPoint.FindDistance( refEndPoint );
         thisNearestPoint = thisPoint;
         refNearestPoint = refEndPoint;
      }

      TGO_Point_c refPoint;
      refLine.FindNearest( thisBeginPoint, &refPoint );
      if( TCTF_IsGT( minDistance, refPoint.FindDistance( thisBeginPoint )))
      {
         minDistance = refPoint.FindDistance( thisBeginPoint );
         thisNearestPoint = thisBeginPoint;
         refNearestPoint = refPoint;
      }

      refLine.FindNearest( thisEndPoint, &refPoint );
      if( TCTF_IsGT( minDistance, refPoint.FindDistance( thisEndPoint )))
      {
         minDistance = refPoint.FindDistance( thisEndPoint );
         thisNearestPoint = thisEndPoint;
         refNearestPoint = refPoint;
      }
   }
   else
   {
      this->FindIntersect( refLine, &refNearestPoint );
      thisNearestPoint = refNearestPoint;
   }

   if( prefNearestPoint )
   {
      *prefNearestPoint = refNearestPoint;
   }
   if( pthisNearestPoint )
   {
      *pthisNearestPoint = thisNearestPoint;
   }
}

//===========================================================================//
void TGO_Line_c::FindNearest( 
      const TGO_Point_c& refPoint,
            TGO_Point_c* pthisNearestPoint ) const
{
   if( pthisNearestPoint )
   {
      pthisNearestPoint->Reset( );

      // First, find nearest point from point 'C' to line segment 'AB'
      TGO_Point_c pointA( this->GetBeginPoint( ));
      TGO_Point_c pointB( this->GetEndPoint( ));
      TGO_Point_c pointC( refPoint );

      int iN = (( pointA.y - pointC.y ) * ( pointA.y - pointB.y ) - 
                ( pointA.x - pointC.x ) * ( pointB.x - pointA.x ));
      int iD = (( pointB.x - pointA.x ) * ( pointB.x - pointA.x ) + 
                ( pointB.y - pointA.y ) * ( pointB.y - pointA.y ));
      double rN = static_cast< double >( iN );
      double rD = static_cast< double >( iD );
      double r = rN / ( TCTF_IsNZE( rD ) ? rD : TC_FLT_EPSILON );
   
      pthisNearestPoint->Set( pointA.x + static_cast< int >( r * ( pointB.x - pointA.x )),
                              pointA.y + static_cast< int >( r * ( pointB.y - pointA.y )));

      // Next, handle case where nearest point is not within segment 'AB'
      TGO_Box_c boxAB( pointA, pointB );
      if( !boxAB.IsWithin( *pthisNearestPoint ))
      {
         double distA = pointA.FindDistance( *pthisNearestPoint );
         double distB = pointB.FindDistance( *pthisNearestPoint );
         pthisNearestPoint->Set( TCTF_IsLE( distA, distB ) ? 
                                 pointA : pointB );
      }
   }
}

//===========================================================================//
// Method         : FindIntersect
// Purpose        : Find and return the point of intersection on this line
//                  based on the given point.  The point of intersection is
//                  found using the following algorithm:
//                  
//                   - Let the line segment be 'AB' from point 'A' to
//                     point 'B' and 'CD' from point 'C' to point 'D',
//                   - Let 'I' be the intersect point for 'AB' and 'CD'.
//                   - Compute 'r' as:
//                  
//                            (A.y-C.y)(D.x-C.x) - (A.x-C.x)(D.y-C.y)
//                        r = ---------------------------------------
//                            (B.x-A.x)(D.y-C.y) - (B.y-A.y)(D.x-C.x)
//                  
//                   - Compute 'I' as:
//                  
//                        I.x = A.x + r(B.x-A.x)
//                        I.y = A.y + r(B.y-A.y)
// 
// Reference      : comp.graphics.algorithms FAQ by O'Rourke
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGO_Line_c::FindIntersect( 
      const TGO_Line_c&  refLine,
            TGO_Point_c* pthisIntersectPoint ) const
{
   if( pthisIntersectPoint )
   {
      pthisIntersectPoint->Reset( );

      if( this->IsIntersecting( refLine ))
      {
         TGO_Point_c pointA( this->GetBeginPoint( ));
         TGO_Point_c pointB( this->GetEndPoint( ));
         TGO_Point_c pointC( refLine.GetBeginPoint( ));
         TGO_Point_c pointD( refLine.GetEndPoint( ));

         int iN = (( pointA.y - pointC.y ) * ( pointD.x - pointC.x ) - 
                   ( pointA.x - pointC.x ) * ( pointD.y - pointC.y ));
         int iD = (( pointB.x - pointA.x ) * ( pointD.y - pointC.y ) -
                   ( pointB.y - pointA.y ) * ( pointD.x - pointC.x ));
         if(( iN != 0 ) || ( iD != 0 ))
         {
            // Find and return intersect point for non-collinear lines
            double rN = static_cast< double >( iN );
            double rD = static_cast< double >( iD );
            double r = rN / ( TCTF_IsNZE( rD ) ? rD : TC_FLT_EPSILON );
            int i = static_cast< int >( r );
   
            pthisIntersectPoint->Set( pointA.x + i * ( pointB.x - pointA.x ),
                                      pointA.y + i * ( pointB.y - pointA.y ));
         }
         else
         {
            // Find and return intersect point for collinear lines
            this->FindNearest( pointD, pthisIntersectPoint );
         }
      }
   }
}

//===========================================================================//
// Method         : CrossProduct
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
int TGO_Line_c::CrossProduct( 
      const TGO_Point_c& refPoint ) const
{
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.CrossProduct( endPoint, refPoint ));
}

//===========================================================================//
// Method         : IsConnected
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGO_Line_c::IsConnected(
      const TGO_Line_c& line ) const 
{
   bool isConnected = false;

   TGO_Point_c thisBeginPoint( this->GetBeginPoint( ));
   TGO_Point_c thisEndPoint( this->GetEndPoint( ));
   TGO_Point_c lineBeginPoint( line.GetBeginPoint( ));
   TGO_Point_c lineEndPoint( line.GetEndPoint( ));

   if(( thisBeginPoint == lineEndPoint ) ||
      ( thisEndPoint == lineBeginPoint ))
   {   
      isConnected = true;
   }
   return( isConnected );
}

//===========================================================================//
// Method         : IsIntersecting
// Purpose        : Return true if this line intersects with the given line.
//                  Intersection between arbitrary lines is decided based on
//                  the cross-product of this line with the given line's 
//                  end-points and the cross-product of the given line with 
//                  this line's end-points.
// Reference      : Algorithms by Cormen, Leiserson, Rivest, pp. 889-890.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGO_Line_c::IsIntersecting( 
      const TGO_Line_c&  line,
            TGO_Point_c* ppoint ) const
{
   bool isIntersecting = false;

   TGO_Point_c thisBeginPoint( this->GetBeginPoint( ));
   TGO_Point_c thisEndPoint( this->GetEndPoint( ));
   TGO_Point_c lineBeginPoint( line.GetBeginPoint( ));
   TGO_Point_c lineEndPoint( line.GetEndPoint( ));

   // First, apply quick rejection test to see if bounding regions intersect
   TGO_Region_c thisRegion( thisBeginPoint, thisEndPoint );
   TGO_Region_c lineRegion( lineBeginPoint, lineEndPoint );
   if( thisRegion.IsIntersecting( lineRegion ))
   {
      int crossProductThisBegin = this->CrossProduct( lineBeginPoint );
      int crossProductThisEnd = this->CrossProduct( lineEndPoint );

      int crossProductLineBegin = line.CrossProduct( thisBeginPoint );
      int crossProductLineEnd = line.CrossProduct( thisEndPoint );

      isIntersecting = true;
      if((( crossProductThisBegin < 0 ) && ( crossProductThisEnd < 0 )) ||
         (( crossProductThisBegin > 0 ) && ( crossProductThisEnd > 0 )) ||
         (( crossProductLineBegin < 0 ) && ( crossProductLineEnd < 0 )) ||
         (( crossProductLineBegin > 0 ) && ( crossProductLineEnd > 0 )))
      {
         isIntersecting = false;
      }
   }

   if( isIntersecting )
   {
      this->FindIntersect( line, ppoint );
   }
   return( isIntersecting );
}

//===========================================================================//
bool TGO_Line_c::IsIntersecting( 
      const TGO_Point_c& point ) const
{
   return( TCTF_IsZE( this->FindDistance( point )) ? true : false );
} 

//===========================================================================//
// Method         : IsWithin
// Purpose        : Return true if the given line is within this line.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGO_Line_c::IsWithin( 
      const TGO_Line_c& line ) const
{
   bool isWithin = false;
   
   TGO_Point_c lineBeginPoint( line.GetBeginPoint( ));
   TGO_Point_c lineEndPoint( line.GetEndPoint( ));

   if( this->IsIntersecting( lineBeginPoint ) &&
       this->IsIntersecting( lineEndPoint ))
   {
      isWithin = true;
   }
   return( isWithin );
}

//===========================================================================//
bool TGO_Line_c::IsWithin( 
      const TGO_Point_c& point ) const
{
   TGO_Line_c line( point, point );
   return( this->IsWithin( line ));
}

//===========================================================================//
// Method         : IsParallel
// Purpose        : Return true if the given line is parallel with this line.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGO_Line_c::IsParallel( 
      const TGO_Line_c& line ) const
{
   bool isParallel = false;

   int thisDx = abs( this->x1 - this->x2 );
   int thisDy = abs( this->y1 - this->y2 );

   int lineDx = abs( line.x1 - line.x2 );
   int lineDy = abs( line.y1 - line.y2 );

   if(( thisDy > 0 ) && ( lineDy > 0 ))
   {
      isParallel = (( thisDx / thisDy == lineDx / lineDy ) ? true : false );
   }
   else if(( thisDy == 0 ) && ( lineDy == 0 ))
   {
      isParallel = true;
   }
   else if(( thisDx == 0 ) && ( lineDx == 0 ))
   {
      isParallel = true;
   }
   return( isParallel );
} 

//===========================================================================//
// Method         : IsLeft/IsRight/IsLower/IsUpper
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGO_Line_c::IsLeft( 
      void ) const
{
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsLeft( endPoint ));
}

//===========================================================================//
bool TGO_Line_c::IsRight( 
      void ) const
{
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsRight( endPoint ));
}

//===========================================================================//
bool TGO_Line_c::IsLower( 
      void ) const
{
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsLower( endPoint ));
}

//===========================================================================//
bool TGO_Line_c::IsUpper( 
      void ) const
{
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsUpper( endPoint ));
}

//===========================================================================//
// Method         : IsLowerLeft/IsLowerRight/IsUpperLeft/IsUpperRight
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGO_Line_c::IsLowerLeft( 
      void ) const
{
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsLowerLeft( endPoint ));
}

//===========================================================================//
bool TGO_Line_c::IsLowerRight( 
      void ) const
{
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsLowerRight( endPoint ));
}

//===========================================================================//
bool TGO_Line_c::IsUpperLeft( 
      void ) const
{ 
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsUpperLeft( endPoint ));
}

//===========================================================================//
bool TGO_Line_c::IsUpperRight( 
      void ) const
{
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsUpperRight( endPoint ));
}

//===========================================================================//
// Method         : IsOrthogonal
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGO_Line_c::IsOrthogonal( 
      void ) const
{
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsOrthogonal( endPoint ));
}

//===========================================================================//
// Method         : IsHorizontal/IsVertical
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGO_Line_c::IsHorizontal( 
      void ) const
{  
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsHorizontal( endPoint ));
}

//===========================================================================//
bool TGO_Line_c::IsVertical( 
      void ) const
{
   TGO_Point_c beginPoint( this->GetBeginPoint( ));
   TGO_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsVertical( endPoint ));
}

//===========================================================================//
// Method         : IsValid
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGO_Line_c::IsValid( 
      void ) const
{
   return(( this->x1 != INT_MIN ) && ( this->x1 != INT_MAX ) &&
          ( this->y1 != INT_MIN ) && ( this->y1 != INT_MAX ) &&
          ( this->x2 != INT_MIN ) && ( this->x2 != INT_MAX ) &&
          ( this->y2 != INT_MIN ) && ( this->y2 != INT_MAX ) ?
          true : false );
}
