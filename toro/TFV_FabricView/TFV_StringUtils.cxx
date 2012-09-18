//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TFV_ExtractStringLayerType
//           - TFV_ExtractStringDataType
//
//===========================================================================//

#include "TFV_StringUtils.h"

//===========================================================================//
// Function       : TFV_ExtractStringLayerType
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TFV_ExtractStringLayerType(
      TFV_LayerType_t type,
      string*         psrType )
{
   if( psrType )
   {
      *psrType = "";

      switch( type )
      {
      case TFV_LAYER_INPUT_OUTPUT:   *psrType = "input_outputs";    break;
      case TFV_LAYER_PHYSICAL_BLOCK: *psrType = "physical_blocks";  break;
      case TFV_LAYER_SWITCH_BOX:     *psrType = "switch_boxes";     break;
      case TFV_LAYER_CONNECTION_BOX: *psrType = "connection_boxes"; break;
      case TFV_LAYER_CHANNEL_HORZ:   *psrType = "channels_horz";    break;
      case TFV_LAYER_CHANNEL_VERT:   *psrType = "channels_vert";    break;
      case TFV_LAYER_SEGMENT_HORZ:   *psrType = "segments_horz";    break;
      case TFV_LAYER_SEGMENT_VERT:   *psrType = "segments_vert";    break;
      case TFV_LAYER_INTERNAL_GRID:  *psrType = "internal_grid";    break;
      case TFV_LAYER_INTERNAL_MAP:   *psrType = "internal_map";     break;
      case TFV_LAYER_INTERNAL_PIN:   *psrType = "internal_pin";     break;
      case TFV_LAYER_BOUNDING_BOX:   *psrType = "bounding_box";     break;
      default:                       *psrType = "?";                break;
      }
   }
}

//===========================================================================//
// Function       : TFV_ExtractStringDataType
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TFV_ExtractStringDataType(
      TFV_DataType_t type,
      string*        psrType )
{
   if( psrType )
   {
      *psrType = "";

      switch( type )
      {
      case TFV_DATA_INPUT_OUTPUT:   *psrType = "IO"; break;
      case TFV_DATA_PHYSICAL_BLOCK: *psrType = "PB"; break;
      case TFV_DATA_SWITCH_BOX:     *psrType = "SB"; break;
      case TFV_DATA_CONNECTION_BOX: *psrType = "CB"; break;
      case TFV_DATA_CHANNEL_HORZ:   *psrType = "CH"; break;
      case TFV_DATA_CHANNEL_VERT:   *psrType = "CV"; break;
      case TFV_DATA_SEGMENT_HORZ:   *psrType = "SH"; break;
      case TFV_DATA_SEGMENT_VERT:   *psrType = "SV"; break;
      default:                      *psrType = "?";  break;
      }
   }
}
