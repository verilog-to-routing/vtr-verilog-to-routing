//===========================================================================//
// Purpose : Declaration and inline definitions for a TNO_SwitchBox class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetInput, GetOutput
//           - SetInput, SetOutput
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

#ifndef TNO_SWITCH_BOX_H
#define TNO_SWITCH_BOX_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_SideIndex.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
class TNO_SwitchBox_c
{
public:

   TNO_SwitchBox_c( void );
   TNO_SwitchBox_c( const string& srName );
   TNO_SwitchBox_c( const char* pszName );
   TNO_SwitchBox_c( const string& srName,
                    const TC_SideIndex_c& input,
                    const TC_SideIndex_c& output );
   TNO_SwitchBox_c( const char* pszName,
                    const TC_SideIndex_c& input,
                    const TC_SideIndex_c& output );
   TNO_SwitchBox_c( const string& srName,
                    TC_SideMode_t inputSide,
                    size_t inputIndex,
                    TC_SideMode_t outputSide,
                    size_t outputIndex );
   TNO_SwitchBox_c( const char* pszName,
                    TC_SideMode_t inputSide,
                    size_t inputIndex,
                    TC_SideMode_t outputSide,
                    size_t outputIndex );
   TNO_SwitchBox_c( const TNO_SwitchBox_c& switchBox );
   ~TNO_SwitchBox_c( void );

   TNO_SwitchBox_c& operator=( const TNO_SwitchBox_c& switchBox );
   bool operator<( const TNO_SwitchBox_c& switchBox ) const;
   bool operator==( const TNO_SwitchBox_c& switchBox ) const;
   bool operator!=( const TNO_SwitchBox_c& switchBox ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrSwitchBox ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   const TC_SideIndex_c& GetInput( void ) const;
   const TC_SideIndex_c& GetOutput( void ) const;

   void SetInput( const TC_SideIndex_c& input );
   void SetOutput( const TC_SideIndex_c& output );

   void Clear( void );

   bool IsValid( void ) const;

private:

   string srName_;
   TC_SideIndex_c input_;
   TC_SideIndex_c output_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
inline void TNO_SwitchBox_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TNO_SwitchBox_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TNO_SwitchBox_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TNO_SwitchBox_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const TC_SideIndex_c& TNO_SwitchBox_c::GetInput( 
      void ) const
{
   return( this->input_ );
}

//===========================================================================//
inline const TC_SideIndex_c& TNO_SwitchBox_c::GetOutput( 
      void ) const
{
   return( this->output_ );
}

//===========================================================================//
inline void TNO_SwitchBox_c::SetInput( 
      const TC_SideIndex_c& input )
{
   this->input_ = input;
}

//===========================================================================//
inline void TNO_SwitchBox_c::SetOutput( 
      const TC_SideIndex_c& output )
{
   this->output_ = output;
}

//===========================================================================//
inline bool TNO_SwitchBox_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
