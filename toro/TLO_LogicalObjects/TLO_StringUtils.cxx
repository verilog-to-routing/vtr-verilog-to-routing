//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TLO_ExtractStringCellSource
//
//===========================================================================//

#include "TLO_StringUtils.h"

//===========================================================================//
// Function       : TLO_ExtractStringCellSource
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TLO_ExtractStringCellSource(
      TLO_CellSource_t source,
      string*          psrSource )
{
   if( psrSource )
   {
      *psrSource = "";

      switch( source )
      {
      case TLO_CELL_SOURCE_STANDARD: *psrSource = "standard"; break;
      case TLO_CELL_SOURCE_CUSTOM:   *psrSource = "custom";   break;
      default:                       *psrSource = "?";        break;
      }
   }
}
