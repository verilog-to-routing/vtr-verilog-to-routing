//===========================================================================//
// Purpose : Template version for a TTPT_TilePlane tile plane iterator class.
//
//           Inline methods include:
//           - IsValid
//
//           Public methods include:
//           - TTPT_TilePlaneIter_c, ~TTPT_TilePlaneIter_c
//           - Init
//           - Next
//           - Reset
//
//           Private methods include:
//           - InitByRegion_, InitBySide_
//           - NextByRegion_, NextBySide_
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

#ifndef TTPT_TILE_PLANE_ITER_H
#define TTPT_TILE_PLANE_ITER_H

#include "TC_Typedefs.h"
#include "TC_MinGrid.h"
#include "TCT_Generic.h"
#include "TCT_Stack.h"

#include "TGS_Region.h"
#include "TGS_Line.h"
#include "TGS_Point.h"

#include "TTP_Typedefs.h"
#include "TTPT_Tile.h"

// Make forward declaration for 'TTPT_TilePlane_c' object
template< class T > class TTPT_TilePlane_c;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > class TTPT_TilePlaneIter_c
{
public:

   TTPT_TilePlaneIter_c( void );
   TTPT_TilePlaneIter_c( const TTPT_TilePlane_c< T >& tilePlane );
   TTPT_TilePlaneIter_c( const TTPT_TilePlane_c< T >& tilePlane,
                         const TGS_Region_c& region );
   TTPT_TilePlaneIter_c( const TTPT_TilePlane_c< T >& tilePlane,
                         const TGS_Region_c& region,
                         TC_SideMode_t side );
   TTPT_TilePlaneIter_c( const TTPT_TilePlane_c< T >& tilePlane,
                         const TTPT_Tile_c< T >& tile,
                         TC_SideMode_t side );
   ~TTPT_TilePlaneIter_c( void );

   bool Init( const TTPT_TilePlane_c< T >& tilePlane );
   bool Init( const TTPT_TilePlane_c< T >& tilePlane,
              const TGS_Region_c& region );
   bool Init( const TTPT_TilePlane_c< T >& tilePlane,
              const TGS_Region_c& region,
              TC_SideMode_t side );
   bool Init( const TTPT_TilePlane_c< T >& tilePlane,
              const TTPT_Tile_c< T >& tile,
              TC_SideMode_t side );

   bool Next( TTPT_Tile_c< T >** pptile,
              TTP_TileMode_t mode = TTP_TILE_ANY );

   void Reset( void );

   bool IsValid( void ) const;

private:

   bool InitByRegion_( const TGS_Region_c& region );
   bool InitBySide_( const TGS_Region_c& region,
                     TC_SideMode_t side );
   bool InitBySide_( const TTPT_Tile_c< T >& tile,
                     TC_SideMode_t side );

   bool NextByRegion_( TTPT_Tile_c< T >** pptile );
   bool NextBySide_( TTPT_Tile_c< T >** pptile );

private:

   TTPT_TilePlane_c< T >* ptilePlane_; // Ptr to a 'TilePlane' object
   TTPT_Tile_c< T >* ptile_;           // Ptr to currently active 'Tile' object

   TCT_Stack_c< TTPT_Tile_c< T >* >* ptileStack_;
                                    // Used for region-specific iteration mode
   TGS_Region_c  region_;           // Used for region-specific iteration mode
   TC_SideMode_t side_;             // Used for side-specific iteration mode
   TTPT_Tile_c< T > tile_;          // Used for side-specific iteration mode

   double minGrid_;                 // Define min-grid value for tile regions
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > inline bool TTPT_TilePlaneIter_c< T >::IsValid( 
      void ) const
{
   return(( this->ptilePlane_ && this->ptile_ ) ? true : false );
}

//===========================================================================//
// Method         : TTPT_TilePlaneIter_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_TilePlaneIter_c< T >::TTPT_TilePlaneIter_c( 
      void )
      :
      ptilePlane_( 0 ),
      ptile_( 0 ),
      ptileStack_( 0 ),
      side_( TC_SIDE_UNDEFINED )
{
   this->minGrid_ = TC_MinGrid_c::GetInstance( ).GetGrid( );
}

//===========================================================================//
template< class T > TTPT_TilePlaneIter_c< T >::TTPT_TilePlaneIter_c( 
      const TTPT_TilePlane_c< T >& tilePlane )
      :
      ptilePlane_( 0 ),
      ptile_( 0 ),
      ptileStack_( 0 ),
      side_( TC_SIDE_UNDEFINED )
{
   this->minGrid_ = TC_MinGrid_c::GetInstance( ).GetGrid( );

   this->Init( tilePlane );
}

//===========================================================================//
template< class T > TTPT_TilePlaneIter_c< T >::TTPT_TilePlaneIter_c( 
      const TTPT_TilePlane_c< T >& tilePlane,
      const TGS_Region_c&          region )
      :
      ptilePlane_( 0 ),
      ptile_( 0 ),
      ptileStack_( 0 ),
      side_( TC_SIDE_UNDEFINED )
{
   this->minGrid_ = TC_MinGrid_c::GetInstance( ).GetGrid( );

   this->Init( tilePlane, region );
}

//===========================================================================//
template< class T > TTPT_TilePlaneIter_c< T >::TTPT_TilePlaneIter_c( 
      const TTPT_TilePlane_c< T >& tilePlane,
      const TGS_Region_c&          region,
            TC_SideMode_t          side )
      :
      ptilePlane_( 0 ),
      ptile_( 0 ),
      ptileStack_( 0 ),
      side_( TC_SIDE_UNDEFINED )
{
   this->minGrid_ = TC_MinGrid_c::GetInstance( ).GetGrid( );

   this->Init( tilePlane, region, side );
}

//===========================================================================//
template< class T > TTPT_TilePlaneIter_c< T >::TTPT_TilePlaneIter_c( 
      const TTPT_TilePlane_c< T >& tilePlane,
      const TTPT_Tile_c< T >&      tile,
            TC_SideMode_t          side )
      :
      ptilePlane_( 0 ),
      ptile_( 0 ),
      ptileStack_( 0 ),
      side_( TC_SIDE_UNDEFINED )
{
   this->minGrid_ = TC_MinGrid_c::GetInstance( ).GetGrid( );

   this->Init( tilePlane, tile, side );
}

//===========================================================================//
// Method         : ~TTPT_TilePlaneIter_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_TilePlaneIter_c< T >::~TTPT_TilePlaneIter_c( 
      void )
{
   delete( this->ptileStack_ );
}

//===========================================================================//
// Method         : Init
// Purpose        : Inititalizes tile plane iteration.  By default, iteration 
//                  "visits" each tile in the tile plane region exactly once.
//                  Optionally, iteration may be limited to a specified
//                  region.  Optionally, iteration may also be limited to
//                  enumerating all neighboring (ie. touching) tiles located 
//                  on the specified side of a given tile.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlaneIter_c< T >::Init( 
      const TTPT_TilePlane_c< T >& tilePlane )
{
   this->ptilePlane_ = const_cast< TTPT_TilePlane_c< T >* >( &tilePlane );

   TGS_Region_c iterRegion( tilePlane.GetRegion( ));
   this->InitByRegion_( iterRegion );

   return( this->IsValid( ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlaneIter_c< T >::Init( 
      const TTPT_TilePlane_c< T >& tilePlane,
      const TGS_Region_c&          region )
{
   this->ptilePlane_ = const_cast< TTPT_TilePlane_c< T >* >( &tilePlane );

   TGS_Region_c iterRegion( tilePlane.GetRegion( ));
   if( region.IsValid( ))
   {
      iterRegion.ApplyIntersect( region );
   }
   this->InitByRegion_( iterRegion );

   return( this->IsValid( ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlaneIter_c< T >::Init( 
      const TTPT_TilePlane_c< T >& tilePlane,
      const TGS_Region_c&          region,
            TC_SideMode_t          side )
{
   this->ptilePlane_ = const_cast< TTPT_TilePlane_c< T >* >( &tilePlane );

   this->InitBySide_( region, side );

   return( this->IsValid( ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlaneIter_c< T >::Init( 
      const TTPT_TilePlane_c< T >& tilePlane,
      const TTPT_Tile_c< T >&      tile,
            TC_SideMode_t          side )
{
   this->ptilePlane_ = const_cast< TTPT_TilePlane_c< T >* >( &tilePlane );

   this->InitBySide_( tile, side );

   return( this->IsValid( ));
}

//===========================================================================//
// Method         : Next
// Purpose        : Find and return the next tile plane iteration.  By
//                  default, iteration "visits" each tile in the tile plane
//                  region exactly once.  Optionally, iteration may be
//                  limited to a specified region.  Optionally, iteration
//                  may also be limited to enumerating all neighboring (ie. 
//                  touching) tiles located on the specified side of a given 
//                  tile.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlaneIter_c< T >::Next(
      TTPT_Tile_c< T >** pptile,
      TTP_TileMode_t     mode )
{
   TTPT_Tile_c< T >* ptile = 0;

   if( this->IsValid( ))
   {
      if( this->side_ == TC_SIDE_UNDEFINED )
      {
         this->NextByRegion_( &ptile );
      }
      else // if( this->side_ != TC_SIDE_UNDEFINED )
      {
         this->NextBySide_( &ptile );
      }

      if( ptile )
      {
         if(( mode == TTP_TILE_CLEAR && ptile->IsSolid( )) ||
	    ( mode == TTP_TILE_SOLID && ptile->IsClear( )))
         {
	    this->Next( &ptile, mode );
         }
      }
   }

   if( pptile )
   {
      *pptile = ptile;
   }
   return( ptile ? true : false );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlaneIter_c< T >::Reset(
      void )
{
   this->ptilePlane_ = 0;
   this->ptile_ = 0;

   if( this->ptileStack_ )
   {
      this->ptileStack_->Clear( );
   }

   this->region_.Reset( );
   this->side_ = TC_SIDE_UNDEFINED;
}

//===========================================================================//
// Method         : InitByRegion_
// Purpose        : Initialize tile plane iteration where iteration is
//                  limited to all tiles contained within or intersecting
//                  with the specified region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlaneIter_c< T >::InitByRegion_(
      const TGS_Region_c& region )
{
   this->region_ = region;
   this->side_ = TC_SIDE_UNDEFINED;

   this->ptile_ = 0;

   if( this->ptileStack_ )
   {
      this->ptileStack_->Clear( );
   }
   else
   {
      this->ptileStack_ = new TC_NOTHROW TCT_Stack_c< TTPT_Tile_c< T >* >;
   }

   // Begin iteration from first tile defined by upper-left corner of region
   if( this->region_.IsValid( ))
   {
      this->ptile_ = this->ptilePlane_->Find( this->region_.x1, this->region_.y2 );

      this->ptileStack_->Push( this->ptile_ );
   }
   return( this->IsValid( ));
}

//===========================================================================//
// Method         : InitBySide_
// Purpose        : Inititalizes tile plane iteration where iteration is
//                  limited to enumerating all neighboring (ie. touching)
//                  tiles located on the specified side of a given tile.
//                  
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlaneIter_c< T >::InitBySide_(
      const TGS_Region_c& region,
            TC_SideMode_t side )
{
   bool ok = true;

   TTPT_Tile_c< T >* ptile = this->ptilePlane_->Find( region );
   if( ptile )
   {
      ok = this->InitBySide_( *ptile, side );
   }
   else
   {
      TGS_Region_c sideRegion;
      region.FindRegion( side, this->minGrid_, &sideRegion );
      ok = this->InitByRegion_( sideRegion );
   }
   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_TilePlaneIter_c< T >::InitBySide_(
      const TTPT_Tile_c< T >& tile,
            TC_SideMode_t     side )
{
   this->region_ = tile.GetRegion( );
   this->side_ = side;

   switch( this->side_ )
   {
   case TC_SIDE_LEFT:

      // Begin iteration from lower-most left neighbor tile via 'lower_left' stitch 
      this->ptile_ = tile.GetStitchLowerLeft( );
      break;

   case TC_SIDE_RIGHT:

      // Begin iteration from upper-most right neighbor tile via 'upper_right' stitch 
      this->ptile_ = tile.GetStitchUpperRight( );
      break;

   case TC_SIDE_LOWER:

      // Begin iteration from left-most lower neighbor tile via 'left_lower' stitch 
      this->ptile_ = tile.GetStitchLeftLower( );
      break;

   case TC_SIDE_UPPER:

      // Begin iteration from right-most upper neighbor tile via 'right_upper' stitch 
      this->ptile_ = tile.GetStitchRightUpper( );
      break;

   default:
      break;
   }

   if( this->ptile_ )
   {
      this->tile_.SetRegion( this->ptile_->GetRegion( ));
   }
   return( this->IsValid( ));
}

//===========================================================================//
// Method         : NextByRegion_
// Purpose        : Find and return the next tile plane iteration where
//                  iteration is limited to all tiles contained within or
//                  intersecting with the specified region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlaneIter_c< T >::NextByRegion_(
      TTPT_Tile_c< T >** pptile )
{
   // Return next tile based on first tile retrieved from tile stack, if any
   this->ptile_ = 0;
   if( this->ptileStack_->Pop( &this->ptile_ ))
   {
      *pptile = this->ptile_;

      TTPT_Tile_c< T >* pseedTile = this->ptile_;

      // Update tile stack with subsequent tiles based on current tile
      const TGS_Region_c& tileRegion = this->ptile_->GetRegion( );

      // Update tile stack if tile intersects left side of iteration region  
      const TGS_Region_c& iterRegion = this->region_;
      if( tileRegion.IsIntersecting( iterRegion, TC_SIDE_LEFT ))
      {
         // Find next tile IFF not lower-most intersecting tile region
         if( TCTF_IsGT( tileRegion.y1, iterRegion.y1 ) ||
             !tileRegion.IsWithinDx( iterRegion ))
         {
            // Find next tile along left side of iteration region
            TGS_Point_c nextPoint( iterRegion.x1, tileRegion.y1 - this->minGrid_ );
            TTPT_Tile_c< T >* pnextTile = this->ptilePlane_->Find( nextPoint, pseedTile );
            if( pnextTile )
            {
               const TGS_Region_c& nextRegion = pnextTile->GetRegion( );
               if( nextRegion.IsIntersecting( iterRegion ))
               {
                  // Push next tile if it intersects left side of iteration region
                  this->ptileStack_->Push( pnextTile );
               }
               pseedTile = pnextTile;
            }
         }
      }

      // Update tile stack if tile has any right adjacent tiles in iteration region  
      if( TCTF_IsLT( tileRegion.x2, iterRegion.x2 ))
      {
         // Iterate for each adjacent neighbor on right side of current tile
         TGS_Line_c rightSide( tileRegion.x2, TCT_Max( tileRegion.y1, iterRegion.y1 ), 
                               tileRegion.x2, TCT_Min( tileRegion.y2, iterRegion.y2 ));
         while( TCTF_IsLE( rightSide.y1, rightSide.y2 ))
         {
            // Find next tile along right side of iteration region
            TGS_Point_c nextPoint( rightSide.x2 + this->minGrid_, rightSide.y1 );
            TTPT_Tile_c< T >* pnextTile = this->ptilePlane_->Find( nextPoint, pseedTile );
            if( pnextTile )
            {
               const TGS_Region_c& nextRegion = pnextTile->GetRegion( );
               if( nextRegion.IsIntersecting( iterRegion ))
               {
                  if( TCTF_IsGE( nextRegion.y1, tileRegion.y1 ))
                  {
                     // Push next tile if lower-left corner "touches" tile
 		     // (ie. if lower-left point "touches" right side of tile)
                     this->ptileStack_->Push( pnextTile );
                  }
                  else if( nextRegion.IsIntersecting( iterRegion, TC_SIDE_LOWER ) &&
                           tileRegion.IsIntersecting( iterRegion, TC_SIDE_LOWER ))
                  {
                     // Push next tile if both tiles intersect at lower edge
		     // (ie. both tiles intersect bottom of iteration region)
                     this->ptileStack_->Push( pnextTile );
                  }
               }

               // Continue iteration from lower to upper on right side of region  
               rightSide.y1 = nextRegion.y2 + this->minGrid_;

               pseedTile = pnextTile;
            }
            else
            {
	      break;
            }
         }
      }
   }
   return( *pptile ? true : false );
}

//===========================================================================//
// Method         : NextBySide_
// Purpose        : Find and return the next tile plane iteration where
//                  iteration is limited to enumerating all neighboring (ie. 
//                  touching) tiles located on the specified side of a given 
//                  tile.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlaneIter_c< T >::NextBySide_(
      TTPT_Tile_c< T >** pptile )
{
   const TGS_Region_c& tileRegion = tile_.GetRegion( );

   switch( this->side_ )
   {
   case TC_SIDE_LEFT:

      // Iterate until no more neighbor tiles found by following 'right_upper' stitch
      // (ie. iterate while next neighbor tile's y1 <= given tile region's y2)
      if( TCTF_IsLE( tileRegion.y1, this->region_.y2 ))
      {      
         *pptile = this->ptile_;
         this->ptile_ = this->ptile_->GetStitchRightUpper( );
      }            
      else
      {
         this->ptile_ = 0;      
      }
      break;

   case TC_SIDE_RIGHT:

      // Iterate until no more neighbor tiles found by following 'left_lower' stitch
      // (ie. iterate while next neighbor tile's y2 >= given tile region's y1)
      if( TCTF_IsGE( tileRegion.y2, this->region_.y1 ))
      {      
         *pptile = this->ptile_;
         this->ptile_ = this->ptile_->GetStitchLeftLower( );
      }            
      else
      {
         this->ptile_ = 0;      
      }
      break;

   case TC_SIDE_LOWER:

      // Iterate until no more neighbor tiles found by following 'upper_right' stitch
      // (ie. iterate while next neighbor tile's x1 <= given tile region's x2)
      if( TCTF_IsLE( tileRegion.x1, this->region_.x2 ))
      {      
         *pptile = this->ptile_;
         this->ptile_ = this->ptile_->GetStitchUpperRight( );
      }            
      else
      {
         this->ptile_ = 0;      
      }
      break;

   case TC_SIDE_UPPER:

      // Iterate until no more neighbor tiles found by following 'lower_left' stitch
      // (ie. iterate while next neighbor tile's x2 >= given tile region's x1)
      if( TCTF_IsGE( tileRegion.x2, this->region_.x1 ))
      {      
         *pptile = this->ptile_;
         this->ptile_ = this->ptile_->GetStitchLowerLeft( );
      }            
      else
      {
         this->ptile_ = 0;      
      }
      break;

   default:
      break;
   }

   if( this->ptile_ )
   {
      this->tile_.SetRegion( this->ptile_->GetRegion( ));
   }
   return( *pptile ? true : false );
}

#endif 
