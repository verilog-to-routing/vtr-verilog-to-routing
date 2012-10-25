//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TFV_ExtractStringLayerType
//           - TFV_ExtractStringDataType
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
//                                                                           //
// This program is free software; you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation; version 3 of the License, or any later version. //
//                                                                           //
// This program is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY //
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   //
// for more details.                                                         //
//                                                                           //
// You should have received a copy of the GNU General Public License along   //
// with this program; if not, see <http://www.gnu.org/licenses>.             //
//---------------------------------------------------------------------------//

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
