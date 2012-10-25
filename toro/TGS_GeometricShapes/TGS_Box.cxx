//===========================================================================//
// Purpose : Method definitions for the TGS_Box class.
//
//           Public methods include:
//           - TGS_Box_c, ~TGS_Box_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set, Reset
//           - FindMin, FindMax
//           - FindRatio
//           - FindVolume
//           - FindCenter
//           - FindOrient
//           - FindWidth, FindLength
//           - FindDistance
//           - FindNearest, FindFarthest
//           - FindBox
//           - FindSide
//           - ApplyScale
//           - ApplyUnion
//           - ApplyIntersect
//           - ApplyOverlap
//           - ApplyAdjacent
//           - IsIntersecting
//           - IsOverlapping
//           - IsAdjacent
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

#if defined( SUN8 ) || defined( SUN10 )
   #include <math.h>
#endif

#include "TC_MinGrid.h"
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TGS_Box.h"

//===========================================================================//
// Method         : TGS_Box_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Box_c::TGS_Box_c( 
      void )
{
}

//===========================================================================//
TGS_Box_c::TGS_Box_c( 
      double x1, 
      double y1,
      int    z1,
      double x2, 
      double y2,
      int    z2 )
      :
      lowerLeft( TCT_Min( x1, x2 ), TCT_Min( y1, y2 ), TCT_Min( z1, z2 )),
      upperRight( TCT_Max( x1, x2 ), TCT_Max( y1, y2 ), TCT_Max( z1, z2 ))
{
}

//===========================================================================//
TGS_Box_c::TGS_Box_c( 
      double         x1, 
      double         y1,
      int            z1,
      double         x2, 
      double         y2,
      int            z2,
      TGS_SnapMode_t snap )
{
   this->Set( x1, y1, z1, x2, y2, z2, snap );
}

//===========================================================================//
TGS_Box_c::TGS_Box_c( 
      double x1, 
      double y1,
      double x2, 
      double y2 )
      :
      lowerLeft( TCT_Min( x1, x2 ), TCT_Min( y1, y2 ), 0 ),
      upperRight( TCT_Max( x1, x2 ), TCT_Max( y1, y2 ), 0 )
{
}

//===========================================================================//
TGS_Box_c::TGS_Box_c( 
      double         x1, 
      double         y1,
      double         x2, 
      double         y2,
      TGS_SnapMode_t snap )
{
   this->Set( x1, y1, x2, y2, snap );
}

//===========================================================================//
TGS_Box_c::TGS_Box_c( 
      const TGS_Point_c& pointA,
      const TGS_Point_c& pointB )
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
TGS_Box_c::TGS_Box_c( 
      const TGS_Point_c&   pointA,
      const TGS_Point_c&   pointB,
            TGS_SnapMode_t snap )
{
   this->Set( pointA, pointB, snap );
}

//===========================================================================//
TGS_Box_c::TGS_Box_c( 
      const TGS_Box_c& box )
      :
      lowerLeft( box.lowerLeft ),
      upperRight( box.upperRight )
{
}

//===========================================================================//
TGS_Box_c::TGS_Box_c( 
      const TGS_Box_c&     box,
            TGS_SnapMode_t snap )
{
   this->Set( box, snap );
}

//===========================================================================//
TGS_Box_c::TGS_Box_c( 
      const TGS_Box_c& boxA,
      const TGS_Box_c& boxB )
{
   this->ApplyUnion( boxA, boxB );
}

//===========================================================================//
// Method         : ~TGS_Box_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Box_c::~TGS_Box_c( 
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
TGS_Box_c& TGS_Box_c::operator=( 
      const TGS_Box_c& box )
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
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Box_c::operator<( 
      const TGS_Box_c& box ) const
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
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Box_c::operator==( 
      const TGS_Box_c& box ) const
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
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Box_c::operator!=( 
      const TGS_Box_c& box ) const
{
   return(( !this->operator==( box )) ? 
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srBox;
   this->ExtractString( &srBox );

   printHandler.Write( pfile, spaceLen, "[box] %s", TIO_SR_STR( srBox ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::ExtractString( 
      string* psrBox,
      size_t  precision ) const
{
   if( psrBox )
   {
      char szBox[TIO_FORMAT_STRING_LEN_BOX];

      if( precision == SIZE_MAX )
      {
         precision = TC_MinGrid_c::GetInstance( ).GetPrecision( );
      }

      if( this->IsValid( ))
      {
         sprintf( szBox, "%0.*f %0.*f %d %0.*f %0.*f %d",
 		         static_cast< int >( precision ), this->lowerLeft.x, 
                         static_cast< int >( precision ), this->lowerLeft.y, 
                         this->lowerLeft.z,
		         static_cast< int >( precision ), this->upperRight.x, 
                         static_cast< int >( precision ), this->upperRight.y, 
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
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::Set( 
      double         x1, 
      double         y1,
      int            z1,
      double         x2, 
      double         y2,
      int            z2,
      TGS_SnapMode_t snap )
{
   this->lowerLeft.Set( TCT_Min( x1, x2 ), TCT_Min( y1, y2 ), TCT_Min( z1, z2 ));
   this->upperRight.Set( TCT_Max( x1, x2 ), TCT_Max( y1, y2 ), TCT_Max( z1, z2 ));

   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      this->lowerLeft.x = MinGrid.SnapToGrid( this->lowerLeft.x );
      this->lowerLeft.y = MinGrid.SnapToGrid( this->lowerLeft.y );
      this->upperRight.x = MinGrid.SnapToGrid( this->upperRight.x );
      this->upperRight.y = MinGrid.SnapToGrid( this->upperRight.y );
   }
}

//===========================================================================//
void TGS_Box_c::Set( 
      double         x1, 
      double         y1,
      double         x2, 
      double         y2,
      TGS_SnapMode_t snap )
{
   this->lowerLeft.Set( TCT_Min( x1, x2 ), TCT_Min( y1, y2 ), 0 );
   this->upperRight.Set( TCT_Max( x1, x2 ), TCT_Max( y1, y2 ), 0 );

   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      this->lowerLeft.x = MinGrid.SnapToGrid( this->lowerLeft.x );
      this->lowerLeft.y = MinGrid.SnapToGrid( this->lowerLeft.y );
      this->upperRight.x = MinGrid.SnapToGrid( this->upperRight.x );
      this->upperRight.y = MinGrid.SnapToGrid( this->upperRight.y );
   }
}

//===========================================================================//
void TGS_Box_c::Set(
      const TGS_Point_c&   pointA,
      const TGS_Point_c&   pointB,
            TGS_SnapMode_t snap )
{
   this->Set( pointA.x, pointA.y, pointA.z, pointB.x, pointB.y, pointB.z, snap );
}

//===========================================================================//
void TGS_Box_c::Set( 
      const TGS_Box_c&     box,
            TGS_SnapMode_t snap )
{
   this->Set( box.lowerLeft, box.upperRight, snap );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::Reset( 
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
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Box_c::FindMin( 
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
double TGS_Box_c::FindMax( 
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
double TGS_Box_c::FindRatio(
      void ) const
{
   double min = this->FindMin( );
   double max = this->FindMax( );

   double ratio = ( TCTF_IsGT( min, 0.0 ) ? max / min : TC_FLT_MAX );

   return( ratio );
}

//===========================================================================//
// Method         : FindVolume
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Box_c::FindVolume( 
      void ) const
{
   double dx = this->GetDx( );
   double dy = this->GetDy( );
   double dz = static_cast< double >( this->GetDz( ) + 1 );

   return( dz * dx * dy );
}

//===========================================================================//
// Method         : FindCenter
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Box_c::FindCenter( 
      TGS_SnapMode_t snap ) const
{
   double dx = this->GetDx( );
   double dy = this->GetDy( );
   int dz = this->GetDz( );

   TGS_Point_c center( this->lowerLeft.x + ( dx / 2.0 ),
                       this->lowerLeft.y + ( dy / 2.0 ),
                       this->lowerLeft.z + ( dz / 2 ));

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
// Method         : FindOrient
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_OrientMode_t TGS_Box_c::FindOrient( 
      TGS_OrientMode_t orient ) const
{
   TGS_OrientMode_t orient_ = TGS_ORIENT_UNDEFINED;
   
   double dx = this->GetDx( );
   double dy = this->GetDy( );
  
   if( TCTF_IsGT( dx, dy ))
   {
      orient_ = TGS_ORIENT_HORIZONTAL;
   }
   else if( TCTF_IsLT( dx, dy ))
   {
      orient_ = TGS_ORIENT_VERTICAL;
   }

   if( orient == TGS_ORIENT_ALTERNATE )
   {
      switch( orient_ )
      {
      case TGS_ORIENT_HORIZONTAL: orient_ = TGS_ORIENT_VERTICAL;   break;
      case TGS_ORIENT_VERTICAL:   orient_ = TGS_ORIENT_HORIZONTAL; break;
      default:                    orient_ = TGS_ORIENT_UNDEFINED;  break;
      }
   }
   return( orient_ );
}

//===========================================================================//
// Method         : FindWidth
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Box_c::FindWidth( 
      TGS_OrientMode_t orient ) const
{
   double width = 0.0;

   switch( orient )
   {
   case TGS_ORIENT_HORIZONTAL: width = this->GetDy( );   break;
   case TGS_ORIENT_VERTICAL:   width = this->GetDx( );   break;
   default:                    width = this->FindMin( ); break;
   }
   return( width );
}

//===========================================================================//
// Method         : FindLength
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Box_c::FindLength( 
      TGS_OrientMode_t orient ) const
{
   double length = 0.0;

   switch( orient )
   {
   case TGS_ORIENT_HORIZONTAL: length = this->GetDx( );   break;
   case TGS_ORIENT_VERTICAL:   length = this->GetDy( );   break;
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
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Box_c::FindDistance( 
      const TGS_Box_c& refBox ) const
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
      TGS_Point_c refPoint, thisPoint;
      this->FindNearest( refBox, &refPoint, &thisPoint );

      distance = thisPoint.FindDistance( refPoint );
   }
   return( distance );
}

//===========================================================================//
double TGS_Box_c::FindDistance( 
      const TGS_Point_c& refPoint ) const
{
   TGS_Box_c refBox( refPoint, refPoint );

   return( this->FindDistance( refBox ));
}

//===========================================================================//
double TGS_Box_c::FindDistance( 
      const TGS_Box_c& refBox,
            TC_SideMode_t side ) const
{
   TGS_Box_c sideBox;

   refBox.FindBox( side, &sideBox );
   if( !sideBox.IsValid( ))
   {
      sideBox = refBox;
   }
   return( this->FindDistance( sideBox ));
}


//===========================================================================//
double TGS_Box_c::FindDistance(
      const TGS_Point_c&     refPoint,
            TGS_CornerMode_t corner ) const
{
   double distance = TC_FLT_MAX;

   switch( corner )
   {
   case TGS_CORNER_LOWER_LEFT:

      distance = fabs( this->lowerLeft.x - refPoint.x ) + 
                 fabs( this->lowerLeft.y - refPoint.y );
      break;

   case TGS_CORNER_LOWER_RIGHT:

      distance = fabs( this->upperRight.x - refPoint.x ) + 
                 fabs( this->lowerLeft.y - refPoint.y );
      break;

   case TGS_CORNER_UPPER_LEFT:

      distance = fabs( this->lowerLeft.x - refPoint.x ) + 
                 fabs( this->upperRight.y - refPoint.y );
      break;

   case TGS_CORNER_UPPER_RIGHT:

      distance = fabs( this->upperRight.x - refPoint.x ) + 
                 fabs( this->upperRight.y - refPoint.y );
      break;

   default:

      distance = this->FindDistance( refPoint );
      break;
   }
   return( distance );
}

//===========================================================================//
// Method         : FindNearest
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::FindNearest(
      const TGS_Box_c&   refBox,
            TGS_Point_c* prefNearestPoint,
            TGS_Point_c* pthisNearestPoint ) const
{
   TGS_Point_c refNearestPoint, thisNearestPoint;

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
void TGS_Box_c::FindNearest( 
      const TGS_Point_c& refPoint, 
            TGS_Point_c* pthisNearestPoint ) const
{
   TGS_Point_c thisNearestPoint;

   TGS_Box_c withinBox( this->lowerLeft.x, this->lowerLeft.y, refPoint.z,
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
      TGS_Box_c leftBox, rightBox, lowerBox, upperBox;
      this->FindBox( TC_SIDE_LEFT, &leftBox );
      this->FindBox( TC_SIDE_RIGHT, &rightBox );
      this->FindBox( TC_SIDE_LOWER, &lowerBox );
      this->FindBox( TC_SIDE_UPPER, &upperBox );

      // Find four nearest points from four new boxes to the given point
      TGS_Point_c leftPoint, rightPoint, lowerPoint, upperPoint;
      TGS_Box_c refBox( refPoint, refPoint );

      refBox.FindNearest( leftBox, &leftPoint );
      refBox.FindNearest( rightBox, &rightPoint );
      refBox.FindNearest( lowerBox, &lowerPoint );
      refBox.FindNearest( upperBox, &upperPoint );

      // Find single nearest point per four nearest points to given point
      double leftDist = refPoint.FindDistance( leftPoint );
      double rightDist = refPoint.FindDistance( rightPoint );
      double lowerDist = refPoint.FindDistance( lowerPoint );
      double upperDist = refPoint.FindDistance( upperPoint );
      TGS_Point_c vertPoint = ( TCTF_IsLE( leftDist, rightDist ) ? 
                                leftPoint : rightPoint );
      TGS_Point_c horzPoint = ( TCTF_IsLE( lowerDist, upperDist ) ? 
                                lowerPoint : upperPoint );

      double vertDist = refPoint.FindDistance( vertPoint );
      double horzDist = refPoint.FindDistance( horzPoint );
      thisNearestPoint = ( TCTF_IsLE( vertDist, horzDist ) ?
                           vertPoint : horzPoint );
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
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::FindFarthest(
      const TGS_Box_c&   refBox,
            TGS_Point_c* prefFarthestPoint,
            TGS_Point_c* pthisFarthestPoint ) const
{
   TGS_Point_c refFarthestPoint, thisFarthestPoint;

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
void TGS_Box_c::FindFarthest( 
      const TGS_Point_c& refPoint, 
            TGS_Point_c* pthisFarthestPoint ) const
{
   TGS_Point_c thisFarthestPoint;

   // Construct four new boxes representing each edge of this box
   TGS_Box_c leftBox, rightBox, lowerBox, upperBox;
   this->FindBox( TC_SIDE_LEFT, &leftBox );
   this->FindBox( TC_SIDE_RIGHT, &rightBox );
   this->FindBox( TC_SIDE_LOWER, &lowerBox );
   this->FindBox( TC_SIDE_UPPER, &upperBox );

   // Find four farthest points from four new boxes to the given point
   TGS_Point_c leftPoint, rightPoint, lowerPoint, upperPoint;
   TGS_Box_c refBox( refPoint, refPoint );

   refBox.FindFarthest( leftBox, &leftPoint );
   refBox.FindFarthest( rightBox, &rightPoint );
   refBox.FindFarthest( lowerBox, &lowerPoint );
   refBox.FindFarthest( upperBox, &upperPoint );

   // Find single farthest point per four farthest points to given point
   double leftDist = refPoint.FindDistance( leftPoint );
   double rightDist = refPoint.FindDistance( rightPoint );
   double lowerDist = refPoint.FindDistance( lowerPoint );
   double upperDist = refPoint.FindDistance( upperPoint );
   TGS_Point_c vertPoint = ( TCTF_IsGE( leftDist, rightDist ) ? 
                             leftPoint : rightPoint );
   TGS_Point_c horzPoint = ( TCTF_IsGE( lowerDist, upperDist ) ? 
                             lowerPoint : upperPoint );

   double vertDist = refPoint.FindDistance( vertPoint );
   double horzDist = refPoint.FindDistance( horzPoint );
   thisFarthestPoint = ( TCTF_IsGE( vertDist, horzDist ) ? 
                         vertPoint : horzPoint );

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
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::FindBox( 
      TC_SideMode_t side, 
      TGS_Box_c*    pbox ) const
{
   if( pbox )
   {
      pbox->Reset( );

      double x1 = this->lowerLeft.x;
      double y1 = this->lowerLeft.y;
      int z1 = this->lowerLeft.z;

      double x2 = this->upperRight.x;
      double y2 = this->upperRight.y;
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
// Method         : FindSide
// Purpose        : Return left|right|lower|upper side based on given box.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TC_SideMode_t TGS_Box_c::FindSide(
      const TGS_Box_c& box ) const
{
   TC_SideMode_t side = TC_SIDE_UNDEFINED;

   TGS_Box_c box_( box );
   if( this->IsIntersecting( box ))
   {
      box_.ApplyIntersect( *this );
      box_.ApplyScale( box_.GetDx( ) / -2.0, box_.GetDy( ) / -2.0, 0 );
   }

   // Construct four new boxes representing each edge of this box
   TGS_Box_c leftBox, rightBox, lowerBox, upperBox;
   this->FindBox( TC_SIDE_LEFT, &leftBox );
   this->FindBox( TC_SIDE_RIGHT, &rightBox );
   this->FindBox( TC_SIDE_LOWER, &lowerBox );
   this->FindBox( TC_SIDE_UPPER, &upperBox );

   // Find four nearest points from four new boxes to the given box
   TGS_Point_c leftPoint, rightPoint, lowerPoint, upperPoint;

   box_.FindNearest( leftBox, &leftPoint );
   box_.FindNearest( rightBox, &rightPoint );
   box_.FindNearest( lowerBox, &lowerPoint );
   box_.FindNearest( upperBox, &upperPoint );

   // Find single nearest point per four nearest points to given box
   double leftDist = box_.FindDistance( leftPoint );
   double rightDist = box_.FindDistance( rightPoint );
   double lowerDist = box_.FindDistance( lowerPoint );
   double upperDist = box_.FindDistance( upperPoint );

   double minDist = TCT_Min( TCT_Min( leftDist, rightDist ), 
                             TCT_Min( lowerDist, upperDist ));

   // Decide side based on min distance 
   // (but, use ascept ratio if multiple nearest sides detected)
   if( TCTF_IsEQ( minDist, leftDist ) &&
       TCTF_IsEQ( minDist, rightDist ) &&
       TCTF_IsEQ( minDist, lowerDist ) &&
       TCTF_IsEQ( minDist, upperDist ))
   {
      side = this->IsWide( ) ? TC_SIDE_LEFT : TC_SIDE_LOWER;
   }
   else if( TCTF_IsEQ( minDist, leftDist ) &&
            TCTF_IsEQ( minDist, lowerDist ))
   {
      side = this->IsWide( ) ? TC_SIDE_LEFT : TC_SIDE_LOWER;
   }
   else if( TCTF_IsEQ( minDist, leftDist ) &&
            TCTF_IsEQ( minDist, upperDist ))
   {
      side = this->IsWide( ) ? TC_SIDE_LEFT : TC_SIDE_UPPER;
   }
   else if( TCTF_IsEQ( minDist, rightDist ) &&
            TCTF_IsEQ( minDist, lowerDist ))
   {
      side = this->IsWide( ) ? TC_SIDE_RIGHT : TC_SIDE_LOWER;
   }
   else if( TCTF_IsEQ( minDist, rightDist ) &&
            TCTF_IsEQ( minDist, upperDist ))
   {
      side = this->IsWide( ) ? TC_SIDE_RIGHT : TC_SIDE_UPPER;
   }
   else if( TCTF_IsEQ( minDist, leftDist ))
   {
      side = TC_SIDE_LEFT;
   }
   else if( TCTF_IsEQ( minDist, rightDist ))
   {
      side = TC_SIDE_RIGHT;
   }
   else if( TCTF_IsEQ( minDist, lowerDist ))
   {
      side = TC_SIDE_LOWER;
   }
   else if( TCTF_IsEQ( minDist, upperDist ))
   {
      side = TC_SIDE_UPPER;
   }
   return( side );
}

//===========================================================================//
// Method         : ApplyScale
// Purpose        : Scale this box, expanding or shrinking it based on the
//                  given value(s).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::ApplyScale( 
      double         dx, 
      double         dy,
      int            dz,
      TGS_SnapMode_t snap )
{
   double dxBox = this->GetDx( );
   double dyBox = this->GetDy( );
   int dzBox = this->GetDz( );

   dx = ( TCTF_IsGE( dx, 0.0 )) ? dx : TCT_Max( dx, -1.0 * dxBox / 2.0 );
   dy = ( TCTF_IsGE( dy, 0.0 )) ? dy : TCT_Max( dy, -1.0 * dyBox / 2.0 );
   dz = ( dz >= 0 ) ? dz : TCT_Max( dz, -1 * dzBox / 2 );

   double x1 = TCT_Min( this->lowerLeft.x - dx, this->upperRight.x );
   double y1 = TCT_Min( this->lowerLeft.y - dy, this->upperRight.y );
   int z1 = TCT_Min( this->lowerLeft.z - dz, this->upperRight.z );

   double x2 = TCT_Max( this->upperRight.x + dx, this->lowerLeft.x );
   double y2 = TCT_Max( this->upperRight.y + dy, this->lowerLeft.y );
   int z2 = TCT_Max( this->upperRight.z + dz, this->lowerLeft.z );

   this->Set( x1, y1, z1, x2, y2, z2, snap );
}

//===========================================================================//
void TGS_Box_c::ApplyScale( 
      double         scale,
      TC_SideMode_t  side,
      TGS_SnapMode_t snap )
{
   switch( side )
   {
   case TC_SIDE_LEFT:
      this->lowerLeft.x = TCT_Min( this->lowerLeft.x - scale, 
                                   this->upperRight.x );
      break;

   case TC_SIDE_RIGHT:
      this->upperRight.x = TCT_Max( this->upperRight.x + scale, 
                                    this->lowerLeft.x );
      break;

   case TC_SIDE_LOWER:
      this->lowerLeft.y = TCT_Min( this->lowerLeft.y - scale, 
                                   this->upperRight.y );
      break;

   case TC_SIDE_UPPER:
      this->upperRight.y = TCT_Max( this->upperRight.y + scale, 
                                    this->lowerLeft.y );
      break;

   default:
      break;
   }

   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      this->lowerLeft.x = MinGrid.SnapToGrid( this->lowerLeft.x );
      this->lowerLeft.y = MinGrid.SnapToGrid( this->lowerLeft.y );
      this->upperRight.x = MinGrid.SnapToGrid( this->upperRight.x );
      this->upperRight.y = MinGrid.SnapToGrid( this->upperRight.y );
   }
}

//===========================================================================//
void TGS_Box_c::ApplyScale( 
      double           scale,
      TGS_OrientMode_t orient,
      TGS_SnapMode_t   snap )
{
   switch( orient )
   {
   case TGS_ORIENT_HORIZONTAL:
      this->ApplyScale( 0.0, scale, 0, snap );
      break;

   case TGS_ORIENT_VERTICAL:
      this->ApplyScale( scale, 0.0, 0, snap );
      break;

   default:
      this->ApplyScale( scale, scale, 0, snap );
      break;
   }
}

//===========================================================================//
void TGS_Box_c::ApplyScale( 
      const TGS_Box_c&     scale,
            TGS_SnapMode_t snap )
{
   this->lowerLeft.x = TCT_Min( this->lowerLeft.x - scale.lowerLeft.x, 
                                this->upperRight.x );
   this->upperRight.x = TCT_Max( this->upperRight.x + scale.upperRight.x, 
                                 this->lowerLeft.x );

   this->lowerLeft.y = TCT_Min( this->lowerLeft.y - scale.lowerLeft.y, 
                                this->upperRight.y );
   this->upperRight.y = TCT_Max( this->upperRight.y + scale.upperRight.y, 
                                 this->lowerLeft.y );

   this->lowerLeft.z = TCT_Min( this->lowerLeft.z - scale.lowerLeft.z,
                                this->upperRight.z );
   this->upperRight.z = TCT_Max( this->upperRight.z + scale.upperRight.z, 
                                 this->lowerLeft.z );

   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      this->lowerLeft.x = MinGrid.SnapToGrid( this->lowerLeft.x );
      this->lowerLeft.y = MinGrid.SnapToGrid( this->lowerLeft.y );
      this->upperRight.x = MinGrid.SnapToGrid( this->upperRight.x );
      this->upperRight.y = MinGrid.SnapToGrid( this->upperRight.y );
   }
}

//===========================================================================//
void TGS_Box_c::ApplyScale( 
      const TGS_Scale_t&   scale,
            TGS_SnapMode_t snap )
{
   this->ApplyScale( scale.x, scale.y, scale.z, snap );
}

//===========================================================================//
// Method         : ApplyUnion
// Purpose        : Set this box dimensions to the union of the given box
//                  or boxes.  The given box or boxes may or may not be
//                  intersecting.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::ApplyUnion( 
      const TGS_Box_c& boxA, 
      const TGS_Box_c& boxB )
{
   if( boxA.IsValid( ) && boxB.IsValid( ))
   {
      double x1 = TCT_Min( boxA.lowerLeft.x, boxB.lowerLeft.x );
      double y1 = TCT_Min( boxA.lowerLeft.y, boxB.lowerLeft.y );
      int z1 = TCT_Min( boxA.lowerLeft.z, boxB.lowerLeft.z );
   
      double x2 = TCT_Max( boxA.upperRight.x, boxB.upperRight.x );
      double y2 = TCT_Max( boxA.upperRight.y, boxB.upperRight.y );
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
void TGS_Box_c::ApplyUnion( 
      const TGS_Box_c& box )
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
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::ApplyIntersect( 
      const TGS_Box_c& boxA, 
      const TGS_Box_c& boxB )
{
   if( boxA.IsIntersecting( boxB ))
   {
      double x1 = TCT_Max( boxA.lowerLeft.x, boxB.lowerLeft.x );
      double y1 = TCT_Max( boxA.lowerLeft.y, boxB.lowerLeft.y );
      int z1 = TCT_Max( boxA.lowerLeft.z, boxB.lowerLeft.z );

      double x2 = TCT_Min( boxA.upperRight.x, boxB.upperRight.x );
      double y2 = TCT_Min( boxA.upperRight.y, boxB.upperRight.y );
      int z2 = TCT_Min( boxA.upperRight.z, boxB.upperRight.z );

      this->Set( x1, y1, z1, x2, y2, z2 );
   }
   else
   {
      this->Reset( );
   }
}

//===========================================================================//
void TGS_Box_c::ApplyIntersect( 
      const TGS_Box_c& box )
{
   this->ApplyIntersect( *this, box );
}

//===========================================================================//
// Method         : ApplyOverlap
// Purpose        : Set this box dimensions based on the overlap of the given
//                  box or boxes.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::ApplyOverlap( 
      const TGS_Box_c& boxA, 
      const TGS_Box_c& boxB )
{
   if( boxA.IsOverlapping( boxB ))
   {
      double x1 = TCT_Max( boxA.lowerLeft.x, boxB.lowerLeft.x );
      double y1 = TCT_Max( boxA.lowerLeft.y, boxB.lowerLeft.y );
      int z1 = boxA.lowerLeft.z;

      double x2 = TCT_Min( boxA.upperRight.x, boxB.upperRight.x );
      double y2 = TCT_Min( boxA.upperRight.y, boxB.upperRight.y );
      int z2 = boxA.lowerLeft.z;

      this->Set( x1, y1, z1, x2, y2, z2 );
   }
   else
   {
      this->Reset( );
   }
}

//===========================================================================//
void TGS_Box_c::ApplyOverlap( 
      const TGS_Box_c& box )
{
   this->ApplyOverlap( *this, box );
}

//===========================================================================//
// Method         : ApplyAdjacent
// Purpose        : Set this box dimensions based on the adjacency of the
//                  given box or boxes.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::ApplyAdjacent( 
      const TGS_Box_c& boxA, 
      const TGS_Box_c& boxB,
            double     minDistance )
{
   if( boxA.IsAdjacent( boxB, minDistance ))
   {
      if(( TCTF_IsEQ( boxA.lowerLeft.x, boxB.upperRight.x + minDistance )) ||
         ( TCTF_IsEQ( boxA.upperRight.x, boxB.lowerLeft.x - minDistance )))
      {
         this->Set( boxA );

         this->lowerLeft.x = TCT_Min( boxA.lowerLeft.x, boxB.lowerLeft.x );
         this->lowerLeft.y = TCT_Max( boxA.lowerLeft.y, boxB.lowerLeft.y );
         this->upperRight.x = TCT_Max( boxA.upperRight.x, boxB.upperRight.x );
         this->upperRight.y = TCT_Min( boxA.upperRight.y, boxB.upperRight.y );
      }
      if(( TCTF_IsEQ( boxA.lowerLeft.y, boxB.upperRight.y + minDistance )) ||
         ( TCTF_IsEQ( boxA.upperRight.y, boxB.lowerLeft.y - minDistance )))
      {
         this->Set( boxA );

         this->lowerLeft.x = TCT_Max( boxA.lowerLeft.x, boxB.lowerLeft.x );
         this->lowerLeft.y = TCT_Min( boxA.lowerLeft.y, boxB.lowerLeft.y );
         this->upperRight.x = TCT_Min( boxA.upperRight.x, boxB.upperRight.x );
         this->upperRight.y = TCT_Max( boxA.upperRight.y, boxB.upperRight.y );
      }
   }
   else
   {
      this->Reset( );
   }
}

//===========================================================================//
void TGS_Box_c::ApplyAdjacent( 
      const TGS_Box_c& box,
            double     minDistance )
{
   this->ApplyAdjacent( *this, box, minDistance );
}

//===========================================================================//
// Method         : IsIntersecting
// Purpose        : Return true if this box is intersecting with the given
//                  box.  Intersecting implies intersecting z layers, as well 
//                  as intersecting x/y region coordinates.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Box_c::IsIntersecting( 
      const TGS_Box_c& box ) const
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
   else if(( TCTF_IsLT( box.lowerLeft.x, this->lowerLeft.x )) &&
           ( TCTF_IsLT( box.upperRight.x, this->lowerLeft.x )))
   {
      isIntersecting = false;
   }
   else if(( TCTF_IsGT( box.lowerLeft.x, this->upperRight.x )) &&
           ( TCTF_IsGT( box.upperRight.x, this->upperRight.x )))
   {
      isIntersecting = false;
   }
   else if(( TCTF_IsLT( box.lowerLeft.y, this->lowerLeft.y )) &&
           ( TCTF_IsLT( box.upperRight.y, this->lowerLeft.y )))
   {
      isIntersecting = false;
   }
   else if(( TCTF_IsGT( box.lowerLeft.y, this->upperRight.y )) &&
           ( TCTF_IsGT( box.upperRight.y, this->upperRight.y )))
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
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Box_c::IsOverlapping( 
      const TGS_Box_c& box ) const
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
   else if(( TCTF_IsLT( box.lowerLeft.x, this->lowerLeft.x )) &&
           ( TCTF_IsLT( box.upperRight.x, this->lowerLeft.x )))
   {
      isOverlapping = false;
   }
   else if(( TCTF_IsGT( box.lowerLeft.x, this->upperRight.x )) &&
           ( TCTF_IsGT( box.upperRight.x, this->upperRight.x )))
   {
      isOverlapping = false;
   }
   else if(( TCTF_IsLT( box.lowerLeft.y, this->lowerLeft.y )) &&
           ( TCTF_IsLT( box.upperRight.y, this->lowerLeft.y )))
   {
      isOverlapping = false;
   }
   else if(( TCTF_IsGT( box.lowerLeft.y, this->upperRight.y )) &&
           ( TCTF_IsGT( box.upperRight.y, this->upperRight.y )))
   {
      isOverlapping = false;
   }
   return( isOverlapping );
}

//===========================================================================//
// Method         : IsAdjacent
// Purpose        : Return true if this box is adjacent to the given box.
//                  Adjacent implies intersecting z layers, however with 
//                  non-intersecting, but adjacent x/y region coordinates.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Box_c::IsAdjacent( 
      const TGS_Box_c& box,
            double     minDistance ) const
{
   bool isAdjacent = true;

   if(( box.lowerLeft.z < this->lowerLeft.z ) &&
      ( box.upperRight.z < this->lowerLeft.z ))
   {
      isAdjacent = false;
   }
   else if(( box.lowerLeft.z > this->upperRight.z ) &&
           ( box.upperRight.z > this->upperRight.z ))
   {
      isAdjacent = false;
   }
   else if(( TCTF_IsLT( box.lowerLeft.x, this->lowerLeft.x - minDistance )) &&
           ( TCTF_IsLT( box.upperRight.x, this->lowerLeft.x - minDistance )))
   {
      isAdjacent = false;
   }
   else if(( TCTF_IsGT( box.lowerLeft.x, this->upperRight.x + minDistance )) &&
           ( TCTF_IsGT( box.upperRight.x, this->upperRight.x + minDistance )))
   {
      isAdjacent = false;
   }
   else if(( TCTF_IsLT( box.lowerLeft.y, this->lowerLeft.y - minDistance )) &&
           ( TCTF_IsLT( box.upperRight.y, this->lowerLeft.y - minDistance )))
   {
      isAdjacent = false;
   }
   else if(( TCTF_IsGT( box.lowerLeft.y, this->upperRight.y + minDistance )) &&
           ( TCTF_IsGT( box.upperRight.y, this->upperRight.y + minDistance )))
   {
      isAdjacent = false;
   }
   else if( this->IsWithin( box ))
   {
      isAdjacent = false;
   }
   return( isAdjacent );
}

//===========================================================================//
// Method         : IsWithin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Box_c::IsWithin( 
      const TGS_Box_c& box ) const
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
bool TGS_Box_c::IsWithin( 
      const TGS_Point_c& point ) const
{
   bool isWithin = false;

   if(( point.z >= this->lowerLeft.z ) &&
      ( point.z <= this->upperRight.z ) &&
      ( TCTF_IsGE( point.x, this->lowerLeft.x )) &&
      ( TCTF_IsLE( point.x, this->upperRight.x )) &&
      ( TCTF_IsGE( point.y, this->lowerLeft.y )) &&
      ( TCTF_IsLE( point.y, this->upperRight.y )))
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
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Box_c::IsWide( 
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
bool TGS_Box_c::IsTall( 
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
bool TGS_Box_c::IsSquare( 
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
bool TGS_Box_c::IsLegal( 
      void ) const
{
   bool isLegal = this->IsValid( );
   if( isLegal )
   {
      isLegal = (( TCTF_IsLE( this->lowerLeft.x, this->upperRight.x )) &&
                 ( TCTF_IsLE( this->lowerLeft.y, this->upperRight.y )) ?
                 true : false );
   }
   return( isLegal );
}


//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Box_c::IsValid( 
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
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::FindNearestCoords_(
      double  coord1i, 
      double  coord1j, 
      double  coord2i, 
      double  coord2j,
      double* pnearestCoord1, 
      double* pnearestCoord2 ) const
{
   // Find nearest coordinates by testing for overlap, then non-overlap cases
   if( TCTF_IsLE( coord1i, coord2j ) && 
       TCTF_IsGE( coord1j, coord2i ))
   {
      *pnearestCoord1 = *pnearestCoord2 = TCT_Max( coord1i, coord2i );
   }
   else if( TCTF_IsLT( coord1j, coord2i ))
   {
      *pnearestCoord1 = coord1j;
      *pnearestCoord2 = coord2i;
   }
   else if( TCTF_IsGT( coord1i, coord2j ))
   {
      *pnearestCoord1 = coord1i;
      *pnearestCoord2 = coord2j;
   }
}

//===========================================================================//
void TGS_Box_c::FindNearestCoords_(
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
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Box_c::FindFarthestCoords_(
      double  coord1i, 
      double  coord1j, 
      double  coord2i, 
      double  coord2j,
      double* pfarthestCoord1, 
      double* pfarthestCoord2 ) const
{
   // Find farthest coordinates by testing for overlap, then non-overlap cases
   if( TCTF_IsLE( coord1i, coord2j ) && 
       TCTF_IsGE( coord1j, coord2i ))
   {
      *pfarthestCoord1 = *pfarthestCoord2 = TCT_Min( coord1i, coord2i );
   }
   else if( TCTF_IsLT( coord1j, coord2i ))
   {
      *pfarthestCoord1 = coord1i;
      *pfarthestCoord2 = coord2j;
   }
   else if( TCTF_IsGT( coord1i, coord2j ))
   {
      *pfarthestCoord1 = coord1j;
      *pfarthestCoord2 = coord2i;
   }
}

//===========================================================================//
void TGS_Box_c::FindFarthestCoords_(
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
