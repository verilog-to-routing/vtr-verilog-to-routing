//===========================================================================//
// Purpose : Definition for the 'TAXP_ArchitectureXmlHandler_c' class.
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

#ifndef TAXP_ARCHITECTURE_XML_HANDLER_H
#define TAXP_ARCHITECTURE_XML_HANDLER_H

#include "TAS_ArchitectureSpec.h"

#include "TAXP_ArchitectureXmlInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAXP_ArchitectureXmlHandler_c : public TAXP_ArchitectureXmlInterface_c
{
public:

   TAXP_ArchitectureXmlHandler_c( TAS_ArchitectureSpec_c* parchitectureSpec );
   ~TAXP_ArchitectureXmlHandler_c( void );

   bool Init( TAS_ArchitectureSpec_c* parchitectureSpec );

   bool SyntaxError( unsigned int lineNum, 
                     const string& srFileName,
                     const char* pszMessageText );

   bool IsValid( void ) const;

private:

   TAS_ArchitectureSpec_c* parchitectureSpec_;
};

#endif
