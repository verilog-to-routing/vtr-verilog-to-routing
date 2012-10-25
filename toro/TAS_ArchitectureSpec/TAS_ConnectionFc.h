//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_ConnectionFc class.
//
//           Inline methods include:
//           - SetDir
//           - GetDir
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

#ifndef TAS_CONNECTION_FC_H
#define TAS_CONNECTION_FC_H

#include <cstdio>
using namespace std;

#include "TAS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
class TAS_ConnectionFc_c
{
public:

   TAS_ConnectionFc_c( void );
   TAS_ConnectionFc_c( const TAS_ConnectionFc_c& connectionFc );
   ~TAS_ConnectionFc_c( void );

   TAS_ConnectionFc_c& operator=( const TAS_ConnectionFc_c& connectionFc );
   bool operator==( const TAS_ConnectionFc_c& connectionFc ) const;
   bool operator!=( const TAS_ConnectionFc_c& connectionFc ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   TC_TypeMode_t GetDir( void ) const;
   void SetDir( TC_TypeMode_t dir );

   bool IsValid( void ) const;

public:

   TAS_ConnectionBoxType_t type; 
                          // Selects type: FRACTION|ABSOLUTE|FULL
                          // Defines number of routing tracks connected 
                          // from input pins via a connection box
   double percent;        // Defines percent of tracks each pin connects to
                          // Applies when connection box type = FRACTION
   unsigned int absolute; // Defines number of tracks that each pin connects
                          // Applies when connection box type = ABSOLUTE

private:

   TC_TypeMode_t dir_;    // Selects Fc_in vs. Fc_out mode
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
inline void TAS_ConnectionFc_c::SetDir( 
      TC_TypeMode_t dir )
{
   this->dir_ = dir;
}

//===========================================================================//
inline TC_TypeMode_t TAS_ConnectionFc_c::GetDir( 
      void ) const
{
   return( this->dir_ );
}

//===========================================================================//
inline bool TAS_ConnectionFc_c::IsValid( 
      void ) const
{
   return( this->type != TAS_CONNECTION_BOX_UNDEFINED ? true : false );
}

#endif
