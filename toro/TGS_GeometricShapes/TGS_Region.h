//===========================================================================//
// Purpose : Declaration and inline definitions for a TGS_Region geometric 
//           shape 2D region class.
//
//           Inline methods include:
//           - GetDx, GetDy
//           - HasArea
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

#ifndef TGS_REGION_H
#define TGS_REGION_H

#include <cstdio>
#include <cmath>
#include <string>
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
class TGS_Region_c
{
public:

   TGS_Region_c( void );
   TGS_Region_c( double x1, double y1, double x2, double y2,
                 TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Region_c( const TGS_Point_c& pointA, 
                 const TGS_Point_c& pointB,
                 TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Region_c( const TGS_Region_c& region,
                 TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Region_c( const TGS_Region_c& region,
                 double dx, double dy,
                 TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Region_c( const TGS_Region_c& region,
                 double scale,
                 TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Region_c( const TGS_Point_c& pointA, 
                 const TGS_Point_c& pointB,
                 double scale,
                 TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Region_c( const TGS_Region_c& regionA,
                 const TGS_Region_c& regionB );
   ~TGS_Region_c( void );

   TGS_Region_c& operator=( const TGS_Region_c& region );
   bool operator<( const TGS_Region_c& region ) const;
   bool operator==( const TGS_Region_c& region ) const;
   bool operator!=( const TGS_Region_c& region ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrRegion,
                       size_t precision = SIZE_MAX ) const;

   void Set( double x1, double y1, double x2, double y2,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( const TGS_Point_c& pointA, 
             const TGS_Point_c& pointB,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( const TGS_Region_c& region,
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

   void FindRegion( TC_SideMode_t side, 
                    TGS_Region_c* pthisSideRegion ) const;
   void FindRegion( TC_SideMode_t side, 
                    double minDistance,
                    TGS_Region_c* pthisSideRegion ) const;
   void FindRegion( TC_SideMode_t side, 
                    double minDistance,
                    double maxDistance,
                    TGS_Region_c* pthisSideRegion ) const;

   TC_SideMode_t FindSide( const TGS_Region_c& refRegion ) const;
   TC_SideMode_t FindSide( const TGS_Point_c& refPoint ) const;

   double FindWidth( TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED ) const;
   double FindLength( TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED ) const;

   double FindDistance( const TGS_Region_c& refRegion ) const;
   double FindDistance( const TGS_Point_c& refPoint ) const;
   double FindDistance( const TGS_Region_c& refRegion, 
                        TC_SideMode_t side ) const;
   double FindDistance( const TGS_Point_c& refPoint,
                        TGS_CornerMode_t corner ) const;
   
   void FindNearest( const TGS_Point_c& refPoint, 
                     TGS_Point_c* pthisNearestPoint = 0 ) const;
   void FindNearest( const TGS_Region_c& refRegion,
                     TGS_Point_c* prefNearestPoint = 0, 
                     TGS_Point_c* pthisNearestPoint = 0 ) const;

   void FindDifference( const TGS_Region_c& regionA,
                        const TGS_Region_c& regionB,
                        TGS_OrientMode_t orient,
                        TGS_RegionList_t* pdifferenceList,
                        double minSpacing = 0.0,
                        double minWidth = 0.0,
                        double minArea = 0.0 ) const;
   void FindDifference( const TGS_Region_c& region,
                        TGS_OrientMode_t orient,
                        TGS_RegionList_t* pdifferenceList,
                        double minSpacing = 0.0,
                        double minWidth = 0.0,
                        double minArea = 0.0 ) const;

   void ApplyScale( double dx, double dy,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ApplyScale( double scale,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ApplyScale( double scale,
                    TC_SideMode_t side,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ApplyScale( double scale,
                    TGS_OrientMode_t orient,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ApplyScale( const TGS_Region_c& scale,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ApplyScale( const TGS_Scale_t& scale,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );

   void ApplyUnion( const TGS_Region_c& regionA, 
                    const TGS_Region_c& regionB );
   void ApplyUnion( const TGS_Region_c& region );
   void ApplyIntersect( const TGS_Region_c& regionA, 
                        const TGS_Region_c& regionB );
   void ApplyIntersect( const TGS_Region_c& region );
   void ApplyAdjacent( const TGS_Region_c& regionA, 
                       const TGS_Region_c& regionB,
                       double minDistance = 0.0 );
   void ApplyAdjacent( const TGS_Region_c& region,
                       double minDistance = 0.0 );

   void ApplyDifference( const TGS_Region_c& regionA,
                         const TGS_Region_c& regionB,
                         TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED );
   void ApplyDifference( const TGS_Region_c& region,
                         TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED );

   void ApplyShift( double dx, double dy,
                    TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED );
   void ApplyShift( double shift,
                    TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED );

   bool IsIntersecting( const TGS_Region_c& region,
                        double minArea = TC_FLT_MAX ) const;
   bool IsIntersecting( const TGS_Region_c& region,
                        TC_SideMode_t side ) const;
   bool IsOverlapping( const TGS_Region_c& region,
                       double minDistance = 0.0 ) const;
   bool IsAdjacent( const TGS_Region_c& region,
                    double minDistance = 0.0 ) const;

   bool IsWithin( const TGS_Region_c& region ) const;
   bool IsWithin( const TGS_Region_c& region,
                  double minDistance ) const;
   bool IsWithin( const TGS_Point_c& point ) const;
   bool IsWithin( const TGS_Point_c& point,
                  double minDistance ) const;
   bool IsWithinDx( const TGS_Region_c& region ) const;
   bool IsWithinDy( const TGS_Region_c& region ) const;

   bool IsCorner( const TGS_Region_c& region ) const;
   bool IsCrossed( const TGS_Region_c& region,
                   double minDistance = 0.0 ) const;
   bool IsCrossed( const TGS_Region_c& region,
                   TGS_OrientMode_t orient,
                   double minDistance = 0.0 ) const;

   bool HasArea( void ) const;

   bool IsWide( void ) const;
   bool IsTall( void ) const;
   bool IsSquare( void ) const;

   bool IsLegal( void ) const;
   bool IsValid( void ) const;

public:

   double x1;
   double y1;
   double x2;
   double y2;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
inline double TGS_Region_c::GetDx( 
      void ) const
{
   return( fabs( this->x1 - this->x2 ));
}

//===========================================================================//
inline double TGS_Region_c::GetDy( 
      void ) const
{
   return( fabs( this->y1 - this->y2 ));
}

//===========================================================================//
inline bool TGS_Region_c::HasArea( 
      void ) const
{
   return( TCTF_IsGT( this->FindArea( ), 0.0 ) ? true : false );
}

#endif
