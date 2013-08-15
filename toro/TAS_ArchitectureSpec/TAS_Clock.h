//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_Clock class.
//
//           Inline methods include:
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

#ifndef TAS_CLOCK_H
#define TAS_CLOCK_H

#include <cstdio>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
class TAS_Clock_c
{
public:

   TAS_Clock_c( void );
   TAS_Clock_c( const TAS_Clock_c& clock );
   ~TAS_Clock_c( void );

   TAS_Clock_c& operator=( const TAS_Clock_c& clock );
   bool operator==( const TAS_Clock_c& clock ) const;
   bool operator!=( const TAS_Clock_c& clock ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   bool   autoSize;     // TRUE => auto size clock buffers
   double bufferSize;   // User-defined clock buffer size
   double capWire;      // User-defined wire capacitance
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
inline bool TAS_Clock_c::IsValid( 
      void ) const
{
   return(( this->autoSize ) || 
          ( TCTF_IsGT( this->bufferSize, 0.0 )) ||
          ( TCTF_IsGT( this->capWire, 0.0 )) ?
          true : false );
}

#endif
