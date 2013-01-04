//===========================================================================//
// Purpose : Method definitions for the TGO_Box class.
//
//           Public methods include:
//           - TGO_Box_c, ~TGO_Box_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set, Reset
//           - FindMin, FindMax
//           - FindOrient
//           - FindWidth, FindLength
//           - FindDistance
//           - FindNearest, FindFarthest
//           - FindBox
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
//           Private methods inlcude:
//           - FindNearestCoords_
//           - FindFarthestCoords_
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

#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TGO_Box.h"

//===========================================================================//
// Method         : TGO_Box_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_Box_c::TGO_Box_c( 
      void )
{
}

//===========================================================================//
TGO_Box_c::TGO_Box_c( 
      int x1, 
      int y1,
      int z1,
      int x2, 
      int y2,
      int z2 )
      :
      lowerLeft( TCT_Min( x1, x2 ), TCT_Min( y1, y2 ), TCT_Min( z1, z2 )),
      upperRight( TCT_Max( x1, x2 ), TCT_Max( y1, y2 ), TCT_Max( z1, z2 ))
{
}

//===========================================================================//
TGO_Box_c::TGO_Box_c( 
      int x1, 
      int y1,
      int x2, 
      int y2 )
      :
      lowerLeft( TCT_Min( x1, x2 ), TCT_Min( y1, y2 ), 0 ),
      upperRight( TCT_Max( x1, x2 ), TCT_Max( y1, y2 ), 0 )
{
}

//===========================================================================//
TGO_Box_c::TGO_Box_c( 
      const TGO_Point_c& pointA,
      const TGO_Point_c& pointB )
      :
      lowerLeft( TCT_Min( pointA.x, pointB.x ),
                 TCT_Min( pointA.y, pointB.y ),
                 TCT_Min( pointA.z, pointB.z )),
      upperRight( TCT_Max( pointA.x, pointB.x ),
                  TCT_Max( pointA.y, pointB.y ),
                  TCT_Max( pointA.z, pointB.z ))
{
}

//===========================================================================//
TGO_Box_c::TGO_Box_c( 
      const TGO_Box_c& box )
      :
      lowerLeft( box.lowerLeft ),
      upperRight( box.upperRight )
{
}

//===========================================================================//
// Method         : ~TGO_Box_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_Box_c::~TGO_Box_c( 
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
TGO_Box_c& TGO_Box_c::operator=( 
      const TGO_Box_c& box )
{
   if( &box != this )
   {
      this->lowerLeft = box.lowerLeft;
      this->upperRight = box.upperRight;
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
bool TGO_Box_c::operator<( 
      const TGO_Box_c& box ) const
{
   bool isLessThan = false;

   if( this->lowerLeft < box.lowerLeft )
   {
      isLessThan = true;
   }
   else if( this->lowerLeft == box.lowerLeft )
   {
      if( this->upperRight < box.upperRight )
      {
         isLessThan = true;
      }
   }
   return( isLessThan );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Box_c::operator==( 
      const TGO_Box_c& box ) const
{
   return(( this->lowerLeft == box.lowerLeft ) &&
          ( this->upperRight == box.upperRight ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Box_c::operator!=( 
      const TGO_Box_c& box ) const
{
   return(( !this->operator==( box )) ? 
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Box_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srBox;
   this->ExtractString( &srBox );

   printHandler.Write( pfile, spaceLen, "[box] %s\n", TIO_SR_STR( srBox ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Box_c::ExtractString( 
      string* psrBox ) const
{
   if( psrBox )
   {
      char szBox[TIO_FORMAT_STRING_LEN_BOX];

      if( this->IsValid( ))
      {
         sprintf( szBox, "%d %d %d %d %d %d",
 		         this->lowerLeft.x, 
                         this->lowerLeft.y, 
                         this->lowerLeft.z,
                         this->upperRight.x, 
                         this->upperRight.y, 
                         this->upperRight.z );
      }
      else
      {
         sprintf( szBox, "? ? ? ? ? ?" );
      }
      *psrBox = szBox;
   }
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Box_c::Set( 
      int x1, 
      int y1,
      int z1,
      int x2, 
      int y2,
      int z2 )
{
   this->lowerLeft.Set( TCT_Min( x1, x2 ), TCT_Min( y1, y2 ), TCT_Min( z1, z2 ));
   this->upperRight.Set( TCT_Max( x1, x2 ), TCT_Max( y1, y2 ), TCT_Max( z1, z2 ));
}

//===========================================================================//
void TGO_Box_c::Set( 
      int x1, 
      int y1,
      int x2, 
      int y2 )
{
   this->lowerLeft.Set( TCT_Min( x1, x2 ), TCT_Min( y1, y2 ), 0 );
   this->upperRight.Set( TCT_Max( x1, x2 ), TCT_Max( y1, y2 ), 0 );
}

//===========================================================================//
void TGO_Box_c::Set(
      const TGO_Point_c& pointA,
      const TGO_Point_c& pointB )
{
   this->Set( pointA.x, pointA.y, pointA.z, pointB.x, pointB.y, pointB.z );
}

//===========================================================================//
void TGO_Box_c::Set( 
      const TGO_Box_c& box )
{
   this->Set( box.lowerLeft, box.upperRight );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Box_c::Reset( 
      void )
{
   this->lowerLeft.Reset( );
   this->upperRight.Reset( );
}

//===========================================================================//
// Method         : FindMin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
int TGO_Box_c::FindMin( 
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
int TGO_Box_c::FindMax( 
      void ) const
{
   int dx = this->GetDx( );
   int dy = this->GetDy( );

   return( TCT_Max( dx, dy ));
}

//===========================================================================//
// Method         : FindOrient
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_OrientMode_t TGO_Box_c::FindOrient( 
      TGO_OrientMode_t orient ) const
{
   TGO_OrientMode_t orient_ = TGO_ORIENT_UNDEFINED;
   
   int dx = this->GetDx( );
   int dy = this->GetDy( );
  
   if( dx > dy )
   {
      orient_ = TGO_ORIENT_HORIZONTAL;
   }
   else if( dx < dy )
   {
      orient_ = TGO_ORIENT_VERTICAL;
   }

   if( orient == TGO_ORIENT_ALTERNATE )
   {
      switch( orient_ )
      {
      case TGO_ORIENT_HORIZONTAL: orient_ = TGO_ORIENT_VERTICAL;   break;
      case TGO_ORIENT_VERTICAL:   orient_ = TGO_ORIENT_HORIZONTAL; break;
      default:                    orient_ = TGO_ORIENT_UNDEFINED;  break;
      }
   }
   return( orient_ );
}

//===========================================================================//
// Method         : FindWidth
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
int TGO_Box_c::FindWidth( 
      TGO_OrientMode_t orient ) const
{
   int width = 0;

   switch( orient )
   {
   case TGO_ORIENT_HORIZONTAL: width = this->GetDy( );   break;
   case TGO_ORIENT_VERTICAL:   width = this->GetDx( );   break;
   default:                    width = this->FindMin( ); break;
   }
   return( width );
}

//===========================================================================//
// Method         : FindLength
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
int TGO_Box_c::FindLength( 
      TGO_OrientMode_t orient ) const
{
   int length = 0;

   switch( orient )
   {
   case TGO_ORIENT_HORIZONTAL: length = this->GetDx( );   break;
   case TGO_ORIENT_VERTICAL:   length = this->GetDy( );   break;
   default:                    length = this->FindMax( ); break;
   }
   return( length );
}

//===========================================================================//
// Method         : FindDistance
// Purpose        : Compute and return the "flightline" distance between
//                  the nearest point on this box and the given box or point.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
double TGO_Box_c::FindDistance( 
      const TGO_Box_c& refBox ) const
{
   double distance = TC_FLT_MAX;

   if( this->IsIntersecting( refBox ) && 
       !this->IsWithin( refBox ) &&
       !refBox.IsWithin( *this ))
   {
      distance = 0.0;
   }
   else
   {
      TGO_Point_c refPoint, thisPoint;
      this->FindNearest( refBox, &refPoint, &thisPoint );

      distance = thisPoint.FindDistance( refPoint );
   }
   return( distance );
}

//===========================================================================//
double TGO_Box_c::FindDistance( 
      const TGO_Point_c& refPoint ) const
{
   TGO_Box_c refBox( refPoint, refPoint );

   return( this->FindDistance( refBox ));
}

//===========================================================================//
// Method         : FindNearest
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Box_c::FindNearest(
      const TGO_Box_c&   refBox,
            TGO_Point_c* prefNearestPoint,
            TGO_Point_c* pthisNearestPoint ) const
{
   TGO_Point_c refNearestPoint, thisNearestPoint;

   // Find nearest points by testing 'x', 'y', and 'z'  coordinates
   this->FindNearestCoords_( this->lowerLeft.x, this->upperRight.x,
                             refBox.lowerLeft.x, refBox.upperRight.x,
                             &thisNearestPoint.x, &refNearestPoint.x );
   this->FindNearestCoords_( this->lowerLeft.y, this->upperRight.y,
                             refBox.lowerLeft.y, refBox.upperRight.y,
                             &thisNearestPoint.y, &refNearestPoint.y );
   this->FindNearestCoords_( this->lowerLeft.z, this->upperRight.z,
                             refBox.lowerLeft.z, refBox.upperRight.z,
                             &thisNearestPoint.z, &refNearestPoint.z );
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
void TGO_Box_c::FindNearest( 
      const TGO_Point_c& refPoint, 
            TGO_Point_c* pthisNearestPoint ) const
{
   TGO_Point_c thisNearestPoint;

   TGO_Box_c withinBox( this->lowerLeft.x, this->lowerLeft.y, refPoint.z,
                        this->upperRight.x, this->upperRight.y, refPoint.z );
   if( withinBox.IsWithin( refPoint ))
   {
      thisNearestPoint = refPoint;
      if( refPoint.z < this->lowerLeft.z )
      {
         thisNearestPoint.z = this->lowerLeft.z;
      }
      else if( refPoint.z > this->upperRight.z )
      {
	 thisNearestPoint.z = this->upperRight.z;
      }
   }
   else
   {
      // Construct four new boxes representing each edge of this box
      TGO_Box_c leftBox, rightBox, lowerBox, upperBox;
      this->FindBox( TC_SIDE_LEFT, &leftBox );
      this->FindBox( TC_SIDE_RIGHT, &rightBox );
      this->FindBox( TC_SIDE_LOWER, &lowerBox );
      this->FindBox( TC_SIDE_UPPER, &upperBox );

      // Find four nearest points from four new boxes to the given point
      TGO_Point_c leftPoint, rightPoint, lowerPoint, upperPoint;
      TGO_Box_c refBox( refPoint, refPoint );

      refBox.FindNearest( leftBox, &leftPoint );
      refBox.FindNearest( rightBox, &rightPoint );
      refBox.FindNearest( lowerBox, &lowerPoint );
      refBox.FindNearest( upperBox, &upperPoint );

      // Find single nearest point per four nearest points to given point
      double leftDist = refPoint.FindDistance( leftPoint );
      double rightDist = refPoint.FindDistance( rightPoint );
      double lowerDist = refPoint.FindDistance( lowerPoint );
      double upperDist = refPoint.FindDistance( upperPoint );
      TGO_Point_c vertPoint = ( leftDist <= rightDist ? leftPoint : rightPoint );
      TGO_Point_c horzPoint = ( lowerDist <= upperDist ? lowerPoint : upperPoint );

      double vertDist = refPoint.FindDistance( vertPoint );
      double horzDist = refPoint.FindDistance( horzPoint );
      thisNearestPoint = ( vertDist <= horzDist ? vertPoint : horzPoint );
   }

   if( pthisNearestPoint )
   {
      *pthisNearestPoint = thisNearestPoint;
   }
}

//===========================================================================//
// Method         : FindFarthest
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Box_c::FindFarthest(
      const TGO_Box_c&   refBox,
            TGO_Point_c* prefFarthestPoint,
            TGO_Point_c* pthisFarthestPoint ) const
{
   TGO_Point_c refFarthestPoint, thisFarthestPoint;

   // Find farthest points by testing 'x', 'y', and 'z' coordinates
   this->FindFarthestCoords_( this->lowerLeft.x, this->upperRight.x,
                              refBox.lowerLeft.x, refBox.upperRight.x,
                              &thisFarthestPoint.x, &refFarthestPoint.x );
   this->FindFarthestCoords_( this->lowerLeft.y, this->upperRight.y,
                              refBox.lowerLeft.y, refBox.upperRight.y,
                              &thisFarthestPoint.y, &refFarthestPoint.y );
   this->FindFarthestCoords_( this->lowerLeft.z, this->upperRight.z,
                              refBox.lowerLeft.z, refBox.upperRight.z,
                              &thisFarthestPoint.z, &refFarthestPoint.z );
   if( prefFarthestPoint )
   {
      *prefFarthestPoint = refFarthestPoint;
   }
   if( pthisFarthestPoint )
   {
      *pthisFarthestPoint = thisFarthestPoint;
   }
}

//===========================================================================//
void TGO_Box_c::FindFarthest( 
      const TGO_Point_c& refPoint, 
            TGO_Point_c* pthisFarthestPoint ) const
{
   TGO_Point_c thisFarthestPoint;

   // Construct four new boxes representing each edge of this box
   TGO_Box_c leftBox, rightBox, lowerBox, upperBox;
   this->FindBox( TC_SIDE_LEFT, &leftBox );
   this->FindBox( TC_SIDE_RIGHT, &rightBox );
   this->FindBox( TC_SIDE_LOWER, &lowerBox );
   this->FindBox( TC_SIDE_UPPER, &upperBox );

   // Find four farthest points from four new boxes to the given point
   TGO_Point_c leftPoint, rightPoint, lowerPoint, upperPoint;
   TGO_Box_c refBox( refPoint, refPoint );

   refBox.FindFarthest( leftBox, &leftPoint );
   refBox.FindFarthest( rightBox, &rightPoint );
   refBox.FindFarthest( lowerBox, &lowerPoint );
   refBox.FindFarthest( upperBox, &upperPoint );

   // Find single farthest point per four farthest points to given point
   double leftDist = refPoint.FindDistance( leftPoint );
   double rightDist = refPoint.FindDistance( rightPoint );
   double lowerDist = refPoint.FindDistance( lowerPoint );
   double upperDist = refPoint.FindDistance( upperPoint );
   TGO_Point_c vertPoint = ( leftDist >= rightDist ? leftPoint : rightPoint );
   TGO_Point_c horzPoint = ( lowerDist >= upperDist ? lowerPoint : upperPoint );

   double vertDist = refPoint.FindDistance( vertPoint );
   double horzDist = refPoint.FindDistance( horzPoint );
   thisFarthestPoint = ( vertDist >= horzDist ? vertPoint : horzPoint );

   if( pthisFarthestPoint )
   {
      *pthisFarthestPoint = thisFarthestPoint;
   }
}

//===========================================================================//
// Method         : FindBox
// Purpose        : Return left|right|lower|upper box based on given side.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Box_c::FindBox( 
      TC_SideMode_t side, 
      TGO_Box_c*    pbox ) const
{
   if( pbox )
   {
      pbox->Reset( );

      int x1 = this->lowerLeft.x;
      int y1 = this->lowerLeft.y;
      int z1 = this->lowerLeft.z;

      int x2 = this->upperRight.x;
      int y2 = this->upperRight.y;
      int z2 = this->upperRight.z;

      switch( side )
      {
      case TC_SIDE_LEFT:
         pbox->Set( x1, y1, z1, x1, y2, z2 );
         break;

      case TC_SIDE_RIGHT:
         pbox->Set( x2, y1, z1, x2, y2, z2 );
         break;

      case TC_SIDE_LOWER:
         pbox->Set( x1, y1, z1, x2, y1, z2 );
         break;

      case TC_SIDE_UPPER:
         pbox->Set( x1, y2, z1, x2, y2, z2 );
         break;

      default:
         break;
      }
   }
}

//===========================================================================//
// Method         : ApplyScale
// Purpose        : Scale this box, expanding or shrinking it based on the
//                  given value(s).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Box_c::ApplyScale( 
      int dx, 
      int dy,
      int dz )
{
   int dxBox = this->GetDx( );
   int dyBox = this->GetDy( );
   int dzBox = this->GetDz( );

   dx = ( dx >= 0 ) ? dx : TCT_Max( dx, -1 * dxBox / 2 );
   dy = ( dy >= 0 ) ? dy : TCT_Max( dy, -1 * dyBox / 2 );
   dz = ( dz >= 0 ) ? dz : TCT_Max( dz, -1 * dzBox / 2 );

   int x1 = TCT_Min( this->lowerLeft.x - dx, this->upperRight.x );
   int y1 = TCT_Min( this->lowerLeft.y - dy, this->upperRight.y );
   int z1 = TCT_Min( this->lowerLeft.z - dz, this->upperRight.z );

   int x2 = TCT_Max( this->upperRight.x + dx, this->lowerLeft.x );
   int y2 = TCT_Max( this->upperRight.y + dy, this->lowerLeft.y );
   int z2 = TCT_Max( this->upperRight.z + dz, this->lowerLeft.z );

   this->Set( x1, y1, z1, x2, y2, z2 );
}

//===========================================================================//
// Method         : ApplyUnion
// Purpose        : Set this box dimensions to the union of the given box
//                  or boxes.  The given box or boxes may or may not be
//                  intersecting.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Box_c::ApplyUnion( 
      const TGO_Box_c& boxA, 
      const TGO_Box_c& boxB )
{
   if( boxA.IsValid( ) && boxB.IsValid( ))
   {
      int x1 = TCT_Min( boxA.lowerLeft.x, boxB.lowerLeft.x );
      int y1 = TCT_Min( boxA.lowerLeft.y, boxB.lowerLeft.y );
      int z1 = TCT_Min( boxA.lowerLeft.z, boxB.lowerLeft.z );
   
      int x2 = TCT_Max( boxA.upperRight.x, boxB.upperRight.x );
      int y2 = TCT_Max( boxA.upperRight.y, boxB.upperRight.y );
      int z2 = TCT_Max( boxA.upperRight.z, boxB.upperRight.z );
   
      this->Set( x1, y1, z1, x2, y2, z2 );
   }
   else if( boxA.IsValid( ))
   {
      this->Set( boxA );
   }
   else if( boxB.IsValid( ))
   {
      this->Set( boxB );
   }
}

//===========================================================================//
void TGO_Box_c::ApplyUnion( 
      const TGO_Box_c& box )
{
   this->ApplyUnion( *this, box );
}

//===========================================================================//
// Method         : ApplyIntersect
// Purpose        : Set this box dimensions to the intersection of the given
//                  box or boxes.  The given box or boxes must already be 
//                  intersecting.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Box_c::ApplyIntersect( 
      const TGO_Box_c& boxA, 
      const TGO_Box_c& boxB )
{
   if( boxA.IsIntersecting( boxB ))
   {
      int x1 = TCT_Max( boxA.lowerLeft.x, boxB.lowerLeft.x );
      int y1 = TCT_Max( boxA.lowerLeft.y, boxB.lowerLeft.y );
      int z1 = TCT_Max( boxA.lowerLeft.z, boxB.lowerLeft.z );

      int x2 = TCT_Min( boxA.upperRight.x, boxB.upperRight.x );
      int y2 = TCT_Min( boxA.upperRight.y, boxB.upperRight.y );
      int z2 = TCT_Min( boxA.upperRight.z, boxB.upperRight.z );

      this->Set( x1, y1, z1, x2, y2, z2 );
   }
   else
   {
      this->Reset( );
   }
}

//===========================================================================//
void TGO_Box_c::ApplyIntersect( 
      const TGO_Box_c& box )
{
   this->ApplyIntersect( *this, box );
}

//===========================================================================//
// Method         : IsIntersecting
// Purpose        : Return true if this box is intersecting with the given
//                  box.  Intersecting implies intersecting z layers, as well 
//                  as intersecting x/y region coordinates.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Box_c::IsIntersecting( 
      const TGO_Box_c& box ) const
{
   bool isIntersecting = true;

   if(( box.lowerLeft.z < this->lowerLeft.z ) &&
      ( box.upperRight.z < this->lowerLeft.z ))
   {
      isIntersecting = false;
   }
   else if(( box.lowerLeft.z > this->upperRight.z ) &&
           ( box.upperRight.z > this->upperRight.z ))
   {
      isIntersecting = false;
   }
   else if(( box.lowerLeft.x < this->lowerLeft.x ) &&
           ( box.upperRight.x < this->lowerLeft.x ))
   {
      isIntersecting = false;
   }
   else if(( box.lowerLeft.x > this->upperRight.x ) &&
           ( box.upperRight.x > this->upperRight.x ))
   {
      isIntersecting = false;
   }
   else if(( box.lowerLeft.y < this->lowerLeft.y ) &&
           ( box.upperRight.y < this->lowerLeft.y ))
   {
      isIntersecting = false;
   }
   else if(( box.lowerLeft.y > this->upperRight.y ) &&
           ( box.upperRight.y > this->upperRight.y ))
   {
      isIntersecting = false;
   }
   return( isIntersecting );
}

//===========================================================================//
// Method         : IsOverlapping
// Purpose        : Return true if this box is overlapping with the given
//                  box.  Overlapping implies intersecting or adjacent (+/-1) 
//                  z layers, as well as intersecting x/y region coordinates.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Box_c::IsOverlapping( 
      const TGO_Box_c& box ) const
{
   bool isOverlapping = true;

   if(( box.lowerLeft.z < this->lowerLeft.z - 1 ) &&
      ( box.upperRight.z < this->lowerLeft.z - 1 ))
   {
      isOverlapping = false;
   }
   else if(( box.lowerLeft.z > this->upperRight.z + 1 ) &&
           ( box.upperRight.z > this->upperRight.z + 1 ))
   {
      isOverlapping = false;
   }
   else if(( box.lowerLeft.x < this->lowerLeft.x ) &&
           ( box.upperRight.x < this->lowerLeft.x ))
   {
      isOverlapping = false;
   }
   else if(( box.lowerLeft.x > this->upperRight.x ) &&
           ( box.upperRight.x > this->upperRight.x ))
   {
      isOverlapping = false;
   }
   else if(( box.lowerLeft.y < this->lowerLeft.y ) &&
           ( box.upperRight.y < this->lowerLeft.y ))
   {
      isOverlapping = false;
   }
   else if(( box.lowerLeft.y > this->upperRight.y ) &&
           ( box.upperRight.y > this->upperRight.y ))
   {
      isOverlapping = false;
   }
   return( isOverlapping );
}

//===========================================================================//
// Method         : IsWithin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Box_c::IsWithin( 
      const TGO_Box_c& box ) const
{
   bool isWithin = false;

   if(( this->IsWithin( box.lowerLeft )) &&
      ( this->IsWithin( box.upperRight )))
   {
      isWithin = true;
   }
   return( isWithin );
} 

//===========================================================================//
bool TGO_Box_c::IsWithin( 
      const TGO_Point_c& point ) const
{
   bool isWithin = false;

   if(( point.x >= this->lowerLeft.x ) && ( point.x <= this->upperRight.x ) &&
      ( point.y >= this->lowerLeft.y ) && ( point.y <= this->upperRight.y ) &&
      ( point.z >= this->lowerLeft.z ) && ( point.z <= this->upperRight.z ))
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
bool TGO_Box_c::IsWide( 
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
bool TGO_Box_c::IsTall( 
      void ) const
{
   int dx = this->GetDx( );
   int dy = this->GetDy( );

   return( dy >= dx ? true : false );
}

//===========================================================================//
// Method         : IsSquare
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Box_c::IsSquare( 
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
bool TGO_Box_c::IsLegal( 
      void ) const
{
   bool isLegal = this->IsValid( );
   if( isLegal )
   {
      isLegal = (( this->lowerLeft.x <= this->upperRight.x ) &&
                 ( this->lowerLeft.y <= this->upperRight.y ) ?
                 true : false );
   }
   return( isLegal );
}


//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Box_c::IsValid( 
      void ) const
{
   return( this->lowerLeft.IsValid( ) && this->upperRight.IsValid( ) ?
           true : false );
}

//===========================================================================//
// Method         : FindNearestCoords_
// Purpose        : Return the nearest two coordinates between the given set
//                  of coordinate pairs.  Cases are:
//             
//                  1)         1i---------1j   returns: 1i
//                       2i---------2j         returns: 1i
//     
//                  2)   1i---------1j         returns: 2i
//                             2i---------2j   returns: 2i
//     
//                  3)         1i---1j         returns: 1i
//                       2i---------------2j   returns: 1i
//     
//                  4)   1i---------------1j   returns: 2i
//                             2i---2j         returns: 2i
//     
//                  5)   1i----1j              returns: 1j
//                                  2i----2j   returns: 2i
//     
//                  6)              1i----1j   returns: 1i
//                       2i----2j              returns: 2j
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Box_c::FindNearestCoords_(
      int  coord1i, 
      int  coord1j, 
      int  coord2i, 
      int  coord2j,
      int* pnearestCoord1, 
      int* pnearestCoord2 ) const
{
   // Find nearest coordinates by testing for overlap, then non-overlap cases
   if(( coord1i <= coord2j ) && ( coord1j >= coord2i ))
   {
      *pnearestCoord1 = *pnearestCoord2 = TCT_Max( coord1i, coord2i );
   }
   else if( coord1j < coord2i )
   {
      *pnearestCoord1 = coord1j;
      *pnearestCoord2 = coord2i;
   }
   else if( coord1i > coord2j )
   {
      *pnearestCoord1 = coord1i;
      *pnearestCoord2 = coord2j;
   }
}

//===========================================================================//
// Method         : FindFarthestCoords_
// Purpose        : Return the farthest two coordinates between the given set
//                  of coordinate pairs.  Cases are:
//             
//                  1)         1i---------1j   returns: 2i
//                       2i---------2j         returns: 2i
//     
//                  2)   1i---------1j         returns: 1i
//                             2i---------2j   returns: 1i
//     
//                  3)         1i---1j         returns: 2i
//                       2i---------------2j   returns: 2i
//     
//                  4)   1i---------------1j   returns: 1i
//                             2i---2j         returns: 1i
//     
//                  5)   1i----1j              returns: 1i
//                                  2i----2j   returns: 2j
//     
//                  6)              1i----1j   returns: 1j
//                       2i----2j              returns: 2i
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Box_c::FindFarthestCoords_(
      int  coord1i, 
      int  coord1j, 
      int  coord2i, 
      int  coord2j,
      int* pfarthestCoord1, 
      int* pfarthestCoord2 ) const
{
   // Find farthest coordinates by testing for overlap, then non-overlap cases
   if(( coord1i <= coord2j ) && ( coord1j >= coord2i ))
   {
      *pfarthestCoord1 = *pfarthestCoord2 = TCT_Min( coord1i, coord2i );
   }
   else if( coord1j < coord2i )
   {
      *pfarthestCoord1 = coord1i;
      *pfarthestCoord2 = coord2j;
   }
   else if( coord1i > coord2j )
   {
      *pfarthestCoord1 = coord1j;
      *pfarthestCoord2 = coord2i;
   }
}
