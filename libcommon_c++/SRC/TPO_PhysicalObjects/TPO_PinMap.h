//===========================================================================//
// Purpose : Declaration and inline definitions for a TPO_PinMap class.
//
//           Inline methods include:
//           - GetInstPinName
//           - GetCellPinName
//           - GetType
//           - Set
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

#ifndef TPO_PIN_MAP_H
#define TPO_PIN_MAP_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Typedefs.h"

#include "TPO_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TPO_PinMap_c
{
public:

   TPO_PinMap_c( void );
   TPO_PinMap_c( const string& srInstPinName,
                 const string& srCellPinName,
                 TC_TypeMode_t type = TC_TYPE_UNDEFINED );
   TPO_PinMap_c( const char* pszInstPinName,
                 const char* pszCellPinName,
                 TC_TypeMode_t type = TC_TYPE_UNDEFINED );
   TPO_PinMap_c( const TPO_PinMap_c& pinMap );
   ~TPO_PinMap_c( void );

   TPO_PinMap_c& operator=( const TPO_PinMap_c& pinMap );
   bool operator<( const TPO_PinMap_c& pinMap ) const;
   bool operator==( const TPO_PinMap_c& pinMap ) const;
   bool operator!=( const TPO_PinMap_c& pinMap ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   const char* GetInstPinName( void ) const;
   const char* GetCellPinName( void ) const;
   TC_TypeMode_t GetType( void ) const;

   void Set( const string& srInstPinName,
             const string& srCellPinName,
             TC_TypeMode_t type = TC_TYPE_UNDEFINED );
   void Set( const char* pszInstPinName,
             const char* pszCellPinName,
             TC_TypeMode_t type = TC_TYPE_UNDEFINED );

   bool IsValid( void ) const;

private:

   string        srInstPinName_;
   string        srCellPinName_;
   TC_TypeMode_t type_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline const char* TPO_PinMap_c::GetInstPinName( 
      void ) const
{
   return( TIO_SR_STR( this->srInstPinName_ ));
}

//===========================================================================//
inline const char* TPO_PinMap_c::GetCellPinName( 
      void ) const
{
   return( TIO_SR_STR( this->srCellPinName_ ));
}

//===========================================================================//
inline TC_TypeMode_t TPO_PinMap_c::GetType( 
      void ) const
{
   return( this->type_ );
}

//===========================================================================//
inline bool TPO_PinMap_c::IsValid( 
      void ) const
{
   return( this->srInstPinName_.length( ) ? true : false );
}

#endif
