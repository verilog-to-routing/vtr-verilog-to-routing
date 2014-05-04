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
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
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
