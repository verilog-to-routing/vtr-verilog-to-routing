//===========================================================================//
// Purpose : Declaration and inline definitions for a TGS_Point geometric 
//           shape 3D point class.
//
//           Inline methods include:
//           - GetDx, GetDy, GetDz
//           - IsLeft, IsRight, IsLower, IsUpper
//           - IsLowerLeft, IsLowerRight, IsUpperLeft, IsUpperRight
//
//===========================================================================//

#ifndef TGS_POINT_H
#define TGS_POINT_H

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
using namespace std;

#include "TC_Typedefs.h"
#include "TCT_Generic.h"

#include "TGS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
class TGS_Point_c
{
public:

   TGS_Point_c( void );
   TGS_Point_c( double x, double y, int z,
                TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Point_c( double x, double y,
                TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Point_c( const TGS_Point_c& point,
                TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   ~TGS_Point_c( void );

   TGS_Point_c& operator=( const TGS_Point_c& point );
   bool operator<( const TGS_Point_c& point ) const;
   bool operator==( const TGS_Point_c& point ) const;
   bool operator!=( const TGS_Point_c& point ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrPoint,
                       size_t precision = SIZE_MAX ) const;

   void Set( double x, double y, int z,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( double x, double y,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( const TGS_Point_c& point,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Reset( void );

   double GetDx( const TGS_Point_c& point ) const;
   double GetDy( const TGS_Point_c& point ) const;
   int GetDz( const TGS_Point_c& point ) const;

   TGS_OrientMode_t FindOrient( const TGS_Point_c& refPoint ) const;

   double FindDistance( const TGS_Point_c& refPoint ) const;

   int CrossProduct( const TGS_Point_c& point1, 
                     const TGS_Point_c& point2 ) const;

   bool IsLeft( const TGS_Point_c& point ) const;
   bool IsRight( const TGS_Point_c& point ) const;
   bool IsLower( const TGS_Point_c& point ) const;
   bool IsUpper( const TGS_Point_c& point ) const;

   bool IsLowerLeft( const TGS_Point_c& point ) const;
   bool IsLowerRight( const TGS_Point_c& point ) const;
   bool IsUpperLeft( const TGS_Point_c& point ) const;
   bool IsUpperRight( const TGS_Point_c& point ) const;

   bool IsOrthogonal( const TGS_Point_c& point ) const;
   bool IsHorizontal( const TGS_Point_c& point ) const;
   bool IsVertical( const TGS_Point_c& point ) const;

   bool IsValid( void ) const;

public:

   double x;
   double y;
   int z;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
inline double TGS_Point_c::GetDx( 
      const TGS_Point_c& point ) const
{
   return( fabs( this->x - point.x ));
}

//===========================================================================//
inline double TGS_Point_c::GetDy( 
      const TGS_Point_c& point ) const
{
   return( fabs( this->y - point.y ));
}

//===========================================================================//
inline int TGS_Point_c::GetDz( 
      const TGS_Point_c& point ) const
{
   return( abs( this->z - point.z ));
}

//===========================================================================//
inline bool TGS_Point_c::IsLeft( 
      const TGS_Point_c& point ) const
{
   return( TCTF_IsLT( this->x, point.x ) && TCTF_IsEQ( this->y, point.y ) ?
           true : false );
}

//===========================================================================//
inline bool TGS_Point_c::IsRight( 
      const TGS_Point_c& point ) const
{
   return( TCTF_IsGT( this->x, point.x ) && TCTF_IsEQ( this->y, point.y ) ?
           true : false );
}

//===========================================================================//
inline bool TGS_Point_c::IsLower( 
      const TGS_Point_c& point ) const
{
   return( TCTF_IsEQ( this->x, point.x ) && TCTF_IsLT( this->y, point.y ) ?
           true : false );
}

//===========================================================================//
inline bool TGS_Point_c::IsUpper( 
      const TGS_Point_c& point ) const
{
   return( TCTF_IsEQ( this->x, point.x ) && TCTF_IsGT( this->y, point.y ) ?
           true : false );
}

//===========================================================================//
inline bool TGS_Point_c::IsLowerLeft( 
      const TGS_Point_c& point ) const
{
   return( TCTF_IsLT( this->x, point.x ) && TCTF_IsLT( this->y, point.y ) ?
           true : false );
}

//===========================================================================//
inline bool TGS_Point_c::IsLowerRight( 
      const TGS_Point_c& point ) const
{
   return( TCTF_IsGT( this->x, point.x ) && TCTF_IsLT( this->y, point.y ) ?
           true : false );
}

//===========================================================================//
inline bool TGS_Point_c::IsUpperLeft( 
      const TGS_Point_c& point ) const
{
   return( TCTF_IsLT( this->x, point.x ) && TCTF_IsGT( this->y, point.y ) ?
           true : false );
}

//===========================================================================//
inline bool TGS_Point_c::IsUpperRight( 
      const TGS_Point_c& point ) const
{
   return( TCTF_IsGT( this->x, point.x ) && TCTF_IsGT( this->y, point.y ) ?
           true : false );
}

#endif
