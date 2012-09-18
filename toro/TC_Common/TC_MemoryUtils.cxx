//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful memory functions.
//
//           Functions include:
//           - TC_new
//           - TC_calloc
//           - TC_realloc
//           - TC_IsValidAlloc
//
//===========================================================================//

#include <memory.h>

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
