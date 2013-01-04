//===========================================================================//
// Purpose : Method definitions for the TGO_Point class.
//
//           Public methods include:
//           - TGO_Point_c, ~TGO_Point_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set, Reset
//           - FindOrient
//           - FindDistance
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

#include "TIO_PrintHandler.h"

#include "TGO_Box.h"
#include "TGO_Point.h"

//===========================================================================//
// Method         : TGO_Point_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_Point_c::TGO_Point_c( 
      void )
      :
      x( INT_MIN ),
      y( INT_MIN ),
      z( INT_MIN )
{
}

//===========================================================================//
TGO_Point_c::TGO_Point_c( 
      int x_, 
      int y_,
      int z_ )
{
   this->Set( x_, y_, z_ );
}

//===========================================================================//
TGO_Point_c::TGO_Point_c( 
      int x_, 
      int y_ )
{
   this->Set( x_, y_ );
}

//===========================================================================//
TGO_Point_c::TGO_Point_c( 
      const TGO_Point_c& point )
{
   this->Set( point );
}

//===========================================================================//
// Method         : ~TGO_Point_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_Point_c::~TGO_Point_c( 
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
TGO_Point_c& TGO_Point_c::operator=( 
      const TGO_Point_c& point )
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
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Point_c::operator<( 
      const TGO_Point_c& point ) const
{
   bool isLessThan = false;

   if( this->z < point.z )
   {
      isLessThan = true;
   }
   else if( this->z == point.z )
   {
      if( this->x < point.x )
      {
         isLessThan = true;
      }
      else if( this->x == point.x )
      {
 	 if( this->y < point.y )
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
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Point_c::operator==( 
      const TGO_Point_c& point ) const
{
   return(( this->x == point.x ) &&
          ( this->y == point.y ) && 
          ( this->z == point.z ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Point_c::operator!=( 
      const TGO_Point_c& point ) const
{
   return(( !this->operator==( point )) ? 
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Point_c::Print( 
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
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Point_c::ExtractString( 
      string* psrPoint ) const
{
   if( psrPoint )
   {
      char szPoint[TIO_FORMAT_STRING_LEN_POINT];

      if( this->IsValid( ))
      {
	 if(( this->z != INT_MIN ) && ( this->z != INT_MAX ))
         {
            sprintf( szPoint, "%d %d %d", this->x, this->y, this->z );
         }
	 else
         {
            sprintf( szPoint, "%d %d", this->x, this->y ); 
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
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Point_c::Set( 
      int x_, 
      int y_,
      int z_ )
{
   this->x = x_;
   this->y = y_;
   this->z = z_;
}

//===========================================================================//
void TGO_Point_c::Set( 
      int x_, 
      int y_ )
{
   this->Set( x_, y_, 0 );
}

//===========================================================================//
void TGO_Point_c::Set( 
      const TGO_Point_c& point )
{
   this->Set( point.x, point.y, point.z );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
void TGO_Point_c::Reset( 
      void )
{
   this->Set( INT_MIN, INT_MIN, INT_MIN );
}

//===========================================================================//
// Method         : FindOrient
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
TGO_OrientMode_t TGO_Point_c::FindOrient(
      const TGO_Point_c& refPoint ) const
{
   TGO_Box_c thisBox( this->x, this->y, this->z,
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
// 10/25/12 jeffr : Original
//===========================================================================//
double TGO_Point_c::FindDistance( 
      const TGO_Point_c& refPoint ) const
{
   double distance = TC_FLT_MAX;

   if( this->IsValid( ) && refPoint.IsValid( ))
   {
      int dx = this->GetDx( refPoint );
      int dy = this->GetDy( refPoint );
      int dz = this->GetDz( refPoint );

      double dx2 = static_cast< double >( dx * dx );
      double dy2 = static_cast< double >( dy * dy );
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
// Method         : IsOrthogonal
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Point_c::IsOrthogonal( 
      const TGO_Point_c& point ) const
{
   return(( this->GetDx( point ) == 0 ) || ( this->GetDy( point ) == 0 ) ?
          true : false );
}

//===========================================================================//
// Method         : IsHorizontal/IsVertical
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Point_c::IsHorizontal( 
      const TGO_Point_c& point ) const
{
   return(( this->IsOrthogonal( point ) && 
          ( this->IsLeft( point ) || this->IsRight( point ))) ?
          true : false );
}

//===========================================================================//
bool TGO_Point_c::IsVertical( 
      const TGO_Point_c& point ) const
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
// 10/25/12 jeffr : Original
//===========================================================================//
bool TGO_Point_c::IsValid( 
      void ) const
{
   return(( this->x != INT_MIN ) && 
          ( this->x != INT_MAX ) &&
          ( this->y != INT_MIN ) && 
          ( this->y != INT_MAX ) ?
          true : false );
} 
