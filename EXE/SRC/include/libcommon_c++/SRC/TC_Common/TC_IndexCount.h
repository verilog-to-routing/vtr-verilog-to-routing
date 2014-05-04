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
