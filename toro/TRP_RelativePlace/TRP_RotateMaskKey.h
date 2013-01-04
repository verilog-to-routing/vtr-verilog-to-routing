//===========================================================================//
// Purpose : Declaration and inline definitions for the TRP_RotateMaskKey 
//           class.
//
//           Inline methods include:
//           - SetOriginPoint, SetNodeIndex
//           - GetOriginPoint, GetNodeIndex
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

#ifndef TRP_ROTATE_MASK_KEY_H
#define TRP_ROTATE_MASK_KEY_H

#include <cstdio>
using namespace std;

#include "TGO_Point.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TRP_RotateMaskKey_c
{
public:

   TRP_RotateMaskKey_c( void );
   TRP_RotateMaskKey_c( const TGO_Point_c& originPoint, 
                        size_t nodeIndex ); 
   TRP_RotateMaskKey_c( int x, int y, int z,
                        size_t nodeIndex ); 
   TRP_RotateMaskKey_c( int x, int y,
                        size_t nodeIndex ); 
   TRP_RotateMaskKey_c( const TRP_RotateMaskKey_c& rotateMaskKey );
   ~TRP_RotateMaskKey_c( void );

   TRP_RotateMaskKey_c& operator=( const TRP_RotateMaskKey_c& rotateMaskKey );
   bool operator<( const TRP_RotateMaskKey_c& rotateMaskKey ) const;
   bool operator==( const TRP_RotateMaskKey_c& rotateMaskKey ) const;
   bool operator!=( const TRP_RotateMaskKey_c& rotateMaskKey ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetOriginPoint( const TGO_Point_c& originPoint );
   void SetNodeIndex( size_t nodeIndex );

   const TGO_Point_c& GetOriginPoint( void ) const;
   size_t GetNodeIndex( void ) const;

   bool IsValid( void ) const;

private:

   TGO_Point_c originPoint_;
   size_t      nodeIndex_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
inline void TRP_RotateMaskKey_c::SetOriginPoint( 
      const TGO_Point_c& originPoint )
{
   this->originPoint_ = originPoint;
} 

//===========================================================================//
inline void TRP_RotateMaskKey_c::SetNodeIndex( 
      size_t nodeIndex )
{
   this->nodeIndex_ = nodeIndex;
} 

//===========================================================================//
inline const TGO_Point_c& TRP_RotateMaskKey_c::GetOriginPoint( 
      void ) const
{
   return( this->originPoint_ );
}

//===========================================================================//
inline size_t TRP_RotateMaskKey_c::GetNodeIndex( 
      void ) const
{
   return( this->nodeIndex_ );
}

//===========================================================================//
inline bool TRP_RotateMaskKey_c::IsValid( 
      void ) const
{
   return( this->originPoint_.IsValid( ) && this->nodeIndex_ != SIZE_MAX ? true : false );
}

#endif
