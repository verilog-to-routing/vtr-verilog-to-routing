//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TTP_ExtractStringTileMode
//
//===========================================================================//

#include "TTP_StringUtils.h"

//===========================================================================//
// Function       : TTP_ExtractStringTileMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TTP_ExtractStringTileMode(
      TTP_TileMode_t tileMode,
      string*        psrTileMode )
{
   if( psrTileMode )
   {
      *psrTileMode = "";

      switch( tileMode )
      {
      case TTP_TILE_CLEAR: *psrTileMode = "CLEAR"; break;
      case TTP_TILE_SOLID: *psrTileMode = "SOLID"; break;
      default:             *psrTileMode = "*";     break;
      }
   }
} 
