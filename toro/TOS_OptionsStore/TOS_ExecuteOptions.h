//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_ExecuteOptions 
//           class.
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

   class TOS_ExecuteTI_Clay_c
   {
   public:

      bool resyncNets;            // TI Clay-specific - disables net list resync
      bool freeNets;              // TI Clay-specific - disables net list free
   } tiClay;
};

#endif 
