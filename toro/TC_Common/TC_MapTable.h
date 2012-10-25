//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_MapTable class.
//
//           Inline methods include:
//           - IsValid
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

#ifndef TC_SIDE_TABLE_H
#define TC_SIDE_TABLE_H

#include <cstdio>
using namespace std;

#include "TC_Typedefs.h"
#include "TC_SideIndex.h"

#include "TCT_SortedVector.h"
#include "TCT_RangeOrderedVector.h"

//---------------------------------------------------------------------------//
// Define side table typedefs
//---------------------------------------------------------------------------//

typedef TCT_SortedVector_c< TC_SideIndex_c > TC_SideList_t;
typedef TCT_RangeOrderedVector_c< TC_SideList_t > TC_MapSideList_t;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TC_MapTable_c
{
public:

   TC_MapTable_c( void );
   TC_MapTable_c( const TC_MapTable_c& mapTable );
   ~TC_MapTable_c( void );

   TC_MapTable_c& operator=( const TC_MapTable_c& mapTable );
   bool operator==( const TC_MapTable_c& mapTable ) const;
   bool operator!=( const TC_MapTable_c& mapTable ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;
   void Print( FILE* pfile, size_t spaceLen,
               TC_SideMode_t side,
               const TC_MapSideList_t& mapSideList ) const;

   void Init( size_t widthHorz,
              size_t widthVert );
   void Init( size_t widthLeft,
              size_t widthRight,
              size_t widthLower,
              size_t widthUpper );
   void Init( size_t width,
              TC_MapSideList_t* pmapSideList );

   bool Add( TC_SideMode_t side, size_t index, 
             TC_SideMode_t side_, size_t index_ );
   bool Add( TC_SideMode_t side, size_t index, 
             const TC_SideIndex_c& sideIndex );
   bool Add( TC_SideMode_t side, size_t index, 
             const TC_SideList_t& sideList );
   bool Add( TC_SideMode_t side,
             const TC_MapSideList_t& mapSideList );
   bool Add( const TC_MapTable_c& mapTable );

   const TC_MapSideList_t* FindMapSideList( TC_SideMode_t side ) const;
   const TC_SideList_t* FindSideList( TC_SideMode_t side, size_t index ) const;

   bool IsValid( void ) const;

private:

   TC_MapSideList_t* FindMapSideList_( TC_SideMode_t side );
   TC_SideList_t* FindSideList_( TC_SideMode_t side, size_t index );

private:

   TC_MapSideList_t leftSideList_;
   TC_MapSideList_t rightSideList_;
   TC_MapSideList_t lowerSideList_;
   TC_MapSideList_t upperSideList_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
inline bool TC_MapTable_c::IsValid( 
      void ) const
{
   return( this->leftSideList_.IsValid( ) ||
           this->rightSideList_.IsValid( ) ||
           this->lowerSideList_.IsValid( ) ||
           this->upperSideList_.IsValid( ) ?
           true : false );
}

#endif
