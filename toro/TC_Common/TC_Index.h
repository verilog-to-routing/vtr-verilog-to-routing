//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_Index class.
//
//           Inline methods include:
//           - GetIndex
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

#ifndef TC_INDEX_H
#define TC_INDEX_H

#include <cstdio>
#include <climits>
#include <cstring>
#include <string>
using namespace std;

#include "TC_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
class TC_Index_c
{
public:

   TC_Index_c( void );
   TC_Index_c( int index );
   TC_Index_c( const TC_Index_c& index );
   ~TC_Index_c( void );

   TC_Index_c& operator=( const TC_Index_c& index );
   bool operator<( const TC_Index_c& index ) const;
   bool operator==( const TC_Index_c& index ) const;
   bool operator!=( const TC_Index_c& index ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrIndex ) const;

   int GetIndex( void ) const;

   void Set( int index );
   void Clear( void );

   bool IsValid( void ) const;

private:

   int index_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
inline int TC_Index_c::GetIndex( 
      void ) const
{
   return( this->index_ );
}

//===========================================================================//
inline bool TC_Index_c::IsValid( 
      void ) const
{
   return( this->index_ != INT_MAX ? true : false );
}

#endif
