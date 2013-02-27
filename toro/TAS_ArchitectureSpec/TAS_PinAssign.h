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
