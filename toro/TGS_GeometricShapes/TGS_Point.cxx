//===========================================================================//
// Purpose : Method definitions for the TGS_Point class.
//
//           Public methods include:
//           - TGS_Point_c, ~TGS_Point_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set, Reset
//           - FindOrient
//           - FindDistance
//           - CrossProduct
//           - IsOrthogonal
//           - IsHorizontal, IsVertical
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

#include "TIO_PrintHandler.h"

#include "TGS_Box.h"
#include "TGS_Point.h"

//===========================================================================//
// Method         : TGS_Point_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Point_c::TGS_Point_c( 
      void )
      :
      x( TC_FLT_MIN ),
      y( TC_FLT_MIN ),
      z( INT_MIN )
{
}

//===========================================================================//
TGS_Point_c::TGS_Point_c( 
      double         x_, 
      double         y_,
      int            z_,
      TGS_SnapMode_t snap )
{
   this->Set( x_, y_, z_, snap );
}

//===========================================================================//
TGS_Point_c::TGS_Point_c( 
      double         x_, 
      double         y_,
      TGS_SnapMode_t snap )
{
   this->Set( x_, y_, snap );
}

//===========================================================================//
TGS_Point_c::TGS_Point_c( 
      const TGS_Point_c&   point,
            TGS_SnapMode_t snap )
{
   this->Set( point, snap );
}

//===========================================================================//
// Method         : ~TGS_Point_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Point_c::~TGS_Point_c( 
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
TGS_Point_c& TGS_Point_c::operator=( 
      const TGS_Point_c& point )
{
   if( &point != this )
   {
      this->x = point.x;
      this->y = point.y;
      this->z = point.z;
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
bool TGS_Point_c::operator<( 
      const TGS_Point_c& point ) const
{
   bool isLessThan = false;

   if( this->z < point.z )
   {
      isLessThan = true;
   }
   else if( this->z == point.z )
   {
      if( TCTF_IsLT( this->x, point.x ))
      {
         isLessThan = true;
      }
      else if( TCTF_IsEQ( this->x, point.x ))
      {
 	 if( TCTF_IsLT( this->y, point.y ))
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
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Point_c::operator==( 
      const TGS_Point_c& point ) const
{
   return(( TCTF_IsEQ( this->x, point.x )) && 
          ( TCTF_IsEQ( this->y, point.y )) &&
          ( this->z == point.z ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Point_c::operator!=( 
      const TGS_Point_c& point ) const
{
   return(( !this->operator==( point )) ? 
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Point_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srPoint;
   this->ExtractString( &srPoint );

   printHandler.Write( pfile, spaceLen, "[point] %s\n", TIO_SR_STR( srPoint ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Point_c::ExtractString( 
      string* psrPoint,
      size_t  precision ) const
{
   if( psrPoint )
   {
      char szPoint[TIO_FORMAT_STRING_LEN_POINT];

      if( precision == SIZE_MAX )
      {
         precision = TC_MinGrid_c::GetInstance( ).GetPrecision( );
      }

      if( this->IsValid( ))
      {
	 if( this->z != INT_MIN )
         {
            sprintf( szPoint, "%0.*f %0.*f %d",
  	   	              static_cast< int >( precision ), this->x, 
                              static_cast< int >( precision ), this->y, 
                              this->z );
         }
	 else
         {
            sprintf( szPoint, "%0.*f %0.*f",
  	   	              static_cast< int >( precision ), this->x, 
                              static_cast< int >( precision ), this->y ); 
         }
      }
      else
      {
         sprintf( szPoint, "? ? ?" );
      }
      *psrPoint = szPoint;
   }
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Point_c::Set( 
      double         x_, 
      double         y_,
      int            z_,
      TGS_SnapMode_t snap )
{
   this->x = x_;
   this->y = y_;
   this->z = z_;

   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      this->x = MinGrid.SnapToGrid( this->x );
      this->y = MinGrid.SnapToGrid( this->y );
   }
}

//===========================================================================//
void TGS_Point_c::Set( 
      double         x_, 
      double         y_,
      TGS_SnapMode_t snap )
{
   this->Set( x_, y_, 0, snap );
}

//===========================================================================//
void TGS_Point_c::Set( 
      const TGS_Point_c&   point,
            TGS_SnapMode_t snap )
{
   this->Set( point.x, point.y, point.z, snap );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Point_c::Reset( 
      void )
{
   this->Set( TC_FLT_MIN, TC_FLT_MIN, INT_MIN );
}

//===========================================================================//
// Method         : FindOrient
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_OrientMode_t TGS_Point_c::FindOrient(
      const TGS_Point_c& refPoint ) const
{
   TGS_Box_c thisBox( this->x, this->y, this->z,
                      refPoint.x, refPoint.y, refPoint.z );

   return( thisBox.FindOrient( ));
}

//===========================================================================//
// Method         : FindDistance
// Purpose        : Compute and return the "flightline" distance between
//                  this point and the given point
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Point_c::FindDistance( 
      const TGS_Point_c& refPoint ) const
{
   double distance = TC_FLT_MAX;

   if( this->IsValid( ) && refPoint.IsValid( ))
   {
      double dx = this->GetDx( refPoint );
      double dy = this->GetDy( refPoint );
      int dz = this->GetDz( refPoint );

      double dx2 = dx * dx;
      double dy2 = dy * dy;
      distance = sqrt( dx2 + dy2 );

      if( dz > 0 )
      {
         double dz2 = dz * dz;
         double distance2 = distance * distance;
         distance = sqrt( dz2 + distance2 );
      }
   }
   return( distance );
}

//===========================================================================//
// Method         : CrossProduct
// Purpose        : Find and return the "cross-product" based on this 
//                  point and the given two points.  The cross-product is 
//                  computed as "(p1-p0)x(p2-p0)".  This method returns
//                  a positive value when point 'Point1' is clockwise with
//                  respect to 'Point2'.  This method returns a negative
//                  value when point 'Point1' is counterclockwise with
//                  respect to 'Point2'.
// Reference      : Algorithms by Cormen, Leiserson, Rivest, pp. 887-888.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
int TGS_Point_c::CrossProduct( 
      const TGS_Point_c& point1, 
      const TGS_Point_c& point2 ) const
{
   double x0 = this->x;
   double y0 = this->y;
   double x1 = point1.x;
   double y1 = point1.y;
   double x2 = point2.x;
   double y2 = point2.y;
   double r = (( x2 - x0 ) * ( y1 - y0 )) - (( x1 - x0 ) * ( y2 - y0 ));

   int crossProduct = 0;
   if( TCTF_IsGT( r, 0.0 ))
   {
      crossProduct = 1;
   }
   else if( TCTF_IsLT( r, 0.0 ))
   {
      crossProduct = -1;
   }
   return( crossProduct );
}

//===========================================================================//
// Method         : IsOrthogonal
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Point_c::IsOrthogonal( 
      const TGS_Point_c& point ) const
{
   return(( TCTF_IsZE( this->GetDx( point ))) || 
          ( TCTF_IsZE( this->GetDy( point ))) ?
          true : false );
}

//===========================================================================//
// Method         : IsHorizontal/IsVertical
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Point_c::IsHorizontal( 
      const TGS_Point_c& point ) const
{
   return(( this->IsOrthogonal( point ) && 
          ( this->IsLeft( point ) || this->IsRight( point ))) ?
          true : false );
}

//===========================================================================//
bool TGS_Point_c::IsVertical( 
      const TGS_Point_c& point ) const
{
   return(( this->IsOrthogonal( point ) && 
          ( this->IsLower( point ) || this->IsUpper( point ))) ?
          true : false );
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Point_c::IsValid( 
      void ) const
{
   return(( TCTF_IsNEQ( this->x, TC_FLT_MIN )) && 
          ( TCTF_IsNEQ( this->x, TC_FLT_MAX )) &&
          ( TCTF_IsNEQ( this->y, TC_FLT_MIN )) && 
          ( TCTF_IsNEQ( this->y, TC_FLT_MAX )) ?
          true : false );
} 
