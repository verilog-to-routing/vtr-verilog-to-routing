//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TOS_ExtractStringPackAlgorithmMode
//           - TOS_ExtractStringPackClusterNetsMode
//           - TOS_ExtractStringPackAffinityMode
//           - TOS_ExtractStringPackCostMode
//           - TOS_ExtractStringPlaceAlgorithmMode
//           - TOS_ExtractStringPlaceCostMode
//           - TOS_ExtractStringRouteAlgorithmMode
//           - TOS_ExtractStringRouteAbstractMode
//           - TOS_ExtractStringRouteResourceMode
//           - TOS_ExtractStringRouteCostMode
//           - TOS_ExtractStringInputDataMode
//           - TOS_ExtractStringOutputLaffMask
//           - TOS_ExtractStringRcDelaysExtractMode
//           - TOS_ExtractStringRcDelaysSortMode
//           - TOS_ExtractStringExecuteToolMask
//
//===========================================================================//

#include "TOS_StringUtils.h"

//===========================================================================//
// Function       : TOS_ExtractStringPackAlgorithmMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringPackAlgorithmMode(
      TOS_PackAlgorithmMode_t mode,
      string*                 psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TOS_PACK_ALGORITHM_AAPACK: *psrMode = "AAPACK"; break;
      default:                        *psrMode = "?";      break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringPackClusterNetsMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringPackClusterNetsMode(
      TOS_PackClusterNetsMode_t mode,
      string*                   psrMode )
{
   if( psrMode )
   {
      *psrMode = "";
      switch( mode )
      {
      case TOS_PACK_CLUSTER_NETS_MIN_CONNECTIONS: *psrMode = "MIN_CONNECTIONS"; break;
      case TOS_PACK_CLUSTER_NETS_MAX_CONNECTIONS: *psrMode = "MAX_CONNECTIONS"; break;
      default:                                    *psrMode = "?";               break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringPackAffinityMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringPackAffinityMode(
      TOS_PackAffinityMode_t mode,
      string*                psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TOS_PACK_AFFINITY_NONE: *psrMode = "NONE"; break;
      case TOS_PACK_AFFINITY_ANY:  *psrMode = "ANY";  break;
      default:                     *psrMode = "?";    break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringPackCostMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringPackCostMode(
      TOS_PackCostMode_t mode,
      string*            psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TOS_PACK_COST_ROUTABILITY_DRIVEN: *psrMode = "ROUTABILITY_DRIVEN"; break;
      case TOS_PACK_COST_TIMING_DRIVEN:      *psrMode = "TIMING_DRIVEN";      break;
      default:                               *psrMode = "?";                  break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringPlaceAlgorithmMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringPlaceAlgorithmMode(
      TOS_PlaceAlgorithmMode_t mode,
      string*                  psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TOS_PLACE_ALGORITHM_ANNEALING: *psrMode = "ANNEALING"; break;
      default:                            *psrMode = "?";         break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringPlaceCostMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringPlaceCostMode(
      TOS_PlaceCostMode_t mode,
      string*             psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TOS_PLACE_COST_ROUTABILITY_DRIVEN: *psrMode = "ROUTABILITY_DRIVEN"; break;
      case TOS_PLACE_COST_PATH_TIMING_DRIVEN: *psrMode = "PATH_TIMING_DRIVEN"; break;
      case TOS_PLACE_COST_NET_TIMING_DRIVEN:  *psrMode = "NET_TIMING_DRIVEN";  break;
      default:                                *psrMode = "?";                  break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringRouteAlgorithmMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringRouteAlgorithmMode(
      TOS_RouteAlgorithmMode_t mode,
      string*                  psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TOS_ROUTE_ALGORITHM_PATHFINDER: *psrMode = "PATHFINDER"; break;
      default:                             *psrMode = "?";          break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringRouteAbstractMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringRouteAbstractMode(
      TOS_RouteAbstractMode_t mode,
      string*                 psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TOS_ROUTE_ABSTRACT_GLOBAL:   *psrMode = "GLOBAL";   break;
      case TOS_ROUTE_ABSTRACT_DETAILED: *psrMode = "DETAILED"; break;
      default:                          *psrMode = "?";        break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringRouteResourceMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringRouteResourceMode(
      TOS_RouteResourceMode_t mode,
      string*                 psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TOS_ROUTE_RESOURCE_DEMAND_ONLY:      *psrMode = "DEMAND_ONLY";      break;
      case TOS_ROUTE_RESOURCE_DELAY_NORMALIZED: *psrMode = "DELAY_NORMALIZED"; break;
      default:                                  *psrMode = "?";                break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringRouteCostMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringRouteCostMode(
      TOS_RouteCostMode_t mode,
      string*             psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TOS_ROUTE_COST_BREADTH_FIRST:   *psrMode = "BREADTH_FIRST";   break;
      case TOS_ROUTE_COST_TIMING_DRIVEN:   *psrMode = "TIMING_DRIVEN";   break;
      default:                             *psrMode = "?";               break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringInputDataMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringInputDataMode(
      TOS_InputDataMode_t mode,
      string*             psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TOS_INPUT_DATA_NONE:   *psrMode = "NONE";   break;
      case TOS_INPUT_DATA_BLOCKS: *psrMode = "BLOCKS"; break;
      case TOS_INPUT_DATA_IOS:    *psrMode = "IOS";    break;
      case TOS_INPUT_DATA_ALL:    *psrMode = "ALL";    break;
      default:                    *psrMode = "?";      break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringOutputLaffMask
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringOutputLaffMask(
      int     mask,
      string* psrMask )
{
   if( psrMask )
   {
      *psrMask = "";

      if( mask & TOS_OUTPUT_LAFF_FABRIC_VIEW )
         *psrMask += "FABRIC_VIEW ";

      if( mask & TOS_OUTPUT_LAFF_BOUNDING_BOX )
         *psrMask += "BOX ";

      if( mask & TOS_OUTPUT_LAFF_INTERNAL_GRID )
         *psrMask += "GRID ";

      if( mask == TOS_OUTPUT_LAFF_ALL )
         *psrMask = "ALL ";

      if( psrMask->length( ) == 0 )
         *psrMask += "NONE ";
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringRcDelaysExtractMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringRcDelaysExtractMode(
      TOS_RcDelaysExtractMode_t mode,
      string*                   psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TOS_RC_DELAYS_EXTRACT_ELMORE: *psrMode = "ELMORE"; break;
      case TOS_RC_DELAYS_EXTRACT_D2M:    *psrMode = "D2M";    break;
      default:                           *psrMode = "?";      break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringRcDelaysSortMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringRcDelaysSortMode(
      TOS_RcDelaysSortMode_t mode,
      string*                psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TOS_RC_DELAYS_SORT_BY_NETS:   *psrMode = "BY_NETS";   break;
      case TOS_RC_DELAYS_SORT_BY_DELAYS: *psrMode = "BY_DELAYS"; break;
      default:                           *psrMode = "?";         break;
      }
   }
}

//===========================================================================//
// Function       : TOS_ExtractStringExecuteToolMask
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExtractStringExecuteToolMask(
      int     mask,
      string* psrMask )
{
   if( psrMask )
   {
      *psrMask = "";

      if( mask & TOS_EXECUTE_TOOL_PACK )
         *psrMask += "PACK ";

      if( mask & TOS_EXECUTE_TOOL_PLACE )
         *psrMask += "PLACE ";

      if( mask & TOS_EXECUTE_TOOL_ROUTE )
         *psrMask += "ROUTE ";

      if( mask == TOS_EXECUTE_TOOL_ALL )
         *psrMask = "ALL ";

      if( psrMask->length( ) == 0 )
         *psrMask = "NONE ";
   }
}
