//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful memory functions.
//
//           Functions include:
//           - TC_new
//           - TC_calloc
//           - TC_realloc
//           - TC_Alloc
//           - TC_IsValidAlloc
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

#include <memory>
using namespace std;

#include "TIO_PrintHandler.h"

#include "TC_Typedefs.h"
#include "TC_MemoryUtils.h"

//===========================================================================//
// Function       : TC_new
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void* TC_New( 
            size_t allocLen,
      const char*  pszSource )
{
   void* pvoid = 0;

   if( allocLen )
   {
      pvoid = new TC_NOTHROW char[allocLen];
   }
   TC_IsValidAlloc( pvoid, allocLen, pszSource );
   return( pvoid );
}

//===========================================================================//
// Function       : TC_calloc
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void* TC_calloc( 
            size_t allocLen,
	    size_t allocSize,
      const char*  pszSource )
{
   void* pvoid = 0;

   if( allocLen )
   {
      pvoid = calloc( allocLen, allocSize );
   }
   TC_IsValidAlloc( pvoid, allocLen, pszSource );
   return( pvoid );
}

//===========================================================================//
// Function       : TC_realloc
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void* TC_realloc( 
	    void*  pvoid,
            size_t allocLen,
	    size_t allocSize,
      const char*  pszSource )
{
   if( allocLen * allocSize )
   {
      pvoid = realloc( pvoid, allocLen * allocSize );
   }
   TC_IsValidAlloc( pvoid, allocLen, pszSource );
   return( pvoid );
}

//===========================================================================//
// Function       : TC_IsValidAlloc
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TC_IsValidAlloc( 
      const void*  pvoid, 
            size_t allocLen,
      const char*  pszSource )
{
   bool ok = ( pvoid ? true : false ); 

   if( !ok && pszSource )
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      printHandler.IsValidNew( pvoid, allocLen, pszSource );
   }
   return( ok );
}
