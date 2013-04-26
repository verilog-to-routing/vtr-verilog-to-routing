//===========================================================================//
// Purpose : Method definitions for the TGS_Path class.
//
//           Public methods include:
//           - TGS_Path_c, ~TGS_Path_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set, Reset
//           - FindNearest
//           - FindIntersect
//           - FindRegion
//           - FindRect
//           - ApplySnap
//           - ApplyUnion
//           - ApplyIntersect
//           - IsValid
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

#if defined( SUN8 ) || defined( SUN10 )
   #include <math.h>
#endif

#include "TC_MinGrid.h"

#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TGS_Path.h"

//===========================================================================//
// Method         : TGS_Path_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
TGS_Path_c::TGS_Path_c( 
      void )
      :
      width( 0.0 )
{
}

//===========================================================================//
TGS_Path_c::TGS_Path_c( 
      double         x1, 
      double         y1,
      double         x2, 
      double         y2,
      int            z,
      double         width_,
      TGS_SnapMode_t snap )
{
   this->Set( x1, y1, x2, y2, z, width_, snap );
}

//===========================================================================//
TGS_Path_c::TGS_Path_c( 
      const TGS_Point_c&   beginPoint,
      const TGS_Point_c&   endPoint,
            double         width_,
            TGS_SnapMode_t snap )
{
   this->Set( beginPoint, endPoint, width_, snap );
}

//===========================================================================//
TGS_Path_c::TGS_Path_c( 
      const TGS_Line_c&    line_,
            double         width_,
            TGS_SnapMode_t snap )
{
   this->Set( line_, width_, snap );
}

//===========================================================================//
TGS_Path_c::TGS_Path_c( 
      const TGS_Region_c&  region,
            TGS_SnapMode_t snap )
{
   this->Set( region, snap );
}

//===========================================================================//
TGS_Path_c::TGS_Path_c( 
      const TGS_Rect_c&    rect,
            TGS_SnapMode_t snap )
{
   this->Set( rect, snap );
}

//===========================================================================//
TGS_Path_c::TGS_Path_c( 
      const TGS_Path_c&    path,
            TGS_SnapMode_t snap )
      :
      line( path.line ),
      width( path.width )
{
   this->ApplySnap( snap );
}

//===========================================================================//
// Method         : ~TGS_Path_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
TGS_Path_c::~TGS_Path_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
TGS_Path_c& TGS_Path_c::operator=( 
      const TGS_Path_c& path )
{
   if( &path != this )
   {
      this->line = path.line;
      this->width = path.width;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
bool TGS_Path_c::operator==( 
      const TGS_Path_c& path ) const
{
   return(( this->line == path.line ) &&
          ( TCTF_IsEQ( this->width, path.width )) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
bool TGS_Path_c::operator!=( 
      const TGS_Path_c& path ) const
{
   return(( !this->operator==( path )) ? 
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
void TGS_Path_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srPath;
   this->ExtractString( &srPath );

   printHandler.Write( pfile, spaceLen, "[path] %s\n", TIO_SR_STR( srPath ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
void TGS_Path_c::ExtractString( 
      string* psrPath,
      size_t  precision ) const
{
   if( psrPath )
   {
      char szPath[TIO_FORMAT_STRING_LEN_PATH];

      if( precision == SIZE_MAX )
      {
         precision = TC_MinGrid_c::GetInstance( ).GetPrecision( );
      }

      if( this->IsValid( ))
      {
         if( this->line.z != INT_MIN )
         {
            sprintf( szPath, "%0.*f %0.*f %0.*f %0.*f %d %0.*f",
                             static_cast< int >( precision ), this->line.x1,
                             static_cast< int >( precision ), this->line.y1,
                             static_cast< int >( precision ), this->line.x2,
                             static_cast< int >( precision ), this->line.y2,
                             this->line.z,
                             static_cast< int >( precision ), this->width );
         }
         else
         {
            sprintf( szPath, "%0.*f %0.*f %0.*f %0.*f %0.*f",
                             static_cast< int >( precision ), this->line.x1,
                             static_cast< int >( precision ), this->line.y1,
                             static_cast< int >( precision ), this->line.x2,
                             static_cast< int >( precision ), this->line.y2,
                             static_cast< int >( precision ), this->width );
         }
      }
      else
      {
         sprintf( szPath, "? ? ? ? ?" );
      }
      *psrPath = szPath;
   }
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
void TGS_Path_c::Set( 
      double         x1, 
      double         y1,
      double         x2, 
      double         y2,
      int            z,
      double         width_,
      TGS_SnapMode_t snap )
{
   this->line.Set( x1, y1, x2, y2, z );
   this->width = width_;

   this->ApplySnap( snap );
}

//===========================================================================//
void TGS_Path_c::Set(
      const TGS_Point_c&   beginPoint,
      const TGS_Point_c&   endPoint,
            double         width_,
            TGS_SnapMode_t snap )
{
   this->line.Set( beginPoint, endPoint );
   this->width = width_;

   this->ApplySnap( snap );
}

//===========================================================================//
void TGS_Path_c::Set(
      const TGS_Line_c&    line_,
            double         width_,
            TGS_SnapMode_t snap )
{
   this->line.Set( line_ );
   this->width = width_;

   this->ApplySnap( snap );
}

//===========================================================================//
void TGS_Path_c::Set( 
      const TGS_Region_c&  region,
            TGS_SnapMode_t snap )
{
   if( region.IsWide( ))
   {
      this->width = region.GetDy( );
      this->line.Set( region.x1, region.y1 + this->width / 2.0,
                      region.x2, region.y2 - this->width / 2.0 );
   }
   else // if( region.IsTall( ))
   {
      this->width = region.GetDx( );
      this->line.Set( region.x1 + this->width / 2.0, region.y1,
                      region.x2 - this->width / 2.0, region.y2 );
   }
   this->ApplySnap( snap );
}

//===========================================================================//
void TGS_Path_c::Set( 
      const TGS_Rect_c&    rect,
            TGS_SnapMode_t snap )
{
   this->Set( rect.region, snap );
   this->line.z = rect.layer;
}

//===========================================================================//
void TGS_Path_c::Set( 
      const TGS_Path_c&    path,
            TGS_SnapMode_t snap )
{
   this->Set( path.line, path.width, snap );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
void TGS_Path_c::Reset( 
      void )
{
   this->line.Reset( );
   this->width = 0.0;
}

//===========================================================================//
// Method         : FindNearest
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
void TGS_Path_c::FindNearest( 
      const TGS_Point_c& refPoint,
            TGS_Point_c* pthisNearestPoint ) const
{
   this->line.FindNearest( refPoint, pthisNearestPoint );

   if( pthisNearestPoint && pthisNearestPoint->IsValid( ))
   {
      pthisNearestPoint->z = this->line.z;
   }
}

//===========================================================================//
void TGS_Path_c::FindNearest( 
      const TGS_Path_c&  refPath,
            TGS_Point_c* prefNearestPoint, 
            TGS_Point_c* pthisNearestPoint ) const
{
   this->line.FindNearest( refPath.line, prefNearestPoint, pthisNearestPoint );

   if( prefNearestPoint && prefNearestPoint->IsValid( ))
   {
      prefNearestPoint->z = refPath.line.z;
   }
   if( pthisNearestPoint && pthisNearestPoint->IsValid( ))
   {
      pthisNearestPoint->z = this->line.z;
   }
}

//===========================================================================//
// Method         : FindIntersect
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
void TGS_Path_c::FindIntersect( 
      const TGS_Path_c&  refPath,
            TGS_Point_c* pthisIntersectPoint ) const
{
   this->line.FindIntersect( refPath.line, pthisIntersectPoint );

   if( pthisIntersectPoint && pthisIntersectPoint->IsValid( ))
   {
      pthisIntersectPoint->z = this->line.z;
   }
}

//===========================================================================//
// Method         : FindRegion
// Purpose        : Compute and return the bounding region for this path.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
void TGS_Path_c::FindRegion( 
      TGS_Region_c*  pregion,
      TGS_SnapMode_t snap ) const
{
   if( pregion )
   {
      TGS_Region_c region( this->line.x1, this->line.y1, 
                           this->line.x2, this->line.y2 );

      double dwidth = this->width / 2.0;
      if( this->line.IsHorizontal( ))
      {
         region.y1 -= dwidth;
         region.y2 += dwidth;
      }
      else if( this->line.IsVertical( ))
      {
         region.x1 -= dwidth;
         region.x2 += dwidth;
      }
      else
      {
         region.x1 -= dwidth;
         region.y1 -= dwidth;
         region.x2 += dwidth;
         region.y2 += dwidth;
      }
      pregion->Set( region, snap );
   }
}

//===========================================================================//
// Method         : FindRect
// Purpose        : Compute and return the bounding rectangle for this path.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
void TGS_Path_c::FindRect( 
      TGS_Rect_c*    prect,
      TGS_SnapMode_t snap ) const
{
   if( prect )
   {
      this->FindRegion( &prect->region );
      prect->layer = this->line.z;

      if( snap == TGS_SNAP_MIN_GRID )
      {
         TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
         if( !MinGrid.IsOnGrid( prect->region.x1 ) ||
             !MinGrid.IsOnGrid( prect->region.y1 ) ||
             !MinGrid.IsOnGrid( prect->region.y2 ) ||
             !MinGrid.IsOnGrid( prect->region.x2 ))
         {
            TGS_Path_c thisPath( *this );
            thisPath.width -= MinGrid.GetGrid( );
            thisPath.FindRegion( &prect->region, snap );
         }
      }
   }
}

//===========================================================================//
// Method         : ApplySnap
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/12/09 jeffr : Original
//===========================================================================//
void TGS_Path_c::ApplySnap( 
      TGS_SnapMode_t snap )
{
   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      this->line.x1 = MinGrid.SnapToGrid( this->line.x1 );
      this->line.y1 = MinGrid.SnapToGrid( this->line.y1 );
      this->line.x2 = MinGrid.SnapToGrid( this->line.x2 );
      this->line.y2 = MinGrid.SnapToGrid( this->line.y2 );
   }
}

//===========================================================================//
// Method         : ApplyUnion
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
void TGS_Path_c::ApplyUnion( 
      const TGS_Path_c& path )
{
   this->line.ApplyUnion( path.line );
}

//===========================================================================//
// Method         : ApplyIntersect
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
void TGS_Path_c::ApplyIntersect( 
      const TGS_Region_c& region )
{
   if( this->line.IsHorizontal( ))
   {
      this->line.ApplyIntersect( region.x1, region.x2 );
   }
   else if( this->line.IsVertical( ))
   {
      this->line.ApplyIntersect( region.y1, region.y2 );
   }
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 09/10/12 jeffr : Original
//===========================================================================//
bool TGS_Path_c::IsValid( 
      void ) const
{
   return(( this->line.IsValid( )) &&
          ( TCTF_IsGT( this->width, static_cast< double >( 0.0 ))) ?
          true : false );
}
