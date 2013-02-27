//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_ControlSwitches 
//           class.
//
//           Inline methods include:
//           - GetInputOptions
//           - GetOutputOptions
//           - GetMessageOptions
//           - GetExecuteOptions
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

#ifndef TOS_CONTROL_SWITCHES_H
#define TOS_CONTROL_SWITCHES_H

#include <cstdio>
using namespace std;

#include "TOS_InputOptions.h"
#include "TOS_OutputOptions.h"
#include "TOS_MessageOptions.h"
#include "TOS_ExecuteOptions.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOS_ControlSwitches_c
{
public:

   TOS_ControlSwitches_c( void );
   ~TOS_ControlSwitches_c( void );

   void Print( void ) const;
   void Print( FILE* pfile, size_t spaceLen ) const;

   void Init( void );
   void Apply( void );

   const TOS_InputOptions_c& GetInputOptions( void ) const;
   const TOS_OutputOptions_c& GetOutputOptions( void ) const;
   const TOS_MessageOptions_c& GetMessageOptions( void ) const;
   const TOS_ExecuteOptions_c& GetExecuteOptions( void ) const;

public:

   TOS_InputOptions_c   inputOptions;
   TOS_OutputOptions_c  outputOptions;
   TOS_MessageOptions_c messageOptions;
   TOS_ExecuteOptions_c executeOptions;

   string srDefaultBaseName; // Used to build default input/output file names
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline const TOS_InputOptions_c& TOS_ControlSwitches_c::GetInputOptions(
      void ) const
{
   return( this->inputOptions );
}

//===========================================================================//
inline const TOS_OutputOptions_c& TOS_ControlSwitches_c::GetOutputOptions(
      void ) const
{
   return( this->outputOptions );
}

//===========================================================================//
inline const TOS_MessageOptions_c& TOS_ControlSwitches_c::GetMessageOptions(
      void ) const
{
   return( this->messageOptions );
}

//===========================================================================//
inline const TOS_ExecuteOptions_c& TOS_ControlSwitches_c::GetExecuteOptions(
      void ) const
{
   return( this->executeOptions );
}

#endif
