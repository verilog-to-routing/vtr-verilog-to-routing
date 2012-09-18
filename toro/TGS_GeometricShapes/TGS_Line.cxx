//===========================================================================//
// Purpose : Method definitions for the TGS_Line class.
//
//           Public methods include:
//           - TGS_Line_c, ~TGS_Line_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set, Reset
//           - GetBeginPoint, GetEndPoint
//           - ExtendLength
//           - ReduceLength
//           - FindLength
//           - FindCenter
//           - FindOrient
//           - FindDistance
//           - FindNearest
//           - FindIntersect
//           - ApplyScale
//           - ApplyUnion
//           - ApplyIntersect
//           - ApplyNormalize
//           - CrossProduct
//           - IsConnected
//           - IsIntersecting
//           - IsOverlapping
//           - IsWithin
//           - IsParallel
//           - IsExtension
//           - IsLeft, IsRight, IsLower, IsUpper
//           - IsLowerLeft, IsLowerRight, IsUpperLeft, IsUpperRight
//           - IsOrthogonal
//           - IsHorizontal, IsVertical
//           - IsValid
//
//===========================================================================//

#if defined( SUN8 ) || defined( SUN10 )
   #include <math.h>
#endif

#include "TC_MinGrid.h"

#include "TIO_PrintHandler.h"

#include "TGS_Box.h"
#include "TGS_Region.h"
#include "TGS_Line.h"

//===========================================================================//
// Method         : TGS_Line_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Line_c::TGS_Line_c( 
      void )
      :
      x1( TC_FLT_MIN ),
      y1( TC_FLT_MIN ),
      x2( TC_FLT_MIN ),
      y2( TC_FLT_MIN ),
      z( INT_MIN )
{
}

//===========================================================================//
TGS_Line_c::TGS_Line_c( 
      double         x1_, 
      double         y1_,
      double         x2_, 
      double         y2_,
      int            z_,
      TGS_SnapMode_t snap )
{
   this->Set( x1_, y1_, x2_, y2_, z_, snap );
}

//===========================================================================//
TGS_Line_c::TGS_Line_c( 
      double         x1_, 
      double         y1_,
      double         x2_, 
      double         y2_,
      TGS_SnapMode_t snap )
{
   this->Set( x1_, y1_, x2_, y2_, snap );
}

//===========================================================================//
TGS_Line_c::TGS_Line_c( 
      const TGS_Point_c&   beginPoint,
      const TGS_Point_c&   endPoint,
            int            z_,
            TGS_SnapMode_t snap )
{
   this->Set( beginPoint, endPoint, z_, snap );
}

//===========================================================================//
TGS_Line_c::TGS_Line_c( 
      const TGS_Point_c&   beginPoint,
      const TGS_Point_c&   endPoint,
            TGS_SnapMode_t snap )
{
   this->Set( beginPoint, endPoint, snap );
}

//===========================================================================//
TGS_Line_c::TGS_Line_c( 
      const TGS_Line_c&    line,
            TGS_SnapMode_t snap )
{
   this->Set( line, snap );
}

//===========================================================================//
// Method         : ~TGS_Line_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Line_c::~TGS_Line_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Line_c& TGS_Line_c::operator=( 
      const TGS_Line_c& line )
{
   if( &line != this )
   {
      this->x1 = line.x1;
      this->y1 = line.y1;
      this->x2 = line.x2;
      this->y2 = line.y2;
      this->z = line.z;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::operator==( 
      const TGS_Line_c& line ) const
{
   return(( TCTF_IsEQ( this->x1, line.x1 )) &&
          ( TCTF_IsEQ( this->y1, line.y1 )) &&
          ( TCTF_IsEQ( this->x2, line.x2 )) &&
          ( TCTF_IsEQ( this->y2, line.y2 )) &&
          ( this->z == line.z ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::operator!=( 
      const TGS_Line_c& line ) const
{
   return(( !this->operator==( line )) ? 
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Line_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srLine;
   this->ExtractString( &srLine );

   printHandler.Write( pfile, spaceLen, "[line] %s", TIO_SR_STR( srLine ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Line_c::ExtractString( 
      string* psrLine,
      size_t  precision ) const
{
   if( psrLine )
   {
      char szLine[TIO_FORMAT_STRING_LEN_LINE];

      if( precision == SIZE_MAX )
      {
         precision = TC_MinGrid_c::GetInstance( ).GetPrecision( );
      }

      if( this->IsValid( ))
      {
	 if( this->z != INT_MIN )
         {
            sprintf( szLine, "%0.*f %0.*f %0.*f %0.*f %d",
                             static_cast< int >( precision ), this->x1,
                             static_cast< int >( precision ), this->y1,
                             static_cast< int >( precision ), this->x2,
	                     static_cast< int >( precision ), this->y2,
                             this->z );
         }
	 else
         {
            sprintf( szLine, "%0.*f %0.*f %0.*f %0.*f",
                             static_cast< int >( precision ), this->x1,
                             static_cast< int >( precision ), this->y1,
                             static_cast< int >( precision ), this->x2,
	                     static_cast< int >( precision ), this->y2 );
         }
      }
      else
      {
         sprintf( szLine, "? ? ? ? ?" );
      }
      *psrLine = szLine;
   }
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Line_c::Set( 
      double         x1_, 
      double         y1_,
      double         x2_, 
      double         y2_,
      int            z_,
      TGS_SnapMode_t snap )
{
   this->x1 = x1_;
   this->y1 = y1_;
   this->x2 = x2_;
   this->y2 = y2_;
   this->z = z_;

   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      this->x1 = MinGrid.SnapToGrid( this->x1 );
      this->y1 = MinGrid.SnapToGrid( this->y1 );
      this->x2 = MinGrid.SnapToGrid( this->x2 );
      this->y2 = MinGrid.SnapToGrid( this->y2 );
   }
}

//===========================================================================//
void TGS_Line_c::Set( 
      double         x1_, 
      double         y1_,
      double         x2_, 
      double         y2_,
      TGS_SnapMode_t snap )
{
   this->Set( x1_, y1_, x2_, y2_, INT_MIN, snap );
}

//===========================================================================//
void TGS_Line_c::Set(
      const TGS_Point_c&   beginPoint,
      const TGS_Point_c&   endPoint,
            int            z_,
            TGS_SnapMode_t snap )
{
   this->Set( beginPoint.x, beginPoint.y, endPoint.x, endPoint.y, z_, snap );
}

//===========================================================================//
void TGS_Line_c::Set(
      const TGS_Point_c&   beginPoint,
      const TGS_Point_c&   endPoint,
            TGS_SnapMode_t snap )
{
   int z_ = TCT_Min( beginPoint.z, endPoint.z );
   this->Set( beginPoint.x, beginPoint.y, endPoint.x, endPoint.y, z_, snap );
}

//===========================================================================//
void TGS_Line_c::Set( 
      const TGS_Line_c&    line,
            TGS_SnapMode_t snap )
{
   this->Set( line.x1, line.y1, line.x2, line.y2, line.z, snap );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Line_c::Reset( 
      void )
{
   this->Set( TC_FLT_MIN, TC_FLT_MIN, TC_FLT_MIN, TC_FLT_MIN, INT_MIN );
}

//===========================================================================//
// Method         : GetBeginPoint
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Line_c::GetBeginPoint( 
      TGS_SnapMode_t snap ) const
{
   TGS_Point_c beginPoint( this->x1, this->y1, this->z );

   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      beginPoint.x = MinGrid.SnapToGrid( beginPoint.x );
      beginPoint.y = MinGrid.SnapToGrid( beginPoint.y );
   }
   return( beginPoint );
}

//===========================================================================//
// Method         : GetEndPoint
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Line_c::GetEndPoint( 
      TGS_SnapMode_t snap ) const
{
   TGS_Point_c endPoint( this->x2, this->y2, this->z );

   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      endPoint.x = MinGrid.SnapToGrid( endPoint.x );
      endPoint.y = MinGrid.SnapToGrid( endPoint.y );
   }
   return( endPoint );
}

//===========================================================================//
// Method         : ExtendLength
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Line_c::ExtendLength( 
      double         length,
      TGS_SnapMode_t snap )
{
   if( this->IsHorizontal( ))
   {
      this->ExtendLength( length, TGS_ORIENT_HORIZONTAL, snap );
   }   
   else if( this->IsVertical( ))
   {
      this->ExtendLength( length, TGS_ORIENT_VERTICAL, snap );
   }
}

//===========================================================================//
void TGS_Line_c::ExtendLength( 
      double           length,
      TGS_OrientMode_t orient,
      TGS_SnapMode_t   snap )
{
   if( orient == TGS_ORIENT_HORIZONTAL )
   {
      if( this->IsLeft( ))
      {
	 if( TCTF_IsGE( length, 0.0 ))
         {
            this->x2 += length;
         }   
	 else // if( TCTF_IsLT( length, 0.0 ))
         {
            this->x1 += length;
         }   
      }   
      else // if( this->IsRight( ))
      {
	 if( TCTF_IsGE( length, 0.0 ))
         {
            this->x2 -= length;
         }   
	 else // if( TCTF_IsLT( length, 0.0 ))
         {
            this->x1 -= length;
         }   
      }   
   }   
   else if( orient == TGS_ORIENT_VERTICAL )
   {
      if( this->IsLower( ))
      {
	 if( TCTF_IsGE( length, 0.0 ))
         {
            this->y2 += length;
         }   
	 else // if( TCTF_IsLT( length, 0.0 ))
         {
            this->y1 += length;
         }   
      }   
      else // if( this->IsUpper( ))
      {
	 if( TCTF_IsGE( length, 0.0 ))
         {
            this->y2 -= length;
         }   
	 else // if( TCTF_IsLT( length, 0.0 ))
         {
            this->y1 -= length;
         }   
      }   
   }

   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      this->x1 = MinGrid.SnapToGrid( this->x1 );
      this->y1 = MinGrid.SnapToGrid( this->y1 );
      this->x2 = MinGrid.SnapToGrid( this->x2 );
      this->y2 = MinGrid.SnapToGrid( this->y2 );
   }
}

//===========================================================================//
void TGS_Line_c::ExtendLength( 
      double         begin,
      double         end,
      TGS_SnapMode_t snap )
{
   this->ExtendLength( begin, snap );
   this->ExtendLength( end, snap );
}

//===========================================================================//
void TGS_Line_c::ExtendLength( 
      double           begin,
      double           end,
      TGS_OrientMode_t orient,
      TGS_SnapMode_t   snap )
{
   this->ExtendLength( begin, orient, snap );
   this->ExtendLength( end, orient, snap );
}

//===========================================================================//
void TGS_Line_c::ExtendLength( 
      const TGS_Line_c&    line,
            TGS_SnapMode_t snap )
{
   this->x2 = line.x2;
   this->y2 = line.y2;

   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      this->x1 = MinGrid.SnapToGrid( this->x1 );
      this->y1 = MinGrid.SnapToGrid( this->y1 );
      this->x2 = MinGrid.SnapToGrid( this->x2 );
      this->y2 = MinGrid.SnapToGrid( this->y2 );
   }
}

//===========================================================================//
// Method         : ReduceLength
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Line_c::ReduceLength( 
      double         begin,
      double         end,
      TGS_SnapMode_t snap )
{
   if( this->IsHorizontal( ))
   {
      this->ReduceLength( begin, end, TGS_ORIENT_HORIZONTAL, snap );
   }   
   else if( this->IsVertical( ))
   {
      this->ReduceLength( begin, end, TGS_ORIENT_VERTICAL, snap );
   }
}

//===========================================================================//
void TGS_Line_c::ReduceLength( 
      double           begin,
      double           end,
      TGS_OrientMode_t orient,
      TGS_SnapMode_t   snap )
{
   if( orient == TGS_ORIENT_HORIZONTAL )
   {
      if( TCTF_IsLT( begin, 0.0 ))
      {
         begin = TCT_Max( begin, -1.0 * this->GetDx( ));
      }
      if( TCTF_IsGT( begin, 0.0 ))
      {
         begin = TCT_Min( begin, this->GetDx( ));
      }

      if( TCTF_IsLT( end, 0.0 ))
      {
         end = TCT_Max( end, -1.0 * this->GetDx( ));
      }
      if( TCTF_IsGT( end, 0.0 ))
      {
         end = TCT_Min( end, this->GetDx( ));
      }

      if( TCTF_IsLE( this->x1, this->x2 ))
      {
         this->x1 -= begin;
         this->x2 += end;
      }
      else
      {
         this->x1 += begin;
         this->x2 -= end;
      }
   }
   else if( orient == TGS_ORIENT_VERTICAL )
   {
      if( TCTF_IsLT( begin, 0.0 ))
      {
         begin = TCT_Max( begin, -1.0 * this->GetDy( ));
      }
      if( TCTF_IsGT( begin, 0.0 ))
      {
         begin = TCT_Min( begin, this->GetDy( ));
      }

      if( TCTF_IsLT( end, 0.0 ))
      {
         end = TCT_Max( end, -1.0 * this->GetDy( ));
      }
      if( TCTF_IsGT( end, 0.0 ))
      {
         end = TCT_Min( end, this->GetDy( ));
      }

      if( TCTF_IsLE( this->y1, this->y2 ))
      {
         this->y1 -= begin;
         this->y2 += end;
      }
      else
      {
         this->y1 += begin;
         this->y2 -= end;
      }
   }

   if( snap == TGS_SNAP_MIN_GRID )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      this->x1 = MinGrid.SnapToGrid( this->x1 );
      this->y1 = MinGrid.SnapToGrid( this->y1 );
      this->x2 = MinGrid.SnapToGrid( this->x2 );
      this->y2 = MinGrid.SnapToGrid( this->y2 );
   }
}

//===========================================================================//
// Method         : FindLength
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Line_c::FindLength( 
      void ) const
{
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.FindDistance( endPoint ));
}

//===========================================================================//
// Method         : FindCenter
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_Point_c TGS_Line_c::FindCenter( 
      TGS_SnapMode_t snap ) const
{
   TGS_Point_c center( this->x1 + (( this->x2 - this->x1 ) / 2.0 ),
                       this->y1 + (( this->y2 - this->y1 ) / 2.0 ),
                       this->z );

   if( snap == TGS_SNAP_MIN_GRID )
   {
      // After divide by 2.0, may need to "snap" point to min grid coordinates
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      center.x = MinGrid.SnapToGrid( center.x );
      center.y = MinGrid.SnapToGrid( center.y );
   }
   return( center );
}

//===========================================================================//
// Method         : FindOrient
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TGS_OrientMode_t TGS_Line_c::FindOrient( 
      void ) const
{
   TGS_Box_c thisBox( this->x1, this->y1, this->z, this->x2, this->y2, this->z );

   return( thisBox.FindOrient( ));
}

//===========================================================================//
// Method         : FindDistance
// Purpose        : Compute and return the "flightline" distance between
//                  the nearest point on this line and the given line or 
//                  point.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
double TGS_Line_c::FindDistance( 
      const TGS_Line_c& refLine ) const
{
   TGS_Point_c refPoint, thisPoint;
   this->FindNearest( refLine, &refPoint, &thisPoint );

   return( thisPoint.FindDistance( refPoint ));
}

//===========================================================================//
double TGS_Line_c::FindDistance( 
      const TGS_Point_c& refPoint ) const
{
   TGS_Point_c thisPoint;
   this->FindNearest( refPoint, &thisPoint );

   thisPoint.z = refPoint.z;
   return( thisPoint.FindDistance( refPoint ));
}

//===========================================================================//
// Method         : FindNearest
// Purpose        : Find and return the nearest points between this line and
//                  the given line or point. The nearest points are found 
//                  using the following algorithm:
//                  
//                   - Let the point be 'C' and the line segment be 'AB'
//                     from point 'A' to point 'B'.
//                   - Let 'P' be the point of perpendicular projection 
//                     of 'C' onto 'AB'.
//                   - Compute 'r' as:
//                  
//                            (A.y-C.y)(A.y-B.y) - (A.x-XC)(B.x-A.x)
//                        r = --------------------------------------
//                                   (B.x-A.x)^2 + (B.y-A.y)^2
//                  
//                   - Compute 'P' as:
//                  
//                        P.x = A.x + r(B.x-A.x)
//                        P.y = A.y + r(B.y-A.y)
//
// Reference      : comp.graphics.algorithms FAQ by O'Rourke
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Line_c::FindNearest( 
      const TGS_Line_c&  refLine,
            TGS_Point_c* prefNearestPoint,
            TGS_Point_c* pthisNearestPoint ) const
{
   TGS_Point_c refNearestPoint, thisNearestPoint;

   if( !this->IsIntersecting( refLine ))
   {
      double minDistance = TC_FLT_MAX;

      TGS_Point_c thisBeginPoint( this->GetBeginPoint( ));
      TGS_Point_c thisEndPoint( this->GetEndPoint( ));
      TGS_Point_c refBeginPoint( refLine.GetBeginPoint( ));
      TGS_Point_c refEndPoint( refLine.GetEndPoint( ));

      TGS_Point_c thisPoint;
      this->FindNearest( refBeginPoint, &thisPoint );
      if( TCTF_IsGT( minDistance, thisPoint.FindDistance( refBeginPoint )))
      {
         minDistance = thisPoint.FindDistance( refBeginPoint );
         thisNearestPoint = thisPoint;
         refNearestPoint = refBeginPoint;
      }

      this->FindNearest( refEndPoint, &thisPoint );
      if( TCTF_IsGT( minDistance, thisPoint.FindDistance( refEndPoint )))
      {
         minDistance = thisPoint.FindDistance( refEndPoint );
         thisNearestPoint = thisPoint;
         refNearestPoint = refEndPoint;
      }

      TGS_Point_c refPoint;
      refLine.FindNearest( thisBeginPoint, &refPoint );
      if( TCTF_IsGT( minDistance, refPoint.FindDistance( thisBeginPoint )))
      {
         minDistance = refPoint.FindDistance( thisBeginPoint );
         thisNearestPoint = thisBeginPoint;
         refNearestPoint = refPoint;
      }

      refLine.FindNearest( thisEndPoint, &refPoint );
      if( TCTF_IsGT( minDistance, refPoint.FindDistance( thisEndPoint )))
      {
         minDistance = refPoint.FindDistance( thisEndPoint );
         thisNearestPoint = thisEndPoint;
         refNearestPoint = refPoint;
      }
   }
   else
   {
      this->FindIntersect( refLine, &refNearestPoint );
      thisNearestPoint = refNearestPoint;
   }

   if( prefNearestPoint )
   {
      *prefNearestPoint = refNearestPoint;
   }
   if( pthisNearestPoint )
   {
      *pthisNearestPoint = thisNearestPoint;
   }
}

//===========================================================================//
void TGS_Line_c::FindNearest( 
      const TGS_Point_c& refPoint,
            TGS_Point_c* pthisNearestPoint ) const
{
   if( pthisNearestPoint )
   {
      pthisNearestPoint->Reset( );

      // First, find nearest point from point 'C' to line segment 'AB'
      TGS_Point_c pointA( this->GetBeginPoint( ));
      TGS_Point_c pointB( this->GetEndPoint( ));
      TGS_Point_c pointC( refPoint );

      double rN = (( pointA.y - pointC.y ) * ( pointA.y - pointB.y ) - 
                   ( pointA.x - pointC.x ) * ( pointB.x - pointA.x ));
      double rD = (( pointB.x - pointA.x ) * ( pointB.x - pointA.x ) + 
                   ( pointB.y - pointA.y ) * ( pointB.y - pointA.y ));
      double r = rN / ( TCTF_IsNZE( rD ) ? rD : TC_FLT_EPSILON );
   
      pthisNearestPoint->Set( pointA.x + r * ( pointB.x - pointA.x ),
                              pointA.y + r * ( pointB.y - pointA.y ),
                              pointA.z );

      // Next, handle case where nearest point is not within segment 'AB'
      TGS_Box_c boxAB( pointA, pointB );
      if( !boxAB.IsWithin( *pthisNearestPoint ))
      {
         double distA = pointA.FindDistance( *pthisNearestPoint );
         double distB = pointB.FindDistance( *pthisNearestPoint );
         pthisNearestPoint->Set( TCTF_IsLE( distA, distB ) ? 
                                 pointA : pointB );
      }
   }
}

//===========================================================================//
// Method         : FindIntersect
// Purpose        : Find and return the point of intersection on this line
//                  based on the given point.  The point of intersection is
//                  found using the following algorithm:
//                  
//                   - Let the line segment be 'AB' from point 'A' to
//                     point 'B' and 'CD' from point 'C' to point 'D',
//                   - Let 'I' be the intersect point for 'AB' and 'CD'.
//                   - Compute 'r' as:
//                  
//                            (A.y-C.y)(D.x-C.x) - (A.x-C.x)(D.y-C.y)
//                        r = ---------------------------------------
//                            (B.x-A.x)(D.y-C.y) - (B.y-A.y)(D.x-C.x)
//                  
//                   - Compute 'I' as:
//                  
//                        I.x = A.x + r(B.x-A.x)
//                        I.y = A.y + r(B.y-A.y)
// 
// Reference      : comp.graphics.algorithms FAQ by O'Rourke
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Line_c::FindIntersect( 
      const TGS_Line_c&  refLine,
            TGS_Point_c* pthisIntersectPoint ) const
{
   if( pthisIntersectPoint )
   {
      pthisIntersectPoint->Reset( );

      if( this->IsIntersecting( refLine ))
      {
         TGS_Point_c pointA( this->GetBeginPoint( ));
         TGS_Point_c pointB( this->GetEndPoint( ));
         TGS_Point_c pointC( refLine.GetBeginPoint( ));
         TGS_Point_c pointD( refLine.GetEndPoint( ));

         double rN = (( pointA.y - pointC.y ) * ( pointD.x - pointC.x ) - 
                      ( pointA.x - pointC.x ) * ( pointD.y - pointC.y ));
         double rD = (( pointB.x - pointA.x ) * ( pointD.y - pointC.y ) -
                      ( pointB.y - pointA.y ) * ( pointD.x - pointC.x ));
         if( TCTF_IsNZE( rN ) || TCTF_IsNZE( rD ))
         {
            // Find and return intersect point for non-collinear lines
            double r = rN / ( TCTF_IsNZE( rD ) ? rD : TC_FLT_EPSILON );
   
            pthisIntersectPoint->Set( pointA.x + r * ( pointB.x - pointA.x ),
                                      pointA.y + r * ( pointB.y - pointA.y ),
                                      pointA.z );
         }
         else
         {
 	    // Find and return intersect point for collinear lines
            this->FindNearest( pointD, pthisIntersectPoint );
         }
      }
   }
}

//===========================================================================//
// Method         : ApplyScale
// Purpose        : Scale this line based on the given value.  This line is
//                  assumed to be orthogonal.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Line_c::ApplyScale( 
      double scale )

{
   if( this->IsOrthogonal( ))
   {
      if( this->IsHorizontal( ))
      {
         scale = TCT_Max( scale, this->GetDx( ) / -2.0 );
   
         if( this->IsLeft( ))
         {
            this->x1 -= scale;
            this->x2 += scale;
         }
         else if( this->IsRight( ))
         {
            this->x1 += scale;
            this->x2 -= scale;
         }
      }
      else if( this->IsVertical( ))
      {
         scale = TCT_Max( scale, this->GetDy( ) / -2.0 );
   
         if( this->IsLower( ))
         {
            this->y1 -= scale;
            this->y2 += scale;
         }
         else if( this->IsUpper( ))
         {
            this->y1 += scale;
            this->y2 -= scale;
         }
      }
   }
}

//===========================================================================//
// Method         : ApplyUnion
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Line_c::ApplyUnion( 
      const TGS_Line_c& line )
{
   double begin = 0.0;
   double end = 0.0;

   if( this->IsHorizontal( ))
   {
      begin = line.x1;
      end = line.x2;
   }   
   else if( this->IsVertical( ))
   {
      begin = line.y1;
      end = line.y2;
   }   
   this->ApplyUnion( begin, end );
}

//===========================================================================//
void TGS_Line_c::ApplyUnion( 
      double begin,
      double end )
{
   if( this->IsHorizontal( ))
   {
      double xMin = TCT_Min( begin, end );
      double xMax = TCT_Max( begin, end );

      if( this->IsLeft( ))
      {
         this->x1 = TCT_Min( this->x1, xMin ); 
	 this->x2 = TCT_Max( this->x2, xMax ); 
      }   
      else if( this->IsRight( ))
      {
         this->x1 = TCT_Max( this->x1, xMax ); 
	 this->x2 = TCT_Min( this->x2, xMin ); 
      }   
   }   
   else if( this->IsVertical( ))
   {
      double yMin = TCT_Min( begin, end );
      double yMax = TCT_Max( begin, end );

      if( this->IsLower( ))
      {
         this->y1 = TCT_Min( this->y1, yMin ); 
	 this->y2 = TCT_Max( this->y2, yMax ); 
      }   
      else if( this->IsUpper( ))
      {
         this->y1 = TCT_Max( this->y1, yMax ); 
	 this->y2 = TCT_Min( this->y2, yMin ); 
      }   
   }
}

//===========================================================================//
// Method         : ApplyIntersect
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Line_c::ApplyIntersect( 
      const TGS_Line_c& line )
{
   double begin = 0.0;
   double end = 0.0;

   if( this->IsHorizontal( ))
   {
      begin = line.x1;
      end = line.x2;
   }   
   else if( this->IsVertical( ))
   {
      begin = line.y1;
      end = line.y2;
   }   
   this->ApplyIntersect( begin, end );
}

//===========================================================================//
void TGS_Line_c::ApplyIntersect( 
      double begin,
      double end )
{
   if( this->IsHorizontal( ))
   {
      double xMin = TCT_Min( begin, end );
      double xMax = TCT_Max( begin, end );

      if( this->IsLeft( ))
      {
         this->x1 = TCT_Max( this->x1, xMin ); 
	 this->x2 = TCT_Min( this->x2, xMax ); 
      }   
      else if( this->IsRight( ))
      {
         this->x1 = TCT_Min( this->x1, xMax ); 
	 this->x2 = TCT_Max( this->x2, xMin ); 
      }   
   }   
   else if( this->IsVertical( ))
   {
      double yMin = TCT_Min( begin, end );
      double yMax = TCT_Max( begin, end );

      if( this->IsLower( ))
      {
         this->y1 = TCT_Max( this->y1, yMin ); 
	 this->y2 = TCT_Min( this->y2, yMax ); 
      }   
      else if( this->IsUpper( ))
      {
         this->y1 = TCT_Min( this->y1, yMax ); 
	 this->y2 = TCT_Max( this->y2, yMin ); 
      }   
   }
}

//===========================================================================//
// Method         : ApplyNormalize
// Purpose        : Normalize this line such that the begin point is the
//                  lower-left-most point, if possible.  Otherwise,
//                  normalize this line such that the begin point is the
//                  lower-most or left-most point.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_Line_c::ApplyNormalize( 
      void )
{
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   if( endPoint.IsLowerLeft( beginPoint ) ||
       endPoint.IsLowerRight( beginPoint ))
   {
      this->Set( endPoint, beginPoint );
   }
   else if( endPoint.IsHorizontal( beginPoint ) &&
            endPoint.IsLeft( beginPoint ))
   {
      this->Set( endPoint, beginPoint );
   }
   else if( endPoint.IsVertical( beginPoint ) &&
            endPoint.IsLower( beginPoint ))
   {
      this->Set( endPoint, beginPoint );
   }
}

//===========================================================================//
// Method         : CrossProduct
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
int TGS_Line_c::CrossProduct( 
      const TGS_Point_c& refPoint ) const
{
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.CrossProduct( endPoint, refPoint ));
}

//===========================================================================//
// Method         : IsConnected
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::IsConnected(
      const TGS_Line_c& line ) const 
{
   bool isConnected = false;

   TGS_Point_c thisBeginPoint( this->GetBeginPoint( ));
   TGS_Point_c thisEndPoint( this->GetEndPoint( ));
   TGS_Point_c lineBeginPoint( line.GetBeginPoint( ));
   TGS_Point_c lineEndPoint( line.GetEndPoint( ));

   if(( thisBeginPoint == lineEndPoint ) ||
      ( thisEndPoint == lineBeginPoint ))
   {   
      isConnected = true;
   }
   return( isConnected );
}

//===========================================================================//
// Method         : IsIntersecting
// Purpose        : Return true if this line intersects with the given line.
//                  Intersection between arbitrary lines is decided based on
//                  the cross-product of this line with the given line's 
//                  end-points and the cross-product of the given line with 
//                  this line's end-points.
// Reference      : Algorithms by Cormen, Leiserson, Rivest, pp. 889-890.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::IsIntersecting( 
      const TGS_Line_c&  line,
            TGS_Point_c* ppoint ) const
{
   bool isIntersecting = false;

   TGS_Point_c thisBeginPoint( this->GetBeginPoint( ));
   TGS_Point_c thisEndPoint( this->GetEndPoint( ));
   TGS_Point_c lineBeginPoint( line.GetBeginPoint( ));
   TGS_Point_c lineEndPoint( line.GetEndPoint( ));

   // First, apply quick rejection test to see if bounding regions intersect
   TGS_Region_c thisRegion( thisBeginPoint, thisEndPoint );
   TGS_Region_c lineRegion( lineBeginPoint, lineEndPoint );
   if( thisRegion.IsIntersecting( lineRegion ))
   {
      int crossProductThisBegin = this->CrossProduct( lineBeginPoint );
      int crossProductThisEnd = this->CrossProduct( lineEndPoint );

      int crossProductLineBegin = line.CrossProduct( thisBeginPoint );
      int crossProductLineEnd = line.CrossProduct( thisEndPoint );

      isIntersecting = true;
      if((( crossProductThisBegin < 0 ) && ( crossProductThisEnd < 0 )) ||
         (( crossProductThisBegin > 0 ) && ( crossProductThisEnd > 0 )) ||
         (( crossProductLineBegin < 0 ) && ( crossProductLineEnd < 0 )) ||
         (( crossProductLineBegin > 0 ) && ( crossProductLineEnd > 0 )))
      {
         isIntersecting = false;
      }
   }

   if( isIntersecting )
   {
      this->FindIntersect( line, ppoint );
   }
   return( isIntersecting );
}

//===========================================================================//
bool TGS_Line_c::IsIntersecting( 
      const TGS_Point_c& point ) const
{
   return( TCTF_IsZE( this->FindDistance( point )) ? true : false );
} 

//===========================================================================//
// Method         : IsOverlapping
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::IsOverlapping( 
      const TGS_Line_c& line ) const
{
   bool isOverlapping = false;

   if(( this->z == line.z ) &&
      ( this->IsIntersecting( line )))
   {
      isOverlapping = true;
   }
   return( isOverlapping );
}

//===========================================================================//
bool TGS_Line_c::IsOverlapping( 
      const TGS_Point_c& point ) const
{
   bool isOverlapping = false;

   if( abs( this->z - point.z ) == 1 )
   {
      TGS_Point_c point_( point.x, point.y, this->z );
      if( this->IsIntersecting( point ))
      {
         isOverlapping = true;
      }
   }
   return( isOverlapping );
}

//===========================================================================//
// Method         : IsWithin
// Purpose        : Return true if the given line is within this line.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::IsWithin( 
      const TGS_Line_c& line ) const
{
   bool isWithin = false;
   
   TGS_Point_c lineBeginPoint( line.GetBeginPoint( ));
   TGS_Point_c lineEndPoint( line.GetEndPoint( ));

   if( this->IsIntersecting( lineBeginPoint ) &&
       this->IsIntersecting( lineEndPoint ))
   {
      isWithin = true;
   }
   return( isWithin );
}

//===========================================================================//
// Method         : IsParallel
// Purpose        : Return true if the given line is parallel with this line.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::IsParallel( 
      const TGS_Line_c& line ) const
{
   bool isParallel = false;

   double thisDx = this->x1 - this->x2;
   double thisDy = this->y1 - this->y2;

   double lineDx = line.x1 - line.x2;
   double lineDy = line.y1 - line.y2;

   if( TCTF_IsNZE( thisDy ) && TCTF_IsNZE( lineDy ))
   {
      double thisDxDy = thisDx / thisDy;
      double lineDxDy = lineDx / lineDy;
      isParallel = TCTF_IsEQ( thisDxDy, lineDxDy );
   }
   else if( TCTF_IsZE( thisDy ) && TCTF_IsZE( lineDy ))
   {
      isParallel = true;
   }
   else if( TCTF_IsZE( thisDx ) && TCTF_IsZE( lineDx ))
   {
      isParallel = true;
   }
   return( isParallel );
} 

//===========================================================================//
// Method         : IsExtension
// Purpose        : Return true if the given line is an extension of this 
//                  line.  An extension implies the given line's begin point
//                  is connected to this line's end point and both lines are
//                  parallel.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::IsExtension( 
      const TGS_Line_c& line ) const
{
   bool isExtension = false;

   TGS_Point_c thisEndPoint( this->GetEndPoint( ));
   TGS_Point_c lineBeginPoint( line.GetBeginPoint( ));
   if( thisEndPoint == lineBeginPoint )
   {
      isExtension = this->IsParallel( line ) ? true : false;
   }
   return( isExtension );
} 

//===========================================================================//
// Method         : IsLeft/IsRight/IsLower/IsUpper
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::IsLeft( 
      void ) const
{
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsLeft( endPoint ));
}

//===========================================================================//
bool TGS_Line_c::IsRight( 
      void ) const
{
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsRight( endPoint ));
}

//===========================================================================//
bool TGS_Line_c::IsLower( 
      void ) const
{
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsLower( endPoint ));
}

//===========================================================================//
bool TGS_Line_c::IsUpper( 
      void ) const
{
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsUpper( endPoint ));
}

//===========================================================================//
// Method         : IsLowerLeft/IsLowerRight/IsUpperLeft/IsUpperRight
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::IsLowerLeft( 
      void ) const
{
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsLowerLeft( endPoint ));
}

//===========================================================================//
bool TGS_Line_c::IsLowerRight( 
      void ) const
{
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsLowerRight( endPoint ));
}

//===========================================================================//
bool TGS_Line_c::IsUpperLeft( 
      void ) const
{ 
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsUpperLeft( endPoint ));
}

//===========================================================================//
bool TGS_Line_c::IsUpperRight( 
      void ) const
{
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsUpperRight( endPoint ));
}

//===========================================================================//
// Method         : IsOrthogonal
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::IsOrthogonal( 
      void ) const
{
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsOrthogonal( endPoint ));
}

//===========================================================================//
// Method         : IsHorizontal/IsVertical
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::IsHorizontal( 
      void ) const
{  
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsHorizontal( endPoint ));
}

//===========================================================================//
bool TGS_Line_c::IsVertical( 
      void ) const
{
   TGS_Point_c beginPoint( this->GetBeginPoint( ));
   TGS_Point_c endPoint( this->GetEndPoint( ));

   return( beginPoint.IsVertical( endPoint ));
}

//===========================================================================//
// Method         : IsValid
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TGS_Line_c::IsValid( 
      void ) const
{
   return(( TCTF_IsNEQ( this->x1, TC_FLT_MIN )) &&
          ( TCTF_IsNEQ( this->x1, TC_FLT_MAX )) &&
          ( TCTF_IsNEQ( this->y1, TC_FLT_MIN )) &&
          ( TCTF_IsNEQ( this->y1, TC_FLT_MAX )) &&
          ( TCTF_IsNEQ( this->x2, TC_FLT_MIN )) &&
          ( TCTF_IsNEQ( this->x2, TC_FLT_MAX )) &&
          ( TCTF_IsNEQ( this->y2, TC_FLT_MIN )) &&
          ( TCTF_IsNEQ( this->y2, TC_FLT_MAX )) ?
          true : false );
}
