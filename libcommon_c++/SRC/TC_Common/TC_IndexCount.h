//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_IndexCount class.
//
//           Inline methods include:
//           - GetIndex, GetCount
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

#ifndef TC_INDEX_COUNT_H
#define TC_INDEX_COUNT_H

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
// 10/05/12 jeffr : Original
//===========================================================================//
class TC_IndexCount_c
{
public:

   TC_IndexCount_c( void );
   TC_IndexCount_c( int index,
                    size_t count = SIZE_MAX );
   TC_IndexCount_c( const TC_IndexCount_c& indexCount );
   ~TC_IndexCount_c( void );

   TC_IndexCount_c& operator=( const TC_IndexCount_c& indexCount );
   bool operator<( const TC_IndexCount_c& indexCount ) const;
   bool operator==( const TC_IndexCount_c& indexCount ) const;
   bool operator!=( const TC_IndexCount_c& indexCount ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrIndexCount ) const;

   int GetIndex( void ) const;
   size_t GetCount( void ) const;

   void Set( int index,
             size_t count = SIZE_MAX );
   void Clear( void );

   bool IsValid( void ) const;

private:

   int    index_;
   size_t count_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
inline int TC_IndexCount_c::GetIndex( 
      void ) const
{
   return( this->index_ );
}

//===========================================================================//
inline size_t TC_IndexCount_c::GetCount( 
      void ) const
{
   return( this->count_ );
}

//===========================================================================//
inline bool TC_IndexCount_c::IsValid( 
      void ) const
{
   return( this->index_ != INT_MAX ? true : false );
}

#endif
