//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful memory functions.
//
//===========================================================================//

#include <stdio.h>

#ifndef TC_MEMORY_UTILS_H
#define TC_MEMORY_UTILS_H

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
