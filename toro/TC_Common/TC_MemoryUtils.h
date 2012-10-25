//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful memory functions.
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

#ifndef TC_MEMORY_UTILS_H
#define TC_MEMORY_UTILS_H

#include <cstdio>
using namespace std;

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//

void* TC_new( size_t allocLen, 
              const char* pszSource = 0 );

void* TC_calloc( size_t allocLen,
                 size_t allocSize,
                 const char* pszSource = 0 );

void* TC_realloc( void* pvoid,
                  size_t allocLen,
                  size_t allocSize,
                  const char* pszSource = 0 );

bool TC_IsValidAlloc( const void* pvoid, 
                      size_t allocLen,
                      const char* pszSource = 0 );

#endif
