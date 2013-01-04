//===========================================================================//
// Purpose : Declaration and inline definitions for the universal 
//           TRP_RelativePlaceHandler singleton class. This class supports
//           relative placement constraints for the VPR placement tool
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TRP_RELATIVE_PLACE_HANDLER_H
#define TRP_RELATIVE_PLACE_HANDLER_H

#include <cstdio>
using namespace std;

#include "TPO_Typedefs.h"
#include "TPO_Inst.h"

#include "TGO_Transform.h"

#include "TRP_Typedefs.h"
#include "TRP_RelativeMacro.h"
#include "TRP_RelativeBlock.h"
#include "TRP_RotateMaskKey.h"
#include "TRP_RotateMaskValue.h"

#include "vpr_api.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TRP_RelativePlaceHandler_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TRP_RelativePlaceHandler_c& GetInstance( bool newInstance = true );
   static bool HasInstance( void );

   bool Configure( bool placeOptions_rotateEnable,
                   size_t placeOptions_maxPlaceRetryCt,
                   size_t placeOptions_maxMacroRetryCt,
                   const TPO_InstList_t& toro_circuitBlockList );

   bool Reset( t_grid_tile** vpr_gridArray,
               int vpr_nx,
               int vpr_ny,
               t_block* vpr_blockArray,
               int vpr_blockCount,
               const t_type_descriptor* vpr_typeArray,
               int vpr_typeCount,
               int* vpr_freeLocationArray,
               t_legal_pos** vpr_legalPosArray );

   bool InitialPlace( t_grid_tile** vpr_gridArray,
                      int vpr_nx,
                      int vpr_ny,
                      t_block* vpr_blockArray,
                      int vpr_blockCount,
                      const t_type_descriptor* vpr_typeArray,
                      int vpr_typeCount,
                      int* vpr_freeLocationArray,
                      t_legal_pos** vpr_legalPosArray );

   bool Place( int x1, int y1, int z1,
               int x2, int y2, int z2,
               t_pl_blocks_to_be_moved& vpr_blocksAffected );
   bool Place( const TGO_Point_c& fromPoint,
               const TGO_Point_c& toPoint,
               t_pl_blocks_to_be_moved& vpr_blocksAffected );

   bool IsCandidate( int x1, int y1, int z1,
                     int x2, int y2, int z2 ) const;
   bool IsCandidate( const TGO_Point_c& fromPoint,
                     const TGO_Point_c& toPoint ) const;

   bool IsValid( void ) const;

protected:

   TRP_RelativePlaceHandler_c( void );
   ~TRP_RelativePlaceHandler_c( void );

private:

   bool AddSideConstraint_( const char* pszFromBlockName,
                            const char* pszToBlockName,
                            TC_SideMode_t side,
                            size_t relativeMacroIndex = TRP_RELATIVE_MACRO_UNDEFINED );
   bool AddSideConstraint_( TRP_RelativeBlock_c* pfromBlock,
                            TRP_RelativeBlock_c* ptoBlock,
                            TC_SideMode_t side,
                            size_t relativeMacroIndex = TRP_RELATIVE_MACRO_UNDEFINED );

   void NewSideConstraint_( TRP_RelativeBlock_c* pfromBlock,
                            TRP_RelativeBlock_c* ptoBlock,
                            TC_SideMode_t side,
                            size_t relativeMacroIndex = TRP_RELATIVE_MACRO_UNDEFINED );
   void NewSideConstraint_( const TRP_RelativeBlock_c& fromBlock,
                            const TRP_RelativeBlock_c& toBlock,
                            TC_SideMode_t side );

   bool MergeSideConstraints_( TRP_RelativeBlock_c* pfromBlock,
                               TRP_RelativeBlock_c* ptoBlock,
			       TC_SideMode_t side );
   bool MergeSideConstraints_( size_t fromMacroIndex,
                               const TRP_RelativeMacro_c& toMacro,
                               const TRP_RelativeNode_c& toNode,
			       TC_SideMode_t side );

   bool ExistingSideConstraint_( const TRP_RelativeBlock_c& fromBlock,
                                 TRP_RelativeBlock_c* ptoBlock,
                                 TC_SideMode_t side,
                                 string* psrFromBlockName = 0,
                                 string* psrToBlockName = 0 );

   bool HasExistingSideConstraint_( const TRP_RelativeBlock_c& fromBlock,
                                    const TRP_RelativeBlock_c& toBlock,
                                    TC_SideMode_t side,
                                    string* psrFromBlockName = 0,
                                    string* psrToBlockName = 0 ) const;
   bool IsAvailableSideConstraint_( const TRP_RelativeBlock_c& fromBlock,
                                    const TRP_RelativeBlock_c& toBlock,
                                    TC_SideMode_t side,
                                    bool* pfromAvailable = 0,
                                    bool* ptoAvailable = 0,
                                    string* psrFromBlockName = 0,
                                    string* psrToBlockName = 0 ) const;

   void SetVPR_Placement_( const TRP_RelativeMacro_c& relativeMacro,
                           const TRP_RelativeBlockList_t& relativeBlockList,
                           t_grid_tile** vpr_gridArray,
                           t_block* vpr_blockArray );

   void ResetVPR_Placement_( t_grid_tile** vpr_gridArray,
                             int vpr_nx,
                             int vpr_ny,
                             t_block* vpr_blockArray,
                             int vpr_blockCount,
	                     int vpr_typeCount,
                             int* vpr_freeLocationArray,
                             t_legal_pos** vpr_legalPosArray,
                             int** pvpr_freeLocationArray,
                             t_legal_pos*** pvpr_legalPosArray );

   void ResetRelativeMacroList_( const TRP_RelativeBlockList_t& relativeBlockList,
                                 TRP_RelativeMacroList_t* prelativeMacroList );
   bool ResetRelativeBlockList_( t_block* vpr_blockArray,
                                 int vpr_blockCount,
                                 TRP_RelativeBlockList_t* prelativeBlockList );

   bool InitialPlaceMacros_( bool* pplacedAllMacros );
   bool InitialPlaceMacroNodes_( TRP_RelativeMacro_c* prelativeMacro,
                                 bool* pvalidPlacement,
                                 bool* ptriedPlacement,
                                 bool* pfoundPlacement );
   void InitialPlaceMacroResetCoords_( void );
   bool InitialPlaceMacroIsLegal_( size_t relativeNodeIndex,
                                   const TGO_Point_c& origin,
                                   TGO_RotateMode_t rotate ) const;
   bool InitialPlaceMacroIsOpen_( const TRP_RelativeMacro_c& relativeMacro,
                                  size_t relativeNodeIndex,
                                  const TGO_Point_c& origin,
                                  TGO_RotateMode_t rotate ) const;
   bool InitialPlaceMacroIsOpen_( size_t relativeMacroIndex,
                                  size_t relativeNodeIndex,
                                  const TGO_Point_c& origin,
                                  TGO_RotateMode_t rotate ) const;

   void PlaceResetMacros_( void );
   bool PlaceSwapMacros_( const TGO_Point_c& fromPoint,
                          const TGO_Point_c& toPoint,
                          TRP_RelativeMoveList_t* prelativeMoveList );
   void PlaceSwapUpdate_( const TRP_RelativeMove_c& relativeMove,
                          t_pl_blocks_to_be_moved& vpr_blocksAffected ) const;

   bool PlaceMacroIsRelativeCoord_( const TGO_Point_c& point ) const;
   bool PlaceMacroIsAvailableCoord_( const TGO_Point_c& fromOrigin,
				     const TGO_Point_c& toOrigin,
                                     TGO_RotateMode_t toRotate,
                                     TRP_RelativeMoveList_t* prelativeMoveList ) const;

   void PlaceMacroResetRotate_( TRP_PlaceMacroRotateMode_t mode );
   bool PlaceMacroIsLegalRotate_( TRP_PlaceMacroRotateMode_t mode,
                                  TGO_RotateMode_t rotate ) const;
   bool PlaceMacroIsValidRotate_( TRP_PlaceMacroRotateMode_t mode ) const;

   void PlaceMacroUpdateMoveList_( TRP_RelativeMoveList_t* prelativeMoveList ) const; 
   bool PlaceMacroIsLegalMoveList_( const TRP_RelativeMoveList_t& fromToMoveList, 
                                    const TRP_RelativeMoveList_t& toFromMoveList ) const; 

   TGO_RotateMode_t FindRandomRotateMode_( void ) const;
   TGO_Point_c FindRandomOriginPoint_( const TRP_RelativeMacro_c& relativeMacro,
                                       size_t relativeNodeIndex ) const;
   TGO_Point_c FindRandomOriginPoint_( const TRP_RelativeNode_c& relativeNode ) const;
   TGO_Point_c FindRandomOriginPoint_( const char* pszBlockName ) const;
   size_t FindRandomRelativeNodeIndex_( const TRP_RelativeMacro_c& relativeMacro ) const;

   size_t FindRelativeMacroIndex_( const TGO_Point_c& point ) const;
   size_t FindRelativeNodeIndex_( const TGO_Point_c& point ) const;
   TRP_RelativeMacro_c* FindRelativeMacro_( const TGO_Point_c& point ) const;
   TRP_RelativeNode_c* FindRelativeNode_( const TGO_Point_c& point ) const;
   const TRP_RelativeBlock_c* FindRelativeBlock_( const TGO_Point_c& point ) const;

   const char* FindGridPointBlockName_( const TGO_Point_c& point ) const;
   int FindGridPointBlockIndex_( const TGO_Point_c& point ) const;
   const t_type_descriptor* FindGridPointType_( const TGO_Point_c& point ) const;

   bool IsEmptyGridPoint_( const TGO_Point_c& point ) const;
   bool IsWithinGrid_( const TGO_Point_c& point ) const;

   TC_SideMode_t DecideAntiSide_( TC_SideMode_t side ) const;

   bool ShowMissingBlockNameError_( const char* pszBlockName ) const;
   bool ShowInvalidConstraintError_( const TRP_RelativeBlock_c& fromBlock,
                                     const TRP_RelativeBlock_c& toBlock,
                                     TC_SideMode_t side,
                                     const string& srExistingFromBlockName,
                                     const string& srExistingToBlockName ) const;
private:

   class TRP_PlaceOptions_c       
   {
   public:
      bool   rotateEnable;      // Local copies of Toro's relative place options
      size_t maxPlaceRetryCt;   // "
      size_t maxMacroRetryCt;   // "
   } placeOptions_;

   class TRP_VPR_c
   {
   public:
      t_grid_tile** gridArray;  // Local pointer to VPR's grid array structure
      int           nx;         // "
      int           ny;         // "

      t_block*      blockArray; // Local pointer to VPR's block array structure
      int           blockCount; // "
      t_type_descriptor* typeArray;
      int                typeCount;

      int*          freeLocationArray;
      t_legal_pos** legalPosArray;

      int*          pfreeLocationArray;
      t_legal_pos** plegalPosArray;
   } vpr_;

   TRP_RelativeMacroList_t relativeMacroList_;
                                // Ordered list of relative placement macros
   TRP_RelativeBlockList_t relativeBlockList_;
                                // Sorted list of VPR's block names and
                                // asso. indices into the relative macro list

   TRP_RotateMaskHash_t  initialPlaceMacroCoords_; // Used during initial place
   TRP_RotateMaskValue_c fromPlaceMacroRotate_;    // "
   TRP_RotateMaskValue_c toPlaceMacroRotate_;      // "

   static TRP_RelativePlaceHandler_c* pinstance_;  // Define ptr for singleton instance

private:

   enum TRP_DefCapacity_e 
   { 
      TRP_RELATIVE_MACRO_LIST_DEF_CAPACITY = 1,
      TRP_RELATIVE_BLOCK_LIST_DEF_CAPACITY = 1
   };
};

#endif
