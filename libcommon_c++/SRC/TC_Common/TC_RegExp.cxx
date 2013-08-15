//===========================================================================//
// Purpose : Method definitions for the TC_RegExp class.
//
//           Public methods include:
//           - TC_RegExp, ~TC_RegExp
//           - Index
//           - Match
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

#include <cstring>
#include <string>
using namespace std;

#include "TC_RegExp.h"

//===========================================================================//
// Method          : TC_RegExp
// Author          : Jon Sykes
//---------------------------------------------------------------------------//
// Version history
// 04/22/03 jsykes : Original
// 07/10/12 jeffr  : Ported from acl to TC_* common library
//===========================================================================//
TC_RegExp::TC_RegExp( 
      const char* pszExpression, 
            int   pcreOptions )
      :
      pszError( 0 ),
      errorOffset( 0 ),
      pszMatch_( 0 ),
      ppcreCode_( 0 ),
      pcreOptions_( pcreOptions )
{
   this->ppcreCode_ = pcre_compile( pszExpression, 
                                    this->pcreOptions_,
                                    &this->pszError, 
                                    &this->errorOffset, 0 );
}

//===========================================================================//
// Method          : ~TC_RegExp
// Author          : Jon Sykes
//---------------------------------------------------------------------------//
// Version history
// 04/22/03 jsykes : Original
// 07/10/12 jeffr  : Ported from acl to TC_* common library
//===========================================================================//
TC_RegExp::~TC_RegExp( 
      void )
{
   if( this->ppcreCode_ )
   {
      pcre_free( this->ppcreCode_ );
   }
}

//===========================================================================//
// Method          : Index
// Author          : Jon Sykes
//---------------------------------------------------------------------------//
// Version history
// 04/22/03 jsykes : Original
// 07/10/12 jeffr  : Ported from acl to TC_* common library
//===========================================================================//
bool TC_RegExp::Index(
      const char*   pszSubject, 
            size_t* pstart, 
            size_t* plen ) const
{
   bool foundMatch = false;

   if( this->ppcreCode_ && pszSubject && pstart && plen )
   {
      int subjectLen = static_cast< int >( strlen( pszSubject ));
      int offsetCount = 0;

      pcre_fullinfo( this->ppcreCode_, 0, 
                     PCRE_INFO_CAPTURECOUNT, &offsetCount );

      offsetCount = 3 * ( offsetCount + 1 );
      int* offsetArray = new int[offsetCount];

      int rc = pcre_exec( this->ppcreCode_, 0, 
                          pszSubject, subjectLen, 0,
                          this->pcreOptions_, 
                          offsetArray, offsetCount );
      if( rc >= 0 )
      {
         *pstart = offsetArray[0];
         *plen   = offsetArray[1] - offsetArray[0];

         foundMatch = true;
      }
      delete[] offsetArray;
   }
   return( foundMatch );
}

//===========================================================================//
// Method          : Match
// Author          : Jon Sykes
//---------------------------------------------------------------------------//
// Version history
// 04/22/03 jsykes : Original
// 07/10/12 jeffr  : Ported from acl to TC_* common library
//===========================================================================//
const char* TC_RegExp::Match( 
      const char* pszSubject )
{
   const char* pszMatch = 0;

   size_t start, len;
   if( this->Index( pszSubject, &start, &len ))
   {
      if( this->pszMatch_ )
      {
         delete[] this->pszMatch_;
      }
      this->pszMatch_ = new char[len + 1];

      strncpy( this->pszMatch_, &pszSubject[start], len );
      this->pszMatch_[len] = 0;
      
      pszMatch = this->pszMatch_;
   }
   return( pszMatch );
}
