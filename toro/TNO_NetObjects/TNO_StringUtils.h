//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful string functions.
//
//===========================================================================//

#ifndef TNO_STRING_UTILS_H
#define TNO_STRING_UTILS_H

#include <string>
using namespace std;

#include "TNO_Typedefs.h"

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_ExtractStringStatusMode( TNO_StatusMode_t mode, string* psrMode );
void TNO_ExtractStringRouteType( TNO_RouteType_t type, string* psrType );

#endif 
