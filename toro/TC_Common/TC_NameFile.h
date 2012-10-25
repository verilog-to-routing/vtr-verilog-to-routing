//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_NameFile class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetFileName
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TC_NAME_FILE_H
#define TC_NAME_FILE_H

#include <cstdio>
#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TC_NameFile_c
{
public:

   TC_NameFile_c( void );
   TC_NameFile_c( const string& srName,
   	          const string& srFileName );
   TC_NameFile_c( const char* pszName,
   	          const char* pszFileName );
   TC_NameFile_c( const TC_NameFile_c& nameFile );
   ~TC_NameFile_c( void );

   TC_NameFile_c& operator=( const TC_NameFile_c& nameFile );
   bool operator<( const TC_NameFile_c& nameFile ) const;
   bool operator==( const TC_NameFile_c& nameFile ) const;
   bool operator!=( const TC_NameFile_c& nameFile ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrNameFile ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   const char* GetFileName( void ) const;

   void Set( const string& srName,
	     const string& srFileName );
   void Set( const char* pszName,
   	     const char* pszFileName );
   void Clear( void );

   bool IsValid( void ) const;

private:

   string srName_;
   string srFileName_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TC_NameFile_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TC_NameFile_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TC_NameFile_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TC_NameFile_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TC_NameFile_c::GetFileName( 
      void ) const
{
   return( TIO_SR_STR( this->srFileName_ ));
}

//===========================================================================//
inline bool TC_NameFile_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
