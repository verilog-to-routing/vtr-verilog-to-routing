//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_SideName class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetSide, GetName
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

#ifndef TC_SIDE_NAME_H
#define TC_SIDE_NAME_H

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
// 05/15/12 jeffr : Original
//===========================================================================//
class TC_SideName_c
{
public:

   TC_SideName_c( void );
   TC_SideName_c( TC_SideMode_t side,
 	          const string& srName );
   TC_SideName_c( TC_SideMode_t side,
 	          const char* pszName );
   TC_SideName_c( const TC_SideName_c& sideName );
   ~TC_SideName_c( void );

   TC_SideName_c& operator=( const TC_SideName_c& sideName );
   bool operator<( const TC_SideName_c& sideName ) const;
   bool operator==( const TC_SideName_c& sideName ) const;
   bool operator!=( const TC_SideName_c& sideName ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrSideName ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   TC_SideMode_t GetSide( void ) const;

   void Set( TC_SideMode_t side,
             const string& srName );
   void Set( TC_SideMode_t side,
             const char* pszName );
   void Clear( void );

   bool IsValid( void ) const;

private:

   TC_SideMode_t side_;
   string        srName_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TC_SideName_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TC_SideName_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TC_SideName_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TC_SideName_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline TC_SideMode_t TC_SideName_c::GetSide( 
      void ) const
{
   return( this->side_ );
}

//===========================================================================//
inline bool TC_SideName_c::IsValid( 
      void ) const
{
   return( this->side_ != TC_SIDE_UNDEFINED ? true : false );
}

#endif
