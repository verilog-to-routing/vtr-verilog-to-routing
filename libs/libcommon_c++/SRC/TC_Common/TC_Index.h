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
