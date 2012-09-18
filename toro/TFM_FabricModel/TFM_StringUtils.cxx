//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TFM_ExtractStringBlockType
//
//===========================================================================//

#include "TFM_StringUtils.h"

//===========================================================================//
// Function       : TFM_ExtractStringBlockType
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TFM_ExtractStringBlockType(
      TFM_BlockType_t type,
      string*         psrType )
{
   if( psrType )
   {
      *psrType = "";

      switch( type )
      {
      case TFM_BLOCK_PHYSICAL_BLOCK: *psrType = "PB"; break;
      case TFM_BLOCK_INPUT_OUTPUT:   *psrType = "IO"; break;
      case TFM_BLOCK_SWITCH_BOX:     *psrType = "SB"; break;
      default:                       *psrType = "?";  break;
      }
   }
}
