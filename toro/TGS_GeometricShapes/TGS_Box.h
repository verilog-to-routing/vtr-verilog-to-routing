//===========================================================================//
// Purpose : Declaration and inline definitions for a TGS_Box geometric shape
//           3D box class.
//
//           Inline methods include:
//           - GetDx, GetDy, GetDz
//
//===========================================================================//

#ifndef TGS_BOX_H
#define TGS_BOX_H

#include <cstdio>
#include <cstdlib>
#include <cmath>
using namespace std;

#include "TC_Typedefs.h"

#include "TGS_Typedefs.h"
#include "TGS_Point.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
class TGS_Box_c
{
public:

   TGS_Box_c( void );
   TGS_Box_c( double x1, double y1, int z1, double x2, double y2, int z2 );
   TGS_Box_c( double x1, double y1, int z1, double x2, double y2, int z2,
              TGS_SnapMode_t snap );
   TGS_Box_c( double x1, double y1, double x2, double y2 );
   TGS_Box_c( double x1, double y1, double x2, double y2,
              TGS_SnapMode_t snap );
   TGS_Box_c( const TGS_Point_c& pointA, 
              const TGS_Point_c& pointB );
   TGS_Box_c( const TGS_Point_c& pointA, 
              const TGS_Point_c& pointB,
              TGS_SnapMode_t snap );
   TGS_Box_c( const TGS_Box_c& box );
   TGS_Box_c( const TGS_Box_c& box,
              TGS_SnapMode_t snap );
   TGS_Box_c( const TGS_Box_c& boxA,
              const TGS_Box_c& boxB );
   ~TGS_Box_c( void );

   TGS_Box_c& operator=( const TGS_Box_c& box );
   bool operator<( const TGS_Box_c& box ) const;
   bool operator==( const TGS_Box_c& box ) const;
   bool operator!=( const TGS_Box_c& box ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrBox,
                       size_t precision = SIZE_MAX ) const;

   void Set( double x1, double y1, int z1, double x2, double y2, int z2,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( double x1, double y1, double x2, double y2, 
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( const TGS_Point_c& pointA, 
             const TGS_Point_c& pointB,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( const TGS_Box_c& box,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Reset( void );

   double GetDx( void ) const;
   double GetDy( void ) const;
   int GetDz( void ) const;

   double FindMin( void ) const;
   double FindMax( void ) const;

   double FindRatio( void ) const;
   double FindVolume( void ) const;

   TGS_Point_c FindCenter( TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED ) const;

   TGS_OrientMode_t FindOrient( TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED ) const;

   double FindWidth( TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED ) const;
   double FindLength( TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED ) const;

   double FindDistance( const TGS_Box_c& refBox ) const;
   double FindDistance( const TGS_Point_c& refPoint ) const;
   double FindDistance( const TGS_Box_c& refBox, 
                        TC_SideMode_t side ) const;
   double FindDistance( const TGS_Point_c& refPoint,
                        TGS_CornerMode_t corner ) const;
   
   void FindNearest( const TGS_Box_c& refBox,
                     TGS_Point_c* prefNearestPoint = 0, 
                     TGS_Point_c* pthisNearestPoint = 0 ) const;
   void FindNearest( const TGS_Point_c& refPoint, 
                     TGS_Point_c* pthisNearestPoint = 0 ) const;
   void FindFarthest( const TGS_Box_c& refBox,
                      TGS_Point_c* prefFarthestPoint = 0,
                      TGS_Point_c* pthisFarthestPoint = 0 ) const;
   void FindFarthest( const TGS_Point_c& refPoint, 
                      TGS_Point_c* pthisFarthestPoint = 0 ) const;

   void FindBox( TC_SideMode_t side, 
                 TGS_Box_c* pbox ) const;

   TC_SideMode_t FindSide( const TGS_Box_c& box ) const;

   void ApplyScale( double dx, double dy, int dz,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ApplyScale( double scale,
                    TC_SideMode_t side,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ApplyScale( double scale,
                    TGS_OrientMode_t orient,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ApplyScale( const TGS_Box_c& scale,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ApplyScale( const TGS_Scale_t& scale,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );

   void ApplyUnion( const TGS_Box_c& boxA, 
                    const TGS_Box_c& boxB );
   void ApplyUnion( const TGS_Box_c& box );
   void ApplyIntersect( const TGS_Box_c& boxA, 
                        const TGS_Box_c& boxB );
   void ApplyIntersect( const TGS_Box_c& box );
   void ApplyOverlap( const TGS_Box_c& boxA, 
                      const TGS_Box_c& boxB );
   void ApplyOverlap( const TGS_Box_c& box );
   void ApplyAdjacent( const TGS_Box_c& boxA, 
                       const TGS_Box_c& boxB,
                       double minDistance = 0.0 );
   void ApplyAdjacent( const TGS_Box_c& box,
                       double minDistance = 0.0 );

   bool IsIntersecting( const TGS_Box_c& box ) const;
   bool IsOverlapping( const TGS_Box_c& box ) const;
   bool IsAdjacent( const TGS_Box_c& box,
                    double minDistance = 0.0 ) const;

   bool IsWithin( const TGS_Box_c& box ) const;
   bool IsWithin( const TGS_Point_c& point ) const;

   bool IsWide( void ) const;
   bool IsTall( void ) const;
   bool IsSquare( void ) const;

   bool IsLegal( void ) const;
   bool IsValid( void ) const;

private:

   void FindNearestCoords_( double coord1i, 
                            double coord1j, 
                            double coord2i, 
                            double coord2j,
                            double* pnearestCoord1, 
                            double* pnearestCoord2 ) const;
   void FindNearestCoords_( int coord1i, 
                            int coord1j, 
                            int coord2i, 
                            int coord2j,
                            int* pnearestCoord1, 
                            int* pnearestCoord2 ) const;

   void FindFarthestCoords_( double coord1i, 
                             double coord1j, 
                             double coord2i, 
                             double coord2j,
                             double* pfarthestCoord1, 
                             double* pfarthestCoord2 ) const;
   void FindFarthestCoords_( int coord1i, 
                             int coord1j, 
                             int coord2i, 
                             int coord2j,
                             int* pfarthestCoord1, 
                             int* pfarthestCoord2 ) const;
public:

   TGS_Point_c lowerLeft;
   TGS_Point_c upperRight;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
inline double TGS_Box_c::GetDx( 
      void ) const
{
   return( fabs( this->lowerLeft.x - this->upperRight.x ));
}

//===========================================================================//
inline double TGS_Box_c::GetDy( 
      void ) const
{
   return( fabs( this->lowerLeft.y - this->upperRight.y ));
}

//===========================================================================//
inline int TGS_Box_c::GetDz( 
      void ) const
{
   return( abs( this->lowerLeft.z - this->upperRight.z ));
}

#endif
