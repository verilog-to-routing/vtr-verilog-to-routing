//===========================================================================//
// Purpose : Declaration and inline definitions for a TGO_Point geometric 
//           object 3D point class.
//
//           Inline methods include:
//           - GetDx, GetDy, GetDz
//           - IsLeft, IsRight, IsLower, IsUpper
//           - IsLowerLeft, IsLowerRight, IsUpperLeft, IsUpperRight
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

#ifndef TGO_POINT_H
#define TGO_POINT_H

#include <cstdio>
#include <cmath>
using namespace std;

#include "TGO_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
class TGO_Point_c
{
public:

   TGO_Point_c( void );
   TGO_Point_c( int x, int y, int z );
   TGO_Point_c( int x, int y );
   TGO_Point_c( const TGO_Point_c& point );
   ~TGO_Point_c( void );

   TGO_Point_c& operator=( const TGO_Point_c& point );
   bool operator<( const TGO_Point_c& point ) const;
   bool operator==( const TGO_Point_c& point ) const;
   bool operator!=( const TGO_Point_c& point ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrPoint ) const;

   void Set( int x, int y, int z );
   void Set( int x, int y );
   void Set( const TGO_Point_c& point );
   void Reset( void );

   int GetDx( const TGO_Point_c& point ) const;
   int GetDy( const TGO_Point_c& point ) const;
   int GetDz( const TGO_Point_c& point ) const;

   TGO_OrientMode_t FindOrient( const TGO_Point_c& refPoint ) const;

   double FindDistance( const TGO_Point_c& refPoint ) const;

   bool IsLeft( const TGO_Point_c& point ) const;
   bool IsRight( const TGO_Point_c& point ) const;
   bool IsLower( const TGO_Point_c& point ) const;
   bool IsUpper( const TGO_Point_c& point ) const;

   bool IsLowerLeft( const TGO_Point_c& point ) const;
   bool IsLowerRight( const TGO_Point_c& point ) const;
   bool IsUpperLeft( const TGO_Point_c& point ) const;
   bool IsUpperRight( const TGO_Point_c& point ) const;

   bool IsOrthogonal( const TGO_Point_c& point ) const;
   bool IsHorizontal( const TGO_Point_c& point ) const;
   bool IsVertical( const TGO_Point_c& point ) const;

   bool IsValid( void ) const;

public:

   int x;
   int y;
   int z;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
inline int TGO_Point_c::GetDx( 
      const TGO_Point_c& point ) const
{
   return( abs( this->x - point.x ));
}

//===========================================================================//
inline int TGO_Point_c::GetDy( 
      const TGO_Point_c& point ) const
{
   return( abs( this->y - point.y ));
}

//===========================================================================//
inline int TGO_Point_c::GetDz( 
      const TGO_Point_c& point ) const
{
   return( abs( this->z - point.z ));
}

//===========================================================================//
inline bool TGO_Point_c::IsLeft( 
      const TGO_Point_c& point ) const
{
   return(( this->x < point.x ) && ( this->y == point.y ) ? true : false );
}

//===========================================================================//
inline bool TGO_Point_c::IsRight( 
      const TGO_Point_c& point ) const
{
   return(( this->x > point.x ) && ( this->y == point.y ) ? true : false );
}

//===========================================================================//
inline bool TGO_Point_c::IsLower( 
      const TGO_Point_c& point ) const
{
   return(( this->x == point.x ) && ( this->y < point.y ) ? true : false );
}

//===========================================================================//
inline bool TGO_Point_c::IsUpper( 
      const TGO_Point_c& point ) const
{
   return(( this->x == point.x ) && ( this->y > point.y ) ? true : false );
}

//===========================================================================//
inline bool TGO_Point_c::IsLowerLeft( 
      const TGO_Point_c& point ) const
{
   return(( this->x < point.x ) && ( this->y < point.y ) ? true : false );
}

//===========================================================================//
inline bool TGO_Point_c::IsLowerRight( 
      const TGO_Point_c& point ) const
{
   return(( this->x > point.x ) && ( this->y < point.y ) ? true : false );
}

//===========================================================================//
inline bool TGO_Point_c::IsUpperLeft( 
      const TGO_Point_c& point ) const
{
   return(( this->x < point.x ) && ( this->y > point.y ) ? true : false );
}

//===========================================================================//
inline bool TGO_Point_c::IsUpperRight( 
      const TGO_Point_c& point ) const
{
   return(( this->x > point.x ) && ( this->y > point.y ) ? true : false );
}

#endif
