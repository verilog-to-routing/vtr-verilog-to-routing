//===========================================================================//
// Purpose : Method definitions for the TVPR_CircuitDesign_c class.
//
//           Public methods include:
//           - TVPR_CircuitDesign_c, ~TVPR_CircuitDesign_c
//           - Export
//           - Import
//
//           Private methods include:
//           - InitStructures_
//           - PokeStructures_
//           - PokeInputOutputList_
//           - PokeInputOutput_
//           - PokeLogicList_
//           - PokeLogic_
//           - PokeLatchList_
//           - PokeLatch_
//           - PokeSubcktList_
//           - PokeSubckt_
//           - PokeBlockList_
//           - UpdateStructures_
//           - UpdateLogicalBlocks_
//           - UpdateVpackNets_
//           - UpdateAbsorbLogic_
//           - UpdateCompressLists_
//           - UpdatePrimaryCounts_
//           - LoadModelLibrary_
//           - PeekInputOutputList_
//           - PeekInputOutput_
//           - PeekPhysicalBlockList_
//           - PeekPhysicalBlock_
//           - PeekHierMapList_
//           - PeekNetList_
//           - ValidateModelList_
//           - ValidateSubcktList_
//           - ValidateInstList_
//           - AddVpackNet_
//           - AddLogicalBlockInputOutput_
//           - AddLogicalBlockLogic_
//           - AddLogicalBlockLatch_
//           - FreeVpackNet_
//           - FreeLogicalBlock_
//           - ExtractNetList_
//           - ExtractNetListInstPins_
//           - ExtractNetListRoutes_
//           - ExtractNetRoutes_
//           - IsNetOpen_
//           - MakeTraceNodeCountList_
//           - UpdateTraceSiblingCount_
//           - LoadTraceRoutes_
//           - LoadTraceNodes_
//           - FindModel_
//           - FindModelPort_
//           - FindModelPortCount_
//           - FindGraphPin_
//           - FindTraceListLength_
//           - FindTraceListNode_
//           - FindSwitchBoxCoord_
//           - FindSwitchBoxSides_
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

#include "TC_Typedefs.h"
#include "TC_MemoryUtils.h"

#include "TGS_ArrayGrid.h"

#include "TGO_Region.h"

#include "TNO_StringUtils.h"

#include "TVPR_CircuitDesign.h"

//===========================================================================//
// Method         : TVPR_CircuitDesign_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
TVPR_CircuitDesign_c::TVPR_CircuitDesign_c( 
      void )
{
}

//===========================================================================//
// Method         : ~TVPR_CircuitDesign_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
TVPR_CircuitDesign_c::~TVPR_CircuitDesign_c( 
      void )
{
}

//===========================================================================//
// Method         : Export
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
bool TVPR_CircuitDesign_c::Export(
      const TCD_CircuitDesign_c& circuitDesign,
      const t_model*             pvpr_standardModels,
      const t_model*             pvpr_customModels,
            t_net**              pvpr_netArray,
            int*                 pvpr_netCount,
            t_logical_block**    pvpr_logicalBlockArray,
            int*                 pvpr_logicalBlockCount,
            int*                 pvpr_primaryInputCount,
            int*                 pvpr_primaryOutputCount,
            bool                 deleteInvalidData ) const
{
   bool ok = true;

   blif_circuit_name = TC_strdup( circuitDesign.srName ); // VPR global trickery...

   TPO_InstList_t instList = circuitDesign.instList;
   const TPO_NameList_t& instNameList = circuitDesign.instNameList;
   const TPO_PortList_t& portList = circuitDesign.portList;
   const TPO_NameList_t& portNameList = circuitDesign.portNameList;
   const TLO_CellList_t& cellList = circuitDesign.cellList;
   const TPO_InstList_t& blockList = circuitDesign.blockList;
   const TNO_NetList_c& netList = circuitDesign.netList;
   const TNO_NameList_t netNameList = circuitDesign.netNameList;

   if( ok ) 
   {
      ok = this->ValidateModelList_( cellList,
                                     pvpr_customModels );
   }
   if( ok ) 
   {
      ok = this->ValidateSubcktList_( instList, cellList );
   }
   if( ok ) 
   {
      ok = this->ValidateInstList_( &instList, cellList,
                                    deleteInvalidData );
   }
   if( ok )
   {
      this->InitStructures_( instList, portList, 
                             netList, netNameList,
                             pvpr_netArray, pvpr_netCount,
                             pvpr_logicalBlockArray, pvpr_logicalBlockCount );

      ok = this->PokeStructures_( instList, instNameList,
                                  portList, portNameList, 
                                  blockList, netList,
                                  pvpr_standardModels, pvpr_customModels,
                                  *pvpr_netArray,
                                  *pvpr_logicalBlockArray, *pvpr_logicalBlockCount,
                                  pvpr_primaryInputCount, pvpr_primaryOutputCount );
   }
   if( ok )
   {
      ok = this->UpdateStructures_( pvpr_netArray, pvpr_netCount,
                                    pvpr_logicalBlockArray, pvpr_logicalBlockCount,
                                    pvpr_primaryInputCount, pvpr_primaryOutputCount );
   }
   return( ok );
}

//===========================================================================//
// Method         : Import
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::Import(
      const t_arch*              vpr_architecture,
            t_net*               vpr_netArray,
            int                  vpr_netCount,
      const t_block*             vpr_blockArray,
            int                  vpr_blockCount,
      const t_logical_block*     vpr_logicalBlockArray,
      const t_rr_node*           vpr_rrNodeArray,
            TCD_CircuitDesign_c* pcircuitDesign ) const
{ 
   TPO_InstList_t blockList( pcircuitDesign->blockList );
   TPO_InstList_t* pblockList = &pcircuitDesign->blockList;
   TNO_NetList_c* pnetList = &pcircuitDesign->netList;

   pblockList->Clear( );
   this->PeekNetList_( vpr_architecture, 
                       vpr_netArray, vpr_netCount,
                       vpr_blockArray, vpr_blockCount,
                       vpr_logicalBlockArray,
                       vpr_rrNodeArray,
                       pnetList );

   this->PeekInputOutputList_( vpr_blockArray, vpr_blockCount,
                               blockList, pblockList );
   this->PeekPhysicalBlockList_( vpr_blockArray, vpr_blockCount,
                                 vpr_logicalBlockArray,
                                 blockList, pblockList );
}

//===========================================================================//
// Method         : InitStructures_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::InitStructures_(
      const TPO_InstList_t&   instList,
      const TPO_PortList_t&   portList,
      const TNO_NetList_c&    netList,
      const TNO_NameList_t&   netNameList,
            t_net**           pvpr_netArray,
            int*              pvpr_netCount,
            t_logical_block** pvpr_logicalBlockArray,
            int*              pvpr_logicalBlockCount ) const
{
   *pvpr_netCount = static_cast< int >( netNameList.GetLength( ));
   *pvpr_logicalBlockCount = static_cast< int >( instList.GetLength( ) + portList.GetLength( ));

   *pvpr_netArray = static_cast< t_net* >( TC_calloc( *pvpr_netCount, sizeof( t_net )));
   t_net* vpr_netArray = *pvpr_netArray;
   for( int netIndex = 0; netIndex < *pvpr_netCount; ++netIndex ) 
   {
      const char* pszNetName = netNameList[netIndex]->GetName( );
      int netIndex_ = static_cast< int >( netList.FindIndex( pszNetName ));
      int count = static_cast< int >( netList[netIndex_]->FindInstPinCount( ));

      vpr_netArray[netIndex].num_sinks = 0;
      vpr_netArray[netIndex].name = TC_strdup( pszNetName );
      vpr_netArray[netIndex].node_block = static_cast< int* >( TC_calloc( count, sizeof( int )));
      vpr_netArray[netIndex].node_block_port = static_cast< int* >( TC_calloc( count, sizeof( int )));
      vpr_netArray[netIndex].node_block_pin = static_cast< int* >( TC_calloc( count, sizeof( int )));
      vpr_netArray[netIndex].is_global = static_cast< boolean >( false );
   }

   *pvpr_logicalBlockArray = static_cast< t_logical_block* >( TC_calloc( *pvpr_logicalBlockCount, sizeof( t_logical_block )));
   t_logical_block* vpr_logicalBlockArray = *pvpr_logicalBlockArray;
   for( int blockIndex = 0; blockIndex < *pvpr_logicalBlockCount; ++blockIndex ) 
   {
      vpr_logicalBlockArray[blockIndex].index = blockIndex;
   }
}

//===========================================================================//
// Method         : PokeStructures_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
bool TVPR_CircuitDesign_c::PokeStructures_(
      const TPO_InstList_t&   instList,
      const TPO_NameList_t&   instNameList,
      const TPO_PortList_t&   portList,
      const TPO_NameList_t&   portNameList,
      const TPO_InstList_t&   blockList,
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_standardModels,
      const t_model*          pvpr_customModels,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int               vpr_logicalBlockCount,
            int*              pvpr_primaryInputCount,
            int*              pvpr_primaryOutputCount ) const
{
   bool ok = true;

   // [VPR] Load default library models (inpad, outpad, logic, & latch)
   t_model* pvpr_inputModel; 
   t_model* pvpr_outputModel;
   t_model* pvpr_logicModel;
   t_model* pvpr_latchModel;
   ok = this->LoadModelLibrary_( pvpr_standardModels,
                                 &pvpr_inputModel, &pvpr_outputModel, 
                                 &pvpr_logicModel, &pvpr_latchModel );
   if( ok )
   {
      int vpr_blockIndex = 0;

      // [VPR] First, write ".inputs" and ".outputs" data
      this->PokeInputOutputList_( portList, portNameList, netList,
                                  pvpr_inputModel, pvpr_outputModel,
                                  vpr_netArray,
                                  vpr_logicalBlockArray, &vpr_blockIndex,
                                  pvpr_primaryInputCount, pvpr_primaryOutputCount );

      // [VPR] Then, write ".latch" flipflop data
      this->PokeLatchList_( instList, instNameList, netList,
                            pvpr_latchModel, 
                            vpr_netArray,
                            vpr_logicalBlockArray,
                            &vpr_blockIndex );

      // [VPR] Next, write ".names" LUT data
      this->PokeLogicList_( instList, instNameList, netList,
                            pvpr_logicModel, 
                            vpr_netArray,
                            vpr_logicalBlockArray,
                            &vpr_blockIndex );

      // [VPR] Next, write ".subckt" data, if any
      this->PokeSubcktList_( instList, instNameList, netList,
                             pvpr_customModels,
                             vpr_netArray,
                             vpr_logicalBlockArray,
                             &vpr_blockIndex );

      // Lastly, update VPR's block array based on optional placement regions
      this->PokeBlockList_( blockList,
                            vpr_logicalBlockArray,
                            vpr_logicalBlockCount );
   }
   return( ok );
}

//===========================================================================//
// Method         : PokeInputOutputList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PokeInputOutputList_(
      const TPO_PortList_t&   portList,
      const TPO_NameList_t&   portNameList,
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_inputModel,
      const t_model*          pvpr_outputModel,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int*              pvpr_blockIndex,
            int*              pvpr_primaryInputCount,
            int*              pvpr_primaryOutputCount ) const
{
   for( size_t i = 0; i < portNameList.GetLength( ); ++i )
   {
      const TC_Name_c& portName = *portNameList[i];
      const char* pszPortName = portName.GetName( );
      const TPO_Port_t& port = *portList.Find( pszPortName );

      this->PokeInputOutput_( port, netList,
                              pvpr_inputModel, pvpr_outputModel,
                              vpr_netArray, 
                              vpr_logicalBlockArray, pvpr_blockIndex,
                              pvpr_primaryInputCount, pvpr_primaryOutputCount );
   }
}

//===========================================================================//
// Method         : PokeInputOutput_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PokeInputOutput_(
      const TPO_Port_t&       port,
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_inputModel,
      const t_model*          pvpr_outputModel,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int*              pvpr_blockIndex,
            int*              pvpr_primaryInputCount,
            int*              pvpr_primaryOutputCount ) const
{
   const char* pszPortName = port.GetName( );
   int pinType = ( port.GetInputOutputType( ) == TC_TYPE_OUTPUT ? DRIVER : RECEIVER );
   bool isGlobal = false;

   int blockIndex = *pvpr_blockIndex;
   int portIndex  = 0;
   int pinIndex = 0;

   int netIndex = this->AddVpackNet_( pszPortName, pinType, isGlobal,
                                      blockIndex, portIndex, pinIndex, 
                                      netList, vpr_netArray );

   this->AddLogicalBlockInputOutput_( pszPortName, pinType, netIndex,
                                      pvpr_inputModel, pvpr_outputModel,
                                      vpr_logicalBlockArray, blockIndex,
                                      pvpr_primaryInputCount, pvpr_primaryOutputCount );
   ++*pvpr_blockIndex;
}

//===========================================================================//
// Method         : PokeLogicList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PokeLogicList_(
      const TPO_InstList_t&   instList,
      const TPO_NameList_t&   instNameList,
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_logicModel,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int*              pvpr_blockIndex ) const
{
   for( size_t i = 0; i < instNameList.GetLength( ); ++i )
   {
      const TC_Name_c& instName = *instNameList[i];
      const char* pszInstName = instName.GetName( );
      const TPO_Inst_c& inst = *instList.Find( pszInstName );

      if( inst.GetSource( ) != TPO_INST_SOURCE_NAMES )
         continue;

      const TPO_Pin_t* poutputPin = inst.FindPin( TC_TYPE_OUTPUT );
      if( poutputPin &&
          strcmp( poutputPin->GetName( ), "unconn" ) == 0 )
      {
         continue;
      }
      this->PokeLogic_( inst, netList, 
                        pvpr_logicModel, vpr_netArray, 
                        vpr_logicalBlockArray, pvpr_blockIndex );
   }
}

//===========================================================================//
// Method         : PokeLogic_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PokeLogic_(
      const TPO_Inst_c&       inst,
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_logicModel,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int*              pvpr_blockIndex ) const
{
   // [VPR] Add a LUT (.names) as VPACK_COMB to vpack_net and logical_block array
   int inputPinIndex = 0;

   const TPO_PinList_t& pinList = inst.GetPinList( );
   for( size_t i = 0; i < pinList.GetLength( ); ++i )
   {
      const TPO_Pin_t& pin = *pinList[i];
      const char* pszPinName = pin.GetName( );
      int pinType = ( pin.GetType( ) == TC_TYPE_OUTPUT ? DRIVER : RECEIVER );
      bool isGlobal = false;

      int blockIndex = *pvpr_blockIndex;
      int portIndex = 0;
      int pinIndex = 0;
      if( pin.GetType( ) == TC_TYPE_INPUT )
      {
         pinIndex = inputPinIndex;
         ++inputPinIndex;
      }
      this->AddVpackNet_( pszPinName, pinType, isGlobal,
                          blockIndex, portIndex, pinIndex,
                          netList, vpr_netArray );
   }

   const TPO_Pin_t* poutputPin = inst.FindPin( TC_TYPE_OUTPUT );
   this->AddLogicalBlockLogic_( poutputPin->GetName( ), pinList, 
                                netList, pvpr_logicModel, 
                                vpr_logicalBlockArray, *pvpr_blockIndex );

   const TPO_LogicBitsList_t& logicBitsList = inst.GetNamesLogicBitsList( );
   for( size_t i = 0; i < logicBitsList.GetLength( ); ++i )
   {
      const TPO_LogicBits_t& logicBits = *logicBitsList[i];

      string srLogicBits;
      for( size_t j = 0; j < logicBits.GetLength( ); ++j )
      {
         string srLogicBit;
         logicBits[j]->ExtractString( &srLogicBit );

         srLogicBits += srLogicBit;
         srLogicBits += ( j + 2 == logicBits.GetLength( ) ? " " : "" );
      }

      t_linked_vptr* ptruth_table = static_cast< t_linked_vptr* >( TC_calloc( 1, sizeof( t_linked_vptr )));
      ptruth_table->next = vpr_logicalBlockArray[*pvpr_blockIndex].truth_table;
      ptruth_table->data_vptr = TC_strdup( srLogicBits );
      vpr_logicalBlockArray[*pvpr_blockIndex].truth_table = ptruth_table;
   }
   ++*pvpr_blockIndex;
}

//===========================================================================//
// Method         : PokeLatchList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PokeLatchList_(
      const TPO_InstList_t&   instList,
      const TPO_NameList_t&   instNameList,
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_latchModel,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int*              pvpr_blockIndex ) const
{
   for( size_t i = 0; i < instNameList.GetLength( ); ++i )
   {
      const TC_Name_c& instName = *instNameList[i];
      const char* pszInstName = instName.GetName( );
      const TPO_Inst_c& inst = *instList.Find( pszInstName );

      if( inst.GetSource( ) != TPO_INST_SOURCE_LATCH )
         continue;

      this->PokeLatch_( inst, netList, 
                        pvpr_latchModel, vpr_netArray, 
                        vpr_logicalBlockArray, pvpr_blockIndex );
   }
}

//===========================================================================//
// Method         : PokeLatch_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PokeLatch_(
      const TPO_Inst_c&       inst,
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_latchModel,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int*              pvpr_blockIndex ) const
{
   const TPO_Pin_t* poutputPin = inst.FindPin( TC_TYPE_OUTPUT );
   const TPO_Pin_t* pinputPin = inst.FindPin( TC_TYPE_INPUT );
   const TPO_Pin_t* pclockPin = inst.FindPin( TC_TYPE_CLOCK );
   if( pinputPin && poutputPin && pclockPin )
   {
      // [VPR] Add a flipflop (.latch) as VPACK_LATCH to the logical_block array
      int blockIndex = *pvpr_blockIndex;
      int portIndex = 0;
      int pinIndex = 0;

      this->AddVpackNet_( poutputPin->GetName( ), DRIVER, false,
                          blockIndex, portIndex, pinIndex,
                          netList, vpr_netArray );
      this->AddVpackNet_( pinputPin->GetName( ), RECEIVER, false,
                          blockIndex, portIndex, pinIndex,
                          netList, vpr_netArray );
      this->AddVpackNet_( pclockPin->GetName( ), RECEIVER, true,
                          blockIndex, portIndex, pinIndex, 
                          netList, vpr_netArray );

      this->AddLogicalBlockLatch_( poutputPin->GetName( ), 
                                   pinputPin->GetName( ), 
                                   pclockPin->GetName( ),
                                   netList, pvpr_latchModel,
                                   vpr_logicalBlockArray, *pvpr_blockIndex );
      ++*pvpr_blockIndex;
   }
}

//===========================================================================//
// Method         : PokeSubcktList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PokeSubcktList_(
      const TPO_InstList_t&   instList,
      const TPO_NameList_t&   instNameList,
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_customModels,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int*              pvpr_blockIndex ) const
{
   for( size_t i = 0; i < instNameList.GetLength( ); ++i )
   {
      const TC_Name_c& instName = *instNameList[i];
      const char* pszInstName = instName.GetName( );
      const TPO_Inst_c& inst = *instList.Find( pszInstName );

      if( inst.GetSource( ) != TPO_INST_SOURCE_SUBCKT )
         continue;

      this->PokeSubckt_( inst, netList, 
                         pvpr_customModels, vpr_netArray, 
                         vpr_logicalBlockArray, pvpr_blockIndex );
   }
}

//===========================================================================//
// Method         : PokeSubckt_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PokeSubckt_(
      const TPO_Inst_c&       inst,
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_customModels,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int*              pvpr_blockIndex ) const
{
   int blockIndex = *pvpr_blockIndex;

   vpr_logicalBlockArray[blockIndex].type = VPACK_COMB;

   // [VPR] Get and recode library model matching this subckt cell name
   const char* pszCellName = inst.GetCellName( );
   const t_model* pvpr_model = this->FindModel_( pvpr_customModels, pszCellName );
   vpr_logicalBlockArray[blockIndex].model = const_cast< t_model* >( pvpr_model );

   // [VPR] Allocate space for inputs and initialize all input nets to OPEN
   int inputPortCount = this->FindModelPortCount_( pvpr_model, TC_TYPE_INPUT );
   vpr_logicalBlockArray[blockIndex].input_nets = static_cast< int** >( TC_calloc( inputPortCount, sizeof( int* )));

   for( const t_model_ports* pvpr_port = pvpr_model->inputs; pvpr_port; pvpr_port = pvpr_port->next )
   {
      if( pvpr_port->is_clock )
         continue;

      int portIndex = pvpr_port->index;
      int pinCount = pvpr_port->size;
      vpr_logicalBlockArray[blockIndex].input_nets[portIndex] = static_cast< int* >( TC_calloc( pinCount, sizeof( int )));
      for( int pinIndex = 0; pinIndex < pinCount; ++pinIndex ) 
      {
         vpr_logicalBlockArray[blockIndex].input_nets[portIndex][pinIndex] = OPEN;
      }
   }

   // [VPR] Allocate space for outputs and initialize all input nets to OPEN
   int outputPortCount = this->FindModelPortCount_( pvpr_model, TC_TYPE_OUTPUT );
   vpr_logicalBlockArray[blockIndex].output_nets = static_cast< int** >( TC_calloc( outputPortCount, sizeof( int* )));

   for( const t_model_ports* pvpr_port = pvpr_model->outputs; pvpr_port; pvpr_port = pvpr_port->next )
   {
      int portIndex = pvpr_port->index;
      int pinCount = pvpr_port->size;
      vpr_logicalBlockArray[blockIndex].output_nets[portIndex] = static_cast< int* >( TC_calloc( pinCount, sizeof( int )));
      for( int pinIndex = 0; pinIndex < pinCount; ++pinIndex ) 
      {
         vpr_logicalBlockArray[blockIndex].output_nets[portIndex][pinIndex] = OPEN;
      }
   }

   // [VPR] Initialize clock data
   vpr_logicalBlockArray[blockIndex].clock_net = OPEN;

   vpr_logicalBlockArray[blockIndex].truth_table = 0;

   // [VPR] Write output/input/clock based on pin-to-pin mappings
   const TPO_PinMapList_t& pinMapList = inst.GetSubcktPinMapList( );
   for( size_t i = 0; i < pinMapList.GetLength( ); ++i )
   {
      const TPO_PinMap_c& pinMap = *pinMapList[i];
      const char* pszCellPinName = pinMap.GetCellPinName( );
      const char* pszInstPinName = pinMap.GetInstPinName( );
   
      size_t index; 
      TNO_ParseNameIndex( pszCellPinName, 0, &index );
      int cellPinIndex = static_cast< int >( index );

      const t_model_ports* pvpr_port = 0;
      pvpr_port = this->FindModelPort_( pvpr_model, pszCellPinName, TC_TYPE_OUTPUT );
      if( pvpr_port )
      {
         int portIndex = pvpr_port->index;
         int pinIndex = cellPinIndex;
         int netIndex = this->AddVpackNet_( pszInstPinName, DRIVER, false,
                                            blockIndex, portIndex, pinIndex, 
                                            netList, vpr_netArray );

         vpr_logicalBlockArray[blockIndex].output_nets[portIndex][pinIndex] = netIndex;

         // [VPR] Record subckt block name based on first output pin processed
         if( !vpr_logicalBlockArray[blockIndex].name )
         {
            vpr_logicalBlockArray[blockIndex].name = TC_strdup( pszInstPinName );
         }
      }
      pvpr_port = this->FindModelPort_( pvpr_model, pszCellPinName, TC_TYPE_INPUT );
      if( pvpr_port )
      {
         int portIndex = pvpr_port->index;
         int pinIndex = cellPinIndex;
         int netIndex = this->AddVpackNet_( pszInstPinName, RECEIVER, false,
                                            blockIndex, portIndex, pinIndex, 
                                            netList, vpr_netArray );

         vpr_logicalBlockArray[blockIndex].input_nets[portIndex][pinIndex] = netIndex;
      }
      pvpr_port = this->FindModelPort_( pvpr_model, pszCellPinName, TC_TYPE_CLOCK );
      if( pvpr_port )
      {
         int portIndex = pvpr_port->index;
         int pinIndex = cellPinIndex;
         int netIndex = this->AddVpackNet_( pszInstPinName, RECEIVER, true,
                                            blockIndex, portIndex, pinIndex, 
                                            netList, vpr_netArray );

         vpr_logicalBlockArray[blockIndex].clock_net = netIndex;
      }
   }
   ++*pvpr_blockIndex;
}

//===========================================================================//
// Method         : PokeBlockList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PokeBlockList_(
      const TPO_InstList_t&  blockList,
            t_logical_block* vpr_logicalBlockArray,
            int              vpr_logicalBlockCount ) const
{
   if( blockList.IsValid( ))
   {
      // Iterate for every logical block to update optional placement regions
      for( int blockIndex = 0; blockIndex < vpr_logicalBlockCount; ++blockIndex ) 
      {
         const char* pszBlockName = vpr_logicalBlockArray[blockIndex].name;
         const TPO_Inst_c* pblock = blockList.Find( pszBlockName );
         if( !pblock )
            continue;

         const TGS_RegionList_t& placeRegionList = pblock->GetPlaceRegionList( );
         for( size_t i = 0; i < placeRegionList.GetLength( ); ++i )
         {
            // For each optional placement region, snap to VPR's placement array grid
            TGS_Region_c placeRegion = *placeRegionList[i];

            TGS_ArrayGrid_c placeArrayGrid( 1.0, 1.0 );
            placeArrayGrid.SnapToGrid( placeRegion, &placeRegion );

            TGO_Region_c placementRegion( static_cast< int >( placeRegion.x1 ),
                                          static_cast< int >( placeRegion.y1 ),
                                          static_cast< int >( placeRegion.x2 ),
                                          static_cast< int >( placeRegion.y2 ));

            // Add optional placement region to VPR's logical block
            // (so that VPR will subsequently write region to VPR's .net file)
            // (so that VPR will subsequently read region into VPR's block array)
            vpr_logicalBlockArray[blockIndex].placement_region_list.Add( placementRegion );
         }
      }
   }
}

//===========================================================================//
// Method         : UpdateStructures_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
bool TVPR_CircuitDesign_c::UpdateStructures_(
            t_net**           pvpr_netArray,
            int*              pvpr_netCount,
            t_logical_block** pvpr_logicalBlockArray,
            int*              pvpr_logicalBlockCount,
            int*              pvpr_primaryInputCount,
            int*              pvpr_primaryOutputCount ) const
{
   bool ok = true;

   // Apply post-processing on the logical block and vpack net lists
   this->UpdateLogicalBlocks_( *pvpr_logicalBlockArray, *pvpr_logicalBlockCount );

   ok = this->UpdateVpackNets_( *pvpr_netArray,
                                *pvpr_logicalBlockArray, *pvpr_logicalBlockCount );

   // [VPR] Remove buffer LUTs (ie. single-input LUTs that are programmed to be a wire)
   if( ok )
   {
      ok = this->UpdateAbsorbLogic_( *pvpr_netArray,
                                     *pvpr_logicalBlockArray, *pvpr_logicalBlockCount );
   }

   // [VPR] Remove VPACK_EMPTY blocks and OPEN nets left post synthesis
   if( ok )
   {
      ok = this->UpdateCompressLists_( pvpr_netArray, pvpr_netCount,
                                       pvpr_logicalBlockArray, pvpr_logicalBlockCount );
   }

   if( ok )
   {
      this->UpdatePrimaryCounts_( *pvpr_logicalBlockArray, *pvpr_logicalBlockCount,
                                  pvpr_primaryInputCount, pvpr_primaryOutputCount );
   }
   return( ok );
}

//===========================================================================//
// Method         : UpdateLogicalBlocks_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::UpdateLogicalBlocks_(
      t_logical_block* vpr_logicalBlockArray,
      int              vpr_logicalBlockCount ) const
{
   for( int blockIndex = 0; blockIndex < vpr_logicalBlockCount; ++blockIndex )
   {
      if( !vpr_logicalBlockArray[blockIndex].model )
         continue;

      int inputPinCount = 0;
      for( t_model_ports* pvpr_port = vpr_logicalBlockArray[blockIndex].model->inputs;
           pvpr_port;
           pvpr_port = pvpr_port->next )
      {
         if( pvpr_port->is_clock ) 
            continue;

         int portIndex = pvpr_port->index;
         int pinCount = pvpr_port->size;
         for( int pinIndex = 0; pinIndex < pinCount; ++pinIndex ) 
         {
            if( vpr_logicalBlockArray[blockIndex].input_nets[portIndex][pinIndex] == OPEN )
               continue;

            ++inputPinCount;
         }
      }
      vpr_logicalBlockArray[blockIndex].used_input_pins = inputPinCount;
   }
}

//===========================================================================//
// Method         : UpdateVpackNets_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
bool TVPR_CircuitDesign_c::UpdateVpackNets_(
            t_net*           vpr_netArray,
      const t_logical_block* vpr_logicalBlockArray,
            int              vpr_logicalBlockCount ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   for( int blockIndex = 0; blockIndex < vpr_logicalBlockCount; ++blockIndex )
   {
      if( !vpr_logicalBlockArray[blockIndex].model )
         continue;

      int inputPinCount = vpr_logicalBlockArray[blockIndex].used_input_pins;

      for( t_model_ports* pvpr_port = vpr_logicalBlockArray[blockIndex].model->outputs;
           pvpr_port;
           pvpr_port = pvpr_port->next )
      {
         int portIndex = pvpr_port->index;
         int pinCount = pvpr_port->size;
         for( int pinIndex = 0; pinIndex < pinCount; ++pinIndex ) 
         {
            if( vpr_logicalBlockArray[blockIndex].output_nets[portIndex][pinIndex] == OPEN )
               continue;

            int netIndex = vpr_logicalBlockArray[blockIndex].output_nets[portIndex][pinIndex];
            vpr_netArray[netIndex].is_const_gen = static_cast< boolean >( false );

            if(( inputPinCount == 0 ) && 
               ( vpr_logicalBlockArray[blockIndex].type != VPACK_INPAD ) &&
               ( vpr_logicalBlockArray[blockIndex].type != VPACK_OUTPAD ) &&
               ( vpr_logicalBlockArray[blockIndex].clock_net == OPEN ))
            {
               vpr_netArray[netIndex].is_const_gen = static_cast< boolean >( true );

               ok = printHandler.Warning( "Net \"%s\" is a constant generator.\n",
                                          TIO_PSZ_STR( vpr_netArray[netIndex].name ));
            }
            if( !ok )
               break;
         }
         if( !ok )
            break;
      }
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : UpdateAbsorbLogic_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
bool TVPR_CircuitDesign_c::UpdateAbsorbLogic_(
      t_net*           vpr_netArray,
      t_logical_block* vpr_logicalBlockArray,
      int              vpr_logicalBlockCount ) const
{ 
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   // [VPR] Delete any logical blocks (non-output) with 0 fanout ports
   for( int blockIndex = 0; blockIndex < vpr_logicalBlockCount; ++blockIndex )
   {
      if( vpr_logicalBlockArray[blockIndex].type == VPACK_OUTPAD ) 
         continue;

      if( !vpr_logicalBlockArray[blockIndex].model )
         continue;

      int outputCount = 0;
      for( const t_model_ports* pvpr_port = vpr_logicalBlockArray[blockIndex].model->outputs;
           pvpr_port;
           pvpr_port = pvpr_port->next )
      {
         for( int i = 0; i < pvpr_port->size; ++i )
         {
            if( vpr_logicalBlockArray[blockIndex].output_nets[pvpr_port->index][i] == OPEN )
               continue;

            ++outputCount;
         }
      }
      if( outputCount )
         continue;

      if( vpr_logicalBlockArray[blockIndex].type == VPACK_INPAD ) 
      {
         ok = printHandler.Warning( "Block \"%s\" has no fanout, deleting block.\n",
                                    TIO_PSZ_STR( vpr_logicalBlockArray[blockIndex].name ));
         vpr_logicalBlockArray[blockIndex].type = VPACK_EMPTY;
      }
      else
      {
         ok = printHandler.Warning( "Block \"%s\" has no fanout.\n",
                                    TIO_PSZ_STR( vpr_logicalBlockArray[blockIndex].name ));
      }
   }

   // [VPR] Delete any single-input LUTs that are programmed to be a wire
   int deletedLogicCount = 0;
   for( int blockIndex = 0; blockIndex < vpr_logicalBlockCount; ++blockIndex )
   {
      if( !vpr_logicalBlockArray[blockIndex].model )
         continue;

      if( strcmp( vpr_logicalBlockArray[blockIndex].model->name, "names") != 0 )
         continue;
   
      if( vpr_logicalBlockArray[blockIndex].truth_table &&
          vpr_logicalBlockArray[blockIndex].truth_table->data_vptr ) 
      {
         const char* pszTruthTable = static_cast< const char* >( vpr_logicalBlockArray[blockIndex].truth_table->data_vptr );
         if(( strcmp( pszTruthTable, "0 0" ) == 0 ) ||
            ( strcmp( pszTruthTable, "1 1" ) == 0 ))
         {
            int inputNetIndex = vpr_logicalBlockArray[blockIndex].input_nets[0][0];   // [VPR] Net driving this buffer
            int outputNetIndex = vpr_logicalBlockArray[blockIndex].output_nets[0][0]; // [VPR] Net this buffer is driving
            int outputBlockIndex = vpr_netArray[outputNetIndex].node_block[1];

            if( vpr_netArray[outputNetIndex].num_sinks != 1 ) 
               continue;

            int inputPinIndex = 0;
            for( inputPinIndex = 1; inputPinIndex <= vpr_netArray[inputNetIndex].num_sinks; inputPinIndex++) 
            {
               if( vpr_netArray[inputNetIndex].node_block[inputPinIndex] == blockIndex )
                  break;
            }

            vpr_logicalBlockArray[blockIndex].type = VPACK_EMPTY;   // [VPR] Mark logical_block that had LUT

            blockIndex = vpr_netArray[outputNetIndex].node_block[1];
            int portIndex = vpr_netArray[outputNetIndex].node_block_port[1];
            int pinIndex = vpr_netArray[outputNetIndex].node_block_pin[1];

            vpr_netArray[inputNetIndex].node_block[inputPinIndex] = blockIndex; // [VPR] New output
            vpr_netArray[inputNetIndex].node_block_port[inputPinIndex] = portIndex;
            vpr_netArray[inputNetIndex].node_block_pin[inputPinIndex] = pinIndex;

            vpr_logicalBlockArray[outputBlockIndex].input_nets[portIndex][pinIndex] = inputNetIndex;

            vpr_netArray[outputNetIndex].node_block[0] = OPEN;      // [VPR] This vpack_net disappears; mark it
            vpr_netArray[outputNetIndex].node_block_pin[0] = OPEN;  // "
            vpr_netArray[outputNetIndex].node_block_port[0] = OPEN; // "
            vpr_netArray[outputNetIndex].num_sinks = 0;             // "

            ++deletedLogicCount;
         }
      }
   }

   if( ok && deletedLogicCount )
   {
      ok = printHandler.Warning( "Deleted %d LUT buffers.\n", 
                                 deletedLogicCount );
   }
   return( ok );
}

//===========================================================================//
// Method         : UpdateCompressLists_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
bool TVPR_CircuitDesign_c::UpdateCompressLists_(
            t_net**           pvpr_netArray,
            int*              pvpr_netCount,
            t_logical_block** pvpr_logicalBlockArray,
            int*              pvpr_logicalBlockCount ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   // [VPR] Delete any VPACK_EMPTY blocks and OPEN nets left behind post synthesis
   //       (insures all VPACK blocks in netlist are in a contiguous list with no unused spots)

   t_net* vpr_netArray = *pvpr_netArray;
   t_logical_block* vpr_logicalBlockArray = *pvpr_logicalBlockArray;

   int* pmapNetIndexArray = static_cast< int* >( TC_calloc( *pvpr_netCount, sizeof( int )));
   int* pmapBlockIndexArray = static_cast< int* >( TC_calloc( *pvpr_logicalBlockCount, sizeof( int )));

   int vpr_netCount = *pvpr_netCount;
   int new_netCount = 0;
   for( int netIndex = 0; netIndex < vpr_netCount; ++netIndex )
   {
      if( vpr_netArray[netIndex].node_block[0] != OPEN ) 
      {
         pmapNetIndexArray[netIndex] = new_netCount;
         ++new_netCount;
      } 
      else 
      {
         pmapNetIndexArray[netIndex] = OPEN;
      }
   }

   int vpr_logicalBlockCount = *pvpr_logicalBlockCount;
   int new_logicalBlockCount = 0;
   for( int blockIndex = 0; blockIndex < vpr_logicalBlockCount; ++blockIndex ) 
   {
      if( vpr_logicalBlockArray[blockIndex].type != VPACK_EMPTY )
      {
         pmapBlockIndexArray[blockIndex] = new_logicalBlockCount;
         ++new_logicalBlockCount;
      } 
      else 
      {
         pmapBlockIndexArray[blockIndex] = OPEN;
      }
   }

   if(( new_netCount != vpr_netCount ) || ( new_logicalBlockCount != vpr_logicalBlockCount ))
   {
      for( int netIndex = 0; netIndex < vpr_netCount; ++netIndex ) 
      {
         if( vpr_netArray[netIndex].node_block[0] != OPEN ) 
         {
            int mapIndex = pmapNetIndexArray[netIndex];
            vpr_netArray[mapIndex] = vpr_netArray[netIndex];
            for( int pinIndex = 0; pinIndex <= vpr_netArray[mapIndex].num_sinks; ++pinIndex ) 
            {
               int blockIndex = vpr_netArray[mapIndex].node_block[pinIndex];
               vpr_netArray[mapIndex].node_block[pinIndex] = pmapBlockIndexArray[blockIndex];
            }
         } 
         else 
         {
            this->FreeVpackNet_( *pvpr_netArray, netIndex );
         }
      }
      vpr_netCount = new_netCount;
      *pvpr_netArray = static_cast< t_net* >( TC_realloc( *pvpr_netArray, vpr_netCount, sizeof( t_net )));

      for( int blockIndex = 0; blockIndex < vpr_logicalBlockCount; ++blockIndex ) 
      {
         if( vpr_logicalBlockArray[blockIndex].type != VPACK_EMPTY ) 
         {
            int mapIndex = pmapBlockIndexArray[blockIndex];
            if( mapIndex != blockIndex ) 
            {
               vpr_logicalBlockArray[mapIndex] = vpr_logicalBlockArray[blockIndex];
               vpr_logicalBlockArray[mapIndex].index = mapIndex;
            }

            for( const t_model_ports* pvpr_port = vpr_logicalBlockArray[mapIndex].model->inputs;
                 pvpr_port;
                 pvpr_port = pvpr_port->next )
            {
               int portIndex = pvpr_port->index;
               int pinCount = pvpr_port->size;
               for( int pinIndex = 0; pinIndex < pinCount; ++pinIndex ) 
               {
                  if( !pvpr_port->is_clock) 
                  {
                     if( vpr_logicalBlockArray[blockIndex].input_nets[portIndex][pinIndex] == OPEN)
                        continue;

                     mapIndex = vpr_logicalBlockArray[blockIndex].input_nets[portIndex][pinIndex];
                     vpr_logicalBlockArray[blockIndex].input_nets[portIndex][pinIndex] = pmapNetIndexArray[mapIndex];
                  }
                  else 
                  {
                     if( vpr_logicalBlockArray[blockIndex].clock_net == OPEN)
                        continue;

                     mapIndex = vpr_logicalBlockArray[blockIndex].clock_net;
                     vpr_logicalBlockArray[blockIndex].clock_net = pmapNetIndexArray[mapIndex];
                  } 
               }
            }

            for( const t_model_ports* pvpr_port = vpr_logicalBlockArray[blockIndex].model->outputs;
                 pvpr_port;
                 pvpr_port = pvpr_port->next )
            {
               int portIndex = pvpr_port->index;
               int pinCount = pvpr_port->size;
               for( int pinIndex = 0; pinIndex < pinCount; ++pinIndex ) 
               {
                  if( vpr_logicalBlockArray[blockIndex].output_nets[portIndex][pinIndex] == OPEN )
                     continue;
   
                  mapIndex = vpr_logicalBlockArray[blockIndex].output_nets[portIndex][pinIndex];
                  vpr_logicalBlockArray[blockIndex].output_nets[portIndex][pinIndex] = pmapNetIndexArray[mapIndex];
               }
            }
         }
         else 
         {
            this->FreeLogicalBlock_( *pvpr_logicalBlockArray, blockIndex );
         }
      }
      vpr_logicalBlockCount = new_logicalBlockCount;
      *pvpr_logicalBlockArray = static_cast< t_logical_block* >( TC_realloc( *pvpr_logicalBlockArray, vpr_logicalBlockCount, sizeof( t_logical_block )));
   }
   free( pmapNetIndexArray );
   free( pmapBlockIndexArray );

   if( *pvpr_netCount != vpr_netCount )
   {
      ok = printHandler.Warning( "Compressed net list, deleted %d open nets.\n", 
                                 *pvpr_netCount - vpr_netCount );

      *pvpr_netCount = vpr_netCount;
   }
   if( *pvpr_logicalBlockCount != vpr_logicalBlockCount )
   {
      ok = printHandler.Warning( "Compressed block list, deleted %d empty blocks.\n", 
                                 *pvpr_logicalBlockCount - vpr_logicalBlockCount );

      *pvpr_logicalBlockCount = vpr_logicalBlockCount;
   }
   return( ok );
}

//===========================================================================//
// Method         : UpdatePrimiaryCounts_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::UpdatePrimaryCounts_(
      t_logical_block* vpr_logicalBlockArray,
      int              vpr_logicalBlockCount,
      int*             pvpr_primaryInputCount,
      int*             pvpr_primaryOutputCount ) const
{
   *pvpr_primaryInputCount = 0;
   *pvpr_primaryOutputCount = 0;

   for( int blockIndex = 0; blockIndex < vpr_logicalBlockCount; ++blockIndex )
   {
      if( vpr_logicalBlockArray[blockIndex].type == VPACK_INPAD )
      {
         ++*pvpr_primaryInputCount;
      }
      else if( vpr_logicalBlockArray[blockIndex].type == VPACK_OUTPAD )
      {
         ++*pvpr_primaryOutputCount;
      }
   }
}

//===========================================================================//
// Method         : LoadModelLibrary_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
bool TVPR_CircuitDesign_c::LoadModelLibrary_(
      const t_model*  pvpr_standardModels,
            t_model** ppvpr_inputModel, 
            t_model** ppvpr_outputModel,
            t_model** ppvpr_logicModel,
            t_model** ppvpr_latchModel ) const
{
   *ppvpr_inputModel = 0;
   *ppvpr_outputModel = 0;
   *ppvpr_logicModel = 0;
   *ppvpr_latchModel = 0;

   for( const t_model* pvpr_standardModel = pvpr_standardModels;
        pvpr_standardModel;
        pvpr_standardModel = pvpr_standardModel->next )
   {
      if(strcmp( pvpr_standardModel->name, MODEL_INPUT ) == 0 )
      {
         *ppvpr_inputModel = const_cast< t_model* >( pvpr_standardModel );
      } 
      else if( strcmp( pvpr_standardModel->name, MODEL_OUTPUT ) == 0 )
      {
         *ppvpr_outputModel = const_cast< t_model* >( pvpr_standardModel );
      } 
      else if( strcmp( pvpr_standardModel->name, MODEL_LOGIC ) == 0 )
      {
         *ppvpr_logicModel = const_cast< t_model* >( pvpr_standardModel );
      } 
      else if( strcmp( pvpr_standardModel->name, MODEL_LATCH ) == 0 )
      {
         *ppvpr_latchModel = const_cast< t_model* >( pvpr_standardModel );
      }

      if( *ppvpr_inputModel && *ppvpr_outputModel && *ppvpr_logicModel && *ppvpr_latchModel )
         break;
   }
   return( *ppvpr_inputModel && *ppvpr_outputModel && *ppvpr_logicModel && *ppvpr_latchModel ? 
           true : false );
}

//===========================================================================//
// Method         : PeekInputOutputList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PeekInputOutputList_(
      const t_block*        vpr_blockArray,
            int             vpr_blockCount,
      const TPO_InstList_t& inputOutputList,
            TPO_InstList_t* pinputOutputList ) const
{ 
   // Iterate VPR's global block list for IO_TYPE blocks
   for( int blockIndex = 0; blockIndex < vpr_blockCount; ++blockIndex )
   {
      const t_block& vpr_block = vpr_blockArray[blockIndex];
      if( vpr_block.type != IO_TYPE )
         continue;

      this->PeekInputOutput_( vpr_block, 
                              inputOutputList, pinputOutputList );
   }
}

//===========================================================================//
// Method         : PeekInputOutput_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PeekInputOutput_(
      const t_block&        vpr_block,
      const TPO_InstList_t& inputOutputList,
            TPO_InstList_t* pinputOutputList ) const
{ 
   const char* pszInstName = vpr_block.name;

   const char* pszCellName = "";
   if( vpr_block.pb &&
       vpr_block.pb->pb_graph_node &&
       vpr_block.pb->pb_graph_node->pb_type )
   {
      pszCellName = vpr_block.pb->pb_graph_node->pb_type->name;
   }

   string srPlaceFabricName;
   if(( vpr_block.x >= 0 ) && ( vpr_block.y >= 0 ))
   {
      char szPlaceFabricName[TIO_FORMAT_STRING_LEN_DATA];
      sprintf( szPlaceFabricName, "fabric[%d][%d].%d", vpr_block.x, vpr_block.y, vpr_block.z );
      srPlaceFabricName = szPlaceFabricName;
   }

   TPO_StatusMode_t placeStatus = ( vpr_block.isFixed ? 
                                    TPO_STATUS_FIXED : TPO_STATUS_PLACED );

   TGO_Point_c placeOrigin( vpr_block.x, vpr_block.y, vpr_block.z );

   TGS_RegionList_t placeRegionList;
   if( vpr_block.placement_region_list.IsValid( ))
   {
      for( size_t i = 0; i < vpr_block.placement_region_list.GetLength( ); ++i )
      {
         const TGO_Region_c& placementRegion = *vpr_block.placement_region_list[i];
         TGS_Region_c placeRegion( static_cast< double >( placementRegion.x1 ),
                                   static_cast< double >( placementRegion.y1 ),
                                   static_cast< double >( placementRegion.x2 ),
                                   static_cast< double >( placementRegion.y2 ));

         placeRegionList.Add( placeRegion );
      }
   }

   TPO_RelativeList_t placeRelativeList; 
   if( inputOutputList.IsMember( pszInstName ))
   {
      const TPO_Inst_c& inputOutput = *inputOutputList.Find( pszInstName );
      const TPO_RelativeList_t& placeRelativeList_ = inputOutput.GetPlaceRelativeList( );
      for( size_t i = 0; i < placeRelativeList_.GetLength( ); ++i )
      {
         const TPO_Relative_t& placeRelative = *placeRelativeList_[i];
         placeRelativeList.Add( placeRelative );
      }
   }

   if( pszInstName && *pszInstName )
   {
      TPO_Inst_c inputOutput( pszInstName );
      inputOutput.SetCellName( pszCellName );
      inputOutput.SetPlaceFabricName( srPlaceFabricName );
      inputOutput.SetPlaceStatus( placeStatus );
      inputOutput.SetPlaceOrigin( placeOrigin );
      inputOutput.SetPlaceRegionList( placeRegionList );
      inputOutput.SetPlaceRelativeList( placeRelativeList );

      pinputOutputList->Add( inputOutput );
   }
}

//===========================================================================//
// Method         : PeekPhysicalBlockList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PeekPhysicalBlockList_(
      const t_block*         vpr_blockArray,
            int              vpr_blockCount,
      const t_logical_block* vpr_logicalBlockArray,
      const TPO_InstList_t&  physicalBlockList,
            TPO_InstList_t*  pphysicalBlockList ) const
{ 
   // Iterate VPR's global block list for non-IO_TYPE blocks
   for( int blockIndex = 0; blockIndex < vpr_blockCount; ++blockIndex )
   {
      const t_block& vpr_block = vpr_blockArray[blockIndex];
      if( vpr_block.type == IO_TYPE )
         continue;

      this->PeekPhysicalBlock_( vpr_block, vpr_logicalBlockArray, blockIndex, 
                                physicalBlockList, pphysicalBlockList );
   }
}

//===========================================================================//
// Method         : PeekPhysicalBlock_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PeekPhysicalBlock_(
      const t_block&         vpr_block,
      const t_logical_block* vpr_logicalBlockArray,
            int              blockIndex,
      const TPO_InstList_t&  physicalBlockList,
            TPO_InstList_t*  pphysicalBlockList ) const
{ 
   const char* pszInstName = vpr_block.name;

   const char* pszCellName = "";
   if( vpr_block.pb &&
       vpr_block.pb->pb_graph_node &&
       vpr_block.pb->pb_graph_node->pb_type )
   {
      pszCellName = vpr_block.pb->pb_graph_node->pb_type->name;
   }

   string srPlaceFabricName;
   if(( vpr_block.x >= 0 ) && ( vpr_block.y >= 0 ))
   {
      char szPlaceFabricName[TIO_FORMAT_STRING_LEN_DATA];
      sprintf( szPlaceFabricName, "fabric[%d][%d]", vpr_block.x, vpr_block.y );
      srPlaceFabricName = szPlaceFabricName;
   }

   TPO_HierMapList_t packHierMapList;
   if( vpr_block.pb )
   {
      this->PeekHierMapList_( *vpr_block.pb, vpr_logicalBlockArray,
                              blockIndex, &packHierMapList );
   }

   TPO_StatusMode_t placeStatus = ( vpr_block.isFixed ? 
                                    TPO_STATUS_FIXED : TPO_STATUS_PLACED );

   TGO_Point_c placeOrigin( vpr_block.x, vpr_block.y );

   TGS_RegionList_t placeRegionList;
   if( vpr_block.placement_region_list.IsValid( ))
   {
      for( size_t i = 0; i < vpr_block.placement_region_list.GetLength( ); ++i )
      {
         const TGO_Region_c& placementRegion = *vpr_block.placement_region_list[i];
         TGS_Region_c placeRegion( static_cast< double >( placementRegion.x1 ),
                                   static_cast< double >( placementRegion.y1 ),
                                   static_cast< double >( placementRegion.x2 ),
                                   static_cast< double >( placementRegion.y2 ));

         placeRegionList.Add( placeRegion );
      }
   }

   TPO_RelativeList_t placeRelativeList; 
   if( physicalBlockList.IsMember( pszInstName ))
   {
      const TPO_Inst_c& physicalBlock = *physicalBlockList.Find( pszInstName );
      const TPO_RelativeList_t& placeRelativeList_ = physicalBlock.GetPlaceRelativeList( );
      for( size_t i = 0; i < placeRelativeList_.GetLength( ); ++i )
      {
         const TPO_Relative_t& placeRelative = *placeRelativeList_[i];
         placeRelativeList.Add( placeRelative );
      }
   }

   if( pszInstName && *pszInstName )
   {
      TPO_Inst_c physicalBlock( pszInstName );
      physicalBlock.SetCellName( pszCellName );
      physicalBlock.SetPackHierMapList( packHierMapList );
      physicalBlock.SetPlaceFabricName( srPlaceFabricName );
      physicalBlock.SetPlaceStatus( placeStatus );
      physicalBlock.SetPlaceOrigin( placeOrigin );
      physicalBlock.SetPlaceRegionList( placeRegionList );
      physicalBlock.SetPlaceRelativeList( placeRelativeList );

      pphysicalBlockList->Add( physicalBlock );
   }
}

//===========================================================================//
// Method         : PeekHierMapList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PeekHierMapList_(
      const t_pb&              vpr_pb,
      const t_logical_block*   vpr_logicalBlockArray,
            int                nodeIndex,
            TPO_HierMapList_t* phierMapList ) const
{
   // Apply recursion to read physical block's hierarchical pack map list
   TPO_NameList_t hierNameList;
   this->PeekHierMapList_( vpr_pb, vpr_logicalBlockArray,
                           nodeIndex, &hierNameList, phierMapList );
}

//===========================================================================//
void TVPR_CircuitDesign_c::PeekHierMapList_(
      const t_pb&                vpr_pb,
      const t_logical_block*     vpr_logicalBlockArray,
              int                nodeIndex,
              TPO_NameList_t*    phierNameList,
              TPO_HierMapList_t* phierMapList ) const
{
   if( vpr_pb.name )
   {
      if( vpr_pb.child_pbs && 
          vpr_pb.pb_graph_node && 
          vpr_pb.pb_graph_node->pb_type )
      {
         char szHierName[TIO_FORMAT_STRING_LEN_DATA];
         sprintf( szHierName, "%s[%d]", vpr_pb.pb_graph_node->pb_type->name, nodeIndex );
         phierNameList->Add( szHierName );

         int modeIndex = vpr_pb.mode;
         const t_mode& vpr_mode = vpr_pb.pb_graph_node->pb_type->modes[modeIndex];
         for( int typeIndex = 0; typeIndex < vpr_mode.num_pb_type_children; ++typeIndex ) 
         {
            for( int instIndex = 0; instIndex < vpr_mode.pb_type_children[typeIndex].num_pb; ++instIndex ) 
            {
               if( vpr_pb.child_pbs && 
                   vpr_pb.child_pbs[typeIndex] &&
                   vpr_pb.child_pbs[typeIndex][instIndex].name )
               {
                  TPO_NameList_t hierNameList( *phierNameList );

                  const t_pb& child_pb = vpr_pb.child_pbs[typeIndex][instIndex];
                  this->PeekHierMapList_( child_pb, vpr_logicalBlockArray,
                                          instIndex, &hierNameList, phierMapList );
               }
            }
         }
      }
      else if( vpr_pb.logical_block != OPEN )
      {
         int blockIndex = vpr_pb.logical_block;
         const char* pszInstName = vpr_logicalBlockArray[blockIndex].name;

         char szHierName[TIO_FORMAT_STRING_LEN_DATA];
         sprintf( szHierName, "%s[%d]", vpr_pb.pb_graph_node->pb_type->name, 0 );
         phierNameList->Add( szHierName );

         TPO_HierMap_c hierMap( pszInstName, *phierNameList );
         phierMapList->Add( hierMap );
      }
   }
}

//===========================================================================//
// Method         : PeekNetList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::PeekNetList_(
      const t_arch*          vpr_architecture,
      const t_net*           vpr_netArray,
            int              vpr_netCount,
      const t_block*         vpr_blockArray,
            int              vpr_blockCount,
      const t_logical_block* vpr_logicalBlockArray,
      const t_rr_node*       vpr_rrNodeArray,
            TNO_NetList_c*   pnetList ) const
{
   // Initialize net list based on all nets defined in given VPR net list
   this->ExtractNetList_( vpr_netArray, vpr_netCount,
                          pnetList );

   // Update net list based on all net instance ports asso. with VPR nets
   this->ExtractNetListInstPins_( vpr_netArray, vpr_netCount,
                                  vpr_logicalBlockArray,
                                  pnetList );

   // Update net list based on any available net global routes
// WIP ??? this->ExtractNetListGlobalRoutes_

   // Update net list based on any available net routes
   this->ExtractNetListRoutes_( vpr_architecture,
                                vpr_netArray, vpr_netCount,
                                vpr_blockArray, vpr_blockCount,
                                vpr_rrNodeArray,
                                pnetList );
}

//===========================================================================//
// Method         : ValidateModelList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
bool TVPR_CircuitDesign_c::ValidateModelList_(
      const TLO_CellList_t& cellList,
      const t_model*        pvpr_customModels ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   for( size_t i = 0; i < cellList.GetLength( ); ++i )
   {
      const TLO_Cell_c& cell = *cellList[i];
      const char* pszCellName = cell.GetName( );

      bool foundModel = false;
      for( const t_model* pvpr_customModel = pvpr_customModels; 
           pvpr_customModel; 
           pvpr_customModel = pvpr_customModel->next )
      {
         if( strcmp( pvpr_customModel->name, pszCellName ) == 0 ) 
         {
            foundModel = true;
            break;
         }
      }

      if( !foundModel )
      {
         ok = printHandler.Error( "Invalid circuit .model \"%s\" detected!\n"
                                  "%sModel not found in architecture's <model> list.\n",
                                  TIO_PSZ_STR( pszCellName ),
                                  TIO_PREFIX_ERROR_SPACE );
         if( !ok )
            break;
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : ValidateSubcktList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
bool TVPR_CircuitDesign_c::ValidateSubcktList_(
      const TPO_InstList_t& instList,
      const TLO_CellList_t& cellList ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   for( size_t i = 0; i < instList.GetLength( ); ++i )
   {
      const TPO_Inst_c& inst = *instList[i];
      if( inst.GetSource( ) != TPO_INST_SOURCE_SUBCKT )
         continue;

      const char* pszCellName = inst.GetCellName( );
      const TLO_Cell_c* pcell = cellList.Find( pszCellName );
      if( pcell )
      {
         const TLO_PortList_t& portList = pcell->GetPortList( );

         const TPO_PinMapList_t& pinMapList = inst.GetSubcktPinMapList( );
         for( size_t j = 0; j < pinMapList.GetLength( ); ++j )
         {
            const TPO_PinMap_c& pinMap = *pinMapList[j];
            const char* pszCellPinName = pinMap.GetCellPinName( );
            if( !portList.Find( pszCellPinName ))
            {
               ok = printHandler.Error( "Invalid circuit .subckt model \"%s\" port \"%s\" detected!\n"
                                        "%sPort not found in circuit's .model definition.\n",
                                        TIO_PSZ_STR( pszCellName ),
                                        TIO_PSZ_STR( pszCellPinName ),
                                        TIO_PREFIX_ERROR_SPACE );
            }
            if( !ok )
               break;
         }
      }
      else
      {
         ok = printHandler.Error( "Invalid circuit .subckt model \"%s\" detected!\n"
                                  "%sModel not found in circuit's .model list.\n",
                                  TIO_PSZ_STR( pszCellName ),
                                  TIO_PREFIX_ERROR_SPACE );
      }
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : ValidateInstList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
bool TVPR_CircuitDesign_c::ValidateInstList_(
            TPO_InstList_t* pinstList,
      const TLO_CellList_t& cellList,
            bool            deleteInvalidInsts ) const
{ 
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   TPO_InstList_t instList( *pinstList );
   pinstList->Clear( );
   pinstList->SetCapacity( instList.GetLength( ));

   size_t zeroOutputPinCount = 0;

   for( size_t i = 0; i < instList.GetLength( ); ++i )
   {
      const TPO_Inst_c& inst = *instList[i];

      if( inst.GetSource( ) == TPO_INST_SOURCE_SUBCKT )
      {  
         // [VPR] Delete instance if 0 output pins (and is not an output pad)
         size_t outputPinCount = inst.FindPinCount( TC_TYPE_OUTPUT, cellList );
         if( outputPinCount == 0 )
         {
            ok = printHandler.Warning( "Invalid subckt block \"%s\" detected.\n"
                                       "%sNo output pins founds.\n",
                                       TIO_PSZ_STR( inst.GetName( )),
                                       TIO_PREFIX_WARNING_SPACE );

            if( !ok )
               break;

            if( deleteInvalidInsts )
            {
               ++zeroOutputPinCount;
               continue;
            } 
         } 
      }
      pinstList->Add( inst );
   }

   if( ok && zeroOutputPinCount )
   {
      ok = printHandler.Warning( "Deleted %d subckt blocks with zero outputs.\n", 
                                 zeroOutputPinCount );
   }
   return( ok );
}

//===========================================================================//
// Method         : AddVpackNet_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
int TVPR_CircuitDesign_c::AddVpackNet_(
      const char*          pszNetName,
            int            pinType, 
            bool           isGlobal,
            int            blockIndex,
            int            portIndex,
            int            pinIndex,
      const TNO_NetList_c& netList,
            t_net*         vpr_netArray ) const
{
   const TNO_Net_c* pnet = netList.Find( pszNetName );
   size_t netIndex = ( pnet ? pnet->GetIndex( ) : SIZE_MAX );
   if( netIndex != SIZE_MAX )
   {
      int nodeIndex = 0;
      if( pinType == DRIVER )
      {
         // [VPR] Driver pin is always placed at position '0'
         nodeIndex = 0; 
      }
      else // if( pinType == RECEIVER )
      {
         ++vpr_netArray[netIndex].num_sinks;
         nodeIndex = vpr_netArray[netIndex].num_sinks;
      }
      vpr_netArray[netIndex].is_global = static_cast< boolean >( isGlobal );

      vpr_netArray[netIndex].node_block[nodeIndex] = blockIndex;
      vpr_netArray[netIndex].node_block_port[nodeIndex] = portIndex;
      vpr_netArray[netIndex].node_block_pin[nodeIndex] = pinIndex;
   }
   else if( strcmp( pszNetName, "unconn") == 0) 
   {
      netIndex = OPEN;
   }
   return( static_cast< int >( netIndex ));
}

//===========================================================================//
// Method         : AddLogicalBlockInputOutput_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::AddLogicalBlockInputOutput_(
      const char*            pszPortName,
            int              pinType, 
            int              netIndex,
      const t_model*         pvpr_inputModel,
      const t_model*         pvpr_outputModel,
            t_logical_block* vpr_logicalBlockArray,
            int              vpr_blockIndex,
            int*             pvpr_primaryInputCount,
            int*             pvpr_primaryOutputCount ) const
{
   int blockIndex = vpr_blockIndex;

   vpr_logicalBlockArray[blockIndex].input_nets = 0;
   vpr_logicalBlockArray[blockIndex].output_nets = 0;
   vpr_logicalBlockArray[blockIndex].clock_net = OPEN;
   vpr_logicalBlockArray[blockIndex].truth_table = 0;

   if( pinType == DRIVER )
   {
      vpr_logicalBlockArray[blockIndex].name = TC_strdup( pszPortName );
      vpr_logicalBlockArray[blockIndex].type = VPACK_INPAD;
      vpr_logicalBlockArray[blockIndex].model = const_cast< t_model* >( pvpr_inputModel );

      vpr_logicalBlockArray[blockIndex].output_nets = static_cast< int** >( TC_calloc( 1, sizeof( int* )));
      vpr_logicalBlockArray[blockIndex].output_nets[0] = static_cast< int* >( TC_calloc( 1, sizeof( int )));
      vpr_logicalBlockArray[blockIndex].output_nets[0][0] = netIndex;

      ++*pvpr_primaryInputCount; // VPR trickery... see VPR's global num_p_inputs
   }
   else // if( pinType == RECEIVER ) 
   {
      // [VPR] Make output names unique from LUTs
      string srPortName = "out:";
      srPortName += pszPortName;
      vpr_logicalBlockArray[blockIndex].name = TC_strdup( srPortName );
      vpr_logicalBlockArray[blockIndex].type = VPACK_OUTPAD;
      vpr_logicalBlockArray[blockIndex].model = const_cast< t_model* >( pvpr_outputModel );

      vpr_logicalBlockArray[blockIndex].input_nets = static_cast< int** >( TC_calloc( 1, sizeof( int* )));
      vpr_logicalBlockArray[blockIndex].input_nets[0] = static_cast< int* >( TC_calloc( 1, sizeof( int )));
      vpr_logicalBlockArray[blockIndex].input_nets[0][0] = netIndex;

      ++*pvpr_primaryOutputCount; // VPR trickery... see VPR's global num_p_outputs
   }
}

//===========================================================================//
// Method         : AddLogicalBlockLogic_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::AddLogicalBlockLogic_(
      const char*            pszOutputPinName,
      const TPO_PinList_t&   pinList,
      const TNO_NetList_c&   netList,
      const t_model*         pvpr_logicModel,
            t_logical_block* vpr_logicalBlockArray,
            int              vpr_blockIndex ) const
{
   int blockIndex = vpr_blockIndex;

   vpr_logicalBlockArray[blockIndex].name = TC_strdup( pszOutputPinName );
   vpr_logicalBlockArray[blockIndex].type = VPACK_COMB;
   vpr_logicalBlockArray[blockIndex].model = const_cast< t_model* >( pvpr_logicModel );

   vpr_logicalBlockArray[blockIndex].output_nets = static_cast< int** >( TC_calloc( 1, sizeof( int* )));
   vpr_logicalBlockArray[blockIndex].output_nets[0] = static_cast< int* >( TC_calloc( 1, sizeof( int )));

   vpr_logicalBlockArray[blockIndex].input_nets = static_cast< int** >( TC_calloc( 1, sizeof( int* )));
   vpr_logicalBlockArray[blockIndex].input_nets[0] = static_cast< int* >( TC_calloc( pvpr_logicModel->inputs->size, sizeof( int )));
   for( int i = 0; i < pvpr_logicModel->inputs->size; ++i )
   {
      vpr_logicalBlockArray[blockIndex].input_nets[0][i] = OPEN;
   }

   int inputPinIndex = 0;
   for( size_t i = 0; i < pinList.GetLength( ); ++i )
   {
      const TPO_Pin_t& pin = *pinList[i];

      const char* pszNetName = pin.GetName( );
      const TNO_Net_c* pnet = netList.Find( pszNetName );
      if( !pnet )
         continue;

      unsigned int netIndex = pnet->GetIndex( );
      if( pin.GetType( ) == TC_TYPE_OUTPUT )
      {
         vpr_logicalBlockArray[blockIndex].output_nets[0][0] = netIndex;
      }
      else
      {
         vpr_logicalBlockArray[blockIndex].input_nets[0][inputPinIndex] = netIndex;
         ++inputPinIndex;
      }
   }
   vpr_logicalBlockArray[blockIndex].clock_net = OPEN;
   vpr_logicalBlockArray[blockIndex].truth_table = 0;
}

//===========================================================================//
// Method         : AddLogicalBlockLatch_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::AddLogicalBlockLatch_( 
      const char*            pszOutputPinName,
      const char*            pszInputPinName,
      const char*            pszClockPinName,
      const TNO_NetList_c&   netList,
      const t_model*         pvpr_latchModel,
            t_logical_block* vpr_logicalBlockArray,
            int              vpr_blockIndex ) const
{
   if( netList.IsMember( pszOutputPinName ) &&
       netList.IsMember( pszInputPinName ) &&
       netList.IsMember( pszClockPinName ))
   {
      int blockIndex = vpr_blockIndex;
   
      const TNO_Net_c& outputNet = *netList.Find( pszOutputPinName );
      const TNO_Net_c& inputNet = *netList.Find( pszInputPinName );
      const TNO_Net_c& clockNet = *netList.Find( pszClockPinName );
   
      unsigned int outputNetIndex = outputNet.GetIndex( );
      unsigned int inputNetIndex = inputNet.GetIndex( );
      unsigned int clockNetIndex = clockNet.GetIndex( );
   
      vpr_logicalBlockArray[blockIndex].name = TC_strdup( pszOutputPinName );
   
      vpr_logicalBlockArray[blockIndex].type = VPACK_LATCH;
      vpr_logicalBlockArray[blockIndex].model = const_cast< t_model* >( pvpr_latchModel );
   
      vpr_logicalBlockArray[blockIndex].output_nets = static_cast< int** >( TC_calloc( 1, sizeof( int* )));
      vpr_logicalBlockArray[blockIndex].output_nets[0] = static_cast< int* >( TC_calloc( 1, sizeof( int )));
      vpr_logicalBlockArray[blockIndex].output_nets[0][0] = static_cast< int >( outputNetIndex );
   
      vpr_logicalBlockArray[blockIndex].input_nets = static_cast< int** >( TC_calloc( 1, sizeof( int* )));
      vpr_logicalBlockArray[blockIndex].input_nets[0] = static_cast< int* >( TC_calloc( 1, sizeof( int )));
      vpr_logicalBlockArray[blockIndex].input_nets[0][0] = static_cast< int >( inputNetIndex );
   
      vpr_logicalBlockArray[blockIndex].clock_net = static_cast< int >( clockNetIndex );
   
      vpr_logicalBlockArray[blockIndex].truth_table = 0;
   }
}

//===========================================================================//
// Method         : FreeVpackNet_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::FreeVpackNet_(
      t_net* vpr_netArray,
      int    netIndex ) const
{
   free( vpr_netArray[netIndex].name );
   free( vpr_netArray[netIndex].node_block );
   free( vpr_netArray[netIndex].node_block_port );
   free( vpr_netArray[netIndex].node_block_pin );
}

//===========================================================================//
// Method         : FreeLogicalBlock_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::FreeLogicalBlock_(
      t_logical_block* vpr_logicalBlockArray,
      int              blockIndex ) const
{
   free( vpr_logicalBlockArray[blockIndex].name );

   int i = 0;
   for( const t_model_ports* pvpr_port = vpr_logicalBlockArray[blockIndex].model->inputs;
        pvpr_port;
        pvpr_port = pvpr_port->next )
   {
      if( pvpr_port->is_clock ) 
         continue;

      if( vpr_logicalBlockArray[blockIndex].input_nets && 
          vpr_logicalBlockArray[blockIndex].input_nets[i] ) 
      {
         free( vpr_logicalBlockArray[blockIndex].input_nets[i] );
         vpr_logicalBlockArray[blockIndex].input_nets[i] = 0;
      }
      ++i;
   }
   if( vpr_logicalBlockArray[blockIndex].input_nets )
   {
      free( vpr_logicalBlockArray[blockIndex].input_nets );
   }

   i = 0;
   for( const t_model_ports* pvpr_port = vpr_logicalBlockArray[blockIndex].model->outputs;
        pvpr_port;
        pvpr_port = pvpr_port->next )
   {
      if( vpr_logicalBlockArray[blockIndex].output_nets &&
          vpr_logicalBlockArray[blockIndex].output_nets[i] ) 
      {
         free( vpr_logicalBlockArray[blockIndex].output_nets[i] );
         vpr_logicalBlockArray[blockIndex].output_nets[i] = 0;
      }
      ++i;
   }
   if( vpr_logicalBlockArray[blockIndex].output_nets)
   {
      free( vpr_logicalBlockArray[blockIndex].output_nets ) ;
   }

   t_linked_vptr *pvpr_linked_vptr = vpr_logicalBlockArray[blockIndex].truth_table;
   while( pvpr_linked_vptr )
   {
      if( pvpr_linked_vptr->data_vptr )
      {
         free( pvpr_linked_vptr->data_vptr );
      }
      t_linked_vptr *pvpr_next_vptr = pvpr_linked_vptr->next;
      free( pvpr_linked_vptr );
      pvpr_linked_vptr = pvpr_next_vptr;
   }
}

//===========================================================================//
// Method         : ExtractNetList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::ExtractNetList_(
      const t_net*         vpr_netArray,
            int            vpr_netCount,
            TNO_NetList_c* pnetList ) const
{
   // Iterate for every net in the existing VPR net list...
   for( int netIndex = 0; netIndex < vpr_netCount; ++netIndex )
   {
      const t_net& vpr_net = vpr_netArray[netIndex];

      const char* pszNetName = vpr_net.name;
      if( pnetList->IsMember( pszNetName ))
         continue;

      // Define and add a new net node based on next VPR net
      TC_TypeMode_t netType = ( vpr_net.is_global ? TC_TYPE_GLOBAL : TC_TYPE_SIGNAL ); 
      
      TNO_Net_c net( pszNetName, netType );
      pnetList->Add( net );
   }
}

//===========================================================================//
// Method         : ExtractNetListInstPins_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::ExtractNetListInstPins_(
      const t_net*           vpr_netArray,
            int              vpr_netCount,
      const t_logical_block* vpr_logicalBlockArray,
            TNO_NetList_c*   pnetList ) const
{
   // Iterate for every net in the existing VPR net list...
   for( int netIndex = 0; netIndex < vpr_netCount; ++netIndex )
   {
      const t_net& vpr_net = vpr_netArray[netIndex];

      const char* pszNetName = vpr_net.name;
      TNO_Net_c* pnet = pnetList->Find( pszNetName );

      pnet->ClearInstPinList( );

      // Iterate for every pin in the existing VPR net...
      int pinCount = 1 + vpr_net.num_sinks; // [VPR] Assume [0] for output pin
      for( int nodeIndex = 0; nodeIndex < pinCount; ++nodeIndex )
      {
         // Extract pin's instance (ie. block) name and port/pin name
         int blockIndex = vpr_net.node_block[nodeIndex];
         const char* pszBlockName = vpr_logicalBlockArray[blockIndex].name;

         const t_pb_graph_pin* pvpr_graphPin = 0;
         pvpr_graphPin = this->FindGraphPin_( vpr_logicalBlockArray, 
                                              vpr_net, nodeIndex );
         if( pvpr_graphPin )
         {
            const char* pszPortName = pvpr_graphPin->port->name;

            TC_TypeMode_t ioType = TC_TYPE_OUTPUT;
            ioType = ( nodeIndex > 0 ? TC_TYPE_INPUT : ioType );
            ioType = ( pvpr_graphPin->port->is_clock ? TC_TYPE_CLOCK : ioType );

            // Update new net's instance/pin list
            TNO_InstPin_c instPin( pszBlockName, pszPortName, ioType );

            pnet->AddInstPin( instPin );
         }
      }
   }
}

//===========================================================================//
// Method         : ExtractNetListRoutes_
// Reference:       See Jason Luu's original vpr_resync_post_route_netlist()
//                  source code.  This includes support for resolving logical
//                  equivalence on CLB pins.
//                  
//                  "Logical equivalence scrambles the packed netlist indices 
//                  with the actual indices, need to resync then re-output 
//                  clustered netlist.  This code assumes we are dealing with 
//                  a TI CLAY v1 architecture." -- Jason Luu
//                  
//                  "Returns a trace array [0..num_logical_nets-1] with the 
//                  final routing of the circuit from the logical_block 
//                  netlist, index of the trace array corresponds to the 
//                  index of a vpack_net." -- Jason Luu
//
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::ExtractNetListRoutes_(
      const t_arch*          vpr_architecture,
      const t_net*           vpr_netArray,
            int              vpr_netCount,
      const t_block*         vpr_blockArray,
            int              vpr_blockCount,
      const t_rr_node*       vpr_rrNodeArray,
            TNO_NetList_c*   pnetList ) const
{
   if( vpr_netCount && vpr_blockCount )
   {
      // Call VPR API's special TI-specific function to find all net trace lists
      t_trace* pvpr_traceArray = 0;
      pvpr_traceArray = vpr_resync_post_route_netlist_to_TI_CLAY_v1_architecture( vpr_architecture );
      if( pvpr_traceArray )
      {
         // Iterate for every net in the existing VPR net list...
         for( int netIndex = 0; netIndex < vpr_netCount; ++netIndex )
         {
            const t_net& vpr_net = vpr_netArray[netIndex];

            const char* pszNetName = vpr_net.name;
            TNO_Net_c* pnet = pnetList->Find( pszNetName );

            pnet->ClearRouteList( );

            if( !vpr_blockArray || !vpr_rrNodeArray )
               continue;

            t_trace* pvpr_traceList = &pvpr_traceArray[netIndex];
            if( this->IsNetOpen_( pvpr_traceList ))
               continue;

            TNO_RouteList_t routeList;
            this->ExtractNetRoutes_( vpr_blockArray, vpr_rrNodeArray,
                                     pvpr_traceList, &routeList );

            pnet->SetStatus( TNO_STATUS_ROUTED );
            pnet->AddRouteList( routeList );
         }
      }
      free( pvpr_traceArray );
   }
}

//===========================================================================//
// Method         : ExtractNetRoutes_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::ExtractNetRoutes_(
      const t_block*         vpr_blockArray,
      const t_rr_node*       vpr_rrNodeArray,
            t_trace*         pvpr_traceList,
            TNO_RouteList_t* prouteList ) const
{
   // Generate a trace node count list based on current trace list
   TVPR_IndexCountList_t traceCountList;
   this->MakeTraceNodeCountList_( pvpr_traceList, &traceCountList );

   // Update current trace list's sibling counts using node count list
   this->UpdateTraceSiblingCount_( pvpr_traceList, traceCountList );

   // Apply recursion to load list of route paths based on current trace list
   const t_trace* pvpr_traceThis = pvpr_traceList;
   const t_trace* pvpr_tracePrev = 0;
   TNO_Route_t route;
   this->LoadTraceRoutes_( vpr_blockArray, vpr_rrNodeArray,
                           pvpr_traceThis, pvpr_tracePrev,
                           &route, prouteList );
}

//===========================================================================//
// Method         : IsNetOpen_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
bool TVPR_CircuitDesign_c::IsNetOpen_(
      const t_trace* pvpr_traceList ) const
{
   // Determine if net is open (or routed) based on current trace list
   // (open impiles unique output/input block and no intra-CLB trace routing)
   int blockIndex = OPEN;
   bool hasSingleBlock = false;
   bool hasRouteTrace = false;

   for( const t_trace* pvpr_trace = pvpr_traceList;
        pvpr_trace; pvpr_trace = pvpr_trace->next )
   {
      if( pvpr_trace->iblock != OPEN ) 
      {
         if( blockIndex == OPEN )
         {
            blockIndex = pvpr_trace->iblock;
            hasSingleBlock = true;
         }
         else if( blockIndex != pvpr_trace->iblock )
         {
            hasSingleBlock = false;
            break;
         }
      } 
      else 
      {
         hasRouteTrace = true;
         break;
      }
   }
   return( !hasSingleBlock && !hasRouteTrace ? true : false );
}

//===========================================================================//
// Method         : MakeTraceNodeCountList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::MakeTraceNodeCountList_(
      const t_trace*               pvpr_traceList,
            TVPR_IndexCountList_t* ptraceCountList ) const
{
   // Allocate a node index/count list used to determine trace sibling counts
   size_t len = this->FindTraceListLength_( pvpr_traceList );
   ptraceCountList->SetCapacity( len );

   // Load node index/count list based on trace sibling (ie. duplicate nodes)
   for( const t_trace* pvpr_trace = pvpr_traceList; 
        pvpr_trace; pvpr_trace = pvpr_trace->next )
   {
      TVPR_IndexCount_c indexCount( pvpr_trace->iblock, pvpr_trace->index );
      if( !ptraceCountList->IsMember( indexCount ))
      {
         TVPR_IndexCount_c* pindexCount = ptraceCountList->Add( indexCount );
         pindexCount->SetSiblingCount( 1 );
      }
      else
      {
         TVPR_IndexCount_c* pindexCount = ( *ptraceCountList )[ indexCount ];
         pindexCount->SetSiblingCount( pindexCount->GetSiblingCount( ) + 1 );
      }
   }
}

//===========================================================================//
// Method         : UpdateTraceSiblingCount_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::UpdateTraceSiblingCount_(
            t_trace*               pvpr_traceList,
      const TVPR_IndexCountList_t& traceCountList ) const
{
   // Update sibling count for all trace nodes baesd on node count list
   for( t_trace* pvpr_trace = pvpr_traceList; 
        pvpr_trace; pvpr_trace = pvpr_trace->next )
   {
      TVPR_IndexCount_c indexCount( pvpr_trace->iblock, pvpr_trace->index );
      TVPR_IndexCount_c* pindexCount = traceCountList[ indexCount ];
      pvpr_trace->num_siblings = static_cast< int >( pindexCount->GetSiblingCount( ));
   }

   for( t_trace* pvpr_trace = pvpr_traceList; 
        pvpr_trace; pvpr_trace = pvpr_trace->next )
   {
      if( pvpr_trace->num_siblings == 1 )
      {
         if( !pvpr_trace->next )
         {
            pvpr_trace->num_siblings = 0;
         }
      }
      if( pvpr_trace->num_siblings > 1 )
      {
         t_trace* pprev_trace = pvpr_trace;
         while(( pprev_trace ) && 
               ( pprev_trace->next ))
         {
            if( pprev_trace->next->index == pvpr_trace->index )
               break;

            pprev_trace = pprev_trace->next;
         }

         if(( pprev_trace ) &&
            ( pprev_trace->iblock != OPEN ))
         {
            pprev_trace->num_siblings = 0;
         }
      }
   }
}

//===========================================================================//
// Method         : LoadTraceRoutes_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::LoadTraceRoutes_(
      const t_block*         vpr_blockArray,
      const t_rr_node*       vpr_rrNodeArray,
      const t_trace*         pvpr_traceThis,
      const t_trace*         pvpr_tracePrev,
            TNO_Route_t*     proute,
            TNO_RouteList_t* prouteList ) const
{
   // Load current track node (ie. 'instPin' or 'segment'+'switchBox')
   this->LoadTraceNodes_( vpr_blockArray, vpr_rrNodeArray,
                          pvpr_traceThis, pvpr_tracePrev, proute );

   if( pvpr_traceThis->num_siblings == 0 )
   {
      // Reached terminal for the current route path
      // (add route to route list and start unwinding recursion)
      prouteList->Add( *proute );
   }
   else if( pvpr_traceThis->num_siblings == 1 )
   {
      // Apply recursion to update route based on next trace node
      pvpr_tracePrev = pvpr_traceThis;
      pvpr_traceThis = pvpr_traceThis->next;
      this->LoadTraceRoutes_( vpr_blockArray, vpr_rrNodeArray,
                              pvpr_traceThis, pvpr_tracePrev,
                              proute, prouteList );
   }
   else if( pvpr_traceThis->num_siblings > 1 )
   {
      // Hande multiple routes based on split at this trace node
      int index = pvpr_traceThis->index;
      int num_siblings = pvpr_traceThis->num_siblings;
      for( int i = 0; i < num_siblings; ++i )
      {
         // Find next trace node with matching index (if any) 
         pvpr_traceThis = this->FindTraceListNode_( pvpr_traceThis, index );
         if( !pvpr_traceThis )
            break;

         // Make local route to handle multiple routes from this split
         TNO_Route_t route( *proute );

         // Apply recursion to update route based on next trace node
         pvpr_tracePrev = pvpr_traceThis;
         pvpr_traceThis = pvpr_traceThis->next;
         this->LoadTraceRoutes_( vpr_blockArray, vpr_rrNodeArray,
                                 pvpr_traceThis, pvpr_tracePrev,
                                 &route, prouteList );
      }
   }
}

//===========================================================================//
// Method         : LoadTraceNodes_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::LoadTraceNodes_(
      const t_block*     vpr_blockArray,
      const t_rr_node*   vpr_rrNodeArray,
      const t_trace*     pvpr_traceThis,
      const t_trace*     pvpr_tracePrev,
            TNO_Route_t* proute ) const
{
   // Decide if we are adding 'instPin' or 'segment'+'switchBox' route nodes
   if( pvpr_traceThis->iblock != OPEN ) 
   {
      // Adding an 'instpin' route node based in block and port name  
      int blockIndex = pvpr_traceThis->iblock;
      int nodeIndex = pvpr_traceThis->index;

      const char* pszBlockName = vpr_blockArray[blockIndex].name;

      const t_rr_node* vpr_rrGraph = vpr_blockArray[blockIndex].pb->rr_graph;
      const t_rr_node& vpr_rrNode = vpr_rrGraph[nodeIndex];
      const t_pb_graph_pin& vpr_graphPin = *vpr_rrNode.pb_graph_pin;

      const char* pszParentName = vpr_graphPin.parent_node->pb_type->name;
      int parentIndex = vpr_graphPin.parent_node->placement_index;
      const char* pszPortName = vpr_graphPin.port->name;
      int portIndex = vpr_graphPin.pin_number;

      // Check if this route node is a PAD (ie. has capacity) and 
      // if the next route node is an OPIN (ie. does not have block index) or
      // if the previous route node was an IPIN (ie. does not have block index)
      // (if so, need to update this node's parent and port indices)
      if( vpr_blockArray[blockIndex].type &&
          vpr_blockArray[blockIndex].type->capacity > 1 )
      {
         int numPins = vpr_blockArray[blockIndex].type->num_pins;
         int capacity = vpr_blockArray[blockIndex].type->capacity;
         if(( pvpr_traceThis->next ) &&
            ( pvpr_traceThis->next->iblock == OPEN ))
         {
            int nextIndex = pvpr_traceThis->next->index;
            int ptcNum = vpr_rrNodeArray[nextIndex].ptc_num;

            parentIndex = ptcNum / ( numPins / capacity );
            portIndex = ptcNum % ( numPins / capacity );
            pszPortName = vpr_blockArray[blockIndex].type->pb_type->ports[portIndex].name;
         }
         else if(( pvpr_tracePrev ) &&
                 ( pvpr_tracePrev->iblock == OPEN ))
         {
            int prevIndex = pvpr_tracePrev->index;
            int ptcNum = vpr_rrNodeArray[prevIndex].ptc_num;

            parentIndex = ptcNum / ( numPins / capacity );
            portIndex = ptcNum % ( numPins / capacity );
            pszPortName = vpr_blockArray[blockIndex].type->pb_type->ports[portIndex].name;
         }
      }

      if( vpr_rrNode.type != SOURCE )
      {
            TNO_InstPin_c instPin( pszBlockName, 
                                   pszParentName, parentIndex,
                                   pszPortName, portIndex );
            TNO_Node_c node( instPin );
            proute->Add( node );
      }
   } 
   else 
   {
      int nodeIndexThis = pvpr_traceThis->index;
      const t_rr_node& vpr_rrNodeThis = vpr_rrNodeArray[nodeIndexThis];

      // First, need to consider adding a 'switchBox' route node
      // (if prev/this nodes are either OPIN->CHANX|Y or CHANX|Y->CHANX|Y)
      if( pvpr_tracePrev )
      {
         int nodeIndexPrev = pvpr_tracePrev->index;
         const t_rr_node& vpr_rrNodePrev = vpr_rrNodeArray[nodeIndexPrev];

         if((( vpr_rrNodePrev.type == CHANX ) || ( vpr_rrNodePrev.type == CHANY )) &&
            (( vpr_rrNodeThis.type == CHANX ) || ( vpr_rrNodeThis.type == CHANY )))
         {  
            int x, y;
            this->FindSwitchBoxCoord_( vpr_rrNodePrev, vpr_rrNodeThis, &x, &y );

            char szName[TIO_FORMAT_STRING_LEN_DATA];
            sprintf( szName, "sb[%d][%d]", x, y );

            TC_SideMode_t sidePrev = TC_SIDE_UNDEFINED;
            TC_SideMode_t sideThis = TC_SIDE_UNDEFINED;
            this->FindSwitchBoxSides_( vpr_rrNodePrev, vpr_rrNodeThis, &sidePrev, &sideThis );

            size_t trackPrev = vpr_rrNodePrev.ptc_num;
            size_t trackThis = vpr_rrNodeThis.ptc_num;

            TNO_SwitchBox_c switchBox( szName, sidePrev, trackPrev, sideThis, trackThis );
            TNO_Node_c node( switchBox );
            proute->Add( node );
         }
      }

      // Next, need to consider adding a 'segment' route node
      if(( vpr_rrNodeThis.type == CHANX ) || ( vpr_rrNodeThis.type == CHANY ))
      {
         char szName[TIO_FORMAT_STRING_LEN_DATA];
         if( vpr_rrNodeThis.type == CHANX )
         {
            sprintf( szName, "sh[%d].%d", vpr_rrNodeThis.ylow, vpr_rrNodeThis.ptc_num );
         }
         else // if( vpr_rrNodeThis.type == CHANY ))
         {
            sprintf( szName, "sv[%d].%d", vpr_rrNodeThis.xlow, vpr_rrNodeThis.ptc_num );
         }
         TGS_OrientMode_t orient( vpr_rrNodeThis.type == CHANX ?
                                  TGS_ORIENT_HORIZONTAL : TGS_ORIENT_VERTICAL );
         TGS_Region_c channel( vpr_rrNodeThis.xlow, vpr_rrNodeThis.ylow, 
                               vpr_rrNodeThis.xhigh, vpr_rrNodeThis.yhigh );
         unsigned int track = vpr_rrNodeThis.ptc_num;

         TNO_Segment_c segment( szName, orient, channel, track );
         TNO_Node_c node( segment );
         proute->Add( node );
      }
      else if(( vpr_rrNodeThis.type == IPIN ) || ( vpr_rrNodeThis.type == OPIN ))
      {
         // No action needed for these cases 
      }
   }
}

//===========================================================================//
// Method         : FindModel_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
const t_model* TVPR_CircuitDesign_c::FindModel_(
      const t_model* pvpr_models,
      const char*    pszModelName ) const
{
   const t_model* pvpr_model = pvpr_models;
   while( pvpr_model )
   {
      if( strcmp( pszModelName, pvpr_model->name ) == 0 ) 
         break;

      pvpr_model = pvpr_model->next;
   }
   return( pvpr_model );
}

//===========================================================================//
// Method         : FindModelPort_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
const t_model_ports* TVPR_CircuitDesign_c::FindModelPort_(
      const t_model*      pvpr_model,
      const char*         pszPortName,
            TC_TypeMode_t portType ) const
{
   const t_model_ports* pvpr_port = 0;
   switch( portType )
   {
   case TC_TYPE_OUTPUT: pvpr_port = pvpr_model->outputs; break;
   case TC_TYPE_INPUT:  pvpr_port = pvpr_model->inputs;  break;
   case TC_TYPE_CLOCK:  pvpr_port = pvpr_model->inputs;  break;
   default:                                              break;
   }

   while( pvpr_port )
   {
      if( strcmp( pvpr_port->name, pszPortName ) == 0 )
      {
         if( portType == TC_TYPE_OUTPUT )
         {
            break;
         }
         else if( portType == TC_TYPE_INPUT && !pvpr_port->is_clock )
         {
            break;
         }
         else if( portType == TC_TYPE_CLOCK && pvpr_port->is_clock )
         {
            break;
         }
      }
      pvpr_port = pvpr_port->next;
   }
   return( pvpr_port );
}

//===========================================================================//
// Method         : FindModelPortCount_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
int TVPR_CircuitDesign_c::FindModelPortCount_(
      const t_model*      pvpr_model,
            TC_TypeMode_t portType ) const
{
   int portCount = 0;

   const t_model_ports* pvpr_port = 0;
   switch( portType )
   {
   case TC_TYPE_INPUT:  pvpr_port = pvpr_model->inputs;  break;
   case TC_TYPE_OUTPUT: pvpr_port = pvpr_model->outputs; break;
   default:                                              break;
   }

   while( pvpr_port )
   {
      if( !pvpr_port->is_clock ) 
      {
         ++portCount;
      }
      pvpr_port = pvpr_port->next;
   }
   return( portCount );
}

//===========================================================================//
// Method         : FindGraphPin_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
const t_pb_graph_pin* TVPR_CircuitDesign_c::FindGraphPin_(
      const t_logical_block* vpr_logicalBlockArray,
      const t_net&           vpr_net,
            int              nodeIndex ) const
{
   const t_pb_graph_pin* pvpr_graphPin = 0;
   const t_model_ports* pvpr_port = 0;

   int blockIndex = vpr_net.node_block[nodeIndex];
   if( vpr_logicalBlockArray[blockIndex].pb ) 
   {
      int portIndex = vpr_net.node_block_port[nodeIndex];

      // [VPR] This net has been packed, hence pb_graph_pin does exist
      if( nodeIndex > 0 ) 
      {
         // [VPR] This is an input or clock pin
         if( !vpr_net.is_global ) 
         {
            for( pvpr_port = vpr_logicalBlockArray[blockIndex].model->inputs; 
                 pvpr_port; pvpr_port = pvpr_port->next )
            {
               if( pvpr_port->is_clock ) 
                  continue;

               if( pvpr_port->index == portIndex )
                  break;
            }
         } 
         else 
         {
            for( pvpr_port = vpr_logicalBlockArray[blockIndex].model->inputs; 
                 pvpr_port; pvpr_port = pvpr_port->next )
            {
               if( !pvpr_port->is_clock ) 
                  continue;

               if( pvpr_port->index == portIndex )
                  break;
            }
         }
      } 
      else 
      {
         // [VPR] This is an output pin
         for( pvpr_port = vpr_logicalBlockArray[blockIndex].model->outputs; 
              pvpr_port; pvpr_port = pvpr_port->next )
         {
            if( pvpr_port->index == portIndex )
               break;
         }
      }
   }

   if( pvpr_port )
   {
      int pinIndex = vpr_net.node_block_pin[nodeIndex];
      const t_pb_graph_node* pvpr_graphNode = 0;
      pvpr_graphNode = vpr_logicalBlockArray[blockIndex].pb->pb_graph_node;
      pvpr_graphPin = this->FindGraphPin_( *pvpr_graphNode, pvpr_port, pinIndex ); 
   }
   return( pvpr_graphPin );
}

//===========================================================================//
const t_pb_graph_pin* TVPR_CircuitDesign_c::FindGraphPin_(
      const t_pb_graph_node& vpr_graphNode,
      const t_model_ports*   pvpr_port, 
            int              pinIndex ) const
{
   const t_pb_graph_pin* pvpr_graphPin = 0;

   if( pvpr_port->dir == IN_PORT )
   {
      if( !pvpr_port->is_clock )
      {
         for( int portIndex = 0; portIndex < vpr_graphNode.num_input_ports; ++portIndex ) 
         {
            if(( vpr_graphNode.input_pins[portIndex][0].port->model_port == pvpr_port ) &&
               ( vpr_graphNode.num_input_pins[portIndex] > pinIndex ))
            {
               pvpr_graphPin = &vpr_graphNode.input_pins[portIndex][pinIndex];
               break;
            }
         }
      }
      else
      {
         for( int portIndex = 0; portIndex < vpr_graphNode.num_clock_ports; ++portIndex ) 
         {
            if(( vpr_graphNode.clock_pins[portIndex][0].port->model_port == pvpr_port ) &&
               ( vpr_graphNode.num_clock_pins[portIndex] > pinIndex ))
            {
               pvpr_graphPin = &vpr_graphNode.clock_pins[portIndex][pinIndex];
               break;
            }
         }
      }
   } 
   else 
   {
      for( int portIndex = 0; portIndex < vpr_graphNode.num_output_ports; ++portIndex ) 
      {
         if(( vpr_graphNode.output_pins[portIndex][0].port->model_port == pvpr_port ) &&
            ( vpr_graphNode.num_output_pins[portIndex] > pinIndex ))
         {
            pvpr_graphPin = &vpr_graphNode.output_pins[portIndex][pinIndex];
            break;
         }
      }
   }
   return( pvpr_graphPin );
}

//===========================================================================//
// Method         : FindTraceListLength_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
size_t TVPR_CircuitDesign_c::FindTraceListLength_(
      const t_trace* pvpr_traceList ) const
{
   size_t len = 0;

   for( const t_trace* pvpr_trace = pvpr_traceList; 
        pvpr_trace; pvpr_trace = pvpr_trace->next )
   {
      ++len;
   }
   return( len );
}

//===========================================================================//
// Method         : FindTraceListNode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
const t_trace* TVPR_CircuitDesign_c::FindTraceListNode_(
      const t_trace* pvpr_traceList,
            int      index ) const
{
   const t_trace* pvpr_trace_ = 0;

   for( const t_trace* pvpr_trace = pvpr_traceList; 
        pvpr_trace; pvpr_trace = pvpr_trace->next )
   {
      if( pvpr_trace->index == index )
      {
         pvpr_trace_ = pvpr_trace;
         break;
      }
   }
   return( pvpr_trace_ );
}

//===========================================================================//
// Method         : FindSwitchBoxCoord_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::FindSwitchBoxCoord_(
      const t_rr_node& vpr_rrNode1,
      const t_rr_node& vpr_rrNode2,
            int*       px,
            int*       py ) const
{
   TGS_Region_c region1( vpr_rrNode1.xlow, vpr_rrNode1.ylow,
                         vpr_rrNode1.xhigh, vpr_rrNode1.yhigh );
   TGS_Region_c region2( vpr_rrNode2.xlow, vpr_rrNode2.ylow,
                         vpr_rrNode2.xhigh, vpr_rrNode2.yhigh );

   TGS_Point_c point1, point2;
   region1.FindNearest( region2, &point2, &point1 );
   
   TGS_Region_c region( point1, point2 );
   TGS_Point_c center = region.FindCenter( );

   *px = static_cast< int >( center.x );
   *py = static_cast< int >( center.y );
}

//===========================================================================//
// Method         : FindSwitchBoxSides_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
void TVPR_CircuitDesign_c::FindSwitchBoxSides_(
      const t_rr_node&     vpr_rrNode1,
      const t_rr_node&     vpr_rrNode2,
            TC_SideMode_t* pside1,
            TC_SideMode_t* pside2 ) const
{
   if( vpr_rrNode1.type == CHANX )
   {
      *pside1 = ( vpr_rrNode1.direction == INC_DIRECTION ?
                  TC_SIDE_LEFT : TC_SIDE_RIGHT );

      if( vpr_rrNode2.type == CHANX )
      {
         *pside2 = ( vpr_rrNode1.direction == INC_DIRECTION ?
                     TC_SIDE_RIGHT : TC_SIDE_LEFT );
      }
      else if( vpr_rrNode2.type == CHANY )
      {
         *pside2 = ( vpr_rrNode2.direction == INC_DIRECTION ?
                     TC_SIDE_UPPER : TC_SIDE_LOWER );
      }
   }
   else if( vpr_rrNode1.type == CHANY )
   {
      *pside1 = ( vpr_rrNode1.direction == INC_DIRECTION ?
                  TC_SIDE_LOWER : TC_SIDE_UPPER );

      if( vpr_rrNode2.type == CHANY )
      {
         *pside2 = ( vpr_rrNode1.direction == INC_DIRECTION ?
                     TC_SIDE_UPPER : TC_SIDE_LOWER );
      }
      else if( vpr_rrNode2.type == CHANX )
      {
         *pside2 = ( vpr_rrNode2.direction == INC_DIRECTION ?
                     TC_SIDE_RIGHT : TC_SIDE_LEFT );
      }
   }
}
