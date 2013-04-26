//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful string functions.
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

#ifndef TNO_STRING_UTILS_H
#define TNO_STRING_UTILS_H

#include <string>
using namespace std;

#include "TNO_Typedefs.h"

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_ExtractStringStatusMode( TNO_StatusMode_t mode, string* psrMode );
void TNO_ExtractStringNodeType( TNO_NodeType_t type, string* psrType );

void TNO_FormatNameIndex( const char* pszName, 
                          size_t index,
                          string* psrNameIndex );
void TNO_FormatNameIndex( const string& srName,
                          size_t index,
                          string* psrNameIndex );

void TNO_ParseNameIndex( const char* pszNameIndex,
                         string* psrName,
                         size_t* pindex );
void TNO_ParseNameIndex( const string& srNameIndex,
                         string* psrName,
                         size_t* pindex );
#endif 
