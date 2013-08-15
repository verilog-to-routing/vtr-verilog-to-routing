//===========================================================================//
// Purpose : Declaration and inline definitions for the universal 
//           TCH_RelativePlaceHandler singleton class. This class supports
//           relative placement constraints for the VPR placement tool
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TCH_RELATIVE_PLACE_HANDLER_H
#define TCH_RELATIVE_PLACE_HANDLER_H

#include <cstdio>
using namespace std;

#include "TPO_Typedefs.h"
#include "TPO_Inst.h"

#include "TGO_Transform.h"

#include "TCH_Typedefs.h"
#include "TCH_RelativeMacro.h"
#include "TCH_RelativeBlock.h"
#include "TCH_RotateMaskKey.h"
#include "TCH_RotateMaskValue.h"
#include "TCH_VPR_Data.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TCH_RelativePlaceHandler_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TCH_RelativePlaceHandler_c& GetInstance( bool newInstance = true );
   static bool HasInstance( void );

   bool Configure( bool placeOptions_rotateEnable,
                   bool placeOptions_carryChainEnable,
                   size_t placeOptions_maxPlaceRetryCt,
                   size_t placeOptions_maxMacroRetryCt,
                   const TPO_InstList_t& toro_circuitBlockList );

   void Set( t_grid_tile** vpr_gridArray,
             int vpr_nx,
             int vpr_ny,
             t_block* vpr_blockArray,
             int vpr_blockCount,
             const t_type_descriptor* vpr_typeArray,
             int vpr_typeCount,
             int* vpr_freeLocationArray,
             t_legal_pos** vpr_legalPosArray );
   bool Reset( void );

   bool LoadCarryChains( const t_block* vpr_blockArray,
                         const t_pl_macro* vpr_placeMacroArray,
                         int vpr_placeMacroCount );

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

   TCH_RelativePlaceHandler_c( void );
   ~TCH_RelativePlaceHandler_c( void );

private:

   bool AddLinkConstraint_( const char* pszFromBlockName,
                            const char* pszToBlockName,
                            const TGO_IntDims_t& fromToLink,
                            bool rotateMacroEnabled = false,
                            size_t relativeMacroIndex = TCH_RELATIVE_MACRO_UNDEFINED );
   bool AddLinkConstraint_( TCH_RelativeBlock_c* pfromBlock,
                            TCH_RelativeBlock_c* ptoBlock,
                            const TGO_IntDims_t& fromToLink,
                            bool rotateMacroEnabled = false,
                            size_t relativeMacroIndex = TCH_RELATIVE_MACRO_UNDEFINED );

   void NewLinkConstraint_( TCH_RelativeBlock_c* pfromBlock,
                            TCH_RelativeBlock_c* ptoBlock,
                            const TGO_IntDims_t& fromToLink,
                            bool rotateMacroEnabled = false,
                            size_t relativeMacroIndex = TCH_RELATIVE_MACRO_UNDEFINED );
   void NewLinkConstraint_( const TCH_RelativeBlock_c& fromBlock,
                            const TCH_RelativeBlock_c& toBlock,
                            const TGO_IntDims_t& fromToLink );

   bool MergeLinkConstraints_( TCH_RelativeBlock_c* pfromBlock,
                               TCH_RelativeBlock_c* ptoBlock,
                               const TGO_IntDims_t& fromToLink,
                               bool rotateMacroEnabled = false );

   bool ExistingLinkConstraint_( const TCH_RelativeBlock_c& fromBlock,
                                 TCH_RelativeBlock_c* ptoBlock,
                                 const TGO_IntDims_t& fromToLink,
                                 bool rotateMacroEnabled = false,
                                 string* psrFromBlockName = 0,
                                 string* psrToBlockName = 0 );

   bool HasExistingLinkConstraint_( const TCH_RelativeBlock_c& fromBlock,
                                    const TCH_RelativeBlock_c& toBlock,
                                    const TGO_IntDims_t& fromToLink,
                                    string* psrFromBlockName = 0,
                                    string* psrToBlockName = 0 ) const;

   bool IsAvailableLinkConstraint_( const TCH_RelativeBlock_c& fromBlock,
                                    const TCH_RelativeBlock_c& toBlock,
                                    const TGO_IntDims_t& fromToLink,
                                    bool* pfromAvailable = 0,
                                    bool* ptoAvailable = 0,
                                    string* psrFromBlockName = 0,
                                    string* psrToBlockName = 0 ) const;

   void ResetRelativeMacroList_( const TCH_RelativeBlockList_t& relativeBlockList,
                                 TCH_RelativeMacroList_t* prelativeMacroList );
   bool ResetRelativeBlockList_( t_block* vpr_blockArray,
                                 int vpr_blockCount,
                                 TCH_RelativeBlockList_t* prelativeBlockList );

   bool InitialPlaceMacros_( bool* pplacedAllMacros );
   bool InitialPlaceMacroNodes_( TCH_RelativeMacro_c* prelativeMacro,
                                 bool* pvalidPlacement,
                                 bool* ptriedPlacement,
                                 bool* pfoundPlacement );
   void InitialPlaceMacroUpdate_( const TCH_RelativeMacro_c& relativeMacro,
                                  const TCH_RelativeBlockList_t& relativeBlockList );
   void InitialPlaceMacroResetCoords_( void );
   bool InitialPlaceMacroIsLegal_( size_t relativeNodeIndex,
                                   const TGO_Point_c& origin,
                                   TGO_RotateMode_t rotate ) const;
   bool InitialPlaceMacroIsOpen_( const TCH_RelativeMacro_c& relativeMacro,
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
                          TCH_RelativeMoveList_t* prelativeMoveList );
   void PlaceSwapUpdate_( const TCH_RelativeMove_c& relativeMove,
                          t_pl_blocks_to_be_moved& vpr_blocksAffected ) const;

   bool PlaceMacroIsRelativeCoord_( const TGO_Point_c& point ) const;
   bool PlaceMacroIsAvailableCoord_( const TGO_Point_c& fromOrigin,
                                     const TGO_Point_c& toOrigin,
                                     TGO_RotateMode_t toRotate,
                                     TCH_RelativeMoveList_t* prelativeMoveList ) const;

   void PlaceMacroResetRotate_( TCH_PlaceMacroRotateMode_t mode,
                                const TGO_Point_c& point );
   bool PlaceMacroIsLegalRotate_( TCH_PlaceMacroRotateMode_t mode,
                                  TGO_RotateMode_t rotate ) const;
   bool PlaceMacroIsValidRotate_( TCH_PlaceMacroRotateMode_t mode ) const;

   void PlaceMacroUpdateMoveList_( TCH_RelativeMoveList_t* prelativeMoveList ) const; 
   bool PlaceMacroIsLegalMoveList_( const TCH_RelativeMoveList_t& fromToMoveList, 
                                    const TCH_RelativeMoveList_t& toFromMoveList ) const; 

   TGO_RotateMode_t FindRandomRotateMode_( const TCH_RelativeMacro_c& relativeMacro ) const;
   TGO_RotateMode_t FindRandomRotateMode_( const TGO_Point_c& point ) const;
   TGO_Point_c FindRandomOriginPoint_( const TCH_RelativeMacro_c& relativeMacro,
                                       size_t relativeNodeIndex ) const;
   TGO_Point_c FindRandomOriginPoint_( const TCH_RelativeNode_c& relativeNode ) const;
   TGO_Point_c FindRandomOriginPoint_( const char* pszBlockName ) const;
   size_t FindRandomRelativeNodeIndex_( const TCH_RelativeMacro_c& relativeMacro ) const;

   size_t FindRelativeMacroIndex_( const TGO_Point_c& point ) const;
   size_t FindRelativeNodeIndex_( const TGO_Point_c& point ) const;
   TCH_RelativeMacro_c* FindRelativeMacro_( const TGO_Point_c& point ) const;
   TCH_RelativeNode_c* FindRelativeNode_( const TGO_Point_c& point ) const;
   const TCH_RelativeBlock_c* FindRelativeBlock_( const TGO_Point_c& point ) const;

   void DecideAntiLink_( const TGO_IntDims_t& relativeLink,
                         TGO_IntDims_t* pantiLink ) const;

   bool ShowIllegalRelativeMacroWarning_( const TCH_RelativeMacro_c& relativeMacro,
                                          size_t relativeNodeIndex,
                                          const TGO_Point_c& origin,
                                          TGO_RotateMode_t rotate ) const;
   bool ShowMissingBlockNameError_( const char* pszBlockName ) const;
   bool ShowInvalidConstraintError_( const TCH_RelativeBlock_c& fromBlock,
                                     const TCH_RelativeBlock_c& toBlock,
                                     const TGO_IntDims_t& fromToLink,
                                     const string& srExistingFromBlockName,
                                     const string& srExistingToBlockName ) const;
private:

   class TCH_PlaceOptions_c       
   {
   public:
      bool   rotateEnable;      // Local copies of Toro's relative place options
      bool   carryChainEnable;  // "
      size_t maxPlaceRetryCt;   // "
      size_t maxMacroRetryCt;   // "
   } placeOptions_;

   TCH_RelativeMacroList_t relativeMacroList_;
                                // Ordered list of relative placement macros
   TCH_RelativeBlockList_t relativeBlockList_;
                                // Sorted list of VPR's block names and
                                // asso. indices into the relative macro list

   TCH_RotateMaskHash_t  initialPlaceMacroCoords_; // Used during initial place
   TCH_RotateMaskValue_c fromPlaceMacroRotate_;    // "
   TCH_RotateMaskValue_c toPlaceMacroRotate_;      // "

   TCH_VPR_Data_c vpr_data_;    // Local copies of VPR's grid and block structures

   static TCH_RelativePlaceHandler_c* pinstance_;  // Define ptr for singleton instance

private:

   enum TCH_DefCapacity_e 
   { 
      TCH_RELATIVE_MACRO_LIST_DEF_CAPACITY = 1,
      TCH_RELATIVE_BLOCK_LIST_DEF_CAPACITY = 1
   };
};

#endif
