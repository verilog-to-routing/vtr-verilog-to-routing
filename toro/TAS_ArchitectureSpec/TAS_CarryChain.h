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
