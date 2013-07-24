//===========================================================================//
// Purpose : Method definitions for the TAS_Config class.
//
//           Public methods include:
//           - TAS_Config_c, ~TAS_Config_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
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

#include "TC_MinGrid.h"
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TAS_StringUtils.h"
#include "TAS_Config.h"

//===========================================================================//
// Method         : TAS_Config_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
// 07/17/13 jeffr : Added TAS_ConfigPower_c and TAS_ConfigClock_c support
//===========================================================================//
TAS_Config_c::TAS_Config_c( 
      void )
      :
      clockList( TAS_CLOCK_LIST_DEF_CAPACITY )
{
   this->layout.sizeMode = TAS_ARRAY_SIZE_UNDEFINED;
   this->layout.autoSize.aspectRatio = 0.0;

   this->device.switchBoxes.modelType = TAS_SWITCH_BOX_MODEL_UNDEFINED;
   this->device.switchBoxes.fs = 0;

   this->device.connectionBoxes.capInput = 0.0;
   this->device.connectionBoxes.delayInput = 0.0;

   this->device.segments.dirType = TAS_SEGMENT_DIR_UNDEFINED;

   this->device.areaModel.resMinWidthNMOS = 0.0;
   this->device.areaModel.resMinWidthPMOS = 0.0;
   this->device.areaModel.sizeInputPinMux = 0.0;
   this->device.areaModel.areaGridTile = 0.0;

   this->power.interconnect.capWire = 0.0;
   this->power.interconnect.factor = 0.0;

   this->power.buffers.logicalEffortFactor = 0.0;

   this->power.sram.transistorsPerBit = 0.0;
} 

//===========================================================================//
TAS_Config_c::TAS_Config_c( 
      const TAS_Config_c& config )
      :
      clockList( config.clockList )
{
   this->layout.sizeMode = config.layout.sizeMode;
   this->layout.autoSize.aspectRatio = config.layout.autoSize.aspectRatio;
   this->layout.manualSize.gridDims = config.layout.manualSize.gridDims;

   this->device.switchBoxes.modelType = config.device.switchBoxes.modelType;
   this->device.switchBoxes.fs = config.device.switchBoxes.fs;

   this->device.connectionBoxes.capInput = config.device.connectionBoxes.capInput;
   this->device.connectionBoxes.delayInput = config.device.connectionBoxes.delayInput;

   this->device.segments.dirType = config.device.segments.dirType;

   this->device.channelWidth.io = config.device.channelWidth.io;
   this->device.channelWidth.x = config.device.channelWidth.x;
   this->device.channelWidth.y = config.device.channelWidth.y;

   this->device.areaModel.resMinWidthNMOS = config.device.areaModel.resMinWidthNMOS;
   this->device.areaModel.resMinWidthPMOS = config.device.areaModel.resMinWidthPMOS;
   this->device.areaModel.sizeInputPinMux = config.device.areaModel.sizeInputPinMux;
   this->device.areaModel.areaGridTile = config.device.areaModel.areaGridTile;

   this->power.interconnect.capWire = this->power.interconnect.capWire;
   this->power.interconnect.factor = this->power.interconnect.factor;

   this->power.buffers.logicalEffortFactor = this->power.buffers.logicalEffortFactor;

   this->power.sram.transistorsPerBit = this->power.sram.transistorsPerBit;
}

//===========================================================================//
// Method         : ~TAS_Config_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_Config_c::~TAS_Config_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
// 07/17/13 jeffr : Added TAS_ConfigPower_c and TAS_ConfigClock_c support
//===========================================================================//
TAS_Config_c& TAS_Config_c::operator=( 
      const TAS_Config_c& config )
{
   if( &config != this )
   {
      this->layout.sizeMode = config.layout.sizeMode;
      this->layout.autoSize.aspectRatio = config.layout.autoSize.aspectRatio;
      this->layout.manualSize.gridDims = config.layout.manualSize.gridDims;
      this->device.switchBoxes.modelType = config.device.switchBoxes.modelType;
      this->device.switchBoxes.fs = config.device.switchBoxes.fs;
      this->device.connectionBoxes.capInput = config.device.connectionBoxes.capInput;
      this->device.connectionBoxes.delayInput = config.device.connectionBoxes.delayInput;
      this->device.segments.dirType = config.device.segments.dirType;
      this->device.channelWidth.io = config.device.channelWidth.io;
      this->device.channelWidth.x = config.device.channelWidth.x;
      this->device.channelWidth.y = config.device.channelWidth.y;
      this->device.areaModel.resMinWidthNMOS = config.device.areaModel.resMinWidthNMOS;
      this->device.areaModel.resMinWidthPMOS = config.device.areaModel.resMinWidthPMOS;
      this->device.areaModel.sizeInputPinMux = config.device.areaModel.sizeInputPinMux;
      this->device.areaModel.areaGridTile = config.device.areaModel.areaGridTile;
      this->power.interconnect.capWire = config.power.interconnect.capWire;
      this->power.interconnect.factor = config.power.interconnect.factor;
      this->power.buffers.logicalEffortFactor = config.power.buffers.logicalEffortFactor;
      this->power.sram.transistorsPerBit = config.power.sram.transistorsPerBit;
      this->clockList = config.clockList;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
// 07/17/13 jeffr : Added TAS_ConfigPower_c and TAS_ConfigClock_c support
//===========================================================================//
bool TAS_Config_c::operator==( 
      const TAS_Config_c& config ) const
{
   return(( this->layout.sizeMode == config.layout.sizeMode ) &&
          ( TCTF_IsEQ( this->layout.autoSize.aspectRatio, config.layout.autoSize.aspectRatio )) &&
          ( this->layout.manualSize.gridDims == config.layout.manualSize.gridDims ) &&
          ( this->device.switchBoxes.modelType == config.device.switchBoxes.modelType ) &&
          ( this->device.switchBoxes.fs == config.device.switchBoxes.fs ) &&
          ( TCTF_IsEQ( this->device.connectionBoxes.capInput, config.device.connectionBoxes.capInput )) &&
          ( TCTF_IsEQ( this->device.connectionBoxes.delayInput, config.device.connectionBoxes.delayInput )) &&
          ( this->device.segments.dirType == config.device.segments.dirType ) &&
          ( this->device.channelWidth.io == config.device.channelWidth.io ) &&
          ( this->device.channelWidth.x == config.device.channelWidth.x ) &&
          ( this->device.channelWidth.y == config.device.channelWidth.y ) &&
          ( TCTF_IsEQ( this->device.areaModel.resMinWidthNMOS, config.device.areaModel.resMinWidthNMOS )) &&
          ( TCTF_IsEQ( this->device.areaModel.resMinWidthPMOS, config.device.areaModel.resMinWidthPMOS )) &&
          ( TCTF_IsEQ( this->device.areaModel.sizeInputPinMux, config.device.areaModel.sizeInputPinMux )) &&
          ( TCTF_IsEQ( this->device.areaModel.areaGridTile, config.device.areaModel.areaGridTile )) &&
          ( TCTF_IsEQ( this->power.interconnect.capWire, config.power.interconnect.capWire )) &&
          ( TCTF_IsEQ( this->power.interconnect.factor, config.power.interconnect.factor )) &&
          ( TCTF_IsEQ( this->power.buffers.logicalEffortFactor, config.power.buffers.logicalEffortFactor )) &&
          ( TCTF_IsEQ( this->power.sram.transistorsPerBit, config.power.sram.transistorsPerBit )) &&
          ( this->clockList == config.clockList ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_Config_c::operator!=( 
      const TAS_Config_c& config ) const
{
   return( !this->operator==( config ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
// 07/17/13 jeffr : Added TAS_ConfigPower_c and TAS_ConfigClock_c support
//===========================================================================//
void TAS_Config_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<config>\n" );
   spaceLen += 3;

   if( this->layout.sizeMode != TAS_ARRAY_SIZE_UNDEFINED )
   {
      string srArraySizeMode;
      TAS_ExtractStringArraySizeMode( this->layout.sizeMode, &srArraySizeMode );

      printHandler.Write( pfile, spaceLen, "<size mode=\"%s\"", 
                                           TIO_SR_STR( srArraySizeMode ));

      if( TCTF_IsGT( this->layout.autoSize.aspectRatio, 0.0 ))
      {
         printHandler.Write( pfile, 0, " ratio=\"%0.*f\"", 
                                       precision, this->layout.autoSize.aspectRatio );
      }
      if( this->layout.manualSize.gridDims.IsValid( ))
      {
         printHandler.Write( pfile, 0, " width=\"%u\" height=\"%u\"", 
                                       this->layout.manualSize.gridDims.dx,
                                       this->layout.manualSize.gridDims.dy );
      }
      printHandler.Write( pfile, 0, "/>\n" );
   }

   if( TCTF_IsGT( this->device.areaModel.resMinWidthNMOS, 0.0 ) ||
       TCTF_IsGT( this->device.areaModel.resMinWidthPMOS, 0.0 ) ||
       TCTF_IsGT( this->device.areaModel.sizeInputPinMux, 0.0 ) ||
       TCTF_IsGT( this->device.areaModel.areaGridTile, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "<est" );

      if( TCTF_IsGT( this->device.areaModel.resMinWidthNMOS, 0.0 ))
      {
         printHandler.Write( pfile, 0, " min_width_nmos_res=\"%0.*f\"", 
                                       precision, this->device.areaModel.resMinWidthNMOS );
      }
      if( TCTF_IsGT( this->device.areaModel.resMinWidthPMOS, 0.0 ))
      {
         printHandler.Write( pfile, 0, " min_width_pmos_res=\"%0.*f\"", 
                                       precision, this->device.areaModel.resMinWidthPMOS );
      }
      if( TCTF_IsGT( this->device.areaModel.sizeInputPinMux, 0.0 ))
      {
         printHandler.Write( pfile, 0, " mux_trans_in_pin_size=\"%0.*f\"", 
                                       precision, this->device.areaModel.sizeInputPinMux );
      }
      if( TCTF_IsGT( this->device.areaModel.areaGridTile, 0.0 ))
      {
         printHandler.Write( pfile, 0, " grid_logic_tile_area=\"%0.*f\"", 
                                       precision, this->device.areaModel.areaGridTile );
      }
      printHandler.Write( pfile, 0, "/>\n" );
   }

   if( this->device.switchBoxes.modelType != TAS_SWITCH_BOX_MODEL_UNDEFINED )
   {
      string srSwitchBoxModel;
      TAS_ExtractStringSwitchBoxModelType( this->device.switchBoxes.modelType, &srSwitchBoxModel );

      printHandler.Write( pfile, spaceLen, "<sb model=\"%s\" fs=\"%u\"/>\n", 
                                           TIO_SR_STR( srSwitchBoxModel ),
                                           this->device.switchBoxes.fs );

   }

   if( TCTF_IsGT( this->device.connectionBoxes.capInput, 0.0 ) ||
       TCTF_IsGT( this->device.connectionBoxes.delayInput, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "<cb cap_in=\"%0.*e\" delay_in=\"%0.*e\"/>\n",
                                           precision + 1, this->device.connectionBoxes.capInput,
                                           precision + 1, this->device.connectionBoxes.delayInput );
   }

   if( this->device.segments.dirType != TAS_SEGMENT_DIR_UNDEFINED )
   {
      string srSegmentDir;
      TAS_ExtractStringSegmentDirType( this->device.segments.dirType, &srSegmentDir );

      printHandler.Write( pfile, spaceLen, "<segments=\"%s\"/>\n", TIO_SR_STR( srSegmentDir ));
   }

   if( TCTF_IsGT( this->power.interconnect.capWire, 0.0 ) ||
       TCTF_IsGT( this->power.interconnect.factor, 0.0 ) ||
       TCTF_IsGT( this->power.buffers.logicalEffortFactor, 0.0 ) ||
       TCTF_IsGT( this->power.sram.transistorsPerBit, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "<power>\n" );
      spaceLen += 3;

      printHandler.Write( pfile, spaceLen, "<interconnect C_wire=\"%0.*e\" factor=\"%0.*f\"/>\n",
                                           precision + 1, this->power.interconnect.capWire,
                                           precision, this->power.interconnect.factor );
      printHandler.Write( pfile, spaceLen, "<buffers logical_effort_factor=\"%0.*f\"/>\n",
                                           precision, this->power.buffers.logicalEffortFactor );
      printHandler.Write( pfile, spaceLen, "<sram transistors_per_bit=\"%0.*f\"/>\n",
                                           precision, this->power.sram.transistorsPerBit );
      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "</power>\n" );
   }

   this->clockList.Print( pfile, spaceLen );

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</config>\n" );
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
// 07/17/13 jeffr : Added TAS_ConfigPower_c and TAS_ConfigClock_c support
//===========================================================================//
void TAS_Config_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_Config_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   if( TCTF_IsGT( this->layout.autoSize.aspectRatio, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "<layout auto=\"%0.*f\"/>\n", 
                                           precision, this->layout.autoSize.aspectRatio );
   }
   else if( this->layout.manualSize.gridDims.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<layout width=\"%u\" height=\"%u\"/>\n", 
                                           this->layout.manualSize.gridDims.dx, 
                                           this->layout.manualSize.gridDims.dy ); 
   }

   printHandler.Write( pfile, spaceLen, "<device>\n" );
   spaceLen += 3;

   printHandler.Write( pfile, spaceLen, "<sizing R_minW_nmos=\"%0.*f\" R_minW_pmos=\"%0.*f\" ipin_mux_trans_size=\"%0.*f\"/>\n",
                                        precision, this->device.areaModel.resMinWidthNMOS,
                                        precision, this->device.areaModel.resMinWidthPMOS,
                                        precision, this->device.areaModel.sizeInputPinMux );
   printHandler.Write( pfile, spaceLen, "<area grid_logic_tile_area=\"%0.*f\"/>\n",
                                        precision, this->device.areaModel.areaGridTile );
   printHandler.Write( pfile, spaceLen, "<timing C_ipin_cblock=\"%0.*e\" T_ipin_cblock=\"%0.*e\"/>\n",
                                        precision + 1, this->device.connectionBoxes.capInput,
                                        precision + 1, this->device.connectionBoxes.delayInput ); 

   string srSwitchBoxModelType;
   TAS_ExtractStringSwitchBoxModelType( this->device.switchBoxes.modelType, &srSwitchBoxModelType );
   printHandler.Write( pfile, spaceLen, "<switch_block type=\"%s\" fs=\"%u\"/>\n",
                                        TIO_SR_STR( srSwitchBoxModelType ),
                                        this->device.switchBoxes.fs );

   printHandler.Write( pfile, spaceLen, "<chan_width_distr>\n" );
   this->device.channelWidth.io.PrintXML( pfile, spaceLen + 3 );
   this->device.channelWidth.x.PrintXML( pfile, spaceLen + 3 );
   this->device.channelWidth.y.PrintXML( pfile, spaceLen + 3 );
   printHandler.Write( pfile, spaceLen, "</chan_width_distr>\n" );

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</device>\n" );

   if( TCTF_IsGT( this->power.interconnect.capWire, 0.0 ) ||
       TCTF_IsGT( this->power.interconnect.factor, 0.0 ) ||
       TCTF_IsGT( this->power.buffers.logicalEffortFactor, 0.0 ) ||
       TCTF_IsGT( this->power.sram.transistorsPerBit, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "<power>\n" );
      spaceLen += 3;

      printHandler.Write( pfile, spaceLen, "<local_interconnect C_wire=\"%0.*e\" factor=\"%0.*f\"/>\n",
                                           precision + 1, this->power.interconnect.capWire,
                                           precision, this->power.interconnect.factor );
      printHandler.Write( pfile, spaceLen, "<buffers logical_effort_factor=\"%0.*f\"/>\n",
                                           precision, this->power.buffers.logicalEffortFactor );
      printHandler.Write( pfile, spaceLen, "<sram transistors_per_bit=\"%0.*f\"/>\n",
                                           precision, this->power.sram.transistorsPerBit );
      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "</power>\n" );
   }

   if( this->clockList.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<clocks>\n" );
      for( size_t i = 0; i < this->clockList.GetLength( ); ++i )
      {
         this->clockList[i]->PrintXML( pfile, spaceLen + 3 );
      }
      printHandler.Write( pfile, spaceLen, "</clocks>\n" );
   }
}
