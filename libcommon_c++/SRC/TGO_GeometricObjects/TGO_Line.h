//===========================================================================//
// Purpose : Declaration and inline definitions for a TGO_Line geometric 
//           object 2D line class.
//
//           Inline methods include:
//           - GetDx, GetDy
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

#ifndef TGO_LINE_H
#define TGO_LINE_H

#include <cstdio>
using namespace std;

#include "TGO_Typedefs.h"
#include "TGO_Point.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
class TGO_Line_c
{
public:

   TGO_Line_c( void );
   TGO_Line_c( int x1, int y1, int x2, int y2 );
   TGO_Line_c( const TGO_Point_c& beginPoint, 
               const TGO_Point_c& endPoint );
   TGO_Line_c( const TGO_Line_c& line );
   ~TGO_Line_c( void );

   TGO_Line_c& operator=( const TGO_Line_c& line );
   bool operator==( const TGO_Line_c& line ) const;
   bool operator!=( const TGO_Line_c& line ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrLine ) const;

   void Set( int x1, int y1, int x2, int y2 );
   void Set( const TGO_Point_c& beginPoint, 
             const TGO_Point_c& endPoint );
   void Set( const TGO_Line_c& line );
   void Reset( void );

   TGO_Point_c GetBeginPoint( void ) const;                              
   TGO_Point_c GetEndPoint( void ) const;

   int GetDx( void ) const;
   int GetDy( void ) const;

   int FindLength( void ) const;

   TGO_Point_c FindCenter( void ) const;

   TGO_OrientMode_t FindOrient( void ) const;

   double FindDistance( const TGO_Line_c& refLine ) const;
   double FindDistance( const TGO_Point_c& refPoint ) const;

   void FindNearest( const TGO_Line_c& refLine,
                     TGO_Point_c* prefNearestPoint = 0, 
                     TGO_Point_c* pthisNearestPoint = 0 ) const;
   void FindNearest( const TGO_Point_c& refPoint, 
                     TGO_Point_c* pthisNearestPoint = 0 ) const;

   void FindIntersect( const TGO_Line_c& refLine, 
                       TGO_Point_c* pthisIntersectPoint = 0 ) const;

   int CrossProduct( const TGO_Point_c& refPoint ) const;

   bool IsConnected( const TGO_Line_c& line ) const;

   bool IsIntersecting( const TGO_Line_c& line,
                        TGO_Point_c* ppoint = 0 ) const;
   bool IsIntersecting( const TGO_Point_c& point ) const;

   bool IsWithin( const TGO_Line_c& line ) const;
   bool IsWithin( const TGO_Point_c& point ) const;

   bool IsParallel( const TGO_Line_c& line ) const;

   bool IsLeft( void ) const;
   bool IsRight( void ) const;
   bool IsLower( void ) const;
   bool IsUpper( void ) const;

   bool IsLowerLeft( void ) const;
   bool IsLowerRight( void ) const;
   bool IsUpperLeft( void ) const;
   bool IsUpperRight( void ) const;

   bool IsOrthogonal( void ) const;
   bool IsHorizontal( void ) const;
   bool IsVertical( void ) const;

   bool IsValid( void ) const;

public:

   int x1;
   int y1;
   int x2;
   int y2;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
inline int TGO_Line_c::GetDx( 
      void ) const
{
   return( abs( this->x1 - this->x2 ));
}

//===========================================================================//
inline int TGO_Line_c::GetDy( 
      void ) const
{
   return( abs( this->y1 - this->y2 ));
}

#endif
