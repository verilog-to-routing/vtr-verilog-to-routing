//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_Power class.
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

#ifndef TAS_POWER_H
#define TAS_POWER_H

#include <cstdio>
using namespace std;

#include "TLO_Typedefs.h"
#include "TLO_Port.h"

#include "TAS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
class TAS_Power_c
{
public:

   TAS_Power_c( void );
   TAS_Power_c( const TAS_Power_c& power );
   ~TAS_Power_c( void );

   TAS_Power_c& operator=( const TAS_Power_c& power );
   bool operator==( const TAS_Power_c& power ) const;
   bool operator!=( const TAS_Power_c& power ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   TAS_PowerMethodMode_t estimateMethod;
                            // Power estimation method for a physical block
   class TAS_StaticPower_c 
   {
   public:

      double absolute;      // User-provided absolute power per block
                            // Required for TOGGLE_PINS, C_INTERNAL, and ABSOLUTE modes
   } staticPower;

   class TAS_DynamicPower_c
   {
   public:

      double absolute;      // User-provided absolute power per block
                            // Required for ABSOLUTE mode
      double capInternal;   // Required for C_INTERNAL mode
   } dynamicPower;

   TLO_PortList_t portList; // Defines optional port power (pin toggle) list

private:

   enum TAS_DefCapacity_e 
   { 
      TAS_POWER_PORT_LIST_DEF_CAPACITY = 1
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
inline bool TAS_Power_c::IsValid( 
      void ) const
{
   return( this->estimateMethod != TAS_POWER_METHOD_UNDEFINED ? true : false );
}

#endif
