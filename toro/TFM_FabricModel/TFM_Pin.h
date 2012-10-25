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
	      TC_SideMode_t side,
	      double offset,
	      double width,
	      unsigned int slice );
   TFM_Pin_c( const char* pszName,
	      TC_SideMode_t side,
	      double offset,
	      double width,
	      unsigned int slice );
   TFM_Pin_c( const TFM_Pin_c& pin );
   ~TFM_Pin_c( void );

   TFM_Pin_c& operator=( const TFM_Pin_c& pin );
   bool operator==( const TFM_Pin_c& pin ) const;
   bool operator!=( const TFM_Pin_c& pin ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   bool IsValid( void ) const;

public:

   TC_SideMode_t    side;      // Defines pin side (LEFT|RIGHT|LOWER|UPPER)
   double           offset;    // Defines pin offset (wrt LEFT|LOWER)
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
