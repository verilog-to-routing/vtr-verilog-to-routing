//===========================================================================//
// Purpose : Declaration and inline definitions for TAP_ArchitectureInterface
//           abstract base class.
//
//           Inline methods include:
//           - TAP_ArchitectureInterface, ~TAP_ArchitectureInterface
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

#ifndef TAP_ARCHITECTURE_INTERFACE_H
#define TAP_ARCHITECTURE_INTERFACE_H

#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAP_ArchitectureInterface_c
{
public:

   TAP_ArchitectureInterface_c( void );
   virtual ~TAP_ArchitectureInterface_c( void );

   virtual bool SyntaxError( unsigned int lineNum, 
                             const string& srFileName,
                             const char* pszMessageText ) = 0;

   virtual bool IsValid( void ) const = 0;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline TAP_ArchitectureInterface_c::TAP_ArchitectureInterface_c( 
      void )
{
}

//===========================================================================//
inline TAP_ArchitectureInterface_c::~TAP_ArchitectureInterface_c( 
      void )
{
}

#endif
