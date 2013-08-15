//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_RulesSwitches 
//           class.
//
//           Inline methods include:
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

#ifndef TOS_RULES_SWITCHES_H
#define TOS_RULES_SWITCHES_H

#include <cstdio>
using namespace std;

#include "TOS_FabricOptions.h"
#include "TOS_PackOptions.h"
#include "TOS_PlaceOptions.h"
#include "TOS_RouteOptions.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOS_RulesSwitches_c
{
public:

   TOS_RulesSwitches_c( void );
   ~TOS_RulesSwitches_c( void );

   void Print( void ) const;
   void Print( FILE* pfile, size_t spaceLen ) const;

   void Init( void );
   void Apply( void );

   const TOS_FabricOptions_c& GetFabricOptions( void ) const;
   const TOS_PackOptions_c& GetPackOptions( void ) const;
   const TOS_PlaceOptions_c& GetPlaceOptions( void ) const;
   const TOS_RouteOptions_c& GetRouteOptions( void ) const;

public:

   TOS_FabricOptions_c fabricOptions;
   TOS_PackOptions_c   packOptions;
   TOS_PlaceOptions_c  placeOptions;
   TOS_RouteOptions_c  routeOptions;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline const TOS_FabricOptions_c& TOS_RulesSwitches_c::GetFabricOptions(
      void ) const
{
   return( this->fabricOptions );
}

//===========================================================================//
inline const TOS_PackOptions_c& TOS_RulesSwitches_c::GetPackOptions(
      void ) const
{
   return( this->packOptions );
}

//===========================================================================//
inline const TOS_PlaceOptions_c& TOS_RulesSwitches_c::GetPlaceOptions(
      void ) const
{
   return( this->placeOptions );
}

//===========================================================================//
inline const TOS_RouteOptions_c& TOS_RulesSwitches_c::GetRouteOptions(
      void ) const
{
   return( this->routeOptions );
}

#endif
