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
//           - ValidateModelList_
//           - ValidateSubcktList_
//           - ValidateInstList_
//           - AddVpackNet_
//           - AddLogicalBlockInputOutput_
//           - AddLogicalBlockLogic_
//           - AddLogicalBlockLatch_
//           - FreeVpackNet_
//           - FreeLogicalBlock_
//           - FindModel_
//           - FindModelPort_
//           - FindModelPortCount_
//           - FindPinIndex_
//
//===========================================================================//

#include <string>
using namespace std;

#include "TC_MemoryUtils.h"

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
   const TPO_PortList_t& portList = circuitDesign.portList;
   const TLO_CellList_t& cellList = circuitDesign.cellList;
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
      this->InitStructures_( instList, portList, netList, netNameList,
                             pvpr_netArray, pvpr_netCount,
                             pvpr_logicalBlockArray, pvpr_logicalBlockCount );

      ok = this->PokeStructures_( instList, portList, netList, 
                                  pvpr_standardModels, pvpr_customModels,
                                  *pvpr_netArray,
                                  *pvpr_logicalBlockArray,
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
      const t_block*             vpr_blockArray,
            int                  vpr_blockCount,
      const t_logical_block*     vpr_logicalBlockArray,
            TCD_CircuitDesign_c* pcircuitDesign ) const
{ 
   TPO_InstList_t* pblockList = &pcircuitDesign->blockList;

   this->PeekInputOutputList_( vpr_blockArray, vpr_blockCount,
                               pblockList );
   this->PeekPhysicalBlockList_( vpr_blockArray, vpr_blockCount,
                                 vpr_logicalBlockArray,
                                 pblockList );

   // WIP ??? this->PeekCellList_
   // WIP ??? this->PeekNetList_
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
      const TPO_PortList_t&   portList,
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_standardModels,
      const t_model*          pvpr_customModels,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
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
      this->PokeInputOutputList_( portList, netList,
                                  pvpr_inputModel, pvpr_outputModel,
                                  vpr_netArray,
                                  vpr_logicalBlockArray, &vpr_blockIndex,
                                  pvpr_primaryInputCount, pvpr_primaryOutputCount );

      // [VPR] Then, write ".latch" flipflop data
      this->PokeLatchList_( instList, netList,
                            pvpr_latchModel, 
                            vpr_netArray,
                            vpr_logicalBlockArray, &vpr_blockIndex );
      // [VPR] Next, write ".names" LUT data
      this->PokeLogicList_( instList, netList,
                            pvpr_logicModel, 
                            vpr_netArray,
                            vpr_logicalBlockArray, &vpr_blockIndex );

      // [VPR] Finally, write ".subckt" data, if any
      this->PokeSubcktList_( instList, netList,
                             pvpr_customModels,
                             vpr_netArray,
                             vpr_logicalBlockArray, &vpr_blockIndex );
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
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_inputModel,
      const t_model*          pvpr_outputModel,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int*              pvpr_blockIndex,
            int*              pvpr_primaryInputCount,
            int*              pvpr_primaryOutputCount ) const
{
   for( size_t i = 0; i < portList.GetLength( ); ++i )
   {
      this->PokeInputOutput_( *portList[i], netList,
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
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_logicModel,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int*              pvpr_blockIndex ) const
{
   for( size_t i = 0; i < instList.GetLength( ); ++i )
   {
      const TPO_Inst_c& inst = *instList[i];
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
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_latchModel,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int*              pvpr_blockIndex ) const
{
   for( size_t i = 0; i < instList.GetLength( ); ++i )
   {
      const TPO_Inst_c& inst = *instList[i];
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
      const TNO_NetList_c&    netList,
      const t_model*          pvpr_customModels,
            t_net*            vpr_netArray,
            t_logical_block*  vpr_logicalBlockArray,
            int*              pvpr_blockIndex ) const
{
   for( size_t i = 0; i < instList.GetLength( ); ++i )
   {
      const TPO_Inst_c& inst = *instList[i];
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
   
      int cellPinIndex = 0;
      this->FindPinIndex_( pszCellPinName, &cellPinIndex );

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
            TPO_InstList_t* pinputOutputList ) const
{ 
   // Iterate VPR's global block list for IO_TYPE blocks
   for( int blockIndex = 0; blockIndex < vpr_blockCount; ++blockIndex )
   {
      const t_block& vpr_block = vpr_blockArray[blockIndex];
      if( vpr_block.type != IO_TYPE )
         continue;

      this->PeekInputOutput_( vpr_block, 
                              pinputOutputList );
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

   TPO_StatusMode_t placeStatus = ( vpr_block.isFixed ? 
                                    TPO_STATUS_FIXED : TPO_STATUS_FLOAT );

   string srPlaceFabricName;
   if(( vpr_block.x >= 0 ) && ( vpr_block.y >= 0 ))
   {
      char szPlaceFabricName[TIO_FORMAT_STRING_LEN_DATA];
      sprintf( szPlaceFabricName, "fabric[%d][%d]", vpr_block.x, vpr_block.y );
      srPlaceFabricName = szPlaceFabricName;
   }

   if( pszInstName && *pszInstName )
   {
      TPO_Inst_c inputOutput( pszInstName );
      inputOutput.SetCellName( pszCellName );
      inputOutput.SetPlaceStatus( placeStatus );
      inputOutput.SetPlaceFabricName( srPlaceFabricName );

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
            TPO_InstList_t*  pphysicalBlockList ) const
{ 
   // Iterate VPR's global block list for non-IO_TYPE blocks
   for( int blockIndex = 0; blockIndex < vpr_blockCount; ++blockIndex )
   {
      const t_block& vpr_block = vpr_blockArray[blockIndex];
      if( vpr_block.type == IO_TYPE )
         continue;

      this->PeekPhysicalBlock_( vpr_block, vpr_logicalBlockArray,
                                blockIndex, pphysicalBlockList );
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

   TPO_HierMapList_t packHierMapList;
   if( vpr_block.pb )
   {
      this->PeekHierMapList_( *vpr_block.pb, vpr_logicalBlockArray,
                              blockIndex, &packHierMapList );
   }

   TPO_StatusMode_t placeStatus = ( vpr_block.isFixed ? 
                                    TPO_STATUS_FIXED : TPO_STATUS_FLOAT );

   string srPlaceFabricName;
   if(( vpr_block.x >= 0 ) && ( vpr_block.y >= 0 ))
   {
      char szPlaceFabricName[TIO_FORMAT_STRING_LEN_DATA];
      sprintf( szPlaceFabricName, "fabric[%d][%d]", vpr_block.x, vpr_block.y );
      srPlaceFabricName = szPlaceFabricName;
   }

// WIP ??? 
   TPO_RelativeList_t placeRelativeList; 
// WIP ??? 
   TGS_RegionList_t placeRegionList;     

   if( pszInstName && *pszInstName )
   {
      TPO_Inst_c physicalBlock( pszInstName );
      physicalBlock.SetCellName( pszCellName );
      physicalBlock.SetPackHierMapList( packHierMapList );
      physicalBlock.SetPlaceStatus( placeStatus );
      physicalBlock.SetPlaceFabricName( srPlaceFabricName );
      physicalBlock.SetPlaceRelativeList( placeRelativeList );
      physicalBlock.SetPlaceRegionList( placeRegionList );

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
   TPO_HierNameList_t hierNameList;
   this->PeekHierMapList_( vpr_pb, vpr_logicalBlockArray,
                           nodeIndex, &hierNameList, phierMapList );
}

//===========================================================================//
void TVPR_CircuitDesign_c::PeekHierMapList_(
      const t_pb&                 vpr_pb,
      const t_logical_block*      vpr_logicalBlockArray,
              int                 nodeIndex,
              TPO_HierNameList_t* phierNameList,
              TPO_HierMapList_t*  phierMapList ) const
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
                  TPO_HierNameList_t hierNameList( *phierNameList );

                  const t_pb& child_pb = vpr_pb.child_pbs[typeIndex][instIndex];
                  this->PeekHierMapList_( child_pb, vpr_logicalBlockArray,
                                          typeIndex, &hierNameList, phierMapList );
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
      const TNO_Net_c& net = *netList.Find( pszNetName );
      unsigned int netIndex = net.GetIndex( );

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
// Method         : FindPinIndex_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/25/12 jeffr : Original
//===========================================================================//
bool TVPR_CircuitDesign_c::FindPinIndex_(
      const char* pszPinName,
            int*  ppinIndex ) const
{
   bool foundIndex = false;
   int pinIndex = 0;

   string srPinName( pszPinName );
   size_t left = srPinName.find( '[' );
   size_t right = srPinName.find( ']' );
   if(( left != string::npos ) &&
      ( right != string::npos ))
   {
      string srPinIndex( srPinName.substr( left + 1, right - left - 1 ));
      pinIndex = atoi( srPinIndex.data( ));
      foundIndex = true;
   }

   if( ppinIndex )
   {
      *ppinIndex = pinIndex;
   }
   return( foundIndex );
}
