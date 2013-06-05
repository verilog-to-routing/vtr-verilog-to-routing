//===========================================================================//
// Purpose : Declaration and inline definitions for a TGS_Path geometric 
//           shape 2D path class.
//
//           Inline methods include:
//           - GetBeginPoint
//           - GetEndPoint
//           - ExtendLength
//           - FindLength
//           - FindOrient
//           - FindDistance
//           - ApplyScale
//           - IsConnected
//           - IsIntersecting
//           - IsOverlapping
//           - IsParallel
//           - IsExtension
//           - IsHorizontal
//           - IsVertical
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

#ifndef TGS_PATH_H
#define TGS_PATH_H

#include <cstdio>
using namespace std;

#include "TGS_Typedefs.h"
#include "TGS_Point.h"
#include "TGS_Line.h"
#include "TGS_Region.h"
#include "TGS_Rect.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
class TGS_Path_c
{
public:

   TGS_Path_c( void );
   TGS_Path_c( double x1, double y1, double x2, double y2, int z,
               double width,
               TGS_SnapMode_t snap );
   TGS_Path_c( const TGS_Point_c& beginPoint, 
               const TGS_Point_c& endPoint,
               double width,
               TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Path_c( const TGS_Line_c& line, 
               double width,
               TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Path_c( const TGS_Region_c& region, 
               TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Path_c( const TGS_Rect_c& rect,
               TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   TGS_Path_c( const TGS_Path_c& path,
               TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   ~TGS_Path_c( void );

   TGS_Path_c& operator=( const TGS_Path_c& path );
   bool operator==( const TGS_Path_c& path ) const;
   bool operator!=( const TGS_Path_c& path ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrLine,
                       size_t precision = SIZE_MAX ) const;

   void Set( double x1, double y1, double x2, double y2, int z,
             double width,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( const TGS_Point_c& beginPoint, 
             const TGS_Point_c& endPoint,
             double width,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( const TGS_Line_c& line, 
             double width,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( const TGS_Region_c& region,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( const TGS_Rect_c& rect,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Set( const TGS_Path_c& path,
             TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void Reset( void );

   TGS_Point_c GetBeginPoint( void ) const;
   TGS_Point_c GetEndPoint( void ) const;

   void ExtendLength( double length,
                      TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ExtendLength( double length,
                      TGS_OrientMode_t orient,
                      TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ExtendLength( double begin, double end,
                      TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ExtendLength( double begin, double end,
                      TGS_OrientMode_t orient,
                      TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );
   void ExtendLength( const TGS_Path_c& path,
                      TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED );

   double FindLength( void ) const;

   TGS_OrientMode_t FindOrient( void ) const;

   double FindDistance( const TGS_Path_c& refPath ) const;

   void FindNearest( const TGS_Point_c& refPoint, 
                     TGS_Point_c* pthisNearestPoint = 0 ) const;
   void FindNearest( const TGS_Path_c& refPath,
                     TGS_Point_c* prefNearestPoint = 0, 
                     TGS_Point_c* pthisNearestPoint = 0 ) const;

   void FindIntersect( const TGS_Path_c& refPath, 
                       TGS_Point_c* pthisIntersectPoint = 0 ) const;

   void FindRegion( TGS_Region_c* pregion,
                    TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED ) const;
   void FindRect( TGS_Rect_c* prect,
                  TGS_SnapMode_t snap = TGS_SNAP_UNDEFINED ) const;

   void ApplySnap( TGS_SnapMode_t snap );

   void ApplyUnion( const TGS_Path_c& path );
   void ApplyIntersect( const TGS_Region_c& region );

   void ApplyScale( double delta );

   bool IsConnected( const TGS_Path_c& path ) const;

   bool IsIntersecting( const TGS_Path_c& path ) const;
   bool IsIntersecting( const TGS_Point_c& point ) const;

   bool IsOverlapping( const TGS_Path_c& path ) const;
   bool IsOverlapping( const TGS_Point_c& point ) const;

   bool IsParallel( const TGS_Path_c& path ) const;
   bool IsExtension( const TGS_Path_c& path ) const;

   bool IsHorizontal( void ) const;
   bool IsVertical( void ) const;

   bool IsValid( void ) const;

public:

   TGS_Line_c line;
   double width;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
inline TGS_Point_c TGS_Path_c::GetBeginPoint( 
      void ) const
{
   return( this->line.GetBeginPoint( ));
}

//===========================================================================//
inline TGS_Point_c TGS_Path_c::GetEndPoint( 
      void ) const
{
   return( this->line.GetEndPoint( ));
}

//===========================================================================//
inline void TGS_Path_c::ExtendLength( 
      double         length,
      TGS_SnapMode_t snap )
{
   this->line.ExtendLength( length, snap );
}

//===========================================================================//
inline void TGS_Path_c::ExtendLength( 
      double           length,
      TGS_OrientMode_t orient,
      TGS_SnapMode_t   snap )
{
   this->line.ExtendLength( length, orient, snap );
}

//===========================================================================//
inline void TGS_Path_c::ExtendLength( 
      double         begin,
      double         end,
      TGS_SnapMode_t snap )
{
   this->line.ExtendLength( begin, end, snap );
}

//===========================================================================//
inline void TGS_Path_c::ExtendLength( 
      double           begin,
      double           end,
      TGS_OrientMode_t orient,
      TGS_SnapMode_t   snap )
{
   this->line.ExtendLength( begin, end, orient, snap );
}

//===========================================================================//
inline void TGS_Path_c::ExtendLength( 
      const TGS_Path_c& path,
      TGS_SnapMode_t    snap )
{
   this->line.ExtendLength( path.line, snap );
}

//===========================================================================//
inline double TGS_Path_c::FindLength( 
      void ) const
{
   return( this->line.FindLength( ));
}

//===========================================================================//
inline TGS_OrientMode_t TGS_Path_c::FindOrient( 
      void ) const
{
   return( this->line.FindOrient( ));
}

//===========================================================================//
inline double TGS_Path_c::FindDistance( 
      const TGS_Path_c& refPath ) const
{
   return( this->line.FindDistance( refPath.line ));
}

//===========================================================================//
inline void TGS_Path_c::ApplyScale(
      double delta )
{
   this->line.ApplyScale( delta );
}

//===========================================================================//
inline bool TGS_Path_c::IsConnected( 
      const TGS_Path_c& path ) const
{
   return( this->line.IsConnected( path.line ));
}

//===========================================================================//
inline bool TGS_Path_c::IsIntersecting( 
      const TGS_Path_c& path ) const
{
   return( this->line.IsIntersecting( path.line ));
}

//===========================================================================//
inline bool TGS_Path_c::IsIntersecting( 
      const TGS_Point_c& point ) const
{
   return( this->line.IsIntersecting( point ));
}

//===========================================================================//
inline bool TGS_Path_c::IsOverlapping( 
      const TGS_Path_c& path ) const
{
   return( this->line.IsOverlapping( path.line ));
}

//===========================================================================//
inline bool TGS_Path_c::IsOverlapping( 
      const TGS_Point_c& point ) const
{
   return( this->line.IsOverlapping( point ));
}

//===========================================================================//
inline bool TGS_Path_c::IsParallel( 
      const TGS_Path_c& path ) const
{
   return( this->line.IsParallel( path.line ));
}

//===========================================================================//
inline bool TGS_Path_c::IsExtension( 
      const TGS_Path_c& path ) const
{
   return( this->line.IsExtension( path.line ));
}

//===========================================================================//
inline bool TGS_Path_c::IsHorizontal( 
      void ) const
{
   return( this->line.IsHorizontal( ));
}

//===========================================================================//
inline bool TGS_Path_c::IsVertical( 
      void ) const
{
   return( this->line.IsVertical( ));
}

#endif
