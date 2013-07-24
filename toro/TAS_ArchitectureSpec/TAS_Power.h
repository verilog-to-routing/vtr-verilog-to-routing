//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_Power class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TAS_POWER_H
#define TAS_POWER_H

#include <cstdio>
using namespace std;

#include "TLO_Typedefs.h"
#include "TLO_Port.h"

#include "TAS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
class TAS_Power_c
{
public:

   TAS_Power_c( void );
   TAS_Power_c( const TAS_Power_c& power );
   ~TAS_Power_c( void );

   TAS_Power_c& operator=( const TAS_Power_c& power );
   bool operator==( const TAS_Power_c& power ) const;
   bool operator!=( const TAS_Power_c& power ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   TAS_PowerMethodMode_t estimateMethod;
                            // Power estimation method for a physical block
   class TAS_StaticPower_c 
   {
   public:

      double absolute;      // User-provided absolute power per block
                            // Required for TOGGLE_PINS, C_INTERNAL, and ABSOLUTE modes
   } staticPower;

   class TAS_DynamicPower_c
   {
   public:

      double absolute;      // User-provided absolute power per block
                            // Required for ABSOLUTE mode
      double capInternal;   // Required for C_INTERNAL mode
   } dynamicPower;

   TLO_PortList_t portList; // Defines optional port power (pin toggle) list

private:

   enum TAS_DefCapacity_e 
   { 
      TAS_POWER_PORT_LIST_DEF_CAPACITY = 1
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
inline bool TAS_Power_c::IsValid( 
      void ) const
{
   return( this->estimateMethod != TAS_POWER_METHOD_UNDEFINED ? true : false );
}

#endif
