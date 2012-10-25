//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_ExecuteOptions 
//           class.
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

#ifndef TOS_EXECUTE_OPTIONS_H
#define TOS_EXECUTE_OPTIONS_H

#include <cstdio>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOS_ExecuteOptions_c
{
public:

   TOS_ExecuteOptions_c( void );
   TOS_ExecuteOptions_c( unsigned long maxWarningCount,
                         unsigned long maxErrorCount,
                         int toolMask );
   ~TOS_ExecuteOptions_c( void );

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

public:

   unsigned long maxWarningCount; // Max #of warnings before (graceful) abort
   unsigned long maxErrorCount;   // Max #of errors before (graceful) abort

   int toolMask;                  // Tool mode: none|pack|place|route|all
};

#endif 
