//===========================================================================//
// Purpose : Declaration and inline definitions for the TCH_RotateMaskValue 
//           class.
//
//           Inline methods include:
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

#ifndef TCH_ROTATE_MASK_VALUE_H
#define TCH_ROTATE_MASK_VALUE_H

#include <cstdio>
using namespace std;

#include "TGO_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TCH_RotateMaskValue_c
{
public:

   TCH_RotateMaskValue_c( void );
   TCH_RotateMaskValue_c( bool rotateEnabled );
   TCH_RotateMaskValue_c( const TCH_RotateMaskValue_c& rotateMaskValue );
   ~TCH_RotateMaskValue_c( void );

   TCH_RotateMaskValue_c& operator=( const TCH_RotateMaskValue_c& rotateMaskValue );
   bool operator==( const TCH_RotateMaskValue_c& rotateMaskValue ) const;
   bool operator!=( const TCH_RotateMaskValue_c& rotateMaskValue ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void Init( bool rotateEnabled );
   bool GetBit( TGO_RotateMode_t rotateMode ) const;
   void SetBit( TGO_RotateMode_t rotateMode );
   void ClearBit( TGO_RotateMode_t rotateMode );

   bool IsValid( void ) const;

private:

   unsigned int bitMask_;

private:

   enum TCH_RotateMask_e 
   { 
      TCH_ROTATE_MASK_UNDEFINED = 0x00,
      TCH_ROTATE_MASK_R0        = 0x01,
      TCH_ROTATE_MASK_R90       = 0x02,
      TCH_ROTATE_MASK_R180      = 0x04,
      TCH_ROTATE_MASK_R270      = 0x08,
      TCH_ROTATE_MASK_MX        = 0x10,
      TCH_ROTATE_MASK_MXR90     = 0x20,
      TCH_ROTATE_MASK_MY        = 0x40,
      TCH_ROTATE_MASK_MYR90     = 0x80,
      TCH_ROTATE_MASK_ALL       = 0xFF
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
inline bool TCH_RotateMaskValue_c::IsValid( 
      void ) const
{
   return( this->bitMask_ != TCH_ROTATE_MASK_UNDEFINED ? true : false );
}

#endif
