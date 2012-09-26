//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_ExecuteOptions 
//           class.
//
//===========================================================================//

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
