//===========================================================================//
// Purpose : Declaration and inline definitions for a TFM_Config class.
//
//           Inline methods include:
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

#ifndef TFM_CONFIG_H
#define TFM_CONFIG_H

#include <cstdio>
using namespace std;

#include "TGS_Region.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
class TFM_Config_c
{
public:

   TFM_Config_c( void );
   TFM_Config_c( const TFM_Config_c& config );
   ~TFM_Config_c( void );

   TFM_Config_c& operator=( const TFM_Config_c& config );
   bool operator==( const TFM_Config_c& config ) const;
   bool operator!=( const TFM_Config_c& config ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   bool IsValid( void ) const;

public:

   TGS_Region_c fabricRegion;   // Fabric floorplan region boundary
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
inline bool TFM_Config_c::IsValid( 
      void ) const
{
   return( true );
}

#endif
