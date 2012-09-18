//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TGS_ExtractStringOrientMode
//
//===========================================================================//

#include "TGS_StringUtils.h"

//===========================================================================//
// Function       : TGS_ExtractStringOrientMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_ExtractStringOrientMode(
      TGS_OrientMode_t orientMode,
      string*          psrOrientMode )
{
   if( psrOrientMode )
   {
      *psrOrientMode = "";

      switch( orientMode )
      {
      case TGS_ORIENT_HORIZONTAL: *psrOrientMode = "horz"; break;
      case TGS_ORIENT_VERTICAL:   *psrOrientMode = "vert"; break;
      default:                    *psrOrientMode = "?";    break;
      }
   }
}
