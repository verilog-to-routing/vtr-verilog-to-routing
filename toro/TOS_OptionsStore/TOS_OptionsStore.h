//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_OptionsStore class.
//
//           Inline methods include:
//           - GetInputOptions
//           - GetOutputOptions
//           - GetMessageOptions
//           - GetExecuteOptions
//           - GetFabricOptions
//           - GetPackOptions
//           - GetPlaceOptions
//           - GetRouteOptions
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
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

   const TOS_FabricOptions_c& GetFabricOptions( void ) const;
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
inline const TOS_FabricOptions_c& TOS_OptionsStore_c::GetFabricOptions(
      void ) const
{
   return( this->rulesSwitches.GetFabricOptions( ));
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
