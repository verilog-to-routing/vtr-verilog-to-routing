//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_RulesSwitches 
//           class.
//
//           Inline methods include:
//           - GetPackOptions
//           - GetPlaceOptions
//           - GetRouteOptions
//
//===========================================================================//

#ifndef TOS_RULES_SWITCHES_H
#define TOS_RULES_SWITCHES_H

#include <cstdio>
using namespace std;

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

   const TOS_PackOptions_c& GetPackOptions( void ) const;
   const TOS_PlaceOptions_c& GetPlaceOptions( void ) const;
   const TOS_RouteOptions_c& GetRouteOptions( void ) const;

public:

   TOS_PackOptions_c  packOptions;
   TOS_PlaceOptions_c placeOptions;
   TOS_RouteOptions_c routeOptions;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
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
