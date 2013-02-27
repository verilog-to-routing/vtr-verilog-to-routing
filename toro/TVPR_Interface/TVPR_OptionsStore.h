//===========================================================================//
// Purpose : Declaration and inline definitions for the TVPR_OptionsStore 
//           class.
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

#ifndef TVPR_OPTIONS_STORE_H
#define TVPR_OPTIONS_STORE_H

#include "TOS_OptionsStore.h"

#include "vpr_api.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
class TVPR_OptionsStore_c
{
public:

   TVPR_OptionsStore_c( void );
   ~TVPR_OptionsStore_c( void );

   bool Export( const TOS_OptionsStore_c& optionsStore,
                t_options* pvpr_options ) const;

private:

   enum e_echo_files FindMessageEchoType_( const char* pszEchoType ) const;
   enum e_place_algorithm FindPlaceCostMode_( TOS_PlaceCostMode_t mode ) const;
   enum e_route_type FindRouteAbstractMode_( TOS_RouteAbstractMode_t mode ) const;
   enum e_base_cost_type FindRouteResourceMode_( TOS_RouteResourceMode_t mode ) const;
   enum e_router_algorithm FindRouteCostMode_( TOS_RouteCostMode_t mode ) const;
};

#endif 
