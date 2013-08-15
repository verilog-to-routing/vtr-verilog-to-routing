//===========================================================================//
// Purpose : Declaration and inline definitions for a TGO_Region geometric 
//           object 2D region class.
//
//           Inline methods include:
//           - GetDx, GetDy
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

#ifndef TGO_REGION_H
#define TGO_REGION_H

#include <cstdio>
#include <cmath>
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
class TGO_Region_c
{
public:

   TGO_Region_c( void );
   TGO_Region_c( int x1, int y1, int x2, int y2 );
   TGO_Region_c( const TGO_Point_c& pointA, 
                 const TGO_Point_c& pointB );
   TGO_Region_c( const TGO_Region_c& region );
   ~TGO_Region_c( void );

   TGO_Region_c& operator=( const TGO_Region_c& region );
   bool operator<( const TGO_Region_c& region ) const;
   bool operator==( const TGO_Region_c& region ) const;
   bool operator!=( const TGO_Region_c& region ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrRegion ) const;

   void Set( int x1, int y1, int x2, int y2 );
   void Set( const TGO_Point_c& pointA, 
             const TGO_Point_c& pointB );
   void Set( const TGO_Region_c& region );
   void Reset( void );

   int GetDx( void ) const;
   int GetDy( void ) const;

   int FindMin( void ) const;
   int FindMax( void ) const;

   TGO_Point_c FindLowerLeft( void ) const;
   TGO_Point_c FindLowerRight( void ) const;
   TGO_Point_c FindUpperLeft( void ) const;
   TGO_Point_c FindUpperRight( void ) const;

   TGO_OrientMode_t FindOrient( TGO_OrientMode_t orient = TGO_ORIENT_UNDEFINED ) const;

   int FindWidth( TGO_OrientMode_t orient = TGO_ORIENT_UNDEFINED ) const;
   int FindLength( TGO_OrientMode_t orient = TGO_ORIENT_UNDEFINED ) const;

   double FindDistance( const TGO_Region_c& refRegion ) const;
   double FindDistance( const TGO_Point_c& refPoint ) const;
   
   void FindNearest( const TGO_Point_c& refPoint, 
                     TGO_Point_c* pthisNearestPoint = 0 ) const;
   void FindNearest( const TGO_Region_c& refRegion,
                     TGO_Point_c* prefNearestPoint = 0, 
                     TGO_Point_c* pthisNearestPoint = 0 ) const;

   void ApplyScale( int dx, int dy );
   void ApplyScale( int scale );

   void ApplyUnion( const TGO_Region_c& regionA, 
                    const TGO_Region_c& regionB );
   void ApplyUnion( const TGO_Region_c& region );
   void ApplyUnion( const TGO_Point_c& point );
   void ApplyIntersect( const TGO_Region_c& regionA, 
                        const TGO_Region_c& regionB );
   void ApplyIntersect( const TGO_Region_c& region );

   bool IsIntersecting( const TGO_Region_c& region ) const;
   bool IsOverlapping( const TGO_Region_c& region ) const;

   bool IsWithin( const TGO_Region_c& region ) const;
   bool IsWithin( const TGO_Point_c& point ) const;

   bool IsWide( void ) const;
   bool IsTall( void ) const;
   bool IsSquare( void ) const;

   bool IsLegal( void ) const;
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
inline int TGO_Region_c::GetDx( 
      void ) const
{
   return( abs( this->x1 - this->x2 ));
}

//===========================================================================//
inline int TGO_Region_c::GetDy( 
      void ) const
{
   return( abs( this->y1 - this->y2 ));
}

#endif
