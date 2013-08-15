//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_PinAssign class.
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

#ifndef TAS_PIN_ASSIGN_H
#define TAS_PIN_ASSIGN_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Typedefs.h"

#include "TAS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAS_PinAssign_c
{
public:

   TAS_PinAssign_c( void );
   TAS_PinAssign_c( const TAS_PinAssign_c& pinAssign );
   ~TAS_PinAssign_c( void );

   TAS_PinAssign_c& operator=( const TAS_PinAssign_c& pinAssign );
   bool operator==( const TAS_PinAssign_c& pinAssign ) const;
   bool operator!=( const TAS_PinAssign_c& pinAssign ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   TAS_PinAssignPatternType_t pattern;
                        // Selects assignment pattern (ie. SPREAD|CUSTOM)
   TAS_PortNameList_t portNameList;
                        // Defines list of 1+ port names
                        // (based on "block_name.port_name[n:m]" syntax)
   TC_SideMode_t side;  // Selects block side (ie. LEFT|RIGHT|LOWER|UPPER)
   unsigned int offset; // Defines block offset (for IOs)

private:

   enum TAS_DefCapacity_e 
   { 
      TAS_PORT_NAME_LIST_DEF_CAPACITY = 1
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TAS_PinAssign_c::IsValid( 
      void ) const
{
   return( this->pattern != TAS_PIN_ASSIGN_PATTERN_UNDEFINED ? true : false );
}

#endif
