//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_MapTable class.
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
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

   bool IsLegal( void ) const;
   bool IsLegal( const TC_MapSideList_t& mapSideList ) const;

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

#endif
