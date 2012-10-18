//===========================================================================//
// Purpose : Declaration and inline definitions for the TVPR_CircuitDesign 
//           class.
//
//===========================================================================//

#ifndef TVPR_CIRCUIT_DESIGN_H
#define TVPR_CIRCUIT_DESIGN_H

#include "TCD_CircuitDesign.h"

#include "TVPR_IndexCount.h"
#include "TVPR_Typedefs.h"

#include "vpr_api.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
class TVPR_CircuitDesign_c
{
public:

   TVPR_CircuitDesign_c( void );
   ~TVPR_CircuitDesign_c( void );

   bool Export( const TCD_CircuitDesign_c& circuitDesign,
                const t_model* pvpr_standardModels,
                const t_model* pvpr_customModels,
                t_net** pvpr_netArray,
                int* pvpr_netCount,
                t_logical_block** pvpr_logicalBlockArray,
                int* pvpr_logicalBlockCount,
                int* pvpr_primaryInputCount,
                int* pvpr_primaryOutputCount,
                bool deleteInvalidData ) const;
   
   void Import( const t_arch* vpr_architecture,
                t_net* vpr_netArray,
                int vpr_netCount,
                const t_block* vpr_blockArray,
                int vpr_blockCount,
                const t_logical_block* vpr_logicalBlockArray,
                const t_rr_node* vpr_rrNodeArray,
                TCD_CircuitDesign_c* pcircuitDesign ) const;

private:

   void InitStructures_( const TPO_InstList_t& instList,
                         const TPO_PortList_t& portList,
                         const TNO_NetList_c& netList,
                         const TNO_NameList_t& netNameList,
                         t_net** pvpr_netArray,
                         int* pvpr_netCount,
                         t_logical_block** pvpr_logicalBlockArray,
                         int* pvpr_logicalBlockCount ) const;
   
   bool PokeStructures_( const TPO_InstList_t& instList,
                         const TPO_PortList_t& portList,
                         const TNO_NetList_c& netList,
                         const t_model* pvpr_standardModels,
                         const t_model* pvpr_customModels,
                         t_net* vpr_netArray,
                         t_logical_block* vpr_logicalBlockArray,
                         int* pvpr_primaryInputCount,
                         int* pvpr_primaryOutputCount ) const;
   void PokeInputOutputList_( const TPO_PortList_t& portList,
                              const TNO_NetList_c& netList,
                              const t_model* pvpr_inputModel,
                              const t_model* pvpr_outputModel,
                              t_net* vpr_netArray,
                              t_logical_block* vpr_logicalBlockArray,
                              int* pvpr_blockIndex,
                              int* pvpr_primaryInputCount,
                              int* pvpr_primaryOutputCount ) const;
   void PokeInputOutput_( const TPO_Port_t& port,
                          const TNO_NetList_c& netList,
                          const t_model* pvpr_inputModel,
                          const t_model* pvpr_outputModel,
                          t_net* vpr_netArray,
                          t_logical_block* vpr_logicalBlockArray,
                          int* pvpr_blockIndex,
                          int* pvpr_primaryInputCount,
                          int* pvpr_primaryOutputCount ) const;
   void PokeLogicList_( const TPO_InstList_t& instList,
                        const TNO_NetList_c& netList,
                        const t_model* pvpr_logicModel,
                        t_net* vpr_netArray,
                        t_logical_block* vpr_logicalBlockArray,
                        int* pvpr_blockIndex ) const;
   void PokeLogic_( const TPO_Inst_c& inst,
                    const TNO_NetList_c& netList,
                    const t_model* pvpr_logicModel,
                    t_net* vpr_netArray,
                    t_logical_block* vpr_logicalBlockArray,
                    int* pvpr_blockIndex ) const;
   void PokeLatchList_( const TPO_InstList_t& instList,
                        const TNO_NetList_c& netList,
                        const t_model* pvpr_latchModel,
                        t_net* vpr_netArray,
                        t_logical_block* vpr_logicalBlockArray,
                        int* pvpr_blockIndex ) const;
   void PokeLatch_( const TPO_Inst_c& inst,
                    const TNO_NetList_c& netList,
                    const t_model* pvpr_latchModel,
                    t_net* vpr_netArray,
                    t_logical_block* vpr_logicalBlockArray,
                    int* pvpr_blockIndex ) const;
   void PokeSubcktList_( const TPO_InstList_t& instList,
                         const TNO_NetList_c& netList,
                         const t_model* pvpr_customModels,
                         t_net* vpr_netArray,
                         t_logical_block* vpr_logicalBlockArray,
                         int* pvpr_blockIndex ) const;
   void PokeSubckt_( const TPO_Inst_c& inst,
                     const TNO_NetList_c& netList,
                     const t_model* pvpr_modelList,
                     t_net* vpr_netArray,
                     t_logical_block* vpr_logicalBlockArray,
                     int* pvpr_blockIndex ) const;
   
   bool UpdateStructures_( t_net** pvpr_netArray,
                           int* pvpr_netCount,
                           t_logical_block** pvpr_logicalBlockArray,
                           int* pvpr_logicalBlockCount,
                           int* pvpr_primaryInputCount,
                           int* pvpr_primaryOutputCount ) const;

   void UpdateLogicalBlocks_( t_logical_block* vpr_logicalBlockArray,
                              int vpr_logicalBlockCount ) const;
   bool UpdateVpackNets_( t_net* vpr_netArray,
                          const t_logical_block* vpr_logicalBlockArray,
                          int vpr_logicalBlockCount ) const;
   bool UpdateAbsorbLogic_( t_net* vpr_netArray,
                            t_logical_block* vpr_logicalBlockArray,
                            int vpr_logicalBlockCount ) const;
   bool UpdateCompressLists_( t_net** pvpr_netArray,
                              int* pvpr_netCount,
                              t_logical_block** pvpr_logicalBlockArray,
                              int* pvpr_logicalBlockCount ) const;
   void UpdatePrimaryCounts_( t_logical_block* vpr_logicalBlockArray,
                              int vpr_logicalBlockCount,
                              int* pvpr_primaryInputCount,
                              int* pvpr_primaryOutputCount ) const;
   
   bool LoadModelLibrary_( const t_model* pvpr_standardModels,
                           t_model** ppvpr_inputModel, 
                           t_model** ppvpr_outputModel,
                           t_model** ppvpr_logicModel,
                           t_model** ppvpr_latchModel ) const;
   
   void PeekInputOutputList_( const t_block* vpr_blockArray,
                              int vpr_blockCount,
                              TPO_InstList_t* pinputOutputList ) const;
   void PeekInputOutput_( const t_block& vpr_block,
                          TPO_InstList_t* pinputOutputList ) const;
   void PeekPhysicalBlockList_( const t_block* vpr_blockArray,
                                int vpr_blockCount,
                                const t_logical_block* vpr_logicalBlockArray,
                                TPO_InstList_t* pphysicalBlockList ) const;
   void PeekPhysicalBlock_( const t_block& vpr_block,
                            const t_logical_block* vpr_logicalBlockArray,
                            int blockIndex,
                            TPO_InstList_t* pphysicalBlockList ) const;
   void PeekHierMapList_( const t_pb& vpr_pb,
                          const t_logical_block* vpr_logicalBlockArray,
                          int blockIndex,
                          TPO_HierMapList_t* phierMapList ) const;
   void PeekHierMapList_( const t_pb& vpr_pb,
                          const t_logical_block* vpr_logicalBlockArray,
                          int nodeIndex,
                          TPO_HierNameList_t* phierNameList,
                          TPO_HierMapList_t* phierMapList ) const;
   void PeekNetList_( const t_arch* vpr_architecture,
                      const t_net* vpr_netArray,
                      int vpr_netCount,
                      const t_block* vpr_blockArray,
                      int vpr_blockCount,
                      const t_logical_block* vpr_logicalBlockArray,
                      const t_rr_node* vpr_rrNodeArray,
                      TNO_NetList_c* pnetList ) const;

   bool ValidateModelList_( const TLO_CellList_t& cellList,
                            const t_model* pvpr_customModels ) const;
   bool ValidateSubcktList_( const TPO_InstList_t& instList,
                             const TLO_CellList_t& cellList ) const;
   bool ValidateInstList_( TPO_InstList_t* pinstList,
                           const TLO_CellList_t& cellList,
                           bool deleteInvalidInsts ) const;
   
   int AddVpackNet_( const char* pszNetName,
                     int pinType, 
                     bool isGlobal,
                     int blockIndex,
                     int portIndex,
                     int pinIndex,
                     const TNO_NetList_c& netList,
                     t_net* vpr_netArray ) const;
   void AddLogicalBlockInputOutput_( const char* pszPortName,
                                     int pinType, 
                                     int netIndex,
                                     const t_model* pvpr_inputModel,
                                     const t_model* pvpr_outputModel,
                                     t_logical_block* vpr_logicalBlockArray,
                                     int vpr_blockIndex,
                                     int* pvpr_primaryInputCount,
                                     int* pvpr_primaryOutputCount ) const;
   void AddLogicalBlockLogic_( const char* pszOutputPinName,
                               const TPO_PinList_t& pinList,
                               const TNO_NetList_c& netList,
                               const t_model* pvpr_logicModel,
                               t_logical_block* vpr_logicalBlockArray,
                               int vpr_blockIndex ) const;
   void AddLogicalBlockLatch_( const char* pszOutputPinName,
                               const char* pszInputPinName,
                               const char* pszClockPinName,
                               const TNO_NetList_c& netList,
                               const t_model* pvpr_latchModel,
                               t_logical_block* vpr_logicalBlockArray,
                               int vpr_blockIndex ) const;
   
   void FreeVpackNet_( t_net* vpr_netArray,
                       int netIndex ) const;
   void FreeLogicalBlock_( t_logical_block* vpr_logicalBlockArray,
                           int blockIndex ) const;
   
   void ExtractNetList_( const t_net* vpr_netArray,
                         int vpr_netCount,
                         TNO_NetList_c* pnetList ) const;
   void ExtractNetListInstPins_( const t_net* vpr_netArray,
                                 int vpr_netCount,
                                 const t_block* vpr_blockArray,
                                 const t_logical_block* vpr_logicalBlockArray,
                                 TNO_NetList_c* pnetList ) const;
   void ExtractNetListRoutes_( const t_arch* vpr_architecture,
                               const t_net* vpr_netArray,
                               int vpr_netCount,
                               const t_block* vpr_blockArray,
                               int vpr_blockCount,
                               const t_rr_node* vpr_rrNodeArray,
                               TNO_NetList_c* pnetList ) const;
   void ExtractNetRoutes_( const t_block* vpr_blockArray,
                           const t_rr_node* vpr_rrNodeArray,
                           t_trace* pvpr_traceList,
                           TNO_RouteList_t* prouteList ) const;
   bool IsNetOpen_( const t_trace* pvpr_traceList ) const;
   
   void MakeTraceNodeCountList_( const t_trace* pvpr_traceList,
                                 TVPR_IndexCountList_t* ptraceCountList ) const;
   void UpdateTraceSiblingCount_( t_trace* pvpr_traceList,
                                  const TVPR_IndexCountList_t& traceCountList ) const;
   
   void LoadTraceRoutes_( const t_block* vpr_blockArray,
                          const t_rr_node* vpr_rrNodeArray,
                          const t_trace* pvpr_traceThis,
                          const t_trace* pvpr_tracePrev,
                          TNO_Route_t* proute,
                          TNO_RouteList_t* prouteList ) const;
   void LoadTraceNodes_( const t_block* vpr_blockArray,
                         const t_rr_node* vpr_rrNodeArray,
                         const t_trace* pvpr_traceThis,
                         const t_trace* pvpr_tracePrev,
                         TNO_Route_t* proute ) const;

   const t_model* FindModel_( const t_model* pvpr_models,
                              const char* pszModelName ) const;
   const t_model_ports* FindModelPort_( const t_model* pvpr_model,
                                        const char* pszPortName,
                                        TC_TypeMode_t portType ) const;
   int FindModelPortCount_( const t_model* pvpr_model,
                            TC_TypeMode_t portType ) const;
   bool FindPinIndex_( const char* pszPinName,
                       int* ppinIndex ) const;

   const t_pb_graph_pin* FindGraphPin_( const t_logical_block* vpr_logicalBlockArray,
                                        const t_net& vpr_net,
                                        int nodeIndex ) const;
   const t_pb_graph_pin* FindGraphPin_( const t_pb_graph_node& vpr_graphNode,
                                        const t_model_ports* pvpr_port, 
                                        int pinIndex ) const;
   size_t FindTraceListLength_( const t_trace* pvpr_traceList ) const;
   const t_trace* FindTraceListNode_( const t_trace* pvpr_traceList,
                                      int index ) const;
   void FindSwitchBoxCoord_( const t_rr_node& vpr_rrNode1,
                             const t_rr_node& vpr_rrNode2,
                             int* px,
                             int* py ) const;
   void FindSwitchBoxSides_( const t_rr_node& vpr_rrNode1,
                             const t_rr_node& vpr_rrNode2,
                             TC_SideMode_t* pside1,
                             TC_SideMode_t* pside2 ) const;
};

#endif 
