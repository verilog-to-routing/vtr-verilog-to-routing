//===========================================================================//
// Purpose : Declaration and inline definitions for a TAP_ArchitectureFile 
//           class.
//
//           Inline methods include:
//           - IsValid
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

#ifndef TAP_ARCHITECTURE_FILE_H
#define TAP_ARCHITECTURE_FILE_H

#include <cstdio>
using namespace std;

#include "TAS_ArchitectureSpec.h"

#include "TAP_ArchitectureInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAP_ArchitectureFile_c
{
public:

   TAP_ArchitectureFile_c( FILE* pfile, 
                           const char* pszFileParserName,
                           TAP_ArchitectureInterface_c* parchitectureInterface,
                           TAS_ArchitectureSpec_c* parchitectureSpec );
   ~TAP_ArchitectureFile_c( void );

   bool SyntaxError( unsigned int lineNum, 
                     const string& srFileName,
                     const char* pszMessageText );

   bool IsValid( void ) const;

private:

   bool ok_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TAP_ArchitectureFile_c::IsValid( void ) const
{
   return( this->ok_ );
}

#endif
