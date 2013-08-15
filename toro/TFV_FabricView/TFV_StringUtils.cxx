//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TFV_ExtractStringLayerType
//           - TFV_ExtractStringDataType
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
