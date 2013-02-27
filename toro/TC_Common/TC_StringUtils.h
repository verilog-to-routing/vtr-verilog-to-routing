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

#ifndef TC_STRING_UTILS_H
#define TC_STRING_UTILS_H

#include <string>
using namespace std;

#include "TC_Typedefs.h"

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
int TC_CompareStrings( const char* pszStringA, const char* pszStringB );
int TC_CompareStrings( const string& srStringA, const string& srStringB );
int TC_CompareStrings( const void* pvoidA, const void* pvoidB );

bool TC_FormatStringCentered( const char* pszString,
                              size_t lenRefer,
                              char* pszCentered,
                              size_t lenCentered );
bool TC_FormatStringFilled( char cFill,
                            char* pszFilled,
                            size_t lenFilled );

bool TC_FormatStringDateTimeStamp( char* pszDateTimeStamp, 
                                   size_t lenDateTimeStamp,
                                   const char* pszPrefix = 0,
                                   const char* pszPostfix = 0 );

void TC_ExtractStringSideMode( TC_SideMode_t sideMode, string* psrSideMode );
void TC_ExtractStringTypeMode( TC_TypeMode_t typeMode, string* psrTypeMode );

bool TC_AddStringPrefix( char* pszString, const char* pszPrefix );
bool TC_AddStringPostfix( char* pszString, const char* pszPostfix );

int TC_stricmp( const char* pszStringA, const char* pszStringB );
int TC_stricmp( const string& srStringA, const string& srStringB );
int TC_strnicmp( const char* pszStringA, const char* pszStringB, int i );
int TC_strnicmp( const string& srStringA, const string& srStringB, int i );

char* TC_strdup( const char* pszString );
char* TC_strdup( const string& srString );

#endif
