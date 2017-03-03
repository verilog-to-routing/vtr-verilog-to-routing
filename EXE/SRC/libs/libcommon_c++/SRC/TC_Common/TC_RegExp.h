//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_RegExp class.
//
//           Inline methods include:
//           - IsValid
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

#ifndef TC_REGEXP_H
#define TC_REGEXP_H

#include "pcre.h"

class TC_RegExp
{
public:

   TC_RegExp( const char* pszExpression, 
              int pcreOptions = 0 );
   ~TC_RegExp( void );

   bool Index( const char* pszSubject, 
               size_t* pstart, 
               size_t* plen ) const;
   const char* Match( const char* pszSubject );

   bool IsValid( void ) const;

public:

   const char* pszError;
   int         errorOffset;

private:

   char* pszMatch_;
   pcre* ppcreCode_;
   int   pcreOptions_;
};

//===========================================================================//
// Purpose:        : Class inline definition(s)
// Author          : Jon Sykes
//---------------------------------------------------------------------------//
// Version history
// 04/22/03 jsykes : Original
// 07/10/12 jeffr  : Ported from acl to TC_* common library
//===========================================================================//
inline bool TC_RegExp::IsValid(
      void ) const
{
   return( this->ppcreCode_ ? true : false );
}

#endif
