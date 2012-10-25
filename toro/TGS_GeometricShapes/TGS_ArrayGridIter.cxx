//===========================================================================//
// Purpose : Method definitions for a TGS_ArrayGridIter iterator class.
//
//           Public methods include:
//           - TGS_ArrayGridIter, ~TGS_ArrayGridIter
//           - Init
//           - Next
//           - Reset
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

#include "TGS_ArrayGridIter.h"

//===========================================================================//
// Method         : TGS_ArrayGridIter_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
TGS_ArrayGridIter_c::TGS_ArrayGridIter_c( 
      const TGS_ArrayGrid_c* parrayGrid,
      const TGS_Region_c*    parrayRegion )
      :
      parrayGrid_( 0 ),
      x_( TC_FLT_MAX ),
      y_( TC_FLT_MAX )
{
   this->Init( parrayGrid, parrayRegion );
}

//===========================================================================//
TGS_ArrayGridIter_c::TGS_ArrayGridIter_c( 
      const TGS_ArrayGrid_c& arrayGrid,
      const TGS_Region_c&    arrayRegion )
      :
      parrayGrid_( 0 ),
      x_( TC_FLT_MAX ),
      y_( TC_FLT_MAX )
{
   this->Init( arrayGrid, arrayRegion );
}

//===========================================================================//
// Method         : ~TGS_ArrayGridIter_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
TGS_ArrayGridIter_c::~TGS_ArrayGridIter_c( 
      void )
{
}

//===========================================================================//
// Method         : Init
// Purpose        : Initialize this iterator based on a given array grid 
//                  object.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
void TGS_ArrayGridIter_c::Init( 
      const TGS_ArrayGrid_c* parrayGrid,
      const TGS_Region_c*    parrayRegion )
{
   this->parrayGrid_ = 0;
   this->arrayRegion_.Reset( );

   this->pitch_.Reset( );

   this->x_ = TC_FLT_MAX;
   this->y_ = TC_FLT_MAX;

   if( parrayGrid && parrayRegion )  
   {
      this->parrayGrid_ = const_cast< TGS_ArrayGrid_c* >( parrayGrid );

      this->arrayRegion_ = *parrayRegion;
      this->parrayGrid_->SnapToGrid( this->arrayRegion_, 
                                     &this->arrayRegion_ );

      if( this->arrayRegion_.IsValid( ))
      {
         // Set cached pitch dimensions based on array grid
         this->parrayGrid_->GetPitch( &this->pitch_ );

         // Default initial indices to array region lower-left corner
         this->x_ = this->arrayRegion_.x1 - this->pitch_.width;
         this->y_ = this->arrayRegion_.y1;
      }
   }
}

//===========================================================================//
void TGS_ArrayGridIter_c::Init( 
      const TGS_ArrayGrid_c& arrayGrid,
      const TGS_Region_c&    arrayRegion )
{
   this->Init( &arrayGrid, &arrayRegion );
}

//===========================================================================//
// Method         : Next
// Purpose        : Return the next pair of indices, if any, based on the 
//                  initial array region dimensions.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
bool TGS_ArrayGridIter_c::Next(
      double* px,
      double* py )
{
   if( this->IsValid( ))
   {
      if( TCTF_IsLE( this->x_, this->arrayRegion_.x2 ) || 
          TCTF_IsLE( this->y_, this->arrayRegion_.y2 ))
      {
	 this->x_ += this->pitch_.width;

         if( TCTF_IsGT( this->x_, this->arrayRegion_.x2 ))
	 {
            this->x_ = this->arrayRegion_.x1;
            this->y_ += this->pitch_.height;

            if( TCTF_IsGT( this->y_, this->arrayRegion_.y2 ))
	    {
	       this->x_ = TC_FLT_MAX;
	       this->y_ = TC_FLT_MAX;
            }
         }
      }
      else
      {
	 this->x_ = TC_FLT_MAX;
         this->y_ = TC_FLT_MAX;
      }

      if( px )
      {
         *px = this->x_;
      }
      if( py )
      {
         *py = this->y_;
      }
   }
   return( TCTF_IsLE( this->x_, this->arrayRegion_.x2 ) &&
           TCTF_IsLE( this->y_, this->arrayRegion_.y2 ) ?
           true : false );
}

//===========================================================================//
// Method         : Reset
// Purpose        : Resets this iterator based on the current array grid 
//                  object.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
void TGS_ArrayGridIter_c::Reset( 
      void )
{
   TGS_ArrayGrid_c* parrayGrid = this->parrayGrid_;
   TGS_Region_c arrayRegion = this->arrayRegion_;

   this->Init( parrayGrid, &arrayRegion );
}
