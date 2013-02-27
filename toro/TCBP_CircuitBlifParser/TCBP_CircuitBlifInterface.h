//===========================================================================//
// Purpose : Declaration and inline definitions for TCBP_CircuitBlifInterface
//           abstract base class.
//
//           Inline methods include:
//           - TCBP_CircuitBlifInterface, ~TCBP_CircuitBlifInterface
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
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

#ifndef TCBP_CIRCUIT_BLIF_INTERFACE_H
#define TCBP_CIRCUIT_BLIF_INTERFACE_H

#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TCBP_CircuitBlifInterface_c
{
public:

   TCBP_CircuitBlifInterface_c( void );
   virtual ~TCBP_CircuitBlifInterface_c( void );

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
// 05/15/12 jeffr : Original
//===========================================================================//
inline TCBP_CircuitBlifInterface_c::TCBP_CircuitBlifInterface_c( 
      void )
{
}

//===========================================================================//
inline TCBP_CircuitBlifInterface_c::~TCBP_CircuitBlifInterface_c( 
      void )
{
}

#endif
