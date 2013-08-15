//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_NameLength class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetLength
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

#ifndef TC_NAME_LENGTH_H
#define TC_NAME_LENGTH_H

#include <cstdio>
#include <climits>
#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TC_NameLength_c
{
public:

   TC_NameLength_c( void );
   TC_NameLength_c( const string& srName,
                    unsigned int length = UINT_MAX );
   TC_NameLength_c( const char* pszName,
                    unsigned int length = UINT_MAX );
   TC_NameLength_c( const TC_NameLength_c& nameLength );
   ~TC_NameLength_c( void );

   TC_NameLength_c& operator=( const TC_NameLength_c& nameLength );
   bool operator<( const TC_NameLength_c& nameLength ) const;
   bool operator==( const TC_NameLength_c& nameLength ) const;
   bool operator!=( const TC_NameLength_c& nameLength ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrNameLength ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   unsigned int GetLength( void ) const;

   void Set( const string& srName,
             unsigned int length = UINT_MAX );
   void Set( const char* pszName,
             unsigned int length = UINT_MAX );
   void Clear( void );

   bool IsValid( void ) const;

private:

   string       srName_;
   unsigned int length_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TC_NameLength_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TC_NameLength_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TC_NameLength_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TC_NameLength_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline unsigned int TC_NameLength_c::GetLength( 
      void ) const
{
   return( this->length_ );
}

//===========================================================================//
inline bool TC_NameLength_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
