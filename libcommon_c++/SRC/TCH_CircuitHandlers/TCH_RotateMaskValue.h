//===========================================================================//
// Purpose : Declaration and inline definitions for the TCH_RotateMaskValue 
//           class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TCH_ROTATE_MASK_VALUE_H
#define TCH_ROTATE_MASK_VALUE_H

#include <cstdio>
using namespace std;

#include "TGO_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TCH_RotateMaskValue_c
{
public:

   TCH_RotateMaskValue_c( void );
   TCH_RotateMaskValue_c( bool rotateEnabled );
   TCH_RotateMaskValue_c( const TCH_RotateMaskValue_c& rotateMaskValue );
   ~TCH_RotateMaskValue_c( void );

   TCH_RotateMaskValue_c& operator=( const TCH_RotateMaskValue_c& rotateMaskValue );
   bool operator==( const TCH_RotateMaskValue_c& rotateMaskValue ) const;
   bool operator!=( const TCH_RotateMaskValue_c& rotateMaskValue ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void Init( bool rotateEnabled );
   bool GetBit( TGO_RotateMode_t rotateMode ) const;
   void SetBit( TGO_RotateMode_t rotateMode );
   void ClearBit( TGO_RotateMode_t rotateMode );

   bool IsValid( void ) const;

private:

   unsigned int bitMask_;

private:

   enum TCH_RotateMask_e 
   { 
      TCH_ROTATE_MASK_UNDEFINED = 0x00,
      TCH_ROTATE_MASK_R0        = 0x01,
      TCH_ROTATE_MASK_R90       = 0x02,
      TCH_ROTATE_MASK_R180      = 0x04,
      TCH_ROTATE_MASK_R270      = 0x08,
      TCH_ROTATE_MASK_MX        = 0x10,
      TCH_ROTATE_MASK_MXR90     = 0x20,
      TCH_ROTATE_MASK_MY        = 0x40,
      TCH_ROTATE_MASK_MYR90     = 0x80,
      TCH_ROTATE_MASK_ALL       = 0xFF
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
inline bool TCH_RotateMaskValue_c::IsValid( 
      void ) const
{
   return( this->bitMask_ != TCH_ROTATE_MASK_UNDEFINED ? true : false );
}

#endif
