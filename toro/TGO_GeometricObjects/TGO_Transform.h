//===========================================================================//
// Purpose : Declaration and inline definitions for a TGO_Transform geometric
//           transform 3D class.
//
//           Inline methods include:
//           - SetOrigin, SetRotate, SetTranslate
//           - GetOrigin, GetRotate, GetTranslate
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TGO_TRANSFORM_H
#define TGO_TRANSFORM_H

#include "TGO_Typedefs.h"

#include "TGO_Point.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TGO_Transform_c
{
public:

   TGO_Transform_c( void );
   TGO_Transform_c( const TGO_Point_c& origin,
                    TGO_RotateMode_t rotate,
                    const TGO_IntDims_t& translate );
   TGO_Transform_c( const TGO_Point_c& origin,
                    TGO_RotateMode_t rotate,
                    const TGO_Point_c& from,
                    const TGO_Point_c& to );
   TGO_Transform_c( TGO_RotateMode_t rotate,
                    const TGO_IntDims_t& translate );
   TGO_Transform_c( TGO_RotateMode_t rotate,
                    const TGO_Point_c& from,
                    const TGO_Point_c& to );
   ~TGO_Transform_c( void );

   TGO_Transform_c& operator=( const TGO_Transform_c& transform );
   bool operator==( const TGO_Transform_c& transform ) const;
   bool operator!=( const TGO_Transform_c& transform ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrTransform ) const;

   void SetOrigin( const TGO_Point_c& origin );
   void SetRotate( TGO_RotateMode_t rotate );
   void SetTranslate( const TGO_IntDims_t& translate );

   const TGO_Point_c& GetOrigin( void ) const;
   TGO_RotateMode_t GetRotate( void ) const;
   const TGO_IntDims_t& GetTranslate( void ) const;

   void Apply( const TGO_Point_c& point, 
               TGO_Point_c* ppoint ) const;
   void Apply( const TGO_PointList_t& pointList,
               TGO_PointList_t* ppointList ) const;
   void Inverse( const TGO_Point_c& point,
                 TGO_Point_c* ppoint ) const;
   void Inverse( const TGO_PointList_t& pointList,
                 TGO_PointList_t* ppointList ) const;

   bool IsValid( void ) const;

private:

   TGO_Point_c      origin_;
   TGO_RotateMode_t rotate_;
   TGO_IntDims_t    translate_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
inline void TGO_Transform_c::SetOrigin( 
      const TGO_Point_c& origin )
{
   this->origin_.Set( origin );
}

//===========================================================================//
inline void TGO_Transform_c::SetRotate( 
      TGO_RotateMode_t rotate )
{
   this->rotate_ = rotate;
}

//===========================================================================//
inline void TGO_Transform_c::SetTranslate( 
      const TGO_IntDims_t& translate )
{
   this->translate_.Set( translate );
}

//===========================================================================//
inline const TGO_Point_c& TGO_Transform_c::GetOrigin( 
      void ) const
{
   return( this->origin_ );
}

//===========================================================================//
inline TGO_RotateMode_t TGO_Transform_c::GetRotate(
      void ) const
{
   return( this->rotate_ );
}

//===========================================================================//
inline const TGO_IntDims_t& TGO_Transform_c::GetTranslate( 
      void ) const
{
   return( this->translate_ );
}

//===========================================================================//
inline bool TGO_Transform_c::IsValid( 
      void ) const
{
   return(( this->origin_.IsValid( )) &&
          ( this->rotate_ != TGO_ROTATE_UNDEFINED ) &&
          ( this->translate_.IsValid( )) ?
          true : false );
} 

#endif
