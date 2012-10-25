//===========================================================================//
// Purpose : Declaration and inline definitions for the 
//           TAXP_ArchitectureXmlInterface abstract base class.
//
//           Inline methods include:
//           - TAXP_ArchitectureXmlInterface, ~TAXP_ArchitectureXmlInterface
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

#ifndef TAXP_ARCHITECTURE_XML_INTERFACE_H
#define TAXP_ARCHITECTURE_XML_INTERFACE_H

#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAXP_ArchitectureXmlInterface_c
{
public:

   TAXP_ArchitectureXmlInterface_c( void );
   virtual ~TAXP_ArchitectureXmlInterface_c( void );

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
inline TAXP_ArchitectureXmlInterface_c::TAXP_ArchitectureXmlInterface_c( 
      void )
{
}

//===========================================================================//
inline TAXP_ArchitectureXmlInterface_c::~TAXP_ArchitectureXmlInterface_c( 
      void )
{
}

#endif
