//===========================================================================//
// Purpose : Declaration and inline definitions for TAP_ArchitectureInterface
//           abstract base class.
//
//           Inline methods include:
//           - TAP_ArchitectureInterface, ~TAP_ArchitectureInterface
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

#ifndef TAP_ARCHITECTURE_INTERFACE_H
#define TAP_ARCHITECTURE_INTERFACE_H

#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAP_ArchitectureInterface_c
{
public:

   TAP_ArchitectureInterface_c( void );
   virtual ~TAP_ArchitectureInterface_c( void );

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
inline TAP_ArchitectureInterface_c::TAP_ArchitectureInterface_c( 
      void )
{
}

//===========================================================================//
inline TAP_ArchitectureInterface_c::~TAP_ArchitectureInterface_c( 
      void )
{
}

#endif
