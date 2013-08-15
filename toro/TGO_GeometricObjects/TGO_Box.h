//===========================================================================//
// Purpose : Declaration and inline definitions for a TGO_Box geometric
//           object 3D box class.
//
//           Inline methods include:
//           - GetDx, GetDy, GetDz
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

#ifndef TGO_BOX_H
#define TGO_BOX_H

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
// 10/25/12 jeffr : Original
//===========================================================================//
class TGO_Box_c
{
public:

   TGO_Box_c( void );
   TGO_Box_c( int x1, int y1, int z1, int x2, int y2, int z2 );
   TGO_Box_c( int x1, int y1, int x2, int y2 );
   TGO_Box_c( const TGO_Point_c& pointA, 
              const TGO_Point_c& pointB );
   TGO_Box_c( const TGO_Box_c& box );
   ~TGO_Box_c( void );

   TGO_Box_c& operator=( const TGO_Box_c& box );
   bool operator<( const TGO_Box_c& box ) const;
   bool operator==( const TGO_Box_c& box ) const;
   bool operator!=( const TGO_Box_c& box ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrBox ) const;

   void Set( int x1, int y1, int z1, int x2, int y2, int z2 );
   void Set( int x1, int y1, int x2, int y2 ); 
   void Set( const TGO_Point_c& pointA, 
             const TGO_Point_c& pointB );
   void Set( const TGO_Box_c& box );
   void Reset( void );

   int GetDx( void ) const;
   int GetDy( void ) const;
   int GetDz( void ) const;

   int FindMin( void ) const;
   int FindMax( void ) const;

   TGO_OrientMode_t FindOrient( TGO_OrientMode_t orient = TGO_ORIENT_UNDEFINED ) const;

   int FindWidth( TGO_OrientMode_t orient = TGO_ORIENT_UNDEFINED ) const;
   int FindLength( TGO_OrientMode_t orient = TGO_ORIENT_UNDEFINED ) const;

   double FindDistance( const TGO_Box_c& refBox ) const;
   double FindDistance( const TGO_Point_c& refPoint ) const;
   
   void FindNearest( const TGO_Box_c& refBox,
                     TGO_Point_c* prefNearestPoint = 0, 
                     TGO_Point_c* pthisNearestPoint = 0 ) const;
   void FindNearest( const TGO_Point_c& refPoint, 
                     TGO_Point_c* pthisNearestPoint = 0 ) const;
   void FindFarthest( const TGO_Box_c& refBox,
                      TGO_Point_c* prefFarthestPoint = 0,
                      TGO_Point_c* pthisFarthestPoint = 0 ) const;
   void FindFarthest( const TGO_Point_c& refPoint, 
                      TGO_Point_c* pthisFarthestPoint = 0 ) const;

   void FindBox( TC_SideMode_t side, 
                 TGO_Box_c* pbox ) const;

   void ApplyScale( int dx, int dy, int dz );

   void ApplyUnion( const TGO_Box_c& boxA, 
                    const TGO_Box_c& boxB );
   void ApplyUnion( const TGO_Box_c& box );
   void ApplyIntersect( const TGO_Box_c& boxA, 
                        const TGO_Box_c& boxB );
   void ApplyIntersect( const TGO_Box_c& box );

   bool IsIntersecting( const TGO_Box_c& box ) const;
   bool IsOverlapping( const TGO_Box_c& box ) const;

   bool IsWithin( const TGO_Box_c& box ) const;
   bool IsWithin( const TGO_Point_c& point ) const;

   bool IsWide( void ) const;
   bool IsTall( void ) const;
   bool IsSquare( void ) const;

   bool IsLegal( void ) const;
   bool IsValid( void ) const;

private:

   void FindNearestCoords_( int coord1i, 
                            int coord1j, 
                            int coord2i, 
                            int coord2j,
                            int* pnearestCoord1, 
                            int* pnearestCoord2 ) const;
   void FindFarthestCoords_( int coord1i, 
                             int coord1j, 
                             int coord2i, 
                             int coord2j,
                             int* pfarthestCoord1, 
                             int* pfarthestCoord2 ) const;
public:

   TGO_Point_c lowerLeft;
   TGO_Point_c upperRight;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/25/12 jeffr : Original
//===========================================================================//
inline int TGO_Box_c::GetDx( 
      void ) const
{
   return( abs( this->lowerLeft.x - this->upperRight.x ));
}

//===========================================================================//
inline int TGO_Box_c::GetDy( 
      void ) const
{
   return( abs( this->lowerLeft.y - this->upperRight.y ));
}

//===========================================================================//
inline int TGO_Box_c::GetDz( 
      void ) const
{
   return( abs( this->lowerLeft.z - this->upperRight.z ));
}

#endif
