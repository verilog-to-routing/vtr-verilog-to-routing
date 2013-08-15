//===========================================================================//
// Purpose : Declaration and inline definitions for the TVPR_OptionsStore 
//           class.
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
