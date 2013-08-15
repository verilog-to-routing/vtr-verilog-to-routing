//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_CarryChain class.
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

#ifndef TAS_CARRY_CHAIN_H
#define TAS_CARRY_CHAIN_H

#include <cstdio>
#include <string>
using namespace std;

#include "TGO_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/13 jeffr : Original
//===========================================================================//
class TAS_CarryChain_c
{
public:

   TAS_CarryChain_c( void );
   TAS_CarryChain_c( const string& srName );
   TAS_CarryChain_c( const char* pszName );
   TAS_CarryChain_c( const TAS_CarryChain_c& carryChain );
   ~TAS_CarryChain_c( void );

   TAS_CarryChain_c& operator=( const TAS_CarryChain_c& carryChain );
   bool operator==( const TAS_CarryChain_c& carryChain ) const;
   bool operator!=( const TAS_CarryChain_c& carryChain ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   string srName;          // Carry chain macro name

   string srFromPinName;   // Carry chain directives
   string srToPinName;     // "
   TGO_IntDims_t offset;   // "
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/13 jeffr : Original
//===========================================================================//
inline bool TAS_CarryChain_c::IsValid( 
      void ) const
{
   return( this->srName.length( ) ? true : false );
}

#endif
