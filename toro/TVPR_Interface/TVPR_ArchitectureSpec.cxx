//===========================================================================//
// Purpose : Method definitions for the TVPR_ArchitectureSpec_c class.
//
//           Public methods include:
//           - TVPR_ArchitectureSpec_c, ~TVPR_ArchitectureSpec_c
//           - Export
//
//           Private methods include:
//           - PokeLayout_
//           - PokeDevice_
//           - PokeModelLists_
//           - PokeModel_
//           - PokePhysicalBlockList_
//           - PokePhysicalBlock_
//           - PokeSwitchBoxList_
//           - PokeSegmentList_
//           - PokePbType_
//           - PokePbTypeChild_
//           - PokePbTypeLutClass_
//           - PokePbTypeMemoryClass_
//           - PokeModeList_
//           - PokeMode_
//           - PokeInterconnectList_
//           - PokeInterconnect_
//           - PokeFc_
//           - PokePinAssignList_
//           - PokeGridAssignList_
//           - PokePortList_
//           - PokePort_
//           - PokeTimingDelayLists_
//           - PokeTimingDelay_
//           - InitModels_
//           - InitPbType_
//           - ValidateModels_
//           - FindSideMode_
//           - FindClassType_
//           - FindInterconnectMapType_
//           - FindPinAssignPatternType_
//           - FindGridAssignDistrMode_
//           - FindChannelDistrMode_
//           - FindSwitchBoxModelType_
//           - FindSegmentDirType_
//           - FindSegmentBitPattern_
//           - FindChannelDistrMode_
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

#include <string>
using namespace std;

#include "TC_MemoryUtils.h"

#include "TVPR_ArchitectureSpec.h"

//===========================================================================//
// Method         : TVPR_ArchitectureSpec_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
TVPR_ArchitectureSpec_c::TVPR_ArchitectureSpec_c( 
      void )
{
}

//===========================================================================//
// Method         : ~TVPR_ArchitectureSpec_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
TVPR_ArchitectureSpec_c::~TVPR_ArchitectureSpec_c( 
      void )
{
}

//===========================================================================//
// Method         : Export
// Reference      : See VPR's "s_arch" data structure, as defined in the 
//                  "physical_types.h" source code file.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TVPR_ArchitectureSpec_c::Export(
      const TAS_ArchitectureSpec_c& architectureSpec,
            t_arch*                 pvpr_architecture,
            t_type_descriptor**     pvpr_physicalBlockArray,
            int*                    pvpr_physicalBlockCount,
            bool                    isTimingEnabled ) const
{
   bool ok = true;

   const TAS_Config_c& config = architectureSpec.config;
   const TAS_CellList_t& cellList = architectureSpec.cellList;
   const TAS_PhysicalBlockList_t& physicalBlockList = architectureSpec.physicalBlockList;
   const TAS_SwitchBoxList_t& switchBoxList = architectureSpec.switchBoxList;
   const TAS_SegmentList_t& segmentList = architectureSpec.segmentList;

   this->PokeLayout_( config, 
                      pvpr_architecture );
   this->PokeDevice_( config, 
                      pvpr_architecture, 
                      isTimingEnabled );
   if( ok )
   {
      this->PokeModelLists_( cellList, 
                             pvpr_architecture );
   }
   if( ok )
   {
      this->PokePhysicalBlockList_( physicalBlockList,
                                    pvpr_physicalBlockArray,
                                    pvpr_physicalBlockCount );
   }
   if( ok )
   {
      ok = this->PokeSwitchBoxList_( switchBoxList, 
                                     &pvpr_architecture->Switches,
                                     &pvpr_architecture->num_switches,
                                     isTimingEnabled );
   }
   if( ok )
   {
      ok = this->PokeSegmentList_( segmentList, 
                                   &pvpr_architecture->Segments,
                                   &pvpr_architecture->num_segments,
                                   pvpr_architecture->Switches,
                                   pvpr_architecture->num_switches,
                                   isTimingEnabled );
   }

   if( ok )
   {
      ok = this->InitModels_( *pvpr_architecture,
                              pvpr_physicalBlockArray, 
                              *pvpr_physicalBlockCount );
   }
   return( ok );
}

//===========================================================================//
// Method         : PokeLayout_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokeLayout_( 
      const TAS_Config_c& config,
            t_arch*       pvpr_architecture ) const
{
   // Write TAS_Config_c's layout parameters
   if( config.layout.manualSize.gridDims.IsValid( ))
   {
      pvpr_architecture->clb_grid.IsAuto = static_cast< boolean >( false );
      pvpr_architecture->clb_grid.W = config.layout.manualSize.gridDims.dx;
      pvpr_architecture->clb_grid.H = config.layout.manualSize.gridDims.dy;
   }
   else if( TCTF_IsGT( config.layout.autoSize.aspectRatio, 0.0 ))
   {
      pvpr_architecture->clb_grid.IsAuto = static_cast< boolean >( true );
      pvpr_architecture->clb_grid.Aspect = static_cast< float >( config.layout.autoSize.aspectRatio );
   }
   else
   {
      pvpr_architecture->clb_grid.IsAuto = static_cast< boolean >( true );
      pvpr_architecture->clb_grid.Aspect = 1.0;
   }
}

//===========================================================================//
// Method         : PokeDevice_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokeDevice_( 
      const TAS_Config_c& config,
            t_arch*       pvpr_architecture,
            bool          isTimingEnabled ) const 
{
   // Write TAS_Config_c's device area model parameters
   if( isTimingEnabled )
   {
      pvpr_architecture->R_minW_nmos = static_cast< float >( config.device.areaModel.resMinWidthNMOS );
   }
   if( isTimingEnabled )
   {
      pvpr_architecture->R_minW_pmos = static_cast< float >( config.device.areaModel.resMinWidthPMOS );
   }
   pvpr_architecture->ipin_mux_trans_size = static_cast< float >( config.device.areaModel.sizeInputPinMux );
   pvpr_architecture->grid_logic_tile_area = static_cast< float >( config.device.areaModel.areaGridTile );

   // Write TAS_Config_c's device timing analysis parameters
   pvpr_architecture->C_ipin_cblock = static_cast< float >( config.device.connectionBoxes.capInput );
   pvpr_architecture->T_ipin_cblock = static_cast< float >( config.device.connectionBoxes.delayInput );

   // Write TAS_Config_c's device switch box parameters
   if( config.device.switchBoxes.modelType != TAS_SWITCH_BOX_MODEL_UNDEFINED )
   {
      pvpr_architecture->SBType = this->FindSwitchBoxModelType_( config.device.switchBoxes.modelType );
   }
   if( config.device.switchBoxes.fs > 0 )
   {
      pvpr_architecture->Fs = config.device.switchBoxes.fs;
   }

   // Write TAS_Config_c's device channel width parameters
   if( config.device.channelWidth.io.IsValid( ))
   {
      pvpr_architecture->Chans.chan_width_io = static_cast< float >( config.device.channelWidth.io.width );
   }
   if( config.device.channelWidth.x.IsValid( ))
   {
      pvpr_architecture->Chans.chan_x_dist.type = this->FindChannelDistrMode_( config.device.channelWidth.x.distrMode );
      pvpr_architecture->Chans.chan_x_dist.peak = static_cast< float >( config.device.channelWidth.x.peak );
      pvpr_architecture->Chans.chan_x_dist.xpeak = static_cast< float >( config.device.channelWidth.x.xpeak );
      pvpr_architecture->Chans.chan_x_dist.dc = static_cast< float >( config.device.channelWidth.x.dc );
      pvpr_architecture->Chans.chan_x_dist.width = static_cast< float >( config.device.channelWidth.x.width );
   }
   if( config.device.channelWidth.y.IsValid( ))
   {
      pvpr_architecture->Chans.chan_y_dist.type = this->FindChannelDistrMode_( config.device.channelWidth.y.distrMode );
      pvpr_architecture->Chans.chan_y_dist.peak = static_cast< float >( config.device.channelWidth.y.peak );
      pvpr_architecture->Chans.chan_y_dist.xpeak = static_cast< float >( config.device.channelWidth.y.xpeak );
      pvpr_architecture->Chans.chan_y_dist.dc = static_cast< float >( config.device.channelWidth.y.dc );
      pvpr_architecture->Chans.chan_y_dist.width = static_cast< float >( config.device.channelWidth.y.width );
   }
}

//===========================================================================//
// Method         : PokeModelLists_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokeModelLists_(
      const TAS_CellList_t& cellList,
            t_arch*         pvpr_architecture ) const
{
   int libraryLen = NUM_MODELS_IN_LIBRARY;
   int libraryIndex = 0;
   pvpr_architecture->model_library = static_cast< t_model* >( TC_calloc( libraryLen, sizeof( t_model )));

   for( size_t i = 0; i < cellList.GetLength( ); ++i )
   {
      const TAS_Cell_c& cell = *cellList[i];
      if( cell.GetSource( ) != TLO_CELL_SOURCE_STANDARD )
         continue;

      t_model* pvpr_model = &pvpr_architecture->model_library[libraryIndex];
      pvpr_model->index = libraryIndex;
      ++libraryIndex;

      this->PokeModel_( cell, pvpr_model );

      if( libraryIndex < libraryLen )
      {
         pvpr_model->next = &( pvpr_architecture->model_library[libraryIndex] );
      }
   }

   int modelIndex = NUM_MODELS_IN_LIBRARY;
   pvpr_architecture->models = 0;

   for( size_t i = 0; i < cellList.GetLength( ); ++i )
   {
      const TAS_Cell_c& cell = *cellList[i];
      if( cell.GetSource( ) != TLO_CELL_SOURCE_CUSTOM )
         continue;

      t_model* pvpr_model = static_cast< t_model* >( TC_calloc( 1, sizeof( t_model )));
      pvpr_model->index = modelIndex;
      ++modelIndex;

      this->PokeModel_( cell, pvpr_model );

      pvpr_model->next = pvpr_architecture->models;
      pvpr_architecture->models = pvpr_model;
   }
}

//===========================================================================//
// Method         : PokeModel_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokeModel_(
      const TAS_Cell_c& cell,
            t_model*    pvpr_model ) const
{
   pvpr_model->name = TC_strdup( cell.GetName( ));

   const TLO_PortList_t& portList = cell.GetPortList( );

   size_t inputCount = 0;
   size_t outputCount = 0;
   for( size_t j = 0; j < portList.GetLength( ); ++j )
   {
      const TLO_Port_c& port = *portList[j];
      inputCount += (( port.GetType( ) == TC_TYPE_INPUT ) ? 1 : 0 );
      inputCount += (( port.GetType( ) == TC_TYPE_CLOCK ) ? 1 : 0 );
      outputCount += (( port.GetType( ) == TC_TYPE_OUTPUT ) ? 1 : 0 );
   }

   pvpr_model->inputs = static_cast< t_model_ports* >( TC_calloc( inputCount, sizeof( t_model_ports )));
   pvpr_model->outputs = static_cast< t_model_ports* >( TC_calloc( outputCount, sizeof( t_model_ports )));

   size_t inputIndex = 0;   
   for( size_t j = 0; j < portList.GetLength( ); ++j )
   {
      const TLO_Port_c& port = *portList[j];
      if(( port.GetType( ) == TC_TYPE_INPUT ) || ( port.GetType( ) == TC_TYPE_CLOCK ))
      {
         t_model_ports* pvpr_modelPort = &( pvpr_model->inputs[inputIndex] );
         ++inputIndex;

         pvpr_modelPort->name = TC_strdup( port.GetName( ));
         pvpr_modelPort->dir = IN_PORT;
         pvpr_modelPort->size = ( cell.GetSource( ) == TLO_CELL_SOURCE_STANDARD ? 1 : -1 );
         pvpr_modelPort->min_size = ( cell.GetSource( ) == TLO_CELL_SOURCE_STANDARD ? 1 : -1 );
         if( port.GetType( ) == TC_TYPE_CLOCK )
         {
            pvpr_modelPort->is_clock = static_cast< boolean >( true );
         }
         else
         {
            pvpr_modelPort->is_clock = static_cast< boolean >( false );
         }
         pvpr_modelPort->is_non_clock_global = static_cast< boolean >( false );

         if( inputIndex < inputCount )
         {
            pvpr_modelPort->next = &( pvpr_model->inputs[inputIndex] );
         }
      }
   }

   size_t outputIndex = 0;   
   for( size_t j = 0; j < portList.GetLength( ); ++j )
   {
      const TLO_Port_c& port = *portList[j];
      if( port.GetType( ) == TC_TYPE_OUTPUT )
      {
         t_model_ports* pvpr_modelPort = &( pvpr_model->outputs[outputIndex] );
         ++outputIndex;

         pvpr_modelPort->name = TC_strdup( port.GetName( ));
         pvpr_modelPort->dir = OUT_PORT;
         pvpr_modelPort->size = ( cell.GetSource( ) == TLO_CELL_SOURCE_STANDARD ? 1 : -1 );
         pvpr_modelPort->min_size = ( cell.GetSource( ) == TLO_CELL_SOURCE_STANDARD ? 1 : -1 );

         if( outputIndex < outputCount )
         {
            pvpr_modelPort->next = &( pvpr_model->outputs[outputIndex] );
         }
      }
   }
}

//===========================================================================//
// Method         : PokePhysicalBlockList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokePhysicalBlockList_(
      const TAS_PhysicalBlockList_t& physicalBlockList,
            t_type_descriptor**      pvpr_physicalBlockArray, 
            int*                     pvpr_physicalBlockCount ) const
{
   // [VPR] Alloc the physical block type list (with extra t_type_desctiptors for empty pseudo-type)
   *pvpr_physicalBlockCount = static_cast< int >( physicalBlockList.GetLength( )) + 1;
   *pvpr_physicalBlockArray = static_cast< t_type_descriptor* >( TC_calloc( *pvpr_physicalBlockCount, sizeof( t_type_descriptor )));

   // [VPR] Initialize various VPR-specific global variables
   type_descriptors = *pvpr_physicalBlockArray;
   type_descriptors[EMPTY_TYPE_INDEX].index = EMPTY_TYPE_INDEX;
   type_descriptors[IO_TYPE_INDEX].index = IO_TYPE_INDEX;
   EMPTY_TYPE = &type_descriptors[EMPTY_TYPE_INDEX];
   IO_TYPE = &type_descriptors[IO_TYPE_INDEX];

   t_type_descriptor* pvpr_emptyBlock = &type_descriptors[EMPTY_TYPE_INDEX];
   pvpr_emptyBlock->name = const_cast< char* >( "<EMPTY>" );
   pvpr_emptyBlock->width = 1;
   pvpr_emptyBlock->height = 1;

   int index = 1; // [VPR] Skip over 'empty' type
   for( size_t i = 0; i < physicalBlockList.GetLength( ); ++i )
   {
      const TAS_PhysicalBlock_c& physicalBlock = *physicalBlockList[i];
      t_type_descriptor* pvpr_physicalBlock = &( *pvpr_physicalBlockArray )[index];
      pvpr_physicalBlock->index = index;
      ++index;

      this->PokePhysicalBlock_( physicalBlock, pvpr_physicalBlock );
   }
}

//===========================================================================//
// Method         : PokePhysicalBlock_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokePhysicalBlock_(
      const TAS_PhysicalBlock_c& physicalBlock,
            t_type_descriptor*   pvpr_physicalBlock ) const
{
   pvpr_physicalBlock->name = TC_strdup( physicalBlock.srName.data( ));
   pvpr_physicalBlock->width = (( physicalBlock.width > 0 ) ? physicalBlock.width : 1 );
   pvpr_physicalBlock->height = (( physicalBlock.height > 0 ) ? physicalBlock.height : 1 );
   pvpr_physicalBlock->capacity = (( physicalBlock.capacity > 0 ) ? physicalBlock.capacity : 1 );
   pvpr_physicalBlock->area = -1;

   // [VPR] Poke pb_type info
   pvpr_physicalBlock->pb_type = static_cast< t_pb_type* >( TC_calloc( 1, sizeof( t_pb_type )));
   this->PokePbType_( physicalBlock, 0, 
                      pvpr_physicalBlock->pb_type );

   pvpr_physicalBlock->pb_type->name = TC_strdup( pvpr_physicalBlock->name );
   pvpr_physicalBlock->num_pins = pvpr_physicalBlock->capacity *
                                  ( pvpr_physicalBlock->pb_type->num_input_pins +
                                    pvpr_physicalBlock->pb_type->num_output_pins +
                                    pvpr_physicalBlock->pb_type->num_clock_pins );
   pvpr_physicalBlock->num_receivers = pvpr_physicalBlock->capacity * 
                                       pvpr_physicalBlock->pb_type->num_input_pins;
   pvpr_physicalBlock->num_drivers = pvpr_physicalBlock->capacity * 
                                     pvpr_physicalBlock->pb_type->num_output_pins;

   //  [VPR] Poke pin names and classes and locations
   pvpr_physicalBlock->pin_location_distribution = this->FindPinAssignPatternType_( physicalBlock.pinAssignPattern );
   this->PokePinAssignList_( physicalBlock.pinAssignList, 
                             pvpr_physicalBlock );
   this->PokeGridAssignList_( physicalBlock.gridAssignList, 
                              pvpr_physicalBlock );

   // [VPR] Poke Fc
   this->PokeFc_( physicalBlock, 
                  pvpr_physicalBlock );
}

//===========================================================================//
// Method         : PokeSwitchBoxList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TVPR_ArchitectureSpec_c::PokeSwitchBoxList_(
      const TAS_SwitchBoxList_t& switchBoxList,
            t_switch_inf**       pvpr_switchBoxArray, 
            int*                 pvpr_switchBoxCount,
            bool                 isTimingEnabled ) const 
{
   bool ok = true;

   *pvpr_switchBoxArray = 0;
   *pvpr_switchBoxCount = static_cast< int >( switchBoxList.GetLength( ));

   if( ok && ( *pvpr_switchBoxCount > 0 ))
   {
      *pvpr_switchBoxArray = static_cast< t_switch_inf* >( TC_calloc( *pvpr_switchBoxCount, sizeof( t_switch_inf )));
      ok = ( pvpr_switchBoxArray ? true : false );
   }

   if( ok && ( *pvpr_switchBoxCount > 0 ))
   {
      for( size_t i = 0; i < switchBoxList.GetLength( ); ++i )
      {
         const TAS_SwitchBox_c& switchBox = *switchBoxList[i];

         ( *pvpr_switchBoxArray )[i].name = TC_strdup( switchBox.srName );

         if(( switchBox.type == TAS_SWITCH_BOX_MUX ) ||
            ( switchBox.type == TAS_SWITCH_BOX_BUFFERED ))
         {
            ( *pvpr_switchBoxArray )[i].buffered = static_cast< boolean >( true );
         }

         if(( switchBox.type == TAS_SWITCH_BOX_MUX ) &&
            ( TCTF_IsGT( switchBox.area.buffer, 0.0 )))
         {
            ( *pvpr_switchBoxArray )[i].buf_size = static_cast< float >( switchBox.area.buffer );
         }
         if( TCTF_IsGT( switchBox.area.muxTransistor, 0.0 ))
         {
            ( *pvpr_switchBoxArray )[i].mux_trans_size = static_cast< float >( switchBox.area.muxTransistor );
         }
         else
         {
            ( *pvpr_switchBoxArray )[i].mux_trans_size = 1.0;
         }

         if( isTimingEnabled )
         {
            ( *pvpr_switchBoxArray )[i].R = static_cast< float >( switchBox.timing.res );
         }
         if( isTimingEnabled )
         {
            ( *pvpr_switchBoxArray )[i].Cin = static_cast< float >( switchBox.timing.capInput );
         }
         if( isTimingEnabled )
         {
            ( *pvpr_switchBoxArray )[i].Cout = static_cast< float >( switchBox.timing.capOutput );
         }
         if( isTimingEnabled )
         {
            ( *pvpr_switchBoxArray )[i].Tdel = static_cast< float >( switchBox.timing.delay );
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : PokeSegmentList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TVPR_ArchitectureSpec_c::PokeSegmentList_(
      const TAS_SegmentList_t& segmentList,
            t_segment_inf**    pvpr_segmentArray, 
            int*               pvpr_segmentCount,
      const t_switch_inf*      vpr_switchBoxArray, 
            int                vpr_switchBoxCount,
            bool               isTimingEnabled ) const 
{
   bool ok = true;

   *pvpr_segmentArray = 0;
   *pvpr_segmentCount = static_cast< int >( segmentList.GetLength( ));

   if( ok && ( *pvpr_segmentCount > 0 ))
   {
      *pvpr_segmentArray = static_cast< t_segment_inf* >( TC_calloc( *pvpr_segmentCount, sizeof( t_segment_inf )));
      ok = ( pvpr_segmentArray ? true : false );
   }

   if( ok && ( *pvpr_segmentCount > 0 ))
   {
      for( size_t i = 0; i < segmentList.GetLength( ); ++i )
      {
         const TAS_Segment_c& segment = *segmentList[i];

         ( *pvpr_segmentArray )[i].length = 1;
         if( segment.length == UINT_MAX )
         {
            ( *pvpr_segmentArray )[i].longline = static_cast< boolean >( true );
         }
         else if( segment.length > 0 )
         {
            ( *pvpr_segmentArray )[i].length = segment.length;
         }

         if( segment.dirType != TAS_SEGMENT_DIR_UNDEFINED )
         {
            ( *pvpr_segmentArray )[i].directionality = this->FindSegmentDirType_( segment.dirType );
         }

         ( *pvpr_segmentArray )[i].frequency = static_cast< int >( 1.0 * MAX_CHANNEL_WIDTH );
         if( TCTF_IsGT( segment.trackFreq, 0.0 ))
         {
            ( *pvpr_segmentArray )[i].frequency = static_cast< int >( segment.trackFreq * MAX_CHANNEL_WIDTH );
         }
         if( isTimingEnabled )
         {
            ( *pvpr_segmentArray )[i].Rmetal = static_cast< float >( segment.timing.res );
         }
         if( isTimingEnabled )
         {
            ( *pvpr_segmentArray )[i].Cmetal = static_cast< float >( segment.timing.cap );
         }

         ok = this->FindSegmentBitPattern_( segment.sbPattern,
                                            ( *pvpr_segmentArray )[i].length + 1,
                                            &( *pvpr_segmentArray )[i].sb,
                                            &(( *pvpr_segmentArray )[i].sb_len ));
         if( !ok )
            break;

         ok = this->FindSegmentBitPattern_( segment.cbPattern,
                                            ( *pvpr_segmentArray )[i].length,
                                            &( *pvpr_segmentArray )[i].cb,
                                            &(( *pvpr_segmentArray )[i].cb_len ));
         if( !ok )
            break;

         if(( *pvpr_segmentArray )[i].directionality == UNI_DIRECTIONAL )
         {
            const char* pszMuxSwitchName = segment.srMuxSwitchName.data( );
            int j = 0;
            for( j = 0; j < vpr_switchBoxCount; ++j ) 
            {
               if( strcmp( pszMuxSwitchName, vpr_switchBoxArray[j].name ) == 0 )
                  break;
            }
            ( *pvpr_segmentArray )[i].wire_switch = static_cast< short >( j );
            ( *pvpr_segmentArray )[i].opin_switch = static_cast< short >( j );
         }
         else // if(( *pvpr_segmentArray )[i].directionality == BI_DIRECTIONAL )
         {
            const char* pszWireSwitchName = segment.srWireSwitchName.data( );
            int j = 0;
            for( j = 0; j < vpr_switchBoxCount; ++j ) 
            {
               if( strcmp( pszWireSwitchName, vpr_switchBoxArray[j].name ) == 0 )
                  break;
            }
            ( *pvpr_segmentArray )[i].wire_switch = static_cast< short >( j );
         
            const char* pszOutputSwitchName = segment.srWireSwitchName.data( );
            for( j = 0; j < vpr_switchBoxCount; ++j ) 
            {
               if( strcmp( pszOutputSwitchName, vpr_switchBoxArray[j].name ) == 0 )
                  break;
            }
            ( *pvpr_segmentArray )[i].opin_switch = static_cast< short >( j );
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : PokePbType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokePbType_(
      const TAS_PhysicalBlock_c& physicalBlock,
      const t_mode*              pvpr_mode,
            t_pb_type*           pvpr_pb_type ) const
{
   pvpr_pb_type->parent_mode = const_cast< t_mode* >( pvpr_mode );
   if( pvpr_mode && pvpr_mode->parent_pb_type ) 
   {
      pvpr_pb_type->depth = pvpr_mode->parent_pb_type->depth + 1;
      pvpr_pb_type->name = TC_strdup( physicalBlock.srName );
   } 
   else
   {
      pvpr_pb_type->depth = 0;
   }

   if( pvpr_mode ) 
   {
      pvpr_pb_type->num_pb = physicalBlock.numPB;
   } 
   else 
   {
      pvpr_pb_type->num_pb = 1;
   }

   if( physicalBlock.srModelName.length( ))
   {
      string srModelName = ( physicalBlock.modelType == TAS_PHYSICAL_BLOCK_MODEL_STANDARD ?
                             "" : ".subckt " );
      srModelName += TC_strdup( physicalBlock.srModelName );

      pvpr_pb_type->blif_model = TC_strdup( srModelName );
   }
   if( physicalBlock.classType != TAS_CLASS_UNDEFINED )
   {
      pvpr_pb_type->class_type = this->FindClassType_( physicalBlock.classType );
   }
   pvpr_pb_type->max_internal_delay = UNDEFINED;

   // [VPR] Process ports
   this->PokePortList_( physicalBlock.portList, pvpr_pb_type );

   // [VPR] Determine if this is a leaf or container pb_type
   if( pvpr_pb_type->blif_model ) 
   {
      // [VPR] Process delay and capacitance annotations
      this->PokeTimingDelayLists_( physicalBlock.timingDelayLists,
                                   &pvpr_pb_type->annotations,
                                   &pvpr_pb_type->num_annotations );

      // [VPR] If leaf is special known class, then read class lib, else treat as primitive
      if( pvpr_pb_type->class_type == LUT_CLASS ) 
      {
         this->PokePbTypeLutClass_( pvpr_pb_type );
      } 
      else if( pvpr_pb_type->class_type == MEMORY_CLASS ) 
      {
         this->PokePbTypeMemoryClass_( pvpr_pb_type );
      } 
      else 
      {
         pvpr_pb_type->num_modes = 0;
      }
   } 
   else 
   {
      // [VPR] Process modes
      this->PokeModeList_( physicalBlock, pvpr_pb_type );
   }
}

//===========================================================================//
// Method         : PokePbTypeChild_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokePbTypeChild_(
      const t_pb_type&  vpr_pb_type,
      const string&     srChildName,
            t_pb_type*  pvpr_pb_type ) const
{
   pvpr_pb_type->name = TC_strdup( srChildName );
   pvpr_pb_type->blif_model = TC_strdup( vpr_pb_type.blif_model );
   pvpr_pb_type->class_type = vpr_pb_type.class_type;
   pvpr_pb_type->depth = vpr_pb_type.depth;
   pvpr_pb_type->model = vpr_pb_type.model;
   pvpr_pb_type->modes = 0;
   pvpr_pb_type->num_modes = 0;
   pvpr_pb_type->num_clock_pins = vpr_pb_type.num_clock_pins;
   pvpr_pb_type->num_input_pins = vpr_pb_type.num_input_pins;
   pvpr_pb_type->num_output_pins = vpr_pb_type.num_output_pins;
   pvpr_pb_type->num_pb = 1;
   pvpr_pb_type->num_ports = vpr_pb_type.num_ports;
   pvpr_pb_type->ports = static_cast< t_port* >( TC_calloc( pvpr_pb_type->num_ports, sizeof( t_port )));
   for( int i = 0; i < vpr_pb_type.num_ports; ++i ) 
   {
      pvpr_pb_type->ports[i].name = TC_strdup( vpr_pb_type.ports[i].name );
      pvpr_pb_type->ports[i].parent_pb_type = pvpr_pb_type;
      pvpr_pb_type->ports[i].type = vpr_pb_type.ports[i].type;
      pvpr_pb_type->ports[i].num_pins = vpr_pb_type.ports[i].num_pins;
      pvpr_pb_type->ports[i].model_port = vpr_pb_type.ports[i].model_port;
      pvpr_pb_type->ports[i].port_class = TC_strdup( vpr_pb_type.ports[i].port_class );
      pvpr_pb_type->ports[i].is_clock = vpr_pb_type.ports[i].is_clock;
   }
   pvpr_pb_type->max_internal_delay = vpr_pb_type.max_internal_delay;

   pvpr_pb_type->num_annotations = vpr_pb_type.num_annotations;
   pvpr_pb_type->annotations = static_cast< t_pin_to_pin_annotation* >( TC_calloc( pvpr_pb_type->num_annotations, sizeof( t_pin_to_pin_annotation )));
   for( int i = 0; i < pvpr_pb_type->num_annotations; ++i ) 
   {
      t_pin_to_pin_annotation* pvpr_annotation = &( pvpr_pb_type->annotations )[i];

      pvpr_annotation->type = vpr_pb_type.annotations[i].type;
      pvpr_annotation->format = vpr_pb_type.annotations[i].format;

      pvpr_annotation->num_value_prop_pairs = vpr_pb_type.annotations[i].num_value_prop_pairs;
      pvpr_annotation->prop = static_cast< int* >( TC_calloc( pvpr_annotation->num_value_prop_pairs, sizeof( int )));
      pvpr_annotation->value = static_cast< char** >( TC_calloc( pvpr_annotation->num_value_prop_pairs, sizeof( char* )));
      for( int j = 0; j < vpr_pb_type.annotations[i].num_value_prop_pairs; ++j ) 
      {
         pvpr_annotation->prop[j] = vpr_pb_type.annotations[i].prop[j];
         pvpr_annotation->value[j] = TC_strdup( vpr_pb_type.annotations[i].value[j] );
      }

      string srDot = strstr( vpr_pb_type.annotations[i].input_pins, "." );
      pvpr_annotation->input_pins = static_cast< char* >( TC_calloc( srChildName.length( ) + srDot.length( ) + 1, sizeof( char )));
      pvpr_annotation->input_pins[0] = 0;
      strcat( pvpr_annotation->input_pins, srChildName.data( ));
      strcat( pvpr_annotation->input_pins, srDot.data( ));
      if( vpr_pb_type.annotations[i].output_pins ) 
      {
         srDot = strstr( vpr_pb_type.annotations[i].output_pins, "." );
         pvpr_annotation->output_pins = static_cast< char* >( TC_calloc( srChildName.length( ) + srDot.length( ) + 1, sizeof( char )));
         pvpr_annotation->output_pins[0] = 0;
         strcat( pvpr_annotation->output_pins, srChildName.data( ));
         strcat( pvpr_annotation->output_pins, srDot.data( ));
      } 
      else 
      {
         pvpr_annotation->output_pins = 0;
      }
      pvpr_annotation->clock = TC_strdup( vpr_pb_type.annotations[i].clock );

      pvpr_annotation->line_num = 0;
   }
}

//===========================================================================//
// Method         : PokePbTypeLutClass_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokePbTypeLutClass_(
      t_pb_type* pvpr_pb_type ) const
{
   string srLutName(( strcmp( pvpr_pb_type->name, "lut" ) != 0 ) ? "lut" : "lut_child" );

   t_port* pinPort = (( strcmp( pvpr_pb_type->ports[0].port_class, "lut_in" ) == 0 ) ?
                      &pvpr_pb_type->ports[0] : &pvpr_pb_type->ports[1] );
   t_port* poutPort = (( strcmp( pvpr_pb_type->ports[0].port_class, "lut_in" ) == 0 ) ?
                      &pvpr_pb_type->ports[1] : &pvpr_pb_type->ports[0] );

   pvpr_pb_type->num_modes = 2;
   pvpr_pb_type->modes = static_cast< t_mode* >( TC_calloc( pvpr_pb_type->num_modes, sizeof( t_mode )));

   // [VPR] First mode, route_through
   pvpr_pb_type->modes[0].name = TC_strdup( pvpr_pb_type->name );
   pvpr_pb_type->modes[0].parent_pb_type = pvpr_pb_type;
   pvpr_pb_type->modes[0].index = 0;
   pvpr_pb_type->modes[0].num_pb_type_children = 0;

   pvpr_pb_type->modes[0].num_interconnect = 1;
   pvpr_pb_type->modes[0].interconnect = static_cast< t_interconnect* >( TC_calloc( 1, sizeof( t_interconnect )));

   t_interconnect* pvpr_interconnect0 = &pvpr_pb_type->modes[0].interconnect[0];
   pvpr_interconnect0->type = COMPLETE_INTERC;
   pvpr_interconnect0->name = static_cast< char* >( TC_calloc( strlen( pvpr_pb_type->name ) + 10, sizeof( char )));
   pvpr_interconnect0->input_string = static_cast< char* >( TC_calloc( strlen( pvpr_pb_type->name ) + strlen( pinPort->name ) + 2, sizeof( char )));
   pvpr_interconnect0->output_string = static_cast< char* >( TC_calloc( strlen( pvpr_pb_type->name ) + strlen( poutPort->name ) + 2, sizeof( char )));
   sprintf( pvpr_interconnect0->name, "complete:%s", pvpr_pb_type->name );
   sprintf( pvpr_interconnect0->input_string, "%s.%s", pvpr_pb_type->name, pinPort->name );
   sprintf( pvpr_interconnect0->output_string, "%s.%s", pvpr_pb_type->name, poutPort->name );

   pvpr_interconnect0->num_annotations = pvpr_pb_type->num_annotations;
   pvpr_interconnect0->annotations = static_cast< t_pin_to_pin_annotation* >( TC_calloc( pvpr_interconnect0->num_annotations, sizeof( t_pin_to_pin_annotation )));
   for( int i = 0; i < pvpr_interconnect0->num_annotations; ++i ) 
   {
      t_pin_to_pin_annotation* pvpr_annotation = &pvpr_interconnect0->annotations[i];
      pvpr_annotation->type = pvpr_pb_type->annotations[i].type;
      pvpr_annotation->format = pvpr_pb_type->annotations[i].format;
      pvpr_annotation->num_value_prop_pairs = pvpr_pb_type->annotations[i].num_value_prop_pairs;
      pvpr_annotation->prop = static_cast< int* >( TC_calloc( pvpr_annotation->num_value_prop_pairs, sizeof( int )));
      pvpr_annotation->value = static_cast< char** >( TC_calloc( pvpr_annotation->num_value_prop_pairs, sizeof( char* )));
      for( int j = 0; j < pvpr_pb_type->annotations[i].num_value_prop_pairs; ++j ) 
      {
         pvpr_annotation->prop[j] = pvpr_pb_type->annotations[i].prop[j];
         pvpr_annotation->value[j] = TC_strdup( pvpr_pb_type->annotations[i].value[j] );
      }
      pvpr_annotation->input_pins = TC_strdup( pvpr_pb_type->annotations[i].input_pins );
      pvpr_annotation->output_pins = TC_strdup( pvpr_pb_type->annotations[i].output_pins );
      pvpr_annotation->clock = TC_strdup( pvpr_pb_type->annotations[i].clock );
      pvpr_annotation->line_num = 0;
   }

   // [VPR] Second mode, LUT
   pvpr_pb_type->modes[1].name = TC_strdup( pvpr_pb_type->name );
   pvpr_pb_type->modes[1].parent_pb_type = pvpr_pb_type;
   pvpr_pb_type->modes[1].index = 1;
   pvpr_pb_type->modes[1].num_pb_type_children = 1;
   pvpr_pb_type->modes[1].pb_type_children = static_cast< t_pb_type* >( TC_calloc( 1, sizeof( t_pb_type )));
   this->PokePbTypeChild_( *pvpr_pb_type, srLutName.data( ), 
                           pvpr_pb_type->modes[1].pb_type_children );

   for( int i = 0; i < pvpr_pb_type->num_annotations; ++i ) 
   {
      for( int j = 0; j < pvpr_pb_type->annotations[i].num_value_prop_pairs; ++j ) 
      {
         free( pvpr_pb_type->annotations[i].value[j] );
      }
      free( pvpr_pb_type->annotations[i].value );
      free( pvpr_pb_type->annotations[i].prop );
      if( pvpr_pb_type->annotations[i].input_pins ) 
      {
         free( pvpr_pb_type->annotations[i].input_pins );
      }
      if( pvpr_pb_type->annotations[i].output_pins ) 
      {
         free( pvpr_pb_type->annotations[i].output_pins );
      }
      if( pvpr_pb_type->annotations[i].clock ) 
      {
         free( pvpr_pb_type->annotations[i].clock );
      }
   }
   pvpr_pb_type->num_annotations = 0;
   free( pvpr_pb_type->annotations );
   pvpr_pb_type->annotations = 0;
   pvpr_pb_type->modes[1].pb_type_children[0].depth = pvpr_pb_type->depth + 1;
   pvpr_pb_type->modes[1].pb_type_children[0].parent_mode = &pvpr_pb_type->modes[1];

   pvpr_pb_type->modes[1].num_interconnect = 2;
   pvpr_pb_type->modes[1].interconnect = static_cast< t_interconnect* >( TC_calloc( 2, sizeof( t_interconnect )));

   pvpr_interconnect0 = &pvpr_pb_type->modes[1].interconnect[0];
   pvpr_interconnect0->type = COMPLETE_INTERC;
   pvpr_interconnect0->name = static_cast< char* >( TC_calloc( strlen( pvpr_pb_type->name ) + 10, sizeof( char )));
   pvpr_interconnect0->input_string = static_cast< char* >( TC_calloc( strlen( pvpr_pb_type->name ) + strlen( pinPort->name ) + 2, sizeof( char )));
   pvpr_interconnect0->output_string = static_cast< char* >( TC_calloc( srLutName.length( ) + strlen( pinPort->name ) + 2, sizeof( char )));
   sprintf( pvpr_interconnect0->name, "complete:%s", pvpr_pb_type->name );
   sprintf( pvpr_interconnect0->input_string, "%s.%s", pvpr_pb_type->name, pinPort->name );
   sprintf( pvpr_interconnect0->output_string, "%s.%s", srLutName.data( ), pinPort->name );

   t_interconnect* pvpr_interconnect1 = &pvpr_pb_type->modes[1].interconnect[1];
   pvpr_interconnect1->type = DIRECT_INTERC;
   pvpr_interconnect1->name = static_cast< char* >( TC_calloc( strlen( pvpr_pb_type->name ) + 11, sizeof( char )));
   pvpr_interconnect1->input_string = static_cast< char* >( TC_calloc( srLutName.length( ) + strlen( poutPort->name ) + 4, sizeof( char )));
   pvpr_interconnect1->output_string = static_cast< char* >( TC_calloc( strlen( pvpr_pb_type->name ) + strlen( poutPort->name ) + strlen( pinPort->name ) + 2, sizeof( char )));
   sprintf( pvpr_interconnect1->name, "direct:%s", pvpr_pb_type->name );
   sprintf( pvpr_interconnect1->input_string, "%s.%s", srLutName.data( ), poutPort->name );
   sprintf( pvpr_interconnect1->output_string, "%s.%s", pvpr_pb_type->name, poutPort->name );
   pvpr_interconnect1->infer_annotations = static_cast< boolean >( true );

   free( pvpr_pb_type->blif_model );
   pvpr_pb_type->blif_model = 0;
   pvpr_pb_type->model = 0;
}

//===========================================================================//
// Method         : PokePbTypeMemoryClass_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokePbTypeMemoryClass_(
      t_pb_type* pvpr_pb_type ) const
{
   string srMemoryName(( strcmp( pvpr_pb_type->name, "memory_slice" ) != 0 ) ? 
                       "memory_slice" : "memory_slice_1bit" );

   pvpr_pb_type->modes = static_cast< t_mode* >( TC_calloc( 1, sizeof( t_mode )));
   t_mode* pvpr_mode = &pvpr_pb_type->modes[0];

   pvpr_mode->name = TC_strdup( srMemoryName );
   pvpr_mode->parent_pb_type = pvpr_pb_type;
   pvpr_mode->index = 0;

   int num_pb = OPEN;
   for( int i = 0; i < pvpr_pb_type->num_ports; ++i ) 
   {
      if( pvpr_pb_type->ports[i].port_class && 
          strstr( pvpr_pb_type->ports[i].port_class, "data" ) == pvpr_pb_type->ports[i].port_class ) 
      {
         if( num_pb == OPEN ) 
         {
            num_pb = pvpr_pb_type->ports[i].num_pins;
         } 
      }
   }

   pvpr_mode->num_pb_type_children = 1;
   pvpr_mode->pb_type_children = static_cast< t_pb_type* >( TC_calloc( 1, sizeof( t_pb_type )));

   this->PokePbTypeChild_( *pvpr_pb_type, srMemoryName.data( ), 
                           &pvpr_mode->pb_type_children[0] );
   pvpr_mode->pb_type_children[0].depth = pvpr_pb_type->depth + 1;
   pvpr_mode->pb_type_children[0].parent_mode = &pvpr_pb_type->modes[0];
   pvpr_mode->pb_type_children[0].num_pb = num_pb;

   pvpr_pb_type->num_modes = 1;
   pvpr_pb_type->blif_model = 0;
   pvpr_pb_type->model = 0;

   pvpr_mode->num_interconnect = pvpr_pb_type->num_ports * num_pb;
   pvpr_mode->interconnect = static_cast< t_interconnect* >( TC_calloc( pvpr_mode->num_interconnect, sizeof( t_interconnect )));

   // [VPR] Process interconnect
   int index = 0;
   for( int i = 0; i < pvpr_pb_type->num_ports; ++i ) 
   {
      t_interconnect* pvpr_interconnect = &pvpr_mode->interconnect[index];
      pvpr_interconnect->type = DIRECT_INTERC;

      string srInputPortName( pvpr_pb_type->ports[i].name );
      string srOutputPortName( pvpr_pb_type->ports[i].name );
      string srInputName(( pvpr_pb_type->ports[i].type == IN_PORT ) ? pvpr_pb_type->name : srMemoryName );
      string srOutputName(( pvpr_pb_type->ports[i].type == IN_PORT ) ? srMemoryName : pvpr_pb_type->name );

      if( pvpr_pb_type->ports[i].port_class &&
          strstr( pvpr_pb_type->ports[i].port_class, "data" ) == pvpr_pb_type->ports[i].port_class ) 
      {
         pvpr_interconnect = &( pvpr_mode->interconnect )[index];

         pvpr_interconnect->name = static_cast< char* >( TC_calloc( index / 10 + 8, sizeof( char )));
         sprintf( pvpr_interconnect->name, "direct%d", index );

         if( pvpr_pb_type->ports[i].type == IN_PORT ) 
         {
            // [VPR] Force data pins to be one bit wide and update stats
            pvpr_mode->pb_type_children[0].ports[i].num_pins = 1;
            pvpr_mode->pb_type_children[0].num_input_pins -= ( pvpr_pb_type->ports[i].num_pins - 1 );

            pvpr_interconnect->input_string = static_cast< char* >( TC_calloc( srInputName.length( ) + srInputPortName.length( ) + 2, sizeof( char )));
            pvpr_interconnect->output_string = static_cast< char* >( TC_calloc( srOutputName.length( ) + srOutputPortName.length( ) + 2 * ( 6 + num_pb / 10 ), sizeof( char )));
            sprintf( pvpr_interconnect->input_string, "%s.%s", srInputName.data( ), srInputPortName.data( ));
            sprintf( pvpr_interconnect->output_string, "%s[%d:0].%s", srOutputName.data( ), num_pb - 1, srOutputPortName.data( ));
         } 
         else 
         {
            // [VPR] Force data pins to be one bit wide and update stats
            pvpr_mode->pb_type_children[0].ports[i].num_pins = 1;
            pvpr_mode->pb_type_children[0].num_output_pins -= ( pvpr_pb_type->ports[i].num_pins - 1 );

            pvpr_interconnect->input_string = static_cast< char* >( TC_calloc( srInputName.length( ) + srInputPortName.length( ) + 2 * ( 6 + num_pb / 10 ), sizeof( char )));
            pvpr_interconnect->output_string = static_cast< char* >( TC_calloc( srOutputName.length( ) + srOutputPortName.length( ) + 2, sizeof( char )));
            sprintf( pvpr_interconnect->input_string, "%s[%d:0].%s", srInputName.data( ), num_pb - 1, srInputPortName.data( ));
            sprintf( pvpr_interconnect->output_string, "%s.%s", srOutputName.data( ), srOutputPortName.data( ));
         }
         ++index;
      } 
      else 
      {
         for( int j = 0; j < num_pb; ++j ) 
         {
            pvpr_interconnect = &( pvpr_mode->interconnect )[index];
            pvpr_interconnect->name = static_cast< char* >( TC_calloc( index / 10 + j / 10 + 10, sizeof( char )));
            sprintf( pvpr_interconnect->name, "direct%d_%d", index, j );

            if( pvpr_pb_type->ports[i].type == IN_PORT ) 
            {
               pvpr_interconnect->type = DIRECT_INTERC;
               pvpr_interconnect->input_string = static_cast< char* >( TC_calloc( srInputName.length( ) + srInputPortName.length( ) + 2, sizeof( char )));
               pvpr_interconnect->output_string = static_cast< char* >( TC_calloc( srOutputName.length( ) + srOutputPortName.length( ) + 2 * ( 6 + num_pb / 10 ), sizeof( char )));
               sprintf( pvpr_interconnect->input_string, "%s.%s", srInputName.data( ), srInputPortName.data( ));
               sprintf( pvpr_interconnect->output_string, "%s[%d:%d].%s", srOutputName.data( ), j, j, srOutputPortName.data( ));
            } 
            else 
            {
               pvpr_interconnect->type = DIRECT_INTERC;
               pvpr_interconnect->input_string = static_cast< char* >( TC_calloc( srInputName.length( ) + srInputPortName.length( ) + 2 * ( 6 + num_pb / 10 ), sizeof( char )));
               pvpr_interconnect->output_string = static_cast< char* >( TC_calloc( srOutputName.length( ) + srOutputPortName.length( ) + 2, sizeof( char )));
               sprintf( pvpr_interconnect->input_string, "%s[%d:%d].%s", srInputName.data( ), j, j, srInputPortName.data( ));
               sprintf( pvpr_interconnect->output_string, "%s.%s", srOutputName.data( ), srOutputPortName.data( ));
            }
            ++index;
         }
      }
   }
   pvpr_mode->num_interconnect = index;
}

//===========================================================================//
// Method         : PokeModeList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokeModeList_(
      const TAS_PhysicalBlock_c& physicalBlock,
            t_pb_type*           pvpr_pb_type ) const
{
   // [VPR] Process modes
   if( physicalBlock.modeList.IsValid( ))
   {
      const TAS_ModeList_t& modeList = physicalBlock.modeList;

      pvpr_pb_type->num_modes = static_cast< int >( modeList.GetLength( ));
      pvpr_pb_type->modes = static_cast< t_mode* >( TC_calloc( pvpr_pb_type->num_modes, sizeof( t_mode )));

      for( size_t i = 0; i < modeList.GetLength( ); ++i )
      {
         pvpr_pb_type->modes[i].parent_pb_type = pvpr_pb_type;
         pvpr_pb_type->modes[i].index = static_cast< int >( i );

         this->PokeMode_( modeList[i]->srName,
                          modeList[i]->physicalBlockList,
                          modeList[i]->interconnectList,
                          &( pvpr_pb_type->modes[i] ));
      }
   }
   else 
   {
      // [VPR] Implied one mode
      pvpr_pb_type->num_modes = 1;
      pvpr_pb_type->modes = static_cast< t_mode* >( TC_calloc( pvpr_pb_type->num_modes, sizeof( t_mode )));

      pvpr_pb_type->modes[0].parent_pb_type = pvpr_pb_type;
      pvpr_pb_type->modes[0].index = 0;

      this->PokeMode_( physicalBlock.srName,
                       physicalBlock.physicalBlockList,
                       physicalBlock.interconnectList,
                       &( pvpr_pb_type->modes[0] ));
   }
}

//===========================================================================//
// Method         : PokeMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokeMode_(
      const string&                  srName,
      const TAS_PhysicalBlockList_t& physicalBlockList,
      const TAS_InterconnectList_t&  interconnectList,
            t_mode*                  pvpr_mode ) const
{
   pvpr_mode->name = TC_strdup( srName );

   if( physicalBlockList.IsValid( ))
   {
      pvpr_mode->num_pb_type_children = static_cast< int >( physicalBlockList.GetLength( ));
      pvpr_mode->pb_type_children = static_cast< t_pb_type* >( TC_calloc( pvpr_mode->num_pb_type_children, sizeof( t_pb_type )));

      for( size_t i = 0; i < physicalBlockList.GetLength( ); ++i )
      {
         const TAS_PhysicalBlock_c& physicalBlock = *physicalBlockList[i];
         this->PokePbType_( physicalBlock, pvpr_mode, 
                            &pvpr_mode->pb_type_children[i] );
      }
   } 
   else 
   {
      pvpr_mode->num_pb_type_children = 0;
      pvpr_mode->pb_type_children = 0;
   }

   if( interconnectList.IsValid( ))
   {
      this->PokeInterconnectList_( interconnectList, pvpr_mode );
   }
}

//===========================================================================//
// Method         : PokeInterconnectList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokeInterconnectList_(
      const TAS_InterconnectList_t& interconnectList,
            t_mode*                 pvpr_mode ) const
{
   pvpr_mode->num_interconnect = static_cast< int >( interconnectList.GetLength( ));
   pvpr_mode->interconnect = static_cast< t_interconnect* >( TC_calloc( pvpr_mode->num_interconnect, sizeof( t_interconnect )));
   int index = 0;
   for( size_t i = 0; i < interconnectList.GetLength( ); ++i )
   {
      const TAS_Interconnect_c& interconnect = *interconnectList[i];
      if( interconnect.mapType != TAS_INTERCONNECT_MAP_COMPLETE )
         continue;
   
      pvpr_mode->interconnect[index].parent_mode_index = pvpr_mode->index;
      this->PokeInterconnect_( interconnect,
                               &pvpr_mode->interconnect[index] );
      ++index;
   }
   for( size_t i = 0; i < interconnectList.GetLength( ); ++i )
   {
      const TAS_Interconnect_c& interconnect = *interconnectList[i];
      if( interconnect.mapType != TAS_INTERCONNECT_MAP_DIRECT )
         continue;
   
      pvpr_mode->interconnect[index].parent_mode_index = pvpr_mode->index;
      this->PokeInterconnect_( interconnect,
                               &pvpr_mode->interconnect[index] );
      ++index;
   }
   for( size_t i = 0; i < interconnectList.GetLength( ); ++i )
   {
      const TAS_Interconnect_c& interconnect = *interconnectList[i];
      if( interconnect.mapType != TAS_INTERCONNECT_MAP_MUX )
         continue;
   
      pvpr_mode->interconnect[index].parent_mode_index = pvpr_mode->index;
      this->PokeInterconnect_( interconnect,
                               &pvpr_mode->interconnect[index] );
      ++index;
   }
}

//===========================================================================//
// Method         : PokeInterconnect_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokeInterconnect_(
      const TAS_Interconnect_c& interconnect,
            t_interconnect*     pvpr_interconnect ) const
{
   pvpr_interconnect->name = TC_strdup( interconnect.srName );
   pvpr_interconnect->type = this->FindInterconnectMapType_( interconnect.mapType );

   string srInputNameList;
   interconnect.inputNameList.ExtractString( &srInputNameList, false );
   pvpr_interconnect->input_string = TC_strdup( srInputNameList );

   string srOutputNameList;
   interconnect.outputNameList.ExtractString( &srOutputNameList, false );
   pvpr_interconnect->output_string = TC_strdup( srOutputNameList );

   this->PokeTimingDelayLists_( interconnect.timingDelayLists,
                                &pvpr_interconnect->annotations,
                                &pvpr_interconnect->num_annotations );
   pvpr_interconnect->line_num = 0;
}

//===========================================================================//
// Method         : PokeFc_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokeFc_( 
      const TAS_PhysicalBlock_c& physicalBlock,
            t_type_descriptor*   pvpr_physicalBlock ) const
{
   TAS_ConnectionBoxType_t fcInType = physicalBlock.fcIn.type;
   float fcInVal = -1.0;
   switch( fcInType )
   {
   case TAS_CONNECTION_BOX_FRACTION:
      fcInVal = static_cast< float >( physicalBlock.fcIn.percent );
      break;
   case TAS_CONNECTION_BOX_ABSOLUTE:
      fcInVal = static_cast< float >( physicalBlock.fcIn.absolute );
      break;
   case TAS_CONNECTION_BOX_FULL:
      break;
   case TAS_CONNECTION_BOX_UNDEFINED:
      break;
   }
   
   TAS_ConnectionBoxType_t fcOutType = physicalBlock.fcOut.type;
   float fcOutVal = -1.0;
   switch( physicalBlock.fcOut.type )
   {
   case TAS_CONNECTION_BOX_FRACTION:
      fcOutVal = static_cast< float >( physicalBlock.fcOut.percent );
      break;
   case TAS_CONNECTION_BOX_ABSOLUTE:
      fcOutVal = static_cast< float >( physicalBlock.fcOut.absolute );
      break;
   case TAS_CONNECTION_BOX_FULL:
      fcOutVal = 0.0;
      break;
   case TAS_CONNECTION_BOX_UNDEFINED:
      break;
   }

   pvpr_physicalBlock->is_Fc_frac = static_cast< boolean* >( TC_calloc( pvpr_physicalBlock->num_pins, sizeof( boolean )));
   pvpr_physicalBlock->is_Fc_full_flex = static_cast< boolean* >( TC_calloc( pvpr_physicalBlock->num_pins, sizeof( boolean )));
   pvpr_physicalBlock->Fc = static_cast< float* >( TC_calloc( pvpr_physicalBlock->num_pins, sizeof( float )));

   for( int i = 0; i < pvpr_physicalBlock->num_pins; ++i )
   {
      int numClass = pvpr_physicalBlock->pin_class[i];
      if( pvpr_physicalBlock->class_inf[numClass].type == DRIVER )
      {
         pvpr_physicalBlock->Fc[i] = fcOutVal;
         pvpr_physicalBlock->is_Fc_full_flex[i] = fcOutType == TAS_CONNECTION_BOX_FULL ? 
                                                  static_cast< boolean >( true ) : static_cast< boolean >( false );
         pvpr_physicalBlock->is_Fc_frac[i] = fcOutType == TAS_CONNECTION_BOX_FRACTION ? 
                                             static_cast< boolean >( true ) : static_cast< boolean >( false );
      } 
      else if( pvpr_physicalBlock->class_inf[numClass].type == RECEIVER ) 
      {
         pvpr_physicalBlock->Fc[i] = fcInVal;
         pvpr_physicalBlock->is_Fc_full_flex[i] = fcInType == TAS_CONNECTION_BOX_FULL ? 
                                                  static_cast< boolean >( true ) : static_cast< boolean >( false );
         pvpr_physicalBlock->is_Fc_frac[i] = fcInType == TAS_CONNECTION_BOX_FRACTION ? 
                                             static_cast< boolean >( true ) : static_cast< boolean >( false );
      } 
      else 
      {
         pvpr_physicalBlock->Fc[i] = -1.0;
         pvpr_physicalBlock->is_Fc_full_flex[i] = static_cast< boolean >( false );
         pvpr_physicalBlock->is_Fc_frac[i] = static_cast< boolean >( false );
      }
   }
}

//===========================================================================//
// Method         : PokePinAssignList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokePinAssignList_(
      const TAS_PinAssignList_t& pinAssignList,
            t_type_descriptor*   pvpr_physicalBlock ) const
{
   // [VPR] Alloc and clear pin locations
   int width = static_cast< int >( pvpr_physicalBlock->width );
   int height = static_cast< int >( pvpr_physicalBlock->height );
   int num_pins = static_cast< int >( pvpr_physicalBlock->num_pins );

   pvpr_physicalBlock->pinloc = static_cast< int**** >( TC_calloc( width, sizeof( int** )));
   for( int i = 0; i < width; ++i ) 
   {
      pvpr_physicalBlock->pinloc[i] = static_cast< int*** >( TC_calloc( height, sizeof( int* )));
      for( int j = 0; j < pvpr_physicalBlock->height; ++j ) 
      {
         pvpr_physicalBlock->pinloc[i][j] = static_cast< int** >( TC_calloc( 4, sizeof( int* )));
         for( int side = 0; side < 4; ++side ) 
         {
            pvpr_physicalBlock->pinloc[i][j][side] = static_cast< int* >( TC_calloc( num_pins, sizeof( int )));
            for( int pin = 0; pin < num_pins; ++pin ) 
            {
               pvpr_physicalBlock->pinloc[i][j][side][pin] = 0;
            }
         }
      }
   }
   pvpr_physicalBlock->pin_width = static_cast< int* >( TC_calloc( pvpr_physicalBlock->num_pins, sizeof( int )));
   pvpr_physicalBlock->pin_height = static_cast< int* >( TC_calloc( pvpr_physicalBlock->num_pins, sizeof( int )));

   pvpr_physicalBlock->pin_loc_assignments = static_cast< char***** >( TC_calloc( width, sizeof( char**** )));
   pvpr_physicalBlock->num_pin_loc_assignments = static_cast< int*** >( TC_calloc( width, sizeof( int** )));
   for( int i = 0; i < width; ++i ) 
   {
      pvpr_physicalBlock->pin_loc_assignments[i] = static_cast< char**** >( TC_calloc( height, sizeof( char*** )));
      pvpr_physicalBlock->num_pin_loc_assignments[i] = static_cast< int** >( TC_calloc( height, sizeof( int* )));
      for( int j = 0; j < height; ++j ) 
      {
         pvpr_physicalBlock->pin_loc_assignments[i][j] = static_cast< char*** >( TC_calloc( 4, sizeof( char** )));
         pvpr_physicalBlock->num_pin_loc_assignments[i][j] = static_cast< int* >( TC_calloc( 4, sizeof( int )));
      }
   }

   if( pinAssignList.IsValid( ))
   {
      const TAS_PinAssign_c& pinAssign = *pinAssignList[0];
      pvpr_physicalBlock->pin_location_distribution = this->FindPinAssignPatternType_( pinAssign.pattern );
   }

   if( pvpr_physicalBlock->pin_location_distribution == E_CUSTOM_PIN_DISTR ) 
   {
      for( size_t i = 0; i < pinAssignList.GetLength( ); ++i )
      {
         const TAS_PinAssign_c& pinAssign = *pinAssignList[i];

         int offset = pinAssign.offset;
         int side = this->FindSideMode_( pinAssign.side );
         int len = static_cast< int >( pinAssign.portNameList.GetLength( ));

         pvpr_physicalBlock->num_pin_loc_assignments[0][offset][side] = len;
         pvpr_physicalBlock->pin_loc_assignments[0][offset][side] = static_cast< char** >( TC_calloc( len, sizeof( char* )));
         for( size_t port = 0; port < pinAssign.portNameList.GetLength( ); ++port )
         {
            const TC_Name_c& portName = *pinAssign.portNameList[port];
            pvpr_physicalBlock->pin_loc_assignments[0][offset][side][port] = TC_strdup( portName.GetName( ));
         }
      }
   }

   // [VPR] Setup pin classes
   const t_pb_type& vpr_pb_type = *pvpr_physicalBlock->pb_type;
   int numClass = 0;
   for( int i = 0; i < vpr_pb_type.num_ports; ++i ) 
   {
      if( vpr_pb_type.ports[i].equivalent ) 
      {
         numClass += pvpr_physicalBlock->capacity;
      } 
      else 
      {
         numClass += pvpr_physicalBlock->capacity * vpr_pb_type.ports[i].num_pins;
      }
   }
   pvpr_physicalBlock->num_class = numClass;
   pvpr_physicalBlock->class_inf = static_cast< t_class* >( TC_calloc( numClass, sizeof( t_class )));

   int numPins = pvpr_physicalBlock->capacity * pvpr_physicalBlock->num_pins;
   pvpr_physicalBlock->pin_class = static_cast< int* >( TC_calloc( numPins, sizeof( int )));
   pvpr_physicalBlock->is_global_pin = static_cast< boolean* >( TC_calloc( numPins, sizeof( boolean )));
   for( int i = 0; i < numPins; ++i ) 
   {
      pvpr_physicalBlock->pin_class[i] = OPEN;
      pvpr_physicalBlock->is_global_pin[i] = static_cast< boolean >( OPEN != 0 ? true : false );
   }

   // [VPR] Equivalent pins share the same class, non-equivalent pins belong to different pin classes
   t_class* pvpr_class_inf = pvpr_physicalBlock->class_inf;
   numClass = 0;
   numPins = 0;
   for( int i = 0; i < pvpr_physicalBlock->capacity; ++i ) 
   {
      for( int j = 0; j < vpr_pb_type.num_ports; ++j ) 
      {
         if( vpr_pb_type.ports[j].equivalent ) 
         {
            pvpr_class_inf[numClass].num_pins = vpr_pb_type.ports[j].num_pins;
            pvpr_class_inf[numClass].pinlist = static_cast< int* >( TC_calloc( pvpr_class_inf[numClass].num_pins, sizeof( int )));
         }

         for( int k = 0; k < vpr_pb_type.ports[j].num_pins; ++k ) 
         {
            if( !vpr_pb_type.ports[j].equivalent ) 
            {
               pvpr_class_inf[numClass].num_pins = 1;
               pvpr_class_inf[numClass].pinlist = static_cast< int* >( TC_calloc( 1, sizeof( int )));
               pvpr_class_inf[numClass].pinlist[0] = numPins;
            }
            else 
            {
               pvpr_class_inf[numClass].pinlist[k] = numPins;
            }

            if( vpr_pb_type.ports[j].type == IN_PORT ) 
            {
               pvpr_class_inf[numClass].type = RECEIVER;
            } 
            else 
            {
               pvpr_class_inf[numClass].type = DRIVER;
            }

            pvpr_physicalBlock->pin_class[numPins] = numClass;
            if( vpr_pb_type.ports[j].is_clock || vpr_pb_type.ports[j].is_non_clock_global )
            {
               pvpr_physicalBlock->is_global_pin[numPins] = static_cast< boolean >( true );
            }
            else
            {
               pvpr_physicalBlock->is_global_pin[numPins] = static_cast< boolean >( false );
            }

            ++numPins;

            if( !vpr_pb_type.ports[j].equivalent ) 
            {
               ++numClass;
            }
         }
         if( vpr_pb_type.ports[j].equivalent ) 
         {
            ++numClass;
         }
      }
   }
}

//===========================================================================//
// Method         : PokeGridAssignList__
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokeGridAssignList_( 
      const TAS_GridAssignList_t& gridAssignList,
            t_type_descriptor*    pvpr_physicalBlock ) const
{
   if( gridAssignList.IsValid( ))
   {
      pvpr_physicalBlock->num_grid_loc_def = static_cast< int >( gridAssignList.GetLength( ));
      pvpr_physicalBlock->grid_loc_def = static_cast< t_grid_loc_def* >( TC_calloc( pvpr_physicalBlock->num_grid_loc_def, sizeof( t_grid_loc_def )));
   }

   for( size_t i = 0; i < gridAssignList.GetLength( ); ++i )
   {
      const TAS_GridAssign_c& gridAssign = *gridAssignList[i];

      if( gridAssign.distr != TAS_GRID_ASSIGN_DISTR_UNDEFINED )
      {
         pvpr_physicalBlock->grid_loc_def[i].grid_loc_type = this->FindGridAssignDistrMode_( gridAssign.distr );

         switch( gridAssign.distr )
         {
         case TAS_GRID_ASSIGN_DISTR_SINGLE:
            pvpr_physicalBlock->grid_loc_def[i].col_rel = static_cast< float >( gridAssign.singlePercent );
            break;
         case TAS_GRID_ASSIGN_DISTR_MULTIPLE:
            pvpr_physicalBlock->grid_loc_def[i].start_col = gridAssign.multipleStart;
            pvpr_physicalBlock->grid_loc_def[i].repeat = gridAssign.multipleRepeat;
            break;
         case TAS_GRID_ASSIGN_DISTR_FILL:
            FILL_TYPE = pvpr_physicalBlock;
            break;
         case TAS_GRID_ASSIGN_DISTR_PERIMETER:
            // assert( IO_TYPE == pvpr_physicalBlock );
            break;
         case TAS_GRID_ASSIGN_DISTR_UNDEFINED:
            break;
         }

         if( gridAssign.priority > 0 )
         {
            pvpr_physicalBlock->grid_loc_def[i].priority = gridAssign.priority;
         }
         else
         {
            pvpr_physicalBlock->grid_loc_def[i].priority = 1;
         }
      }
   }
}

//===========================================================================//
// Method         : PokePortList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokePortList_(
      const TLO_PortList_t& portList,
            t_pb_type*      pvpr_pb_type ) const
{
   pvpr_pb_type->num_ports = static_cast< int >( portList.GetLength( ));
   pvpr_pb_type->ports = static_cast< t_port* >( TC_calloc( pvpr_pb_type->num_ports, sizeof( t_port )));
   pvpr_pb_type->num_clock_pins = 0;
   pvpr_pb_type->num_input_pins = 0;
   pvpr_pb_type->num_output_pins = 0;

   int index = 0;
   for( size_t i = 0; i < portList.GetLength( ); ++i )
   {   
      const TLO_Port_c& port = *portList[i];
      if( port.GetType( ) != TC_TYPE_INPUT )
         continue;

      this->PokePort_( port, &pvpr_pb_type->ports[index] );
      pvpr_pb_type->num_input_pins += pvpr_pb_type->ports[index].num_pins;

      pvpr_pb_type->ports[index].parent_pb_type = pvpr_pb_type;
      pvpr_pb_type->ports[index].index = index;
      pvpr_pb_type->ports[index].port_index_by_type = 0;
      ++index;
   }
   for( size_t i = 0; i < portList.GetLength( ); ++i )
   {   
      const TLO_Port_c& port = *portList[i];
      if( port.GetType( ) != TC_TYPE_OUTPUT )
         continue;

      this->PokePort_( port, &pvpr_pb_type->ports[index] );
      pvpr_pb_type->num_output_pins += pvpr_pb_type->ports[index].num_pins;

      pvpr_pb_type->ports[index].parent_pb_type = pvpr_pb_type;
      pvpr_pb_type->ports[index].index = index;
      pvpr_pb_type->ports[index].port_index_by_type = 0;
      ++index;
   }
   for( size_t i = 0; i < portList.GetLength( ); ++i )
   {   
      const TLO_Port_c& port = *portList[i];
      if( port.GetType( ) != TC_TYPE_CLOCK )
         continue;

      this->PokePort_( port, &pvpr_pb_type->ports[index] );
      pvpr_pb_type->num_clock_pins += pvpr_pb_type->ports[index].num_pins;

      pvpr_pb_type->ports[index].parent_pb_type = pvpr_pb_type;
      pvpr_pb_type->ports[index].index = index;
      pvpr_pb_type->ports[index].port_index_by_type = 0;
      ++index;
   }
}

//===========================================================================//
// Method         : PokePort_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokePort_(
      const TLO_Port_c& port,
            t_port*     pvpr_port ) const
{
   pvpr_port->name = TC_strdup( port.GetName( ));

   if( port.GetClass( ))
   {
      pvpr_port->port_class = TC_strdup( port.GetClass( ));
   }
   pvpr_port->equivalent = static_cast< boolean >( port.IsEquivalent( ));
   pvpr_port->num_pins = static_cast< int >( port.GetCount( ));

   if( port.GetType( ) == TC_TYPE_INPUT )
   {
      pvpr_port->type = IN_PORT;
      pvpr_port->is_clock = static_cast< boolean >( false );
   }
   if( port.GetType( ) == TC_TYPE_OUTPUT )
   {
      pvpr_port->type = OUT_PORT;
      pvpr_port->is_clock = static_cast< boolean >( false );
   }
   if( port.GetType( ) == TC_TYPE_CLOCK )
   {
      pvpr_port->type = IN_PORT;
      pvpr_port->is_clock = static_cast< boolean >( true );
   }
}

//===========================================================================//
// Method         : PokeTimingDelayLists_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokeTimingDelayLists_(
      const TAS_TimingDelayLists_c&   timingDelayLists,
            t_pin_to_pin_annotation** pvpr_annotationArray,
            int*                      pvpr_annotationCount ) const
{
   const TAS_TimingDelayList_t& delayList = timingDelayLists.delayList;
   const TAS_TimingDelayList_t& delayMatrixList = timingDelayLists.delayMatrixList;
   const TAS_TimingDelayList_t& tSetupList = timingDelayLists.tSetupList;
   const TAS_TimingDelayList_t& tHoldList = timingDelayLists.tHoldList;
   const TAS_TimingDelayList_t& clockToQList = timingDelayLists.clockToQList;
   const TAS_TimingDelayList_t& capList = timingDelayLists.capList;
   const TAS_TimingDelayList_t& capMatrixList = timingDelayLists.capMatrixList;
   const TAS_TimingDelayList_t& packPatternList = timingDelayLists.packPatternList;

   *pvpr_annotationCount = static_cast< int >( delayList.GetLength( ) +
                                               delayMatrixList.GetLength( ) +
                                               tSetupList.GetLength( ) +
                                               tHoldList.GetLength( ) +
                                               clockToQList.GetLength( ) +
                                               capList.GetLength( ) +
                                               capMatrixList.GetLength( ) +
                                               packPatternList.GetLength( ));

   *pvpr_annotationArray = static_cast< t_pin_to_pin_annotation* >( TC_calloc( *pvpr_annotationCount, sizeof( t_pin_to_pin_annotation )));
   int index = 0;
   for( size_t i = 0; i < delayList.GetLength( ); ++i )
   {
      const TAS_TimingDelay_c& delayConstant = *delayList[i];
      this->PokeTimingDelay_( delayConstant,
                              &( *pvpr_annotationArray )[index] );
      ++index;
   }
   for( size_t i = 0; i < delayMatrixList.GetLength( ); ++i )
   {
      const TAS_TimingDelay_c& delayMatrix = *delayMatrixList[i];
      this->PokeTimingDelay_( delayMatrix,
                              &( *pvpr_annotationArray )[index] );
      ++index;
   }
   for( size_t i = 0; i < tSetupList.GetLength( ); ++i )
   {
      const TAS_TimingDelay_c& tSetup = *tSetupList[i];
      this->PokeTimingDelay_( tSetup,
                              &( *pvpr_annotationArray )[index] );
      ++index;
   }
   for( size_t i = 0; i < tHoldList.GetLength( ); ++i )
   {
      const TAS_TimingDelay_c& tHold = *tHoldList[i];
      this->PokeTimingDelay_( tHold,
                              &( *pvpr_annotationArray )[index] );
      ++index;
   }
   for( size_t i = 0; i < clockToQList.GetLength( ); ++i )
   {
      const TAS_TimingDelay_c& clockToQ = *clockToQList[i];
      this->PokeTimingDelay_( clockToQ,
                              &( *pvpr_annotationArray )[index] );
      ++index;
   }
   for( size_t i = 0; i < capList.GetLength( ); ++i )
   {
      const TAS_TimingDelay_c& capConstant = *capList[i];
      this->PokeTimingDelay_( capConstant,
                              &( *pvpr_annotationArray )[index] );
      ++index;
   }
   for( size_t i = 0; i < capMatrixList.GetLength( ); ++i )
   {
      const TAS_TimingDelay_c& capMatrix = *capMatrixList[i];
      this->PokeTimingDelay_( capMatrix,
                              &( *pvpr_annotationArray )[index] );
      ++index;
   }
   for( size_t i = 0; i < packPatternList.GetLength( ); ++i )
   {
      const TAS_TimingDelay_c& packPattern = *packPatternList[i];
      this->PokeTimingDelay_( packPattern,
                              &( *pvpr_annotationArray )[index] );
      ++index;
   }
}

//===========================================================================//
// Method         : PokeTimingDelay_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
void TVPR_ArchitectureSpec_c::PokeTimingDelay_(
      const TAS_TimingDelay_c&       timingDelay,
            t_pin_to_pin_annotation* pvpr_annotation ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   char szValueMin[TIO_FORMAT_STRING_LEN_DATA];
   memset( szValueMin, 0, sizeof( szValueMin ));
   sprintf( szValueMin, "%0.*e", precision + 1, timingDelay.valueMin );

   char szValueMax[TIO_FORMAT_STRING_LEN_DATA];
   memset( szValueMax, 0, sizeof( szValueMax ));
   sprintf( szValueMax, "%0.*e", precision + 1, timingDelay.valueMax );

   char szValueNom[TIO_FORMAT_STRING_LEN_DATA];
   memset( szValueNom, 0, sizeof( szValueNom ));
   sprintf( szValueNom, "%0.*e", precision + 1, timingDelay.valueNom );

   string srDelayMatrix, srCapMatrix;
   timingDelay.valueMatrix.ExtractString( TC_DATA_EXP, &srDelayMatrix, precision + 1, SIZE_MAX, 0 );
   timingDelay.valueMatrix.ExtractString( TC_DATA_FLOAT, &srCapMatrix, precision, SIZE_MAX, 0 );

   size_t i = 0;
   size_t valueCt = ( TCTF_IsGT( timingDelay.valueMin, 0.0 ) && 
                      TCTF_IsGT( timingDelay.valueMax, 0.0 ) ?
                      2 : 1 );

   pvpr_annotation->num_value_prop_pairs = static_cast< int >( valueCt );
   pvpr_annotation->prop = static_cast< int* >( TC_calloc( valueCt, sizeof( int )));
   pvpr_annotation->value = static_cast< char** >( TC_calloc( valueCt, sizeof( char* )));

   switch( timingDelay.mode )
   {
   case TAS_TIMING_MODE_DELAY_CONSTANT:

      pvpr_annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
      pvpr_annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
      if( TCTF_IsGT( timingDelay.valueMax, 0.0 ))
      {
         pvpr_annotation->prop[i] = E_ANNOT_PIN_TO_PIN_DELAY_MAX;
         pvpr_annotation->value[i] = TC_strdup( szValueMax );
         ++i;
      }
      if( TCTF_IsGT( timingDelay.valueMin, 0.0 ))
      {
         pvpr_annotation->prop[i] = E_ANNOT_PIN_TO_PIN_DELAY_MIN;
         pvpr_annotation->value[i] = TC_strdup( szValueMin );
      }
      pvpr_annotation->input_pins = TC_strdup( timingDelay.srInputPortName );
      pvpr_annotation->output_pins = TC_strdup( timingDelay.srOutputPortName );
      break;

   case TAS_TIMING_MODE_DELAY_MATRIX:

      pvpr_annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
      pvpr_annotation->format = E_ANNOT_PIN_TO_PIN_MATRIX;
      pvpr_annotation->prop[0] = ( timingDelay.type == TAS_TIMING_TYPE_MAX_MATRIX ?
                                   E_ANNOT_PIN_TO_PIN_DELAY_MAX : 
                                   E_ANNOT_PIN_TO_PIN_DELAY_MIN );
      pvpr_annotation->value[0] = TC_strdup( srDelayMatrix );
      pvpr_annotation->input_pins = TC_strdup( timingDelay.srInputPortName );
      pvpr_annotation->output_pins = TC_strdup( timingDelay.srOutputPortName );
      break;

   case TAS_TIMING_MODE_T_SETUP:

      pvpr_annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
      pvpr_annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
      pvpr_annotation->prop[0] = E_ANNOT_PIN_TO_PIN_DELAY_TSETUP;
      pvpr_annotation->value[0] = TC_strdup( szValueNom );
      pvpr_annotation->input_pins = TC_strdup( timingDelay.srInputPortName );
      pvpr_annotation->clock = TC_strdup( timingDelay.srClockPortName );
      break;

   case TAS_TIMING_MODE_T_HOLD:

      pvpr_annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
      pvpr_annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
      pvpr_annotation->prop[0] = E_ANNOT_PIN_TO_PIN_DELAY_THOLD;
      pvpr_annotation->value[0] = TC_strdup( szValueNom );
      pvpr_annotation->input_pins = TC_strdup( timingDelay.srInputPortName );
      pvpr_annotation->clock = TC_strdup( timingDelay.srClockPortName );
      break;

   case TAS_TIMING_MODE_CLOCK_TO_Q:

      pvpr_annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
      pvpr_annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
      if( TCTF_IsGT( timingDelay.valueMax, 0.0 ))
      {
         pvpr_annotation->prop[i] = E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX;
         pvpr_annotation->value[i] = TC_strdup( szValueMax );
         ++i;
      }
      if( TCTF_IsGT( timingDelay.valueMin, 0.0 ))
      {
         pvpr_annotation->prop[i] = E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN;
         pvpr_annotation->value[i] = TC_strdup( szValueMin );
      }
      pvpr_annotation->input_pins = TC_strdup( timingDelay.srOutputPortName );
      pvpr_annotation->clock = TC_strdup( timingDelay.srClockPortName );
      break;

   case TAS_TIMING_MODE_CAP_CONSTANT:

      pvpr_annotation->type = E_ANNOT_PIN_TO_PIN_CAPACITANCE;
      pvpr_annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
      pvpr_annotation->prop[0] = E_ANNOT_PIN_TO_PIN_CAPACITANCE_C;
      pvpr_annotation->value[0] = TC_strdup( szValueMax );
      pvpr_annotation->input_pins = TC_strdup( timingDelay.srInputPortName );
      pvpr_annotation->output_pins = TC_strdup( timingDelay.srOutputPortName );
      break;

   case TAS_TIMING_MODE_CAP_MATRIX:

      pvpr_annotation->type = E_ANNOT_PIN_TO_PIN_CAPACITANCE;
      pvpr_annotation->format = E_ANNOT_PIN_TO_PIN_MATRIX;
      pvpr_annotation->prop[0] = E_ANNOT_PIN_TO_PIN_CAPACITANCE_C;
      pvpr_annotation->value[0] = TC_strdup( srCapMatrix );
      pvpr_annotation->input_pins = TC_strdup( timingDelay.srInputPortName );
      pvpr_annotation->output_pins = TC_strdup( timingDelay.srOutputPortName );
      break;

   case TAS_TIMING_MODE_PACK_PATTERN:

      pvpr_annotation->type = E_ANNOT_PIN_TO_PIN_PACK_PATTERN;
      pvpr_annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
      pvpr_annotation->prop[0] = E_ANNOT_PIN_TO_PIN_PACK_PATTERN_NAME;
      pvpr_annotation->value[0] = TC_strdup( timingDelay.srPackPatternName );
      pvpr_annotation->input_pins = TC_strdup( timingDelay.srInputPortName );
      pvpr_annotation->output_pins = TC_strdup( timingDelay.srOutputPortName );
      break;

   default:
      break;
   }
}

//===========================================================================//
// Method         : InitModels_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TVPR_ArchitectureSpec_c::InitModels_(
      const t_arch&             vpr_architecture,
            t_type_descriptor** pvpr_physicalBlockArray, 
            int                 vpr_physicalBlockCount ) const
{
   bool ok = true;

   for( int i = 0; i < vpr_physicalBlockCount; ++i ) 
   {
      if(( *pvpr_physicalBlockArray )[i].pb_type ) 
      {
         ok = this->InitPbType_( vpr_architecture, 
                                 ( *pvpr_physicalBlockArray )[i].pb_type );
      }
   }

   if( ok )
   {
      ok = this->ValidateModels_( vpr_architecture );
   }
   return( ok );
}

//===========================================================================//
// Method         : InitPbType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TVPR_ArchitectureSpec_c::InitPbType_(
      const t_arch&    vpr_architecture,
            t_pb_type* pvpr_pb_type ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   if( pvpr_pb_type->blif_model ) 
   {
      const char* pszBlifModelName = pvpr_pb_type->blif_model;
      char delimiter = (( strstr( pszBlifModelName, ".subckt " ) == pszBlifModelName ) ? ' ' : '.' );
      pszBlifModelName = strchr( pszBlifModelName, delimiter );
      if( pszBlifModelName ) 
      {
         ++pszBlifModelName;
      } 
      else 
      {
         ok = printHandler.Error( "Invalid architecture model detected!\n"
                                  "%sUnknown BLIF model defined in pb_type \"%s\".\n",
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_PSZ_STR( pvpr_pb_type->name ));
      }

      // [VPR] Apply two sets of models, standard library of models and user defined models
      t_model* pmodel = 0;
      if(( strcmp( pszBlifModelName, "input" ) == 0 ) ||
         ( strcmp( pszBlifModelName, "output" ) == 0 ) ||
         ( strcmp( pszBlifModelName, "names" ) == 0 ) ||
         ( strcmp( pszBlifModelName, "latch" ) == 0 )) 
      {
         pmodel = vpr_architecture.model_library;
      } 
      else 
      {
         pmodel = vpr_architecture.models;
      }

      // [VPR] Determine which logical model to use
      t_model* pvpr_model = 0;
      for( ; pmodel && !pvpr_model; pmodel = pmodel->next )
      {
         // [VPR] Blif model always starts with .subckt, need to skip first 8 characters
         if( strcmp( pszBlifModelName, pmodel->name ) == 0 ) 
         {
               pvpr_model = pmodel;
         }
      }
      if( ok && !pvpr_model )
      {
         ok = printHandler.Error( "Invalid architecture model pb_type detected!\n"
                                  "%sNo model found for pb_type \"%s\".\n",
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_PSZ_STR( pvpr_pb_type->blif_model ));
      }
      if( ok )
      {
         pvpr_pb_type->model = pvpr_model;

         t_linked_vptr* pvpr_pb_link = pvpr_model->pb_types;
         pvpr_model->pb_types = static_cast< t_linked_vptr* >( TC_calloc( 1, sizeof( t_linked_vptr )));
         pvpr_model->pb_types->next = pvpr_pb_link;
         pvpr_model->pb_types->data_vptr = pvpr_pb_type;

         for( int i = 0; i < pvpr_pb_type->num_ports; ++i ) 
         {
            bool foundPort = false;

            for( t_model_ports* pvpr_port = pvpr_model->inputs; 
                 pvpr_port && !foundPort ; 
                 pvpr_port = pvpr_port->next )
            {
               if( strcmp( pvpr_port->name, pvpr_pb_type->ports[i].name ) == 0 ) 
               {
                  if( pvpr_port->size < pvpr_pb_type->ports[i].num_pins ) 
                  {
                     pvpr_port->size = pvpr_pb_type->ports[i].num_pins;
                  }
                  if( pvpr_port->min_size > pvpr_pb_type->ports[i].num_pins || 
                      pvpr_port->min_size == -1 ) 
                  {
                     pvpr_port->min_size = pvpr_pb_type->ports[i].num_pins;
                  }
                  pvpr_pb_type->ports[i].model_port = pvpr_port;
                  foundPort = true;
               }
            }
            for( t_model_ports* pvpr_port = pvpr_model->outputs; 
                 pvpr_port && !foundPort ; 
                 pvpr_port = pvpr_port->next )
            {
               if( strcmp( pvpr_port->name, pvpr_pb_type->ports[i].name ) == 0 ) 
               {
                  if( pvpr_port->size < pvpr_pb_type->ports[i].num_pins ) 
                  {
                     pvpr_port->size = pvpr_pb_type->ports[i].num_pins;
                  }
                  if( pvpr_port->min_size > pvpr_pb_type->ports[i].num_pins || 
                      pvpr_port->min_size == -1 ) 
                  {
                     pvpr_port->min_size = pvpr_pb_type->ports[i].num_pins;
                  }
                  pvpr_pb_type->ports[i].model_port = pvpr_port;
                  foundPort = true;
               }
            }
            if( !foundPort )
            {
               ok = printHandler.Error( "Invalid architecture model pb_type detected!\n"
                                        "%sNo model port found for pb_type \"%s\" port \"%s\".\n",
                                        TIO_PREFIX_ERROR_SPACE,
                                        TIO_PSZ_STR( pvpr_pb_type->name ),
                                        TIO_PSZ_STR( pvpr_pb_type->ports[i].name )); 
            }
         }
      }
   } 
   else 
   {
      for( int i = 0; i < pvpr_pb_type->num_modes; ++i ) 
      {
         for( int j = 0; j < pvpr_pb_type->modes[i].num_pb_type_children; ++j ) 
         {
            ok = this->InitPbType_( vpr_architecture,
                                    &( pvpr_pb_type->modes[i].pb_type_children[j] ));
            if( !ok )
               break;
         }
         if( !ok )
            break;
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : ValidateModels_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TVPR_ArchitectureSpec_c::ValidateModels_(
      const t_arch& vpr_architecture ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   const t_model* pmodel = vpr_architecture.models;
   while( pmodel ) 
   {
      if( !pmodel->pb_types ) 
      {
         ok = printHandler.Error( "Invalid architecture model detected!\n"
                                  "%sNo pb_type found for model \"%s\".\n", 
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_PSZ_STR( pmodel->name ));
         if( !ok )
            break;
      }
      pmodel = pmodel->next;
   }
   return( ok );
}

//===========================================================================//
// Method         : FindSideMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
enum e_side TVPR_ArchitectureSpec_c::FindSideMode_(
      TC_SideMode_t mode ) const
{
   enum e_side mode_ = static_cast< enum e_side >( 0 );

   switch( mode )
   {
   case TC_SIDE_LEFT:   mode_ = LEFT;   break;
   case TC_SIDE_RIGHT:  mode_ = RIGHT;  break;
   case TC_SIDE_LOWER:
   case TC_SIDE_BOTTOM: mode_ = BOTTOM; break;
   case TC_SIDE_UPPER:
   case TC_SIDE_TOP:    mode_ = TOP;    break;
   default:                             break;
   }
   return( mode_ );
}

//===========================================================================//
// Method         : FindClassType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
enum e_pb_type_class TVPR_ArchitectureSpec_c::FindClassType_(
      TAS_ClassType_t type ) const
{
   enum e_pb_type_class type_ = UNKNOWN_CLASS;

   switch( type )
   {
   case TAS_CLASS_LUT:      type_ = LUT_CLASS;    break;
   case TAS_CLASS_FLIPFLOP: type_ = LATCH_CLASS;  break;
   case TAS_CLASS_MEMORY:   type_ = MEMORY_CLASS; break;
   default:                                       break;
   }
   return( type_ );
}

//===========================================================================//
// Method         : FindInterconnectMapType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
enum e_interconnect TVPR_ArchitectureSpec_c::FindInterconnectMapType_(
      TAS_InterconnectMapType_t type ) const
{
   enum e_interconnect type_ = static_cast< enum e_interconnect >( 0 );

   switch( type )
   {
   case TAS_INTERCONNECT_MAP_COMPLETE:  type_ = COMPLETE_INTERC; break;
   case TAS_INTERCONNECT_MAP_DIRECT:    type_ = DIRECT_INTERC;   break;
   case TAS_INTERCONNECT_MAP_MUX:       type_ = MUX_INTERC;      break;
   case TAS_INTERCONNECT_MAP_UNDEFINED:                          break;
   }
   return( type_ );
}

//===========================================================================//
// Method         : FindPinAssignPatternType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
enum e_pin_location_distr TVPR_ArchitectureSpec_c::FindPinAssignPatternType_(
      TAS_PinAssignPatternType_t type ) const
{
   enum e_pin_location_distr type_ = static_cast< enum e_pin_location_distr >( 0 );

   switch( type )
   {
   case TAS_PIN_ASSIGN_PATTERN_SPREAD:    type_ = E_SPREAD_PIN_DISTR; break;
   case TAS_PIN_ASSIGN_PATTERN_CUSTOM:    type_ = E_CUSTOM_PIN_DISTR; break;
   case TAS_PIN_ASSIGN_PATTERN_UNDEFINED:                             break;
   }
   return( type_ );
}

//===========================================================================//
// Method         : FindGridAssignDistrMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
enum e_grid_loc_type TVPR_ArchitectureSpec_c::FindGridAssignDistrMode_(
      TAS_GridAssignDistrMode_t mode ) const
{
   enum e_grid_loc_type type_ = static_cast< enum e_grid_loc_type >( 0 );

   switch( mode )
   {
   case TAS_GRID_ASSIGN_DISTR_SINGLE:    type_ = COL_REL;    break;
   case TAS_GRID_ASSIGN_DISTR_MULTIPLE:  type_ = COL_REPEAT; break;
   case TAS_GRID_ASSIGN_DISTR_FILL:      type_ = FILL;       break;
   case TAS_GRID_ASSIGN_DISTR_PERIMETER: type_ = BOUNDARY;   break;
   case TAS_GRID_ASSIGN_DISTR_UNDEFINED:                     break;
   }
   return( type_ );
}

//===========================================================================//
// Method         : FindChannelDistrMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/15/12 jeffr : Original
//===========================================================================//
enum e_stat TVPR_ArchitectureSpec_c::FindChannelDistrMode_( 
      TAS_ChannelDistrMode_t mode ) const
{
   enum e_stat mode_ = static_cast< enum e_stat >( 0 );

   switch( mode )
   {
   case TAS_CHANNEL_DISTR_UNIFORM:  mode_ = UNIFORM;  break;
   case TAS_CHANNEL_DISTR_GAUSSIAN: mode_ = GAUSSIAN; break;
   case TAS_CHANNEL_DISTR_PULSE:    mode_ = PULSE;    break;
   case TAS_CHANNEL_DISTR_DELTA:    mode_ = DELTA;    break;
   case TAS_CHANNEL_DISTR_UNDEFINED:                  break;
   }
   return( mode_ );
}

//===========================================================================//
// Method         : FindSwitchBoxModelType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
enum e_switch_block_type TVPR_ArchitectureSpec_c::FindSwitchBoxModelType_( 
      TAS_SwitchBoxModelType_t type ) const
{
   enum e_switch_block_type type_ = static_cast< enum e_switch_block_type >( 0 );

   switch( type )
   {
   case TAS_SWITCH_BOX_MODEL_WILTON:    type_ = WILTON;    break;
   case TAS_SWITCH_BOX_MODEL_SUBSET:    type_ = SUBSET;    break;
   case TAS_SWITCH_BOX_MODEL_UNIVERSAL: type_ = UNIVERSAL; break;
   case TAS_SWITCH_BOX_MODEL_CUSTOM:                       break;
   case TAS_SWITCH_BOX_MODEL_UNDEFINED:                    break;
   }
   return( type_ );
}

//===========================================================================//
// Method         : FindSegmentDirType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
enum e_directionality TVPR_ArchitectureSpec_c::FindSegmentDirType_( 
      TAS_SegmentDirType_t type ) const
{
   enum e_directionality type_ = static_cast< enum e_directionality >( 0 );

   switch( type )
   {
   case TAS_SEGMENT_DIR_UNIDIRECTIONAL: type_ = UNI_DIRECTIONAL; break;
   case TAS_SEGMENT_DIR_BIDIRECTIONAL:  type_ = BI_DIRECTIONAL;  break;
   case TAS_SEGMENT_DIR_UNDEFINED:                               break;
   }
   return( type_ );
}

//===========================================================================//
// Method         : FindSegmentBitPattern_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
bool TVPR_ArchitectureSpec_c::FindSegmentBitPattern_(
      const TAS_BitPattern_t& bitPattern,
            size_t            bitPatternLen,
            boolean**         pvpr_patternArray,
            int*              pvpr_patternLen ) const
{
   bool ok = true;

   *pvpr_patternLen = static_cast< int >( TCT_Max( bitPattern.GetLength( ), bitPatternLen ));
   *pvpr_patternArray = static_cast< boolean* >( TC_calloc( *pvpr_patternLen, sizeof( boolean )));
   ok = ( *pvpr_patternArray ? true : false );
   if( ok )
   {
      for( int i = 0; i < *pvpr_patternLen; ++i )
      {
         ( *pvpr_patternArray )[i] = static_cast< boolean >( true );
      }

      for( size_t i = 0; i < bitPattern.GetLength( ); ++i )
      {
         const TC_Bit_c& bit = *bitPattern[i];
         if( bit.IsTrue( ))
         {
            ( *pvpr_patternArray )[i] = static_cast< boolean >( true );
         }
         else // if( bit.IsFalse( ))
         {
            ( *pvpr_patternArray )[i] = static_cast< boolean >( false );
         }
      }
   }
   return( ok );
}
