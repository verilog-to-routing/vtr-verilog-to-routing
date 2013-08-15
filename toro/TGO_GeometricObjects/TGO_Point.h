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
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
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

   int CrossProduct( const TGO_Point_c& point1, 
                     const TGO_Point_c& point2 ) const;

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
