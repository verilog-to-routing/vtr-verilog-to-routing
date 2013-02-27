//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_NameIndex class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetIndex
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

#ifndef TC_NAME_INDEX_H
#define TC_NAME_INDEX_H

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
class TC_NameIndex_c
{
public:

   TC_NameIndex_c( void );
   TC_NameIndex_c( const string& srName,
  	           size_t index = SIZE_MAX );
   TC_NameIndex_c( const char* pszName,
  	           size_t index = SIZE_MAX );
   TC_NameIndex_c( const TC_NameIndex_c& nameIndex );
   ~TC_NameIndex_c( void );

   TC_NameIndex_c& operator=( const TC_NameIndex_c& nameIndex );
   bool operator<( const TC_NameIndex_c& nameIndex ) const;
   bool operator==( const TC_NameIndex_c& nameIndex ) const;
   bool operator!=( const TC_NameIndex_c& nameIndex ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrNameIndex ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   size_t GetIndex( void ) const;

   void Set( const string& srName,
             size_t index = SIZE_MAX );
   void Set( const char* pszName,
             size_t index = SIZE_MAX );
   void Clear( void );

   bool IsValid( void ) const;

private:

   string srName_;
   size_t index_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TC_NameIndex_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TC_NameIndex_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TC_NameIndex_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TC_NameIndex_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline size_t TC_NameIndex_c::GetIndex( 
      void ) const
{
   return( this->index_ );
}

//===========================================================================//
inline bool TC_NameIndex_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
