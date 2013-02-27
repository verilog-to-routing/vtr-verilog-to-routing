//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_Interconnect class.
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

#ifndef TAS_INTERCONNECT_H
#define TAS_INTERCONNECT_H

#include <cstdio>
#include <string>
using namespace std;

#include "TAS_Typedefs.h"
#include "TAS_TimingDelayLists.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAS_Interconnect_c
{
public:

   TAS_Interconnect_c( void );
   TAS_Interconnect_c( const string& srName );
   TAS_Interconnect_c( const char* pszName );
   TAS_Interconnect_c( const TAS_Interconnect_c& interconnect );
   ~TAS_Interconnect_c( void );

   TAS_Interconnect_c& operator=( const TAS_Interconnect_c& interconnect );
   bool operator==( const TAS_Interconnect_c& interconnect ) const;
   bool operator!=( const TAS_Interconnect_c& interconnect ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   string srName;                       // Interconnect name

   TAS_InterconnectMapType_t mapType;   // Selects map: COMPLETE|DIRECT|MUX

   TAS_InputNameList_t inputNameList;   // List of 1+ input pin names
   TAS_OutputNameList_t outputNameList; // List of 1+ output pin names

   TAS_TimingDelayLists_c timingDelayLists; 
                             // Defines max, clock setup, and clock-to-Q delays
private:

   enum TAS_DefCapacity_e 
   { 
      TAS_INPUT_NAME_LIST_DEF_CAPACITY = 64,
      TAS_OUTPUT_NAME_LIST_DEF_CAPACITY = 64
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TAS_Interconnect_c::IsValid( 
      void ) const
{
   return( this->srName.length( ) ? true : false );
}

#endif
