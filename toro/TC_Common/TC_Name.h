//===========================================================================//
// Purpose : Declaration and inline definitions for a (regexp) TC_Name class.
//
//           Inline methods include:
//           - TC_Name, ~TC_Name
//           - SetValue
//           - GetValue
//           - GetLength
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// This program is free software; you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation; version 3 of the License, or any later version. //
//                                                                           //
// This program is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY //
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   //
// for more details.                                                         //
//                                                                           //
// You should have received a copy of the GNU General Public License along   //
// with this program; if not, see <http://www.gnu.org/licenses>.             //
//---------------------------------------------------------------------------//

#ifndef TC_NAME_H
#define TC_NAME_H

#include <cstdio>
#include <string>
using namespace std;

#include "TIO_Typedefs.h"

#include "TC_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TC_Name_c
{
public:

   TC_Name_c( const char* pszName = 0 );
   TC_Name_c( const string& srName );
   TC_Name_c( const char* pszName,
              int value );
   TC_Name_c( const string& srName,
              int value );
   TC_Name_c( const TC_Name_c& name );
   ~TC_Name_c( void );

   TC_Name_c& operator=( const TC_Name_c& name );
   bool operator<( const TC_Name_c& name ) const;
   bool operator==( const TC_Name_c& name ) const;
   bool operator!=( const TC_Name_c& name ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrName ) const;

   void SetName( const char* pszName );
   void SetName( const string& srName );
   void SetValue( int value );

   const string& GetString( void ) const;
   const char* GetCompare( void ) const;
   const char* GetName( void ) const;
   int GetValue( void ) const;
   size_t GetLength( void ) const;

   bool IsValid( void ) const;

private:

   string srName_;
   int    value_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline TC_Name_c::TC_Name_c( 
      const char* pszName )
      :
      srName_( TIO_PSZ_STR( pszName )),
      value_( 0 )
{
}

//===========================================================================//
inline TC_Name_c::TC_Name_c( 
      const string& srName )
      :
      srName_( srName ),
      value_( 0 )
{
}

//===========================================================================//
inline TC_Name_c::TC_Name_c( 
      const char*  pszName,
            int    value )
      :
      srName_( TIO_PSZ_STR( pszName )),
      value_( value )
{
}

//===========================================================================//
inline TC_Name_c::TC_Name_c( 
      const string& srName,
            int     value )
      :
      srName_( srName ),
      value_( value )
{
}

//===========================================================================//
inline TC_Name_c::TC_Name_c( 
      const TC_Name_c& name )
      :
      srName_( name.srName_ ),
      value_( name.value_ )
{
}

//===========================================================================//
inline TC_Name_c::~TC_Name_c( 
      void )
{
}

//===========================================================================//
inline void TC_Name_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline void TC_Name_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TC_Name_c::SetValue(
      int value ) 
{
   this->value_ = value;
}

//===========================================================================//
inline const string& TC_Name_c::GetString( 
      void ) const 
{
   return( this->srName_ );
}

//===========================================================================//
inline const char* TC_Name_c::GetName( 
      void ) const 
{
   return( this->srName_.length( ) ? this->srName_.data( ) : 0 );
}

//===========================================================================//
inline int TC_Name_c::GetValue( 
      void ) const 
{
   return( this->value_ );
}

//===========================================================================//
inline size_t TC_Name_c::GetLength( 
      void ) const 
{
   return( this->srName_.length( ));
}

//===========================================================================//
inline bool TC_Name_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif 
