//===========================================================================//
// Purpose : Declaration and inline definitions for a TPO_Relative class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetSide, GetDx, GetDy, GetRotateEnable, GetName
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

#ifndef TPO_RELATIVE_H
#define TPO_RELATIVE_H

#include <cstdio>
#include <climits>
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
class TPO_Relative_c
{
public:

   TPO_Relative_c( void );
   TPO_Relative_c( TC_SideMode_t side,
                   int dx, int dy,
                   bool rotateEnable,
                   const string& srName );
   TPO_Relative_c( TC_SideMode_t side,
                   int dx, int dy,
                   bool rotateEnable,
                   const char* pszName );
   TPO_Relative_c( const TPO_Relative_c& relative );
   ~TPO_Relative_c( void );

   TPO_Relative_c& operator=( const TPO_Relative_c& relative );
   bool operator<( const TPO_Relative_c& relative ) const;
   bool operator==( const TPO_Relative_c& relative ) const;
   bool operator!=( const TPO_Relative_c& relative ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrRelative ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   TC_SideMode_t GetSide( void ) const;
   int GetDx( void ) const;
   int GetDy( void ) const;
   bool GetRotateEnable( void ) const;

   void Set( TC_SideMode_t side,
             int dx, int dy,
             bool rotateEnable,
             const string& srName );
   void Set( TC_SideMode_t side,
             int dx, int dy,
             bool rotateEnable,
             const char* pszName );
   void Clear( void );

   bool IsValid( void ) const;

private:

   TC_SideMode_t side_;
   int           dx_;
   int           dy_;
   bool          rotateEnable_;
   string        srName_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TPO_Relative_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TPO_Relative_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TPO_Relative_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TPO_Relative_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline TC_SideMode_t TPO_Relative_c::GetSide( 
      void ) const
{
   return( this->side_ );
}

//===========================================================================//
inline int TPO_Relative_c::GetDx( 
      void ) const
{
   return( this->dx_ );
}

//===========================================================================//
inline int TPO_Relative_c::GetDy( 
      void ) const
{
   return( this->dy_ );
}

//===========================================================================//
inline bool TPO_Relative_c::GetRotateEnable( 
      void ) const
{
   return( this->rotateEnable_ );
}

//===========================================================================//
inline bool TPO_Relative_c::IsValid( 
      void ) const
{
   return(( this->side_ != TC_SIDE_UNDEFINED ) || 
          ( this->dx_ != INT_MAX && this->dy_ != INT_MAX ) ? 
          true : false );
}

#endif
