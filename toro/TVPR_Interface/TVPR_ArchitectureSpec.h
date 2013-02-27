//===========================================================================//
// Purpose : Declaration and inline definitions for the TVPR_ArchitectureSpec
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

#ifndef TVPR_ARCHITECTURE_SPEC_H
#define TVPR_ARCHITECTURE_SPEC_H

#include "TAS_ArchitectureSpec.h"

#include "vpr_api.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/10/12 jeffr : Original
//===========================================================================//
class TVPR_ArchitectureSpec_c
{
public:

   TVPR_ArchitectureSpec_c( void );
   ~TVPR_ArchitectureSpec_c( void );

   bool Export( const TAS_ArchitectureSpec_c& architectureSpec,
                t_arch* pvpr_architecture,
                t_type_descriptor** pvpr_physicalBlockArray,
                int* pvpr_physicalBlockCount,
                bool isTimingEnabled ) const;

private:

   void PokeLayout_( const TAS_Config_c& config,
                     t_arch* pvrpArchitecture ) const;
   void PokeDevice_( const TAS_Config_c& config,
                     t_arch* pvrpArchitecture,
                     bool isTimingEnabled ) const;

   void PokeModelLists_( const TAS_CellList_t& cellList,
                         t_arch* pvpr_architecture ) const;
   void PokeModel_( const TAS_Cell_c& cell,
                    t_model* pvpr_model ) const;
   void PokePhysicalBlockList_( const TAS_PhysicalBlockList_t& physicalBlockList,
                                t_type_descriptor** pvpr_physicalBlockArray, 
                                int* pvpr_physicalBlockCount ) const;
   void PokePhysicalBlock_( const TAS_PhysicalBlock_c& physicalBlock,
                            t_type_descriptor* pvpr_physicalBlock ) const; 
   bool PokeSwitchBoxList_( const TAS_SwitchBoxList_t& switchBoxList,
                            t_switch_inf** pvpr_switchBoxArray, 
                            int* pvpr_switchBoxCount,
                            bool isTimingEnabled ) const;
   bool PokeSegmentList_( const TAS_SegmentList_t& segmentList,
                          t_segment_inf** pvpr_segmentArray, 
                          int* pvpr_segmentCount,
                          const t_switch_inf* pvpr_switchBoxArray, 
                          int vpr_switchBoxCount,
                          bool isTimingEnabled ) const;

   void PokePbType_( const TAS_PhysicalBlock_c& physicalBlock,
                     const t_mode* pvpr_mode,
                     t_pb_type* pvpr_pb_type ) const;
   void PokePbTypeChild_( const t_pb_type& pb_type,
                          const string& srChildName,
                          t_pb_type* pvpr_pb_type ) const;
   void PokePbTypeLutClass_( t_pb_type* pvpr_pb_type ) const;
   void PokePbTypeMemoryClass_( t_pb_type* pvpr_pb_type ) const;

   void PokeModeList_( const TAS_PhysicalBlock_c& physicalBlock,
                       t_pb_type* pvpr_pb_type ) const;
   void PokeMode_( const string& srName,
                   const TAS_PhysicalBlockList_t& physicalBlockList,
                   const TAS_InterconnectList_t& interconnectList,
                   t_mode* pvpr_mode ) const;
   void PokeInterconnectList_( const TAS_InterconnectList_t& interconnectList,
                               t_mode* pvpr_mode ) const;
   void PokeInterconnect_( const TAS_Interconnect_c& interconnect,
                           t_interconnect* pvpr_interconnect ) const;

   void PokeFc_( const TAS_PhysicalBlock_c& physicalBlock,
                 t_type_descriptor* pvpr_physicalBlock ) const;
   void PokePinAssignList_( const TAS_PinAssignList_t& pinAssignList,
                            t_type_descriptor* pvpr_physicalBlock ) const;
   void PokeGridAssignList_( const TAS_GridAssignList_t& gridAssignList,
                             t_type_descriptor* pvpr_physicalBlock ) const;

   void PokePortList_( const TLO_PortList_t& portList,
                       t_pb_type* pvpr_pb_type ) const;
   void PokePort_( const TLO_Port_c& port,
                   t_port* pvpr_port ) const;

   void PokeTimingDelayLists_( const TAS_TimingDelayLists_c& timingDelayLists,
                               t_pin_to_pin_annotation** pvpr_annotationArray,
                               int* pvpr_annotationCount ) const;
   void PokeTimingDelay_( const TAS_TimingDelay_c& timingDelay,
                          t_pin_to_pin_annotation* pvpr_annotation ) const;

   bool InitModels_( const t_arch& vpr_architecture,
                     t_type_descriptor** pvpr_physicalBlockArray, 
                     int vpr_physicalBlockCount ) const;
   bool InitPbType_( const t_arch& vpr_architecture,
                     t_pb_type* pvpr_pb_type ) const;

   bool ValidateModels_( const t_arch& vpr_architecture ) const;

   enum e_side FindSideMode_( TC_SideMode_t mode ) const;
   enum e_pb_type_class FindClassType_( TAS_ClassType_t type ) const;
   enum e_interconnect FindInterconnectMapType_( TAS_InterconnectMapType_t type ) const;
   enum e_pin_location_distr FindPinAssignPatternType_( TAS_PinAssignPatternType_t type ) const;
   enum e_grid_loc_type FindGridAssignDistrMode_( TAS_GridAssignDistrMode_t mode ) const;

   enum e_stat FindChannelDistrMode_( TAS_ChannelDistrMode_t mode ) const;
   enum e_switch_block_type FindSwitchBoxModelType_( TAS_SwitchBoxModelType_t type ) const;
   enum e_directionality FindSegmentDirType_( TAS_SegmentDirType_t type ) const;
   bool FindSegmentBitPattern_( const TAS_BitPattern_t& bitPattern,
                                size_t bitPatternLen,
                                boolean** pvpr_patternArray,
                                int* pvpr_patternLen ) const;
};

#endif 
