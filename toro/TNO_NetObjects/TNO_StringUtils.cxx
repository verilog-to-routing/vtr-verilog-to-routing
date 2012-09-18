//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TNO_ExtractStringStatusMode
//           - TNO_ExtractStringRouteType
//
//===========================================================================//

#include "TNO_StringUtils.h"

//===========================================================================//
// Function       : TNO_ExtractStringStatusMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_ExtractStringStatusMode(
      TNO_StatusMode_t mode,
      string*          psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TNO_STATUS_OPEN:    *psrMode = "open";    break;
      case TNO_STATUS_GROUTED: *psrMode = "grouted"; break;
      case TNO_STATUS_ROUTED:  *psrMode = "routed";  break;
      default:                 *psrMode = "*";       break;
      }
   }
} 

//===========================================================================//
// Function       : TNO_ExtractStringRouteType
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_ExtractStringRouteType(
      TNO_RouteType_t type,
      string*         psrType )
{
   if( psrType )
   {
      *psrType = "";

      switch( type )
      {
      case TNO_ROUTE_INST_PIN:   *psrType = "instpin";   break;
      case TNO_ROUTE_SWITCH_BOX: *psrType = "switchbox"; break;
      case TNO_ROUTE_CHANNEL:    *psrType = "channel";   break;
      default:                   *psrType = "*";         break;
      }
   }
} 
