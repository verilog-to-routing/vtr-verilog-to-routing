//===========================================================================//
// Purpose : Declaration and inline definitions for the TVPR_FabricModel 
//           class.
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

#ifndef TVPR_FABRIC_MODEL_H
#define TVPR_FABRIC_MODEL_H

#include "TGS_Typedefs.h"

#include "TFM_FabricModel.h"

#include "TVPR_Typedefs.h"

#include "vpr_api.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
class TVPR_FabricModel_c
{
public:

   TVPR_FabricModel_c( void );
   ~TVPR_FabricModel_c( void );

   bool Export( const TFM_FabricModel_c& fabricModel,
                t_arch* pvpr_architecture,
                const t_type_descriptor* vpr_physicalBlockArray, 
                int vpr_physicalBlockCount,
                bool overrideBlocks,
                bool overrideSwitchBoxes,
                bool overrideConnectionBlocks,
                bool overrideChannels ) const;
   bool Import( t_grid_tile** vpr_gridArray,
                int vpr_nx,
                int vpr_ny,
                const t_rr_node* vpr_rrNodeArray,
                int vpr_rrNodeCount,
                const int* vpr_xChanWidthArray,
                const int* vpr_yChanWidthArray,
                TFM_FabricModel_c* pfabricModel ) const;

private:

   bool GenerateFabricView_( const TFM_FabricModel_c& fabricModel,
                             TFV_FabricView_c* pfabricView ) const;
   void GenerateBlockPlane_( const TFM_BlockList_t& blockList,
                             TFV_FabricView_c* pfabricView ) const;
   void GenerateChannelPlane_( const TFM_ChannelList_t& channelList,
                               TFV_FabricView_c* pfabricView ) const;
   void GenerateSegmentPlane_( const TFM_SegmentList_t& segmentList,
                               TFV_FabricView_c* pfabricView ) const;
   bool GenerateConnectionPlane_( const TFM_BlockList_t& blockList,
                                  TFV_FabricView_c* pfabricView ) const;

   bool PokeFabricView_( const TFM_FabricModel_c& fabricModel,
                         const TFV_FabricView_c& fabricView,
                         t_arch* pvpr_architecture,
                         const t_type_descriptor* vpr_physicalBlockArray, 
                         int vpr_physicalBlockCount,
                         bool overrideBlocks,
                         bool overrideSwitchBoxes,
                         bool overrideConnectionBlocks,
                         bool overrideChannels ) const;
   bool PokeGridBlocks_( const TFM_BlockList_t& blockList,
                         const t_type_descriptor* vpr_physicalBlockArray, 
                         int vpr_physicalBlockCount ) const;
   void PokeSwitchBoxes_( const TFM_SwitchBoxList_t& switchBoxList ) const;
   void PokeConnectionBlocks_( const TFM_BlockList_t& blockList ) const;
   void PokeChannelWidths_( const TFM_ChannelList_t& channelList ) const;

   bool PeekFabricView_( t_grid_tile** vpr_gridArray,
                         const TGS_IntDims_t& vpr_gridDims,
                         const t_rr_node* vpr_rrNodeArray,
                         int vpr_rrNodeCount,
                         const int* vpr_xChanWidthArray,
                         const int* vpr_yChanWidthArray,
                         TFV_FabricView_c* pfabricView ) const;

   void PeekInputOutputs_( t_grid_tile** vpr_gridArray,
                           const TGS_IntDims_t& vpr_gridDims,
                           TFV_FabricView_c* pfabricView ) const;
   void PeekPhysicalBlocks_( t_grid_tile** vpr_gridArray,
                             const TGS_IntDims_t& vpr_gridDims,
                             TFV_FabricView_c* pfabricView ) const;
   void PeekChannels_( const TGS_IntDims_t& vpr_gridDims,
                       const t_rr_node* vpr_rrNodeArray,
                       int vpr_rrNodeCount,
                       const int* vpr_xChanWidthArray,
                       const int* vpr_yChanWidthArray,
                       TFV_FabricView_c* pfabricView ) const;
   void PeekSegments_( const t_rr_node* vpr_rrNodeArray,
                       int vpr_rrNodeCount,
                       TFV_FabricView_c* pfabricView ) const;
   void PeekSwitchBoxes_( const TGS_IntDims_t& vpr_gridDims,
                          const t_rr_node* vpr_rrNodeArray,
                          int vpr_rrNodeCount,
                          TFV_FabricView_c* pfabricView ) const;
   bool PeekConnectionBoxes_( const TGS_IntDims_t& vpr_gridDims,
                              const t_rr_node* vpr_rrNodeArray,
                              int vpr_rrNodeCount,
                              TFV_FabricView_c* pfabricView ) const;

   void ExtractFabricView_( const TFV_FabricView_c& fabricView,
                            TFM_FabricModel_c* pfabricModel ) const;
   void ExtractBlockPlane_( const TFV_FabricPlane_t& fabricPlane,
                            TFM_BlockList_t* pblockList ) const;
   void ExtractChannelPlane_( const TFV_FabricPlane_t& fabricPlane,
                              TFM_ChannelList_t* pchannelList ) const;
   void ExtractSegmentPlane_( const TFV_FabricPlane_t& fabricPlane,
                              TFM_SegmentList_t* psegmentList ) const;
   void ExtractPinList_( const TFV_FabricPinList_t& fabricPinList,
                         TFM_PinList_t* ppinList ) const;

   void AddBlockPinList_( const TFM_PinList_t& pinList,
                          TFV_FabricData_c* pfabricData ) const;
   void AddBlockMapTable_( const TC_MapTable_c& mapTable,
                           TFV_FabricData_c* pfabricData ) const;

   void UpdatePinList_( const TGS_Region_c& region,
                        TC_SideMode_t onlySide,
                        TFV_DataType_t dataType,
                        const t_type_descriptor vpr_type,
                        TFV_FabricView_c* pfabricView ) const;
   void UpdatePinList_( const TGS_Region_c& region,
                        TC_SideMode_t onlySide,
                        const t_type_descriptor vpr_type,
                        TFV_FabricData_c* pfabricData ) const;

   void BuildChannelDefaults_( const TGS_IntDims_t& vpr_gridDims,
                               TFV_FabricView_c* pfabricView ) const;
   void UpdateChannelCounts_( const t_rr_node* vpr_rrNodeArray,
                              int vpr_rrNodeCount,
                              const int* vpr_xChanWidthArray,
                              const int* vpr_yChanWidthArray,
                              const TFV_FabricView_c& fabricView ) const;
   void ResizeChannelWidths_( const TGS_IntDims_t& vpr_gridDims,
                              TFV_FabricView_c* pfabricView ) const;
   void ResizeChannelLengths_( const TGS_IntDims_t& vpr_gridDims,
                               TFV_FabricView_c* pfabricView ) const;

   void BuildSwitchBoxes_( const TGS_IntDims_t& vpr_gridDims,
                           TFV_FabricView_c* pfabricView ) const;
   void ClearSwitchBoxes_( TFV_FabricView_c* pfabricView ) const;
   void UpdateSwitchMapTables_( const t_rr_node* vpr_rrNodeArray,
                                int vpr_rrNodeCount,
                                const TFV_FabricView_c& fabricView ) const;

   bool BuildConnectionBoxes_( const TGS_IntDims_t& vpr_gridDims,
                               TFV_FabricView_c* pfabricView ) const;
   bool BuildConnectionRegion_( const TFV_FabricView_c& fabricView,
                                const TFV_FabricPin_c& pin,
                                const TGS_Region_c& blockRegion,
                                TGS_Region_c* pconnectionRegion ) const;
   void UpdateConnectionPoints_( const t_rr_node* vpr_rrNodeArray,
                                 int vpr_rrNodeCount,
                                 TFV_FabricView_c* pfabricView ) const;
   void UpdateConnectionPoints_( const t_rr_node& vpr_rrNodePin,
                                 const t_rr_node& vpr_rrNodeChan,
                                 TFV_FabricView_c* pfabricView ) const;

   unsigned int CalcMaxPinCount_( t_grid_tile** vpr_gridArray,
                                  const TGS_IntDims_t& vpr_gridDims ) const;
   unsigned int CalcMaxPinCount_( const t_type_descriptor vpr_type ) const;
   void CalcPinCountArray_( const t_type_descriptor vpr_type,
                            unsigned int* pcountArray ) const;
   void CalcPinDeltaArray_( const t_type_descriptor vpr_type,
                            const TGS_Region_c& region,
                            unsigned int index,
                            const unsigned int* pcountArray,
                            double* pdeltaArray ) const;

   void FindPinName_( const t_type_descriptor vpr_type,
                      int pinIndex,
                      string* psrPinName,
                      TC_TypeMode_t* ptypeMode = 0 ) const;
   void FindPinName_( const t_pb_graph_pin* pvpr_pb_graph_pin,
                      string* psrPinName ) const;
   TC_SideMode_t FindPinSide_( int side ) const;
   TC_SideMode_t FindPinSide_( const t_rr_node& vpr_rrNodePin,
                               const t_rr_node& vpr_rrNodeChan ) const;
   double FindPinDelta_( int side,
                         double* pdeltaArray ) const;

   unsigned int FindChannelCount_( const TFV_FabricView_c& fabricView,
                                   const TGS_Point_c& point,
                                   TC_SideMode_t side ) const;
   bool FindChannelRegion_( const t_rr_node& vpr_rrNode,
                            const TFV_FabricView_c& fabricView,
                            TGS_Region_c* pregion,
                            TFV_FabricData_c** ppfabricData = 0 ) const;
   bool FindSegmentRegion_( const t_rr_node& vpr_rrNode,
                            const TFV_FabricView_c& fabricView,
                            TGS_Region_c* pregion,
                            TFV_FabricData_c** ppfabricData = 0 ) const;
   bool FindBlockRegion_( const t_rr_node& vpr_rrNode,
                          const TFV_FabricView_c& fabricView,
                          TGS_Region_c* pregion,
                          TFV_FabricData_c** ppfabricData = 0 ) const;
   bool FindNodeRegion_( const t_rr_node& vpr_rrNode,
                         const TFV_FabricView_c& fabricView,
                         TGS_Layer_t layer,
                         TGS_Region_c* pregion,
                         TFV_FabricData_c** ppfabricData = 0,
                         bool applyShift = true,
                         bool applyTrack = true ) const;

   TC_SideMode_t FindSwitchSide_( const t_rr_node& vpr_rrNode,
                                  TGS_DirMode_t refDir,
                                  const TGS_Point_c& refPoint,
                                  const TGS_Region_c& switchRegion ) const;

   void LoadBlockNameIndexList_( const t_type_descriptor* vpr_physicalBlockArray, 
                                 int vpr_physicalBlockCount,
                                 TVPR_NameIndexList_t* pblockNameIndexList ) const;

   TGS_Layer_t MapDataTypeToLayer_( TFV_DataType_t dataType ) const;
   TFM_BlockType_t MapDataTypeToBlockType_( TFV_DataType_t dataType ) const;
   TFV_DataType_t MapBlockTypeToDataType_( TFM_BlockType_t blockType ) const;

   bool AddFabricViewRegion_( const TGS_Region_c& region,
                              TFV_DataType_t dataType,
                              TFV_FabricView_c* pfabricView,
                              TFV_FabricData_c** ppfabricData = 0 ) const;
   bool AddFabricViewRegion_( const TGS_Region_c& region,
                              TFV_DataType_t dataType,
                              const char* pszName,
                              TFV_FabricView_c* pfabricView,
                              TFV_FabricData_c** ppfabricData = 0 ) const;
   bool AddFabricViewRegion_( const TGS_Region_c& region,
                              TFV_DataType_t dataType,
                              const char* pszName,
                              const char* pszMasterName,
                              const TGO_Point_c& origin,
                              unsigned int sliceCount,
                              unsigned int sliceCapacity,
                              TFV_FabricView_c* pfabricView,
                              TFV_FabricData_c** ppfabricData = 0 ) const;
   bool AddFabricViewRegion_( const TGS_Region_c& region,
                              TFV_DataType_t dataType,
                              const char* pszName,
                              const TGO_Point_c& origin,
                              unsigned int trackIndex,
                              unsigned int trackHorzCount,
                              unsigned int trackVertCount,
                              TFV_FabricView_c* pfabricView,
                              TFV_FabricData_c** ppfabricData = 0 ) const;
   bool AddFabricViewRegion_( const TGS_Region_c& region,
                              TFV_DataType_t dataType,
                              const char* pszName,
                              unsigned int trackIndex,
                              TFV_FabricView_c* pfabricView,
                              TFV_FabricData_c** ppfabricData = 0 ) const;
   bool AddFabricViewRegion_( const TGS_Region_c& region,
                              const TFV_FabricData_c& fabricData,
                              TFV_FabricView_c* pfabricView,
                              TFV_FabricData_c** ppfabricData = 0 ) const;
   void DeleteFabricViewRegion_( const TGS_Region_c& region,
                                 const TFV_FabricData_c& fabricData,
                                 TFV_FabricView_c* pfabricView ) const;
   bool ReplaceFabricViewRegion_( const TGS_Region_c& region,
                                  const TGS_Region_c& region_,
                                  const TFV_FabricData_c& fabricData,
                                  TFV_FabricView_c* pfabricView ) const;
};

#endif 
