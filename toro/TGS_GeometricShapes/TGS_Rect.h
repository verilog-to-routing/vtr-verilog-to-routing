//===========================================================================//
// Purpose : Declaration and inline definitions for the TGS_Rect geometric 
//           shape 2D rectangle class.
//
//           Inline methods include:
//           - GetDx, GetDy
//           - FindMin, FindMax
//           - FindRatio
//           - FindArea
//           - FindOrient
//           - FindWidth
//           - FindLength
//           - HasArea
//           - IsWide, IsTall
//           - IsSquare
//
//===========================================================================//

#ifndef TGS_RECT_H
#define TGS_RECT_H

#include <cstdio>
#include <string>
using namespace std;

#include "TGS_Typedefs.h"
#include "TGS_Region.h"
#include "TGS_Point.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
class TGS_Rect_c
{
public:

   TGS_Rect_c( TGS_Layer_t layer = TGS_LAYER_UNDEFINED,
               double x1 = FLT_MIN, double y1 = FLT_MIN, 
               double x2 = FLT_MIN, double y2 = FLT_MIN,
               TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Rect_c( TGS_Layer_t layer, 
               const TGS_Region_c& region,
               TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Rect_c( TGS_Layer_t layer, 
               const TGS_Point_c& pointA, const TGS_Point_c& pointB,
               TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Rect_c( const TGS_Point_c& pointA, const TGS_Point_c& pointB,
               TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Rect_c( const TGS_Rect_c& rect,
               TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Rect_c( const TGS_Rect_c& rect,
               double scale,
               TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Rect_c( const TGS_Rect_c& rectA,
               const TGS_Rect_c& rectB );
   ~TGS_Rect_c( void );

   TGS_Rect_c& operator=( const TGS_Rect_c& rect );
   bool operator<( const TGS_Rect_c& rect ) const;
   bool operator==( const TGS_Rect_c& rect ) const;
   bool operator!=( const TGS_Rect_c& rect ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrRect,
                       size_t precision = SIZE_MAX ) const;

   void Set( TGS_Layer_t layer, 
             double x1, double y1, 
             double x2, double y2,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( TGS_Layer_t layer, 
             const TGS_Region_c& region,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( TGS_Layer_t layer, 
             const TGS_Point_c& pointA, 
             const TGS_Point_c& pointB,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( const TGS_Point_c& pointA, 
             const TGS_Point_c& pointB,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( const TGS_Rect_c& rect,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Reset( void );

   double GetDx( void ) const;
   double GetDy( void ) const;

   double FindMin( void ) const;
   double FindMax( void ) const;

   double FindRatio( void ) const;
   double FindArea( void ) const;

   TGS_Point_c FindCenter( TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED ) const;

   TGS_Point_c FindLowerLeft( void ) const;
   TGS_Point_c FindLowerRight( void ) const;
   TGS_Point_c FindUpperLeft( void ) const;
   TGS_Point_c FindUpperRight( void ) const;

   TGS_OrientMode_t FindOrient( TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED ) const;

   TGS_Point_c FindPoint( TGS_CornerMode_t corner ) const;

   void FindRect( TC_SideMode_t side, 
                  double minDistance,
                  TGS_Rect_c* pthisSideRect ) const;

   TC_SideMode_t FindSide( const TGS_Rect_c& refRect ) const;

   double FindWidth( TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED ) const;
   double FindLength( TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED ) const;

   double FindDistance( TGS_Layer_t refLayer ) const;
   double FindDistance( const TGS_Region_c& refRegion ) const;
   double FindDistance( const TGS_Point_c& refPoint ) const;
   double FindDistance( const TGS_Rect_c& refRect ) const;

   void FindFarthest( const TGS_Point_c& refPoint, 
                      TGS_Point_c* pthisFarthestPoint = 0 ) const;
   void FindFarthest( const TGS_Rect_c& refRect,
                      TGS_Point_c* prefNearthestPoint = 0, 
                      TGS_Point_c* pthisFarthestPoint = 0 ) const;

   void FindNearest( const TGS_Point_c& refPoint, 
                     TGS_Point_c* pthisNearestPoint = 0 ) const;
   void FindNearest( const TGS_Rect_c& refRect,
                     TGS_Point_c* prefNearestPoint = 0, 
                     TGS_Point_c* pthisNearestPoint = 0 ) const;

   void FindDifference( const TGS_Rect_c& rectA,
                        const TGS_Rect_c& rectB,
                        TGS_OrientMode_t orient,
                        TGS_RectList_t* pdifferenceList,
                        double minSpacing = 0.0,
                        double minWidth = 0.0,
                        double minArea = 0.0 ) const;
   void FindDifference( const TGS_Rect_c& rect,
                        TGS_OrientMode_t orient,
                        TGS_RectList_t* pdifferenceList,
                        double minSpacing = 0.0,
                        double minWidth = 0.0,
                        double minArea = 0.0 ) const;

   void ApplyScale( double dx, double dy,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ApplyScale( double scale,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ApplyScale( double delta,
                    TGS_OrientMode_t orient,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ApplyScale( double delta,
                    TC_SideMode_t side,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );

   void ApplyUnion( const TGS_Rect_c& rectA, 
                    const TGS_Rect_c& rectB );
   void ApplyUnion( const TGS_Rect_c& rect );

   void ApplyIntersect( const TGS_Rect_c& rectA, 
                        const TGS_Rect_c& rectB );
   void ApplyIntersect( const TGS_Rect_c& rect );

   void ApplyOverlap( const TGS_Rect_c& rectA,  
                      const TGS_Rect_c& rectB );
   void ApplyOverlap( const TGS_Rect_c& rect );

   void ApplyAdjacent( const TGS_Rect_c& rectA, 
                       const TGS_Rect_c& rectB,
                       double minDistance = 0.0 );
   void ApplyAdjacent( const TGS_Rect_c& rect,
                       double minDistance = 0.0 );

   void ApplyDifference( const TGS_Rect_c& rectA,
                         const TGS_Rect_c& rectB );
   void ApplyDifference( const TGS_Rect_c& rect );

   bool IsConnected( const TGS_Rect_c& rect ) const;
   bool IsAdjacent( const TGS_Rect_c& rect ) const;
   bool IsIntersecting( const TGS_Rect_c& rect ) const;
   bool IsOverlapping( const TGS_Rect_c& rect ) const;

   bool IsWithin( const TGS_Point_c& point ) const;
   bool IsWithin( const TGS_Rect_c& rect ) const;
   bool IsWithinDx( const TGS_Rect_c& rect ) const;
   bool IsWithinDy( const TGS_Rect_c& rect ) const;

   bool HasArea( void ) const;

   bool IsWide( void ) const;
   bool IsTall( void ) const;
   bool IsSquare( void ) const;

   bool IsLegal( void ) const;
   bool IsValid( void ) const;

public:

   TGS_Layer_t  layer;
   TGS_Region_c region;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
inline double TGS_Rect_c::GetDx( 
      void ) const
{
   return( this->region.GetDx( ));
}

//===========================================================================//
inline double TGS_Rect_c::GetDy( 
      void ) const
{
   return( this->region.GetDy( ));
}

//===========================================================================//
inline double TGS_Rect_c::FindMin( 
      void ) const
{
   return( this->region.FindMin( ));
}

//===========================================================================//
inline double TGS_Rect_c::FindMax( 
      void ) const
{
   return( this->region.FindMax( ));
}

//===========================================================================//
inline double TGS_Rect_c::FindRatio( 
      void ) const
{
   return( this->region.FindRatio( ));
}

//===========================================================================//
inline double TGS_Rect_c::FindArea( 
      void ) const
{
   return( this->region.FindArea( ));
}

//===========================================================================//
inline TGS_OrientMode_t TGS_Rect_c::FindOrient( 
      TGS_OrientMode_t orient ) const
{
   return( this->region.FindOrient( orient ));
}

//===========================================================================//
inline double TGS_Rect_c::FindWidth( 
      TGS_OrientMode_t orient ) const
{
   return( this->region.FindWidth( orient ));
}

//===========================================================================//
inline double TGS_Rect_c::FindLength( 
      TGS_OrientMode_t orient ) const
{
   return( this->region.FindLength( orient ));
}

//===========================================================================//
inline bool TGS_Rect_c::HasArea( 
      void ) const
{
   return( this->region.HasArea( ));
}

//===========================================================================//
inline bool TGS_Rect_c::IsWide( 
      void ) const
{
   return( this->region.IsWide( ));
}

//===========================================================================//
inline bool TGS_Rect_c::IsTall( 
      void ) const
{
   return( this->region.IsTall( ));
}

//===========================================================================//
inline bool TGS_Rect_c::IsSquare( 
      void ) const
{
   return( this->region.IsSquare( ));
}

#endif
