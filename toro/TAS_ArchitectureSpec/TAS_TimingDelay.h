//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_TimingDelay class.
//
//           Inline methods include:
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

#ifndef TAS_TIMING_DELAY_H
#define TAS_TIMING_DELAY_H

#include <cstdio>
#include <string>
using namespace std;

#include "TAS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAS_TimingDelay_c
{
public:

   TAS_TimingDelay_c( void );
   TAS_TimingDelay_c( const TAS_TimingDelay_c& timingDelay );
   ~TAS_TimingDelay_c( void );

   TAS_TimingDelay_c& operator=( const TAS_TimingDelay_c& timingDelay );
   bool operator==( const TAS_TimingDelay_c& timingDelay ) const;
   bool operator!=( const TAS_TimingDelay_c& timingDelay ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   TAS_TimingMode_t mode; // Selects delay type: CONSTANT|MATRIX|etc.
   TAS_TimingType_t type; // Selects delay type: MIN|MAX

   double valueMin;
   double valueMax;
   double valueNom;
   TAS_TimingValueMatrix_t valueMatrix;

   string srInputPortName;
   string srOutputPortName;
   string srClockPortName;
   string srPackPatternName;

private:

   enum TAS_DefCapacity_e 
   { 
      TAS_TIMING_VALUE_MATRIX_DEF_CAPACITY = 0
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TAS_TimingDelay_c::IsValid( 
      void ) const
{
   return( this->mode != TAS_TIMING_MODE_UNDEFINED ? true : false );
}

#endif
