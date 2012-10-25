//===========================================================================//
// Purpose : Declaration and inline definitions for TCP_CircuitInterface
//           abstract base class.
//
//           Inline methods include:
//           - TCP_CircuitInterface, ~TCP_CircuitInterface
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TCP_CIRCUIT_INTERFACE_H
#define TCP_CIRCUIT_INTERFACE_H

#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
class TCP_CircuitInterface_c
{
public:

   TCP_CircuitInterface_c( void );
   virtual ~TCP_CircuitInterface_c( void );

   virtual bool SyntaxError( unsigned int lineNum, 
                             const string& srFileName,
			     const char* pszMessageText ) = 0;

   virtual bool IsValid( void ) const = 0;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
inline TCP_CircuitInterface_c::TCP_CircuitInterface_c( 
      void )
{
}

//===========================================================================//
inline TCP_CircuitInterface_c::~TCP_CircuitInterface_c( 
      void )
{
}

#endif
