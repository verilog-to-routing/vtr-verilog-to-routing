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
