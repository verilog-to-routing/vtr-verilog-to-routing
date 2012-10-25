//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_OptionsStore class.
//
//           Inline methods include:
//           - GetInputOptions
//           - GetOutputOptions
//           - GetMessageOptions
//           - GetExecuteOptions
//           - GetPackOptions
//           - GetPlaceOptions
//           - GetRouteOptions
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

#ifndef TOS_OPTIONS_STORE_H
#define TOS_OPTIONS_STORE_H

#include <cstdio>
using namespace std;

#include "TOS_ControlSwitches.h"
#include "TOS_RulesSwitches.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOS_OptionsStore_c
{
public:

   TOS_OptionsStore_c( void );
   ~TOS_OptionsStore_c( void );

   void Print( void ) const;
   void Print( FILE* pfile, size_t spaceLen = 0 ) const;

   void Init( void );
   void Apply( void );

   const TOS_InputOptions_c& GetInputOptions( void ) const;
   const TOS_OutputOptions_c& GetOutputOptions( void ) const;
   const TOS_MessageOptions_c& GetMessageOptions( void ) const;
   const TOS_ExecuteOptions_c& GetExecuteOptions( void ) const;

   const TOS_PackOptions_c& GetPackOptions( void ) const;
   const TOS_PlaceOptions_c& GetPlaceOptions( void ) const;
   const TOS_RouteOptions_c& GetRouteOptions( void ) const;

public:

   TOS_ControlSwitches_c controlSwitches;
   TOS_RulesSwitches_c   rulesSwitches;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline const TOS_InputOptions_c& TOS_OptionsStore_c::GetInputOptions(
      void ) const
{
   return( this->controlSwitches.GetInputOptions( ));
}

//===========================================================================//
inline const TOS_OutputOptions_c& TOS_OptionsStore_c::GetOutputOptions(
      void ) const
{
   return( this->controlSwitches.GetOutputOptions( ));
}

//===========================================================================//
inline const TOS_MessageOptions_c& TOS_OptionsStore_c::GetMessageOptions(
      void ) const
{
   return( this->controlSwitches.GetMessageOptions( ));
}

//===========================================================================//
inline const TOS_ExecuteOptions_c& TOS_OptionsStore_c::GetExecuteOptions(
      void ) const
{
   return( this->controlSwitches.GetExecuteOptions( ));
}

//===========================================================================//
inline const TOS_PackOptions_c& TOS_OptionsStore_c::GetPackOptions(
      void ) const
{
   return( this->rulesSwitches.GetPackOptions( ));
}

//===========================================================================//
inline const TOS_PlaceOptions_c& TOS_OptionsStore_c::GetPlaceOptions(
      void ) const
{
   return( this->rulesSwitches.GetPlaceOptions( ));
}

//===========================================================================//
inline const TOS_RouteOptions_c& TOS_OptionsStore_c::GetRouteOptions(
      void ) const
{
   return( this->rulesSwitches.GetRouteOptions( ));
}

#endif
