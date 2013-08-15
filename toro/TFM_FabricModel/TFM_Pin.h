//===========================================================================//
// Purpose : Declaration and inline definitions for a TFM_Pin class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
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

#ifndef TFM_PIN_H
#define TFM_PIN_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_NameType.h"
#include "TC_Bit.h"

#include "TPO_Typedefs.h"

#include "TFM_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TFM_Pin_c
      :
      public TPO_Pin_t
{
public:

   TFM_Pin_c( void );
   TFM_Pin_c( const string& srName,
              TC_TypeMode_t type,
              TC_SideMode_t side,
              int offset,
              double delta,
              double width,
              unsigned int slice );
   TFM_Pin_c( const char* pszName,
              TC_TypeMode_t type,
              TC_SideMode_t side,
              int offset,
              double delta,
              double width,
              unsigned int slice );
   TFM_Pin_c( const TFM_Pin_c& pin );
   ~TFM_Pin_c( void );

   TFM_Pin_c& operator=( const TFM_Pin_c& pin );
   bool operator<( const TFM_Pin_c& pin ) const;
   bool operator==( const TFM_Pin_c& pin ) const;
   bool operator!=( const TFM_Pin_c& pin ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   bool IsValid( void ) const;

public:

   TC_TypeMode_t    type;      // Defines pin type (INPUT|OUTPUT|CLOCK)
   TC_SideMode_t    side;      // Defines pin side (LEFT|RIGHT|LOWER|UPPER)
   int              offset;    // Defines pin offset (index wrt LEFT|LOWER)
   double           delta;     // Defines pin delta (distance wrt LEFT|LOWER)
   double           width;     // Defines pin width (optional visualization)

   unsigned int     slice;     // Describes IO block slice (optional)

   TFM_BitPattern_t connectionPattern; 
                               // Describes connection box pattern (optional)
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TFM_Pin_c::SetName( 
      const string& srName )
{
   TPO_Pin_t::SetName( srName );
}

//===========================================================================//
inline void TFM_Pin_c::SetName( 
      const char* pszName )
{
   TPO_Pin_t::SetName( pszName );
}

//===========================================================================//
inline const char* TFM_Pin_c::GetName( 
      void ) const
{
   return( TPO_Pin_t::GetName( ));
}

//===========================================================================//
inline const char* TFM_Pin_c::GetCompare( 
      void ) const 
{
   return( TPO_Pin_t::GetCompare( ));
}

//===========================================================================//
inline bool TFM_Pin_c::IsValid( 
      void ) const
{
   return( TPO_Pin_t::IsValid( ));
}

#endif
