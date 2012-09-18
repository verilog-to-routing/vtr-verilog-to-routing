//===========================================================================//
// Purpose : Declaration and inline definitions for the TVPR_OptionsStore 
//           class.
//
//===========================================================================//

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
