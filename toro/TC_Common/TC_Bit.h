//===========================================================================//
// Purpose : Declaration and inline definitions for a (regexp) TC_Bit class.
//
//           Inline methods include:
//           - IsTrue
//           - IsFalse
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

#ifndef TC_BIT_H
#define TC_BIT_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TC_Bit_c
{
public:

   TC_Bit_c( TC_BitMode_t value = TC_BIT_UNDEFINED );
   TC_Bit_c( char value );
   TC_Bit_c( const TC_Bit_c& bit );
   ~TC_Bit_c( void );

   TC_Bit_c& operator=( const TC_Bit_c& value );
   bool operator==( const TC_Bit_c& value ) const;
   bool operator!=( const TC_Bit_c& value ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrValue ) const;

   TC_BitMode_t GetValue( void ) const;

   void SetValue( TC_BitMode_t value );
   void SetValue( char value );

   bool IsTrue( void ) const;
   bool IsFalse( void ) const;

   bool IsValid( void ) const;

private:

   char value_; // States include 'true', 'false', 'don't care', and 'undefined'
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TC_Bit_c::IsTrue( 
      void ) const
{
   return( this->value_ == TC_BIT_TRUE ? true : false );
}

//===========================================================================//
inline bool TC_Bit_c::IsFalse( 
      void ) const
{
   return( this->value_ == TC_BIT_FALSE ? true : false );
}

//===========================================================================//
inline bool TC_Bit_c::IsValid( 
      void ) const
{
   return( this->value_ != TC_BIT_UNDEFINED ? true : false );
}

#endif 
