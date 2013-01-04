//===========================================================================//
// Purpose : Method definitions for the TRP_RelativePlaceHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - Configure
//           - Reset
//           - InitialPlace
//           - Place
//           - IsCandidate
//           - IsValid
//
//           Protected methods include:
//           - TRP_RelativePlaceHandler_c, ~TRP_RelativePlaceHandler_c
//
//           Private methods include:
//           - AddSideConstraint_
//           - NewSideConstraint_
//           - MergeSideConstraints_
//           - ExistingSideConstraint_
//           - HasExistingSideConstraint_
//           - IsAvailableSideConstraint_
//           - SetVPR_Placement_
//           - ResetVPR_Placement_
//           - ResetRelativeMacroList_
//           - ResetRelativeBlockList_
//           - InitialPlaceMacros_
//           - InitialPlaceMacroNodes_
//           - InitialPlaceMacroResetCoords_
//           - InitialPlaceMacroIsLegal_
//           - InitialPlaceMacroIsOpen_
//           - PlaceSwapMacros_
//           - PlaceSwapUpdate_
//           - PlaceMacroIsRelativeCoord_
//           - PlaceMacroIsAvailableCoord_
//           - PlaceMacroResetRotate_
//           - PlaceMacroIsLegalRotate_
//           - PlaceMacroIsValidRotate_
//           - PlaceMacroUpdateMoveList_
//           - PlaceMacroIsLegalMoveList_
//           - FindRandomRotateMode_
//           - FindRandomOriginPoint_
//           - FindRandomRelativeNodeIndex_
//           - FindRelativeMacroIndex_
//           - FindRelativeNodeIndex_
//           - FindRelativeMacro_
//           - FindRelativeNode_
//           - FindRelativeBlock_
//           - FindGridPointBlockName_
//           - FindGridPointBlockIndex_
//           - FindGridPointType_
//           - IsEmptyGridPoint_
//           - IsWithinGrid_
//           - DecideAntiSide_
//           - ShowMissingBlockNameError_
//           - ShowInvalidConstraintError_
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

#include <cstdio>
#include <cstring>
#include <string>
using namespace std;

#include "TC_Typedefs.h"
#include "TC_MemoryUtils.h"
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TGO_StringUtils.h"

#include "TRP_Typedefs.h"
#include "TRP_RelativeMove.h"
#include "TRP_RelativePlaceHandler.h"

// Initialize the relative place handler "singleton" class, as needed
TRP_RelativePlaceHandler_c* TRP_RelativePlaceHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TRP_RelativePlaceHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TRP_RelativePlaceHandler_c::TRP_RelativePlaceHandler_c( 
      void )
      :
      relativeMacroList_( TRP_RELATIVE_MACRO_LIST_DEF_CAPACITY ),
      relativeBlockList_( TRP_RELATIVE_BLOCK_LIST_DEF_CAPACITY )
{
   this->placeOptions_.rotateEnable = false;
   this->placeOptions_.maxPlaceRetryCt = 0;
   this->placeOptions_.maxMacroRetryCt = 0;

   this->vpr_.gridArray = 0;
   this->vpr_.nx = 0;
   this->vpr_.ny = 0;
   this->vpr_.blockArray = 0;
   this->vpr_.blockCount = 0;
   this->vpr_.typeArray = 0;
   this->vpr_.typeCount = 0;
   this->vpr_.freeLocationArray = 0;
   this->vpr_.legalPosArray = 0;
   this->vpr_.pfreeLocationArray = 0;
   this->vpr_.plegalPosArray = 0;

   pinstance_ = 0;
}

//===========================================================================//
// Method         : ~TRP_RelativePlaceHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TRP_RelativePlaceHandler_c::~TRP_RelativePlaceHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativePlaceHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TRP_RelativePlaceHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativePlaceHandler_c::DeleteInstance( 
      void )
{
   if( pinstance_ )
   {
      delete pinstance_;
      pinstance_ = 0;
   }
}

//===========================================================================//
// Method         : GetInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TRP_RelativePlaceHandler_c& TRP_RelativePlaceHandler_c::GetInstance(
      bool newInstance )
{
   if( !pinstance_ )
   {
      if( newInstance )
      {
         NewInstance( );
      }
   }
   return( *pinstance_ );
}

//===========================================================================//
// Method         : HasInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::HasInstance(
      void )
{
   return( pinstance_ ? true : false );
}

//===========================================================================//
// Method         : Configure
// Description    : Configure a relative place handler (singleton) based on
//                  VPR's block array (ie. instance list), along with Toro's
//                  block list and associated options. This may include
//                  defining any relative macros based on relative placement
//                  constraints.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::Configure(
            bool   placeOptions_rotateEnable,        // See Toro's placement options
            size_t placeOptions_maxPlaceRetryCt,     // "
            size_t placeOptions_maxMacroRetryCt,     // "
      const TPO_InstList_t&  toro_circuitBlockList ) // Defined by Toro's block list
{
   bool ok = true;

   // Start by caching placement options and VPR data (for later reference)
   this->placeOptions_.rotateEnable = placeOptions_rotateEnable;
   this->placeOptions_.maxPlaceRetryCt = placeOptions_maxPlaceRetryCt;
   this->placeOptions_.maxMacroRetryCt = placeOptions_maxMacroRetryCt;

   // Continue by setting relative block and macro list estimated capacities
   int toro_circuitBlockCount = static_cast<int>( toro_circuitBlockList.GetLength( ));
   this->relativeMacroList_.SetCapacity( toro_circuitBlockCount );
   this->relativeBlockList_.SetCapacity( toro_circuitBlockCount );

   // Initialize the local relative block list based on Toro's block list
   for( size_t i = 0; i < toro_circuitBlockList.GetLength( ); ++i )
   {
      const TPO_Inst_c& toro_circuitBlock = *toro_circuitBlockList[i];
      const char* pszBlockName = toro_circuitBlock.GetName( );

      TRP_RelativeBlock_c relativeBlock( pszBlockName );
      this->relativeBlockList_.Add( relativeBlock );
   }

   // Update the local relative block list based on Toro's block list
   // (specifically, update based on any relative placement constraints)
   for( size_t i = 0; i < toro_circuitBlockList.GetLength( ); ++i )
   {
      const TPO_Inst_c& toro_circuitBlock = *toro_circuitBlockList[i];
      const TPO_RelativeList_t& placeRelativeList = toro_circuitBlock.GetPlaceRelativeList( );
      if( !placeRelativeList.IsValid( ))
         continue;

      const char* pszFromBlockName = toro_circuitBlock.GetName( );
      for( size_t j = 0; j < placeRelativeList.GetLength( ); ++j )
      {
         const TPO_Relative_t& placeRelative = *placeRelativeList[j];

         const char* pszToBlockName = placeRelative.GetName( );
         TC_SideMode_t side = placeRelative.GetSide( );

         // Add a new relative placement side constraint
         // (by default, this also includes appropriate validation)
         ok = this->AddSideConstraint_( pszFromBlockName, pszToBlockName, side );
         if( !ok )
            break;
      }
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : Reset
// Description    : Resets the relative place handler in order to clear any
//                  existing placement information in VPR's grid array, the
//                  local relative macro list, and local relative block list.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::Reset(
            t_grid_tile**      vpr_gridArray,
            int                vpr_nx,
            int                vpr_ny,
            t_block*           vpr_blockArray,
            int                vpr_blockCount,
      const t_type_descriptor* vpr_typeArray,
            int                vpr_typeCount,
            int*               vpr_freeLocationArray,
            t_legal_pos**      vpr_legalPosArray )
{
   bool ok = true;

   // Set local reference to VPR's grid array, then reset placement information
   this->vpr_.gridArray = vpr_gridArray;
   this->vpr_.nx = vpr_nx;
   this->vpr_.ny = vpr_ny;

   this->vpr_.blockArray = vpr_blockArray;
   this->vpr_.blockCount = vpr_blockCount;
   this->vpr_.typeArray = const_cast< t_type_descriptor* >( vpr_typeArray );
   this->vpr_.typeCount = vpr_typeCount;

   this->vpr_.freeLocationArray = vpr_freeLocationArray;
   this->vpr_.legalPosArray = vpr_legalPosArray;

   this->ResetVPR_Placement_( this->vpr_.gridArray, 
                              this->vpr_.nx, this->vpr_.ny,
                              this->vpr_.blockArray, 
                              this->vpr_.blockCount,
	                      this->vpr_.typeCount,
                              this->vpr_.freeLocationArray,
                              this->vpr_.legalPosArray,
                              &this->vpr_.pfreeLocationArray,
                              &this->vpr_.plegalPosArray );

   // Reset placement and type information for relative macro and block lists
   ok = this->ResetRelativeBlockList_( this->vpr_.blockArray,
                                       this->vpr_.blockCount,
                                       &this->relativeBlockList_ );
   if( ok )
   {
      this->ResetRelativeMacroList_( this->relativeBlockList_,
                                     &this->relativeMacroList_ );
   }
   return( ok );
}

//===========================================================================//
// Method         : InitialPlace
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::InitialPlace(
            t_grid_tile**      vpr_gridArray,
            int                vpr_nx,
            int                vpr_ny,
            t_block*           vpr_blockArray,
	    int                vpr_blockCount,
      const t_type_descriptor* vpr_typeArray,
            int                vpr_typeCount,
            int*               vpr_freeLocationArray,
            t_legal_pos**      vpr_legalPosArray )
{
   bool ok = true;

   // At least one relative placement constraint has been defined
   bool placedAllMacros = true;

   // Define how many times we can retry when searching for initial placement
   size_t maxPlaceCt = this->placeOptions_.maxPlaceRetryCt;
   size_t tryPlaceCt = 0;

   while( tryPlaceCt < maxPlaceCt )
   {
      // Reset VPR grid array, and relative macro and block lists, then try place
      ok = this->Reset( vpr_gridArray, vpr_nx, vpr_ny,
                        vpr_blockArray, vpr_blockCount,
                        vpr_typeArray, vpr_typeCount,
                        vpr_freeLocationArray, vpr_legalPosArray );
      if( !ok )
         break;

      ok = this->InitialPlaceMacros_( &placedAllMacros );
      if( !ok )
         break;

      if( placedAllMacros )
         break;

      ++tryPlaceCt;
      if( tryPlaceCt < maxPlaceCt )
      {
         TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
         printHandler.Info( "Retrying initial relative macro placement, attempt %d of %d...\n", 
                            tryPlaceCt, maxPlaceCt );
      }
   }

   if( !placedAllMacros ) // Failed to comply - the Force is weak here
   {
      ok = this->Reset( vpr_gridArray, vpr_nx, vpr_ny,
                        vpr_blockArray, vpr_blockCount,
                        vpr_typeArray, vpr_typeCount,
                        vpr_freeLocationArray, vpr_legalPosArray );

      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Error( "Failed to find initial placement based on given relative placement constraints.\n" );
   }
   return( ok );
}

//===========================================================================//
// Method         : Place
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::Place( 
      int x1, int y1, int z1,
      int x2, int y2, int z2,
      t_pl_blocks_to_be_moved& vpr_blocksAffected )
{
   TGO_Point_c fromPoint( x1, y1, z1 );
   TGO_Point_c toPoint( x2, y2, z2 );

   return( this->Place( fromPoint, toPoint, vpr_blocksAffected ));
}

//===========================================================================//
bool TRP_RelativePlaceHandler_c::Place( 
      const TGO_Point_c&             fromPoint,
      const TGO_Point_c&             toPoint,
            t_pl_blocks_to_be_moved& vpr_blocksAffected )
{
   bool ok = true;

   if( this->PlaceMacroIsRelativeCoord_( fromPoint ) || 
       this->PlaceMacroIsRelativeCoord_( toPoint ))
   {
      // Reset relative macros based on latest VPR block placement coordinates
      // (which may no longer match if the most recent macro move was rejected)
      this->PlaceResetMacros_( );

      // Search for valid relative move list based on "from" and "to" grid points
      TRP_RelativeMoveList_t relativeMoveList;
      ok = this->PlaceSwapMacros_( fromPoint, toPoint,
                                   &relativeMoveList );
      if( ok )
      {
         // Load VPR's "blocks_affected" array based on relative move list
         for( size_t i = 0; i < relativeMoveList.GetLength( ); ++i )
         {
            const TRP_RelativeMove_c& relativeMove = *relativeMoveList[i];
            this->PlaceSwapUpdate_( relativeMove,
                                    vpr_blocksAffected );
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : IsCandidate
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::IsCandidate( 
      int x1, int y1, int z1,
      int x2, int y2, int z2 ) const
{
   TGO_Point_c fromPoint( x1, y1, z1 );
   TGO_Point_c toPoint( x2, y2, z2 );

   return( this->IsCandidate( fromPoint, toPoint ));
}

//===========================================================================//
bool TRP_RelativePlaceHandler_c::IsCandidate( 
      const TGO_Point_c& fromPoint,
      const TGO_Point_c& toPoint ) const
{
   return( this->PlaceMacroIsRelativeCoord_( fromPoint ) || 
           this->PlaceMacroIsRelativeCoord_( toPoint ) ?
           true : false );
}

//===========================================================================//
// Method         : IsValid
// Description    : Returns TRUE if at least one relative macro exists
//                  (ie. at least one placement constraint has been defined).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::IsValid(
      void ) const
{
   return( this->relativeMacroList_.IsValid( ) ? true : false );
}

//===========================================================================//
// Method         : AddSideConstraint_
// Description    : Adds a new relative placement constraint based on the
//                  given "from"|"to" names and side.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::AddSideConstraint_(
      const char*         pszFromBlockName,
      const char*         pszToBlockName,
            TC_SideMode_t side,
            size_t        relativeMacroIndex )
{
   bool ok = true;

   // Look up "from" and "to" relative macro indices based on given block names
   TRP_RelativeBlock_c* pfromBlock = this->relativeBlockList_.Find( pszFromBlockName );
   TRP_RelativeBlock_c* ptoBlock = this->relativeBlockList_.Find( pszToBlockName );

   if( pfromBlock && ptoBlock )
   {
      ok = this->AddSideConstraint_( pfromBlock, ptoBlock, side,
                                     relativeMacroIndex );
   }
   else
   {
      if( !pfromBlock )
      {
         ok = this->ShowMissingBlockNameError_( pszFromBlockName );
      }
      if( !ptoBlock )
      {
         ok = this->ShowMissingBlockNameError_( pszToBlockName );
      }
   }
   return( ok );
}

//===========================================================================//
bool TRP_RelativePlaceHandler_c::AddSideConstraint_(
      TRP_RelativeBlock_c* pfromBlock,
      TRP_RelativeBlock_c* ptoBlock,
      TC_SideMode_t        side,
      size_t               relativeMacroIndex )
{
   bool ok = true;

   TC_SideMode_t antiSide = this->DecideAntiSide_( side );

   if( !pfromBlock->HasRelativeMacroIndex( ) && 
       !ptoBlock->HasRelativeMacroIndex( ))
   {
      // No relative macros are currently asso. with either "from" or "to" block

      // Add new relative macro, along with asso. "from" and "to" relative nodes
      this->NewSideConstraint_( pfromBlock, ptoBlock, side, relativeMacroIndex );
   }
   else if( pfromBlock->HasRelativeMacroIndex( ) &&
            !ptoBlock->HasRelativeMacroIndex( ))
   {
      // Update existing relative macro based on "from" block, add a "to" block
      string srFromBlockName, srToBlockName;
      ok = this->ExistingSideConstraint_( *pfromBlock, ptoBlock, side,
                                          &srFromBlockName, &srToBlockName );
      if( !ok )
      {
         // Side is already in use for the existing macro "from" node, report error
         ok = this->ShowInvalidConstraintError_( *pfromBlock, *ptoBlock, side,
                                                 srFromBlockName, srToBlockName );
      }
   }
   else if( !pfromBlock->HasRelativeMacroIndex( ) &&
            ptoBlock->HasRelativeMacroIndex( ))
   {
      // Update existing relative macro based on "to" block, add a "from" block
      string srToBlockName, srFromBlockName;
      ok = this->ExistingSideConstraint_( *ptoBlock, pfromBlock, antiSide,
                                          &srToBlockName, &srFromBlockName );
      if( !ok )
      {
         // Side is already in use for the existing macro "to" node, report error
         ok = this->ShowInvalidConstraintError_( *ptoBlock, *pfromBlock, antiSide,
                                                 srToBlockName, srFromBlockName );
      }
   }
   else if( pfromBlock->HasRelativeMacroIndex( ) &&
            ptoBlock->HasRelativeMacroIndex( ))
   {
      if( pfromBlock->GetRelativeMacroIndex( ) == ptoBlock->GetRelativeMacroIndex( ))
      {
         // Same macro index => "from" and "to" blocks are from same relative macro

         // Verify side constraint is "existing" wrt. "from" and "to" relative nodes
         string srFromBlockName, srToBlockName;
         if( this->HasExistingSideConstraint_( *pfromBlock, *ptoBlock, side, 
                                               &srFromBlockName, &srToBlockName ))
         {
            // Move along, nothing to see here (given relative constraint is redundant)
         }
         else
         {
            ok = this->ShowInvalidConstraintError_( *pfromBlock, *ptoBlock, side,
                                                    srFromBlockName, srToBlockName );
         }
      }
      else
      {
         // Diff macro index => "from" and "to" blocks are from different relative macros

         // Verify side constraint is "available" wrt. "from" and "to" relative nodes
         bool fromAvailable = true;
         bool toAvailable = true;
         string srFromBlockName, srToBlockName;
         if( this->IsAvailableSideConstraint_( *pfromBlock, *ptoBlock, side,
                                                &fromAvailable, &toAvailable,
                                                &srFromBlockName, &srToBlockName ))
         {
            // Merge the "from" and "to" relative macros based on common side constraint
            ok = this->MergeSideConstraints_( pfromBlock, ptoBlock, side );
         }
         else
         {
            if( !fromAvailable )
            {
               ok = this->ShowInvalidConstraintError_( *pfromBlock, *ptoBlock, antiSide,
                                                       srFromBlockName, srToBlockName );
            }
            if( !toAvailable )
            {
               ok = this->ShowInvalidConstraintError_( *ptoBlock, *pfromBlock, antiSide,
                                                       srToBlockName, srFromBlockName );
            }
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : NewSideConstraint_
// Description    : Add a new relative macro, along with the associated 
//                  "from" and "to" relative nodes, based on the given side 
//                  constraint.  
//
//                  Note: This method assumes no relative macros are already
//                        associated with either the "from" or "to" relative
//                        blocks.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativePlaceHandler_c::NewSideConstraint_(
      TRP_RelativeBlock_c* pfromBlock,
      TRP_RelativeBlock_c* ptoBlock,
      TC_SideMode_t        side,
      size_t               relativeMacroIndex )
{
   // First, add new relative macro (or use existing, per optional parameter)
   if( relativeMacroIndex == TRP_RELATIVE_MACRO_UNDEFINED )
   {
      relativeMacroIndex = this->relativeMacroList_.GetLength( );

      TRP_RelativeMacro_c relativeMacro;
      this->relativeMacroList_.Add( relativeMacro );
   }
   TRP_RelativeMacro_c* prelativeMacro = this->relativeMacroList_[relativeMacroIndex];

   // Next, add new relative nodes based on given block names and side
   const char* pszFromBlockName = pfromBlock->GetName( );
   const char* pszToBlockName = ptoBlock->GetName( );
   size_t fromNodeIndex = TRP_RELATIVE_NODE_UNDEFINED;
   size_t toNodeIndex = TRP_RELATIVE_NODE_UNDEFINED;
   prelativeMacro->Add( pszFromBlockName, pszToBlockName, side,
                        &fromNodeIndex, &toNodeIndex );

   // Finally, update relative block macro and node indices
   pfromBlock->SetRelativeMacroIndex( relativeMacroIndex );
   pfromBlock->SetRelativeNodeIndex( fromNodeIndex );

   ptoBlock->SetRelativeMacroIndex( relativeMacroIndex );
   ptoBlock->SetRelativeNodeIndex( toNodeIndex );
}

//===========================================================================//
void TRP_RelativePlaceHandler_c::NewSideConstraint_(
      const TRP_RelativeBlock_c& fromBlock,
      const TRP_RelativeBlock_c& toBlock,
            TC_SideMode_t        side )
{
   size_t relativeMacroIndex = fromBlock.GetRelativeMacroIndex( );
   size_t fromNodeIndex = fromBlock.GetRelativeNodeIndex( );
   size_t toNodeIndex = toBlock.GetRelativeNodeIndex( );

   // First, find existing relative macro
   TRP_RelativeMacro_c* prelativeMacro = this->relativeMacroList_[relativeMacroIndex];

   // Next, add new relative nodes based on given block names and side
   prelativeMacro->Add( fromNodeIndex, toNodeIndex, side );
}

//===========================================================================//
// Method         : MergeSideConstraints_
// Description    : Merge the "from" and "to" relative macros based on a 
//                  common side constraint.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::MergeSideConstraints_(
      TRP_RelativeBlock_c* pfromBlock,
      TRP_RelativeBlock_c* ptoBlock,
      TC_SideMode_t        side )
{
   bool ok = true;

   size_t fromMacroIndex = pfromBlock->GetRelativeMacroIndex( );
   size_t toMacroIndex = ptoBlock->GetRelativeMacroIndex( );

   TRP_RelativeMacro_c& toMacro = *this->relativeMacroList_[toMacroIndex];

   // Start by clearing all block indices asso. with existing "to" relative macro 
   for( size_t i = 0; i < toMacro.GetLength( ); ++i )
   {
      const TRP_RelativeNode_c& toNode = *toMacro[i];

      const char* pszToBlockName = toNode.GetBlockName( );
      TRP_RelativeBlock_c* ptoBlock_ = this->relativeBlockList_.Find( pszToBlockName );
      ptoBlock_->Reset( );
   }

   // Then, iterate the "to" relative macro...
   // Recursively add side constraints to existing "from" relative macro
   for( size_t i = 0; i < toMacro.GetLength( ); ++i )
   {
      const TRP_RelativeNode_c& toNode = *toMacro[i];

      // Apply merge to each possible node side
      this->MergeSideConstraints_( fromMacroIndex, toMacro, toNode, TC_SIDE_LEFT );
      this->MergeSideConstraints_( fromMacroIndex, toMacro, toNode, TC_SIDE_RIGHT );
      this->MergeSideConstraints_( fromMacroIndex, toMacro, toNode, TC_SIDE_LOWER );
      this->MergeSideConstraints_( fromMacroIndex, toMacro, toNode, TC_SIDE_UPPER );
   }

   // And, add common "from->to" side constraint in existing "from" relative macro
   this->NewSideConstraint_( *pfromBlock, *ptoBlock, side );

   // All done with the original "to" relative macro...
   // but need to leave an empty macro in the list to avoid having to 
   // re-index all existing relative block index references
   toMacro.Clear( );

   return( ok );
}

//===========================================================================//
bool TRP_RelativePlaceHandler_c::MergeSideConstraints_(
            size_t               fromMacroIndex,
      const TRP_RelativeMacro_c& toMacro,
      const TRP_RelativeNode_c&  toNode,
            TC_SideMode_t        side )
{
   bool ok = true;

   if( toNode.HasSideIndex( side ))
   {
      const char* pszToBlockName = toNode.GetBlockName( );

      size_t neighborNodeIndex = toNode.GetSideIndex( side );
      const TRP_RelativeNode_c& neighborNode = *toMacro[neighborNodeIndex];
      const char* pszNeighborBlockName = neighborNode.GetBlockName( );

      ok = this->AddSideConstraint_( pszToBlockName, pszNeighborBlockName, side,
                                     fromMacroIndex );
   }
   return( ok );
}

//===========================================================================//
// Method         : ExistingSideConstraint_
// Description    : Update an existing relative macro and "from" relative 
//                  node with a new "to" relative node, based on the given 
//                  side constraint.
//
//                  Note: This method assumes a relative macro is currently 
//                        associated with the "from" relative block.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::ExistingSideConstraint_(
      const TRP_RelativeBlock_c& fromBlock,
            TRP_RelativeBlock_c* ptoBlock,
            TC_SideMode_t        side,
            string*              psrFromBlockName,
            string*              psrToBlockName )
{
   bool ok = true;

   // Find existing relative macro and node based on "from" block's indices
   size_t relativeMacroIndex = fromBlock.GetRelativeMacroIndex( );
   size_t fromNodeIndex = fromBlock.GetRelativeNodeIndex( );

   TRP_RelativeMacro_c* prelativeMacro = this->relativeMacroList_[relativeMacroIndex];
   const TRP_RelativeNode_c& fromNode = *prelativeMacro->Find( fromNodeIndex );

   if( !fromNode.HasSideIndex( side ))
   {
      // Side is available on existing macro "from" node, now add a "to" node

      // Add new "to" relative node
      const char* pszToBlockName = ptoBlock->GetName( );
      size_t toNodeIndex = TRP_RELATIVE_NODE_UNDEFINED;
      prelativeMacro->Add( fromNodeIndex, pszToBlockName, side, &toNodeIndex );

      // And, update relative block macro and node indices
      ptoBlock->SetRelativeMacroIndex( relativeMacroIndex );
      ptoBlock->SetRelativeNodeIndex( toNodeIndex );
   }
   else
   {
      // Side is already in use for existing macro "from" node, return error
      ok = false;
   }

   if( psrFromBlockName )
   {
      *psrFromBlockName = fromBlock.GetBlockName( );
   }
   if( psrToBlockName )
   {
      size_t toNodeIndex = fromNode.GetSideIndex( side );
      const TRP_RelativeNode_c* ptoNode = prelativeMacro->Find( toNodeIndex );
      *psrToBlockName = ( ptoNode ? ptoNode->GetBlockName( ) : "" );
   }
   return( ok );
}

//===========================================================================//
// Method         : HasExistingSideConstraint_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::HasExistingSideConstraint_(
      const TRP_RelativeBlock_c& fromBlock,
      const TRP_RelativeBlock_c& toBlock,
            TC_SideMode_t        side,
            string*              psrFromBlockName,
            string*              psrToBlockName ) const
{
   bool isExisting = false;

   size_t fromMacroIndex = fromBlock.GetRelativeMacroIndex( );
   size_t fromNodeIndex = fromBlock.GetRelativeNodeIndex( );

   size_t toMacroIndex = toBlock.GetRelativeMacroIndex( );
   size_t toNodeIndex = toBlock.GetRelativeNodeIndex( );

   if(( fromMacroIndex != TRP_RELATIVE_MACRO_UNDEFINED ) &&
      ( toMacroIndex != TRP_RELATIVE_MACRO_UNDEFINED ))
   {
      if(( fromMacroIndex == toMacroIndex ) &&
         ( fromNodeIndex != toNodeIndex ))
      {
         const TRP_RelativeMacro_c& relativeMacro = *this->relativeMacroList_[fromMacroIndex];
         const TRP_RelativeNode_c& fromNode = *relativeMacro[fromNodeIndex];
         const TRP_RelativeNode_c& toNode = *relativeMacro[toNodeIndex];

         TC_SideMode_t antiSide = this->DecideAntiSide_( side );
         if(( fromNode.GetSideIndex( side ) == toNodeIndex ) &&
            ( toNode.GetSideIndex( antiSide ) == fromNodeIndex ))
         {
            isExisting = true;
         }

         if( psrFromBlockName )
         {
	    fromNodeIndex = ( toNode.HasSideIndex( side ) ? 
                              toNode.GetSideIndex( side ) : fromNodeIndex );
            const TRP_RelativeNode_c* pfromNode = relativeMacro[fromNodeIndex];
            *psrFromBlockName = ( pfromNode ? pfromNode->GetBlockName( ) : "" );
         }
         if( psrToBlockName )
         {
	    toNodeIndex = ( fromNode.HasSideIndex( antiSide ) ? 
                            fromNode.GetSideIndex( antiSide ) : toNodeIndex );
            const TRP_RelativeNode_c* ptoNode = relativeMacro[toNodeIndex];
            *psrToBlockName = ( ptoNode ? ptoNode->GetBlockName( ) : "" );
         }
      }
   }
   return( isExisting );
}

//===========================================================================//
// Method         : IsAvailableSideConstraint_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::IsAvailableSideConstraint_(
      const TRP_RelativeBlock_c& fromBlock,
      const TRP_RelativeBlock_c& toBlock,
            TC_SideMode_t        side,
            bool*                pfromAvailable,
            bool*                ptoAvailable,
            string*              psrFromBlockName,
            string*              psrToBlockName ) const
{
   bool isAvailable = false;

   size_t fromMacroIndex = fromBlock.GetRelativeMacroIndex( );
   size_t fromNodeIndex = fromBlock.GetRelativeNodeIndex( );

   size_t toMacroIndex = toBlock.GetRelativeMacroIndex( );
   size_t toNodeIndex = toBlock.GetRelativeNodeIndex( );

   if(( fromMacroIndex != TRP_RELATIVE_MACRO_UNDEFINED ) &&
      ( toMacroIndex != TRP_RELATIVE_MACRO_UNDEFINED ))
   {
      if( fromMacroIndex != toMacroIndex )
      {
         const TRP_RelativeMacro_c& fromMacro = *this->relativeMacroList_[fromMacroIndex];
         const TRP_RelativeMacro_c& toMacro = *this->relativeMacroList_[toMacroIndex];

         const TRP_RelativeNode_c& fromNode = *fromMacro[fromNodeIndex];
         const TRP_RelativeNode_c& toNode = *toMacro[toNodeIndex];

         TC_SideMode_t antiSide = this->DecideAntiSide_( side );
         if(( fromNode.GetSideIndex( side ) == TRP_RELATIVE_NODE_UNDEFINED ) &&
            ( toNode.GetSideIndex( antiSide ) == TRP_RELATIVE_NODE_UNDEFINED ))
         {
            isAvailable = true;
         }

         if( pfromAvailable )
         {
            *pfromAvailable = ( fromNode.GetSideIndex( side ) == TRP_RELATIVE_NODE_UNDEFINED ? 
                                true : false );
         }
         if( ptoAvailable )
         {
            *ptoAvailable = ( toNode.GetSideIndex( side ) == TRP_RELATIVE_NODE_UNDEFINED ? 
                              true : false );
         }

         if( psrFromBlockName )
         {
            fromNodeIndex = toNode.GetSideIndex( antiSide );
            const TRP_RelativeNode_c* pfromNode = fromMacro.Find( fromNodeIndex );
            *psrFromBlockName = ( pfromNode ? pfromNode->GetBlockName( ) : "" );
         }
         if( psrToBlockName )
         {
            toNodeIndex = fromNode.GetSideIndex( side );
            const TRP_RelativeNode_c* ptoNode = toMacro.Find( toNodeIndex );
            *psrToBlockName = ( ptoNode ? ptoNode->GetBlockName( ) : "" );
         }
      }
   }
   return( isAvailable );
}

//===========================================================================//
// Method         : SetVPR_Placement_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativePlaceHandler_c::SetVPR_Placement_(
      const TRP_RelativeMacro_c&     relativeMacro,
      const TRP_RelativeBlockList_t& relativeBlockList,
            t_grid_tile**            vpr_gridArray,
            t_block*                 vpr_blockArray )
{
   for( size_t i = 0; i < relativeMacro.GetLength( ); ++i )
   {
      const TRP_RelativeNode_c& relativeNode = *relativeMacro[i];
      const TGO_Point_c& vpr_gridPoint = relativeNode.GetVPR_GridPoint( );

      const char* pszBlockName = relativeNode.GetBlockName( );
      const TRP_RelativeBlock_c& relativeBlock = *relativeBlockList.Find( pszBlockName );
      int vpr_blockIndex = relativeBlock.GetVPR_Index( );

      int x = vpr_gridPoint.x;
      int y = vpr_gridPoint.y;
      int z = vpr_gridPoint.z;
      vpr_gridArray[x][y].blocks[z] = vpr_blockIndex;
      vpr_gridArray[x][y].usage++;

      vpr_blockArray[vpr_blockIndex].x = x;
      vpr_blockArray[vpr_blockIndex].y = y;
      vpr_blockArray[vpr_blockIndex].z = z;
   }
}

//===========================================================================//
// Method         : ResetVPR_Placement_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativePlaceHandler_c::ResetVPR_Placement_(
      t_grid_tile**  vpr_gridArray,
      int            vpr_nx,
      int            vpr_ny,
      t_block*       vpr_blockArray,
      int            vpr_blockCount,
      int            vpr_typeCount,
      int*           vpr_freeLocationArray,
      t_legal_pos**  vpr_legalPosArray,
      int**          pvpr_freeLocationArray,
      t_legal_pos*** pvpr_legalPosArray )
{
   // For reference, see place.c - initial_placement() code snippet...
   for( int x = 0; x <= vpr_nx + 1; ++x )
   {
      for( int y = 0; y <= vpr_ny + 1; ++y )
      {
         if( !vpr_gridArray[x][y].type )
            continue;

         for( int z = 0; z < vpr_gridArray[x][y].type->capacity; ++z )
         {
            vpr_gridArray[x][y].blocks[z] = EMPTY;
         }
         vpr_gridArray[x][y].usage = 0;
      }
   }

   for( int blockIndex = 0; blockIndex < vpr_blockCount; ++blockIndex )
   {
      vpr_blockArray[blockIndex].x = EMPTY;
      vpr_blockArray[blockIndex].y = EMPTY;
      vpr_blockArray[blockIndex].z = EMPTY;
   }

   if( !*pvpr_freeLocationArray )
   {
      *pvpr_freeLocationArray = static_cast< int* >( TC_calloc( vpr_typeCount, sizeof(int)));

      for( int typeIndex = 0; typeIndex <= vpr_typeCount; ++typeIndex )
      {
         (*pvpr_freeLocationArray)[typeIndex] = vpr_freeLocationArray[typeIndex];
      }
   }

   for( int typeIndex = 0; typeIndex <= vpr_typeCount; ++typeIndex )
   {
      vpr_freeLocationArray[typeIndex] = (*pvpr_freeLocationArray)[typeIndex];
   }

   if( !*pvpr_legalPosArray )
   {
      *pvpr_legalPosArray = static_cast< t_legal_pos** >( TC_calloc( vpr_typeCount, sizeof(t_legal_pos*)));

      for( int typeIndex = 0; typeIndex < vpr_typeCount; ++typeIndex )
      {
         int freeCount = vpr_freeLocationArray[typeIndex];
	 (*pvpr_legalPosArray)[typeIndex] = static_cast< t_legal_pos* >( TC_calloc( freeCount, sizeof(t_legal_pos)));

         for( int freeIndex = 0; freeIndex < vpr_freeLocationArray[typeIndex]; ++freeIndex )
         {
            (*pvpr_legalPosArray)[typeIndex][freeIndex].x = vpr_legalPosArray[typeIndex][freeIndex].x;
            (*pvpr_legalPosArray)[typeIndex][freeIndex].y = vpr_legalPosArray[typeIndex][freeIndex].y;
            (*pvpr_legalPosArray)[typeIndex][freeIndex].z = vpr_legalPosArray[typeIndex][freeIndex].z;
         }
      }
   }

   for( int typeIndex = 0; typeIndex <= vpr_typeCount; ++typeIndex )
   { 
      for( int freeIndex = 0; freeIndex < vpr_freeLocationArray[typeIndex]; ++freeIndex )
      {
         vpr_legalPosArray[typeIndex][freeIndex].x = (*pvpr_legalPosArray)[typeIndex][freeIndex].x;
         vpr_legalPosArray[typeIndex][freeIndex].y = (*pvpr_legalPosArray)[typeIndex][freeIndex].y;
         vpr_legalPosArray[typeIndex][freeIndex].z = (*pvpr_legalPosArray)[typeIndex][freeIndex].z;
      }
   }
}

//===========================================================================//
// Method         : ResetRelativeMacroList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativePlaceHandler_c::ResetRelativeMacroList_(
      const TRP_RelativeBlockList_t& relativeBlockList,
            TRP_RelativeMacroList_t* prelativeMacroList )
{
   // Reset all macro node 'vpr_type' fields based on relative block list
   for( size_t i = 0; i < prelativeMacroList->GetLength( ); ++i )
   {
      TRP_RelativeMacro_c* prelativeMacro = (*prelativeMacroList)[i];
      for( size_t j = 0; j < prelativeMacro->GetLength( ); ++j )
      {
	 TRP_RelativeNode_c* prelativeNode = (*prelativeMacro)[j];

         TGO_Point_c vpr_gridPoint;
         prelativeNode->SetVPR_GridPoint( vpr_gridPoint );

         const char* pszBlockName = prelativeNode->GetBlockName( );
         const TRP_RelativeBlock_c* prelativeBlock = relativeBlockList.Find( pszBlockName );
	 if( !prelativeBlock )
            continue;

         const t_type_descriptor* vpr_type = prelativeBlock->GetVPR_Type( );
	 prelativeNode->SetVPR_Type( vpr_type );
      }
   }
}

//===========================================================================//
// Method         : ResetRelativeBlockList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::ResetRelativeBlockList_(
      t_block*                 vpr_blockArray,
      int                      vpr_blockCount,
      TRP_RelativeBlockList_t* prelativeBlockList )
{
   bool ok = true;

   // Initialize relative block list based on given VPR block array types
   for( int blockIndex = 0; blockIndex < vpr_blockCount; ++blockIndex )
   {
      const char* pszBlockName = vpr_blockArray[blockIndex].name;
      TRP_RelativeBlock_c* prelativeBlock = prelativeBlockList->Find( pszBlockName );
      if( !prelativeBlock )
         continue;

      int vpr_index = blockIndex;
      prelativeBlock->SetVPR_Index( vpr_index );

      const t_type_descriptor* vpr_type = vpr_blockArray[blockIndex].type;
      prelativeBlock->SetVPR_Type( vpr_type );
   }

   // Validate relative block list based on local VPR block name list
   TRP_RelativeBlockList_t vpr_blockList( vpr_blockCount );
   for( int blockIndex = 0; blockIndex < vpr_blockCount; ++blockIndex )
   {
      const char* pszBlockName = vpr_blockArray[blockIndex].name;
      vpr_blockList.Add( pszBlockName );
   }

   for( size_t i = 0; i < prelativeBlockList->GetLength( ); ++i )
   {
      const TRP_RelativeBlock_c& relativeBlock = *(*prelativeBlockList)[i];
      const char* pszBlockName = relativeBlock.GetBlockName( );
      if( !vpr_blockList.IsMember( pszBlockName ))
      {
         ok = this->ShowMissingBlockNameError_( pszBlockName );
         if( !ok )
            break;
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : InitialPlaceMacros_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::InitialPlaceMacros_(
      bool* pplacedAllMacros )
{
   bool ok = true;

   bool foundAllPlacements = true;

   for( size_t i = 0; i < this->relativeMacroList_.GetLength( ); ++i )
   {
      TRP_RelativeMacro_c* prelativeMacro = this->relativeMacroList_[i];

      // Reset (ie. clear) and existing macro placement coords
      this->InitialPlaceMacroResetCoords_( );

      // Define how many times we can retry when searching for an initial placement
      size_t maxMacroCt = this->placeOptions_.maxMacroRetryCt * prelativeMacro->GetLength( );
      size_t tryMacroCt = 0;

      while( tryMacroCt < maxMacroCt )
      {
         bool validPlacement = false;
         bool triedPlacement = false;
         bool foundPlacement = false;
         ok = this->InitialPlaceMacroNodes_( prelativeMacro, 
                                             &validPlacement, &triedPlacement, &foundPlacement );
	 if( !ok )
	    break;

         foundAllPlacements = validPlacement;
         if( !validPlacement )
            break;

         if( !triedPlacement )
            continue;

         if( foundPlacement )
            break;

         ++tryMacroCt;
         if( tryMacroCt < maxMacroCt )
         {
            TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
            printHandler.Info( "Retrying relative macro placement, attempt %d of %d...\n",
                               tryMacroCt, maxMacroCt );
         }
      }

      if( tryMacroCt > maxMacroCt )
      {
         foundAllPlacements = false;
      }
      if( !foundAllPlacements )
         break;
   }

   if( !foundAllPlacements ) // Failed to place at least one of the relative macros
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Warning( "Failed initial relative placement based on given relative placement constraints.\n" );
   }

   if( *pplacedAllMacros )
   {
      *pplacedAllMacros = foundAllPlacements;
   }
   return( ok );
}

//===========================================================================//
// Method         : InitialPlaceMacroNodes_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::InitialPlaceMacroNodes_(
      TRP_RelativeMacro_c* prelativeMacro,
      bool*                pvalidPlacement,
      bool*                ptriedPlacement,
      bool*                pfoundPlacement )
{
   bool ok = true;

   *pvalidPlacement = false;
   *ptriedPlacement = false;
   *pfoundPlacement = false;

   // Select a random relative node index based on relative macro length
   size_t relativeNodeIndex = this->FindRandomRelativeNodeIndex_( *prelativeMacro );

   // Select a random (and available) point from within VPR's grid array
   TGO_Point_c origin = this->FindRandomOriginPoint_( *prelativeMacro, 
                                                      relativeNodeIndex );
   // Select a random rotate mode (if enabled)
   TGO_RotateMode_t rotate = this->FindRandomRotateMode_( );

   // Test if we already tried this combination of node, origin, and rotate
   if( origin.IsValid( ) &&
       this->InitialPlaceMacroIsLegal_( relativeNodeIndex, origin, rotate ))
   {
      *pvalidPlacement = true;
      *ptriedPlacement = true;

      if( this->InitialPlaceMacroIsOpen_( *prelativeMacro, 
                                          relativeNodeIndex, origin, rotate ))
      {
         // Accept current relative placement node, origin, and rotate
         prelativeMacro->Set( relativeNodeIndex, origin, rotate );

         // And, update VPR's grid and block arrays based on relative macro
         this->SetVPR_Placement_( *prelativeMacro,
				  this->relativeBlockList_,
                                  this->vpr_.gridArray,
                                  this->vpr_.blockArray );
         *pfoundPlacement = true;
      }
      else
      {
         const TRP_RelativeNode_c& relativeNode = *prelativeMacro->Find( relativeNodeIndex );
         const char* pszBlockName = relativeNode.GetBlockName( );

         string srOrigin;
         origin.ExtractString( &srOrigin );

         string srRotate;
         TGO_ExtractStringRotateMode( rotate, &srRotate );

         TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
         ok = printHandler.Warning( "Failed relative macro placement based on reference block \"%s\" at origin (%s) and rotate %s.\n",
                                    TIO_PSZ_STR( pszBlockName ),
                                    TIO_SR_STR( srOrigin ),
                                    TIO_SR_STR( srRotate ));
      }
   }
   else if( origin.IsValid( ))
   {
      *pvalidPlacement = true;
   }
   return( ok );
}

//===========================================================================//
// Method         : InitialPlaceMacroResetCoords_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativePlaceHandler_c::InitialPlaceMacroResetCoords_(
      void )
{
   TRP_RotateMaskHash_t* protateMaskHash = &this->initialPlaceMacroCoords_;

   protateMaskHash->Clear( );
}

//===========================================================================//
// Method         : InitialPlaceMacroIsLegal_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::InitialPlaceMacroIsLegal_(
            size_t           relativeNodeIndex,
      const TGO_Point_c&     origin,
            TGO_RotateMode_t rotate ) const
{
   bool isLegal = true;

   TRP_RotateMaskHash_t* protateMaskHash = const_cast< TRP_RotateMaskHash_t* >( &this->initialPlaceMacroCoords_ );

   TRP_RotateMaskKey_c rotateMaskKey( origin, relativeNodeIndex );
   TRP_RotateMaskValue_c* protateMaskValue;
   if( protateMaskHash->Find( rotateMaskKey, &protateMaskValue ) &&
       !protateMaskValue->GetBit( rotate ))
   {
      isLegal = false;
   }
   else
   {
      isLegal = true;

      if( !protateMaskHash->IsMember( rotateMaskKey ))
      {
         TRP_RotateMaskValue_c rotateMaskValue( this->placeOptions_.rotateEnable );

         protateMaskHash->Add( rotateMaskKey, rotateMaskValue );
         protateMaskHash->Find( rotateMaskKey, &protateMaskValue );
      }
      protateMaskValue->ClearBit( rotate );
   }
   return( isLegal );
}

//===========================================================================//
// Method         : InitialPlaceMacroIsOpen_
// Description    : Returns TRUE if the given relative macro placement origin
//                  point and associated transform is open, as determined by
//                  the current state of VPR's grid array.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::InitialPlaceMacroIsOpen_(
      const TRP_RelativeMacro_c& relativeMacro,
            size_t               relativeNodeIndex,
      const TGO_Point_c&         origin,
            TGO_RotateMode_t     rotate ) const
{
   bool isOpen = false;

   TRP_RelativeMacro_c relativeMacro_( relativeMacro );
   relativeMacro_.Set( relativeNodeIndex, origin, rotate );

   for( size_t i = 0; i < relativeMacro_.GetLength( ); ++i )
   {
      const TRP_RelativeNode_c& relativeNode = *relativeMacro_[i];

      // Query VPR's grid array to test (1) open && (2) matching type ptr
      const TGO_Point_c& gridPoint = relativeNode.GetVPR_GridPoint( );
      const t_type_descriptor* vpr_type = relativeNode.GetVPR_Type( );
      if(( this->IsEmptyGridPoint_( gridPoint )) &&
         ( this->FindGridPointType_( gridPoint ) == vpr_type ))
      {
         isOpen = true;
         continue;
      }
      else
      {
         isOpen = false;
         break;
      }
   }
   return( isOpen );
}

//===========================================================================//
bool TRP_RelativePlaceHandler_c::InitialPlaceMacroIsOpen_(
            size_t           relativeMacroIndex,
            size_t           relativeNodeIndex,
      const TGO_Point_c&     origin,
            TGO_RotateMode_t rotate ) const
{
   bool isOpen = false;

   const TRP_RelativeMacro_c* prelativeMacro = this->relativeMacroList_[relativeMacroIndex];
   if( prelativeMacro )
   {
      isOpen = this->InitialPlaceMacroIsOpen_( *prelativeMacro, 
                                               relativeNodeIndex, origin, rotate );
   }
   return( isOpen );
}

//===========================================================================//
// Method         : PlaceResetMacros_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativePlaceHandler_c::PlaceResetMacros_(
      void )
{
   for( size_t i = 0; i < this->relativeMacroList_.GetLength( ); ++i )
   {
      TRP_RelativeMacro_c* prelativeMacro = this->relativeMacroList_[i];
      for( size_t j = 0; j < prelativeMacro->GetLength( ); ++j )
      {
         TRP_RelativeNode_c* prelativeNode = (*prelativeMacro)[j];

         const char* pszBlockName = prelativeNode->GetBlockName( );
         const TRP_RelativeBlock_c& relativeBlock = *this->relativeBlockList_.Find( pszBlockName );

         int vpr_index = relativeBlock.GetVPR_Index( );
         TGO_Point_c vpr_gridPoint( this->vpr_.blockArray[vpr_index].x,
                                    this->vpr_.blockArray[vpr_index].y,
                                    this->vpr_.blockArray[vpr_index].z );

         prelativeNode->SetVPR_GridPoint( vpr_gridPoint );
      }
   }
}

//===========================================================================//
// Method         : PlaceSwapMacros_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::PlaceSwapMacros_(
      const TGO_Point_c&            fromPoint,
      const TGO_Point_c&            toPoint,
            TRP_RelativeMoveList_t* prelativeMoveList )
{
   TRP_RelativeMoveList_t fromToMoveList;
   TRP_RelativeMoveList_t toFromMoveList;

   bool foundPlaceSwap = false;

   // Iterate attempt to swap "from" => "to" based available (optional) rotation
   this->PlaceMacroResetRotate_( TRP_PLACE_MACRO_ROTATE_TO );
   while( this->PlaceMacroIsValidRotate_( TRP_PLACE_MACRO_ROTATE_TO ))
   {
      // Select a random rotate mode (if enabled)
      TGO_RotateMode_t toRotate = this->FindRandomRotateMode_( );

      if( this->PlaceMacroIsLegalRotate_( TRP_PLACE_MACRO_ROTATE_TO, toRotate ))
      {
         fromToMoveList.Clear( );

         // Query if "to" is available based on "from" origin and "to" transform
         // (and return list of "from->to" moves corresponding to availability)
         if( this->PlaceMacroIsAvailableCoord_( fromPoint, toPoint, toRotate,
                                                &fromToMoveList ))
         {
            // Iterate attempt to swap "to" => "from" based available rotation
            this->PlaceMacroResetRotate_( TRP_PLACE_MACRO_ROTATE_FROM );
            while( this->PlaceMacroIsValidRotate_( TRP_PLACE_MACRO_ROTATE_FROM ))
            {
               // Select a random rotate mode (if enabled)
               TGO_RotateMode_t fromRotate = this->FindRandomRotateMode_( );
               if( this->PlaceMacroIsLegalRotate_( TRP_PLACE_MACRO_ROTATE_FROM, fromRotate ))
               {
                  toFromMoveList.Clear( );

                  // Query if "from" is available based on "to" origin and inverse
                  // (and return list of "to->from" moves corresponding to availability)
                  if( this->PlaceMacroIsAvailableCoord_( toPoint, fromPoint, fromRotate,
                                                         &toFromMoveList ) &&
                     this->PlaceMacroIsLegalMoveList_( fromToMoveList, 
                                                       toFromMoveList ))

                  {
                     // Found swappable "from" and "to" move lists
                     foundPlaceSwap = true;
                     break;
                  }
               }
            }
            if( foundPlaceSwap )
               break;
         }
      }
   }

   if( foundPlaceSwap )
   {
      prelativeMoveList->Add( fromToMoveList );
      prelativeMoveList->Add( toFromMoveList );

      this->PlaceMacroUpdateMoveList_( prelativeMoveList );
   }
   return( foundPlaceSwap );
}

//===========================================================================//
// Method         : PlaceSwapUpdate_
// Reference      : Set VPR's setup_blocks_affected function
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativePlaceHandler_c::PlaceSwapUpdate_( 
      const TRP_RelativeMove_c&      relativeMove,
            t_pl_blocks_to_be_moved& vpr_blocksAffected ) const
{
   const TGO_Point_c& fromPoint = relativeMove.GetFromPoint( );
   const TGO_Point_c& toPoint = relativeMove.GetToPoint( );

   // Update relative node grid points per from/to points, as needed
   TRP_RelativeNode_c* pfromNode = this->FindRelativeNode_( fromPoint );
   if( pfromNode )
   {
      pfromNode->SetVPR_GridPoint( toPoint );
   }
   TRP_RelativeNode_c* ptoNode = this->FindRelativeNode_( toPoint );
   if( ptoNode )
   {
      ptoNode->SetVPR_GridPoint( fromPoint );
   }

   // Find VPR's global "block" array indices per from/to points
   int fromBlockIndex = this->FindGridPointBlockIndex_( fromPoint );
   bool fromEmpty = relativeMove.GetFromEmpty( );
   bool toEmpty = relativeMove.GetToEmpty( );

   // Update VPR's global "block" array using locally cached reference
   this->vpr_.blockArray[fromBlockIndex].x = toPoint.x;
   this->vpr_.blockArray[fromBlockIndex].y = toPoint.y;
   this->vpr_.blockArray[fromBlockIndex].z = toPoint.z;

   // Update VPR's global "blocks_affected" array using locally passed reference
   int i = vpr_blocksAffected.num_moved_blocks;
   vpr_blocksAffected.moved_blocks[i].block_num = fromBlockIndex;
   vpr_blocksAffected.moved_blocks[i].xold = fromPoint.x;
   vpr_blocksAffected.moved_blocks[i].yold = fromPoint.y;
   vpr_blocksAffected.moved_blocks[i].zold = fromPoint.z;
   vpr_blocksAffected.moved_blocks[i].xnew = toPoint.x;
   vpr_blocksAffected.moved_blocks[i].ynew = toPoint.y;
   vpr_blocksAffected.moved_blocks[i].znew = toPoint.z;
   vpr_blocksAffected.moved_blocks[i].swapped_to_was_empty = toEmpty;
   vpr_blocksAffected.moved_blocks[i].swapped_from_is_empty = fromEmpty;
   vpr_blocksAffected.num_moved_blocks ++;
}

//===========================================================================//
// Method         : PlaceMacroIsRelativeCoord_
// Description    : Returns TRUE if the given VPR grid array coordinate is
//                  associated with a relative macro.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::PlaceMacroIsRelativeCoord_(
      const TGO_Point_c& point ) const
{
   bool isMacro = false;

   // Find relative block based on VPR's grid array point
   const TRP_RelativeBlock_c* prelativeBlock = this->FindRelativeBlock_( point );

   // Decide if relative block has a relative macro associated with it
   if( prelativeBlock &&
       prelativeBlock->HasRelativeMacroIndex( ) &&
       prelativeBlock->HasRelativeNodeIndex( ))
   {
      isMacro = true;
   }
   return( isMacro );
}

//===========================================================================//
// Method         : PlaceMacroIsAvailableCoord_
// Description    : Returns TRUE if the given relative macro placement can be
//                  transformed to a legal placement based on the current
//                  state of VPR's grid array. Optionally, returns a list of
//                  moves corresponding to the availability testing.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::PlaceMacroIsAvailableCoord_(
      const TGO_Point_c&            fromOrigin,
      const TGO_Point_c&            toOrigin,
            TGO_RotateMode_t        toRotate,
            TRP_RelativeMoveList_t* prelativeMoveList ) const
{
   bool isAvailable = true;

   size_t fromMacroIndex = this->FindRelativeMacroIndex_( fromOrigin );
   size_t toMacroIndex = this->FindRelativeMacroIndex_( toOrigin );

   TRP_RelativeMacro_c fromMacro_;
   const TRP_RelativeMacro_c* pfromMacro = this->FindRelativeMacro_( fromOrigin );
   if( !pfromMacro )
   {
      fromMacro_.Add( );
      fromMacro_.Set( 0, fromOrigin ); 
      pfromMacro = &fromMacro_;
   }

   TRP_RelativeMoveList_t relativeMoveList( 2 * pfromMacro->GetLength( ));

   for( size_t i = 0; i < pfromMacro->GetLength( ); ++i )
   {
      const TRP_RelativeNode_c& fromNode = *(*pfromMacro)[i];
      TGO_Point_c fromPoint = fromNode.GetVPR_GridPoint( );

      // Apply transform to relative node's grid point (based on origin point)
      TGO_Transform_c toTransform( toOrigin, toRotate, fromOrigin, fromPoint );
      TGO_Point_c toPoint;
      toTransform.Apply( toOrigin, &toPoint );

      // Validate that transformed point is still valid wrt. VPR's grid array
      if( !this->IsWithinGrid_( toPoint ))
      {
         isAvailable = false;
         break;
      }

      // Validate that we are attempting between non-identifical macros
      if( fromMacroIndex == toMacroIndex )
      {
         isAvailable = false;
         break;
      }

      const t_type_descriptor* vpr_fromType = this->FindGridPointType_( fromPoint );
      const t_type_descriptor* vpr_toType = this->FindGridPointType_( toPoint );
      if( vpr_fromType != vpr_toType )
      {
         // VPR's grid types do not match => location is *not* available
         isAvailable = false;
         break;
      }

      if( this->IsEmptyGridPoint_( fromPoint ))
      {
         // VPR's grid location is empty => really not much we can do today
         continue;
      }

      if( this->IsEmptyGridPoint_( toPoint ))
      {
         // VPR's grid location is empty => it is available

         // Add move "from" relative macro node -> "to" empty block location
         TRP_RelativeMove_c fromMove( fromMacroIndex, fromPoint, toPoint );
         relativeMoveList.Add( fromMove );
         continue;
      }

      const TRP_RelativeBlock_c* pfromBlock = this->FindRelativeBlock_( fromPoint );
      if( !pfromBlock ||
          !pfromBlock->HasRelativeMacroIndex( ))
      {
         // VPR's grid block name is not asso. with a relative macro => not available
         continue;
      }
      
      const TRP_RelativeBlock_c* ptoBlock = this->FindRelativeBlock_( toPoint );
      if( !ptoBlock ||
          !ptoBlock->HasRelativeMacroIndex( ))
      {
         // VPR's grid block name is not asso. with a relative macro => it is available

         // Add move "from" relative macro node -> "to" single block location,
         // and add move "to" single block -> "from" relative macro node
         // (since "to" block location not a member of a relative macro)
         TRP_RelativeMove_c fromMove( fromMacroIndex, fromPoint, toPoint );
         TRP_RelativeMove_c toMove( toPoint, fromMacroIndex, fromPoint );
         relativeMoveList.Add( fromMove );
         relativeMoveList.Add( toMove );
         continue;
      }

      if( ptoBlock && 
          ptoBlock->GetRelativeMacroIndex( ) == toMacroIndex )
      {
         // VPR's grid block name matches the given "to" relative macro => it is available

         // Add move "from" relative macro node -> "to" single block location,
         // but ignore move "to" relative macro node -> "from" relative macro node
         // (since "to" move will be captured by to->from availability processing)
         TRP_RelativeMove_c fromMove( fromMacroIndex, fromPoint, toMacroIndex, toPoint );
         relativeMoveList.Add( fromMove );
         continue;
      }

      // If we arrive at this point, then we know that VPR's grid block must 
      // be a member of some other unrelated relative macro
      isAvailable = FALSE;
      break;
   }

   if( isAvailable )
   {
      // Add list of local moves to the optional relative move list
      prelativeMoveList->Add( relativeMoveList );
   } 
   return( isAvailable ); // TRUE => all relative nodes can be transformed
}

//===========================================================================//
// Method         : PlaceMacroResetRotate_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativePlaceHandler_c::PlaceMacroResetRotate_(
      TRP_PlaceMacroRotateMode_t mode )
{
   TRP_RotateMaskValue_c* protateMaskValue = ( mode == TRP_PLACE_MACRO_ROTATE_FROM ?
                                               &this->fromPlaceMacroRotate_ :
                                               &this->toPlaceMacroRotate_ );
   protateMaskValue->Init( this->placeOptions_.rotateEnable );
}

//===========================================================================//
// Method         : PlaceMacroIsLegalRotate_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::PlaceMacroIsLegalRotate_( 
      TRP_PlaceMacroRotateMode_t mode,
      TGO_RotateMode_t           rotate ) const
{
   bool isLegal = true;

   const TRP_RotateMaskValue_c* pplaceMacroRotate = ( mode == TRP_PLACE_MACRO_ROTATE_FROM ?
                                                      &this->fromPlaceMacroRotate_ :
                                                      &this->toPlaceMacroRotate_ );
   TRP_RotateMaskValue_c* protateMaskValue = const_cast< TRP_RotateMaskValue_c* >( pplaceMacroRotate );
   if( !protateMaskValue->GetBit( rotate ))
   {
      isLegal = false;
   }
   else
   {
      isLegal = true;

      protateMaskValue->ClearBit( rotate );
   }
   return( isLegal );
}

//===========================================================================//
// Method         : PlaceMacroIsValidRotate_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::PlaceMacroIsValidRotate_( 
      TRP_PlaceMacroRotateMode_t mode ) const
{
   const TRP_RotateMaskValue_c* pplaceMacroRotate = ( mode == TRP_PLACE_MACRO_ROTATE_FROM ?
                                                      &this->fromPlaceMacroRotate_ :
                                                      &this->toPlaceMacroRotate_ );
   return( pplaceMacroRotate->IsValid( ) ? 
           true : false );
}

//===========================================================================//
// Method         : PlaceMacroUpdateMoveList_
// Description    : Updates a relative move list to insure the "fromEmpty" 
//                  and "toEmpty" values accurately reflect when a grid point
//                  will become empty after a relative move. This situation 
//                  may occur undetected in "PlaceMacroIsAvailableCoord_()" 
//                  if a move has been made from a relative macro node point
//                  that is not subsequently used by another move (which can
//                  occur due to different random "from" and "to" rotate 
//                  modes). This relative move list update is needed in order
//                  to insure that VPR's 'blocks_affected array' is 
//                  subsequently correctly loaded by "PlaceSwapUpdate_()" 
//                  such that the 'swapped_to_empty' fields properly indicate
//                  when a move leaves a grid point empty. 
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativePlaceHandler_c::PlaceMacroUpdateMoveList_(
      TRP_RelativeMoveList_t* prelativeMoveList ) const
{
   for( size_t i = 0; i < prelativeMoveList->GetLength( ); ++i )
   {
      TRP_RelativeMove_c* prelativeMove = (*prelativeMoveList)[i];
      const TGO_Point_c& fromPoint = prelativeMove->GetFromPoint( );
      const TGO_Point_c& toPoint = prelativeMove->GetToPoint( );

      bool fromEmpty = true;
      bool toEmpty = true;

      for( size_t j = 0; j < prelativeMoveList->GetLength( ); ++j )
      {
         const TRP_RelativeMove_c& relativeMove = *(*prelativeMoveList)[j];
         
         if( relativeMove.GetToPoint( ) == fromPoint )
	 {
            fromEmpty = false;
         }
         if( relativeMove.GetFromPoint( ) == toPoint )
	 {
            toEmpty = false;
         }
      }
      prelativeMove->SetFromEmpty( fromEmpty );
      prelativeMove->SetToEmpty( toEmpty );
   }
}

//===========================================================================//
// Method         : PlaceMacroIsLegalMoveList_
// Description    : Returns TRUE if-and-only-if no two relative moves share
//                  the same "to" point. This situation may exist undetected
//                  during "PlaceMacroIsAvailableCoord_()" if two relative 
//                  macros are near enough to each other and they end of 
//                  sharing a common "to" point.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::PlaceMacroIsLegalMoveList_(
      const TRP_RelativeMoveList_t& fromToMoveList, 
      const TRP_RelativeMoveList_t& toFromMoveList ) const
{
   bool isLegal = true;
   for( size_t i = 0; i < toFromMoveList.GetLength( ); ++i )
   {
      const TRP_RelativeMove_c& toFromMove = *toFromMoveList[i];
      for( size_t j = 0; j < fromToMoveList.GetLength( ); ++j )
      {
         const TRP_RelativeMove_c& fromToMove = *fromToMoveList[j];
         if( toFromMove.GetToPoint( ) == fromToMove.GetToPoint( ))
         {
            // Found at least two relative moves that share same "to" point
            isLegal = false;
         }
         if( !isLegal )
            break;
      }
      if( !isLegal )
         break;
   }
   return( isLegal );
}

//===========================================================================//
// Method         : FindRandomRotateMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TGO_RotateMode_t TRP_RelativePlaceHandler_c::FindRandomRotateMode_(
      void ) const
{
   // Select a random rotate mode (if enabled)
   int rotate = ( this->placeOptions_.rotateEnable ? 
                  TCT_Rand( 1, TGO_ROTATE_MAX - 1 ) : 
                  TGO_ROTATE_R0 );
   return( static_cast< TGO_RotateMode_t >( rotate ));
}

//===========================================================================//
// Method         : FindRandomOriginPoint_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TGO_Point_c TRP_RelativePlaceHandler_c::FindRandomOriginPoint_(
      const TRP_RelativeMacro_c& relativeMacro,
            size_t               relativeNodeIndex ) const
{
   const TRP_RelativeNodeList_t& relativeNodeList = relativeMacro.GetRelativeNodeList( );
   const TRP_RelativeNode_c& relativeNode = *relativeNodeList[relativeNodeIndex];

   return( this->FindRandomOriginPoint_( relativeNode ));
}

//===========================================================================//
TGO_Point_c TRP_RelativePlaceHandler_c::FindRandomOriginPoint_(
      const TRP_RelativeNode_c& relativeNode ) const
{
   const char* pszBlockName = relativeNode.GetBlockName( );

   return( this->FindRandomOriginPoint_( pszBlockName ));
}

//===========================================================================//
TGO_Point_c TRP_RelativePlaceHandler_c::FindRandomOriginPoint_(
      const char* pszBlockName ) const
{
   TGO_Point_c originPoint;

   const TRP_RelativeBlock_c& relativeBlock = *this->relativeBlockList_.Find( pszBlockName );
   const t_type_descriptor* vpr_type = relativeBlock.GetVPR_Type( );
   int typeIndex = ( vpr_type ? vpr_type->index : EMPTY );
   int posIndex = 0;

   if(( typeIndex != EMPTY ) &&
      ( this->vpr_.freeLocationArray[typeIndex] >= 0 ))
   {
      // Attempt to select a random (and available) point from within VPR's grid array
      // (based on VPR's internal 'free_locations' and 'legal_pos' structures)
      for( size_t i = 0; i < TRP_FIND_RANDOM_ORIGIN_POINT_MAX_ATTEMPTS; ++i )
      {
         posIndex = TCT_Rand( 0, this->vpr_.freeLocationArray[typeIndex] - 1 );
         int x = this->vpr_.legalPosArray[typeIndex][posIndex].x;
         int y = this->vpr_.legalPosArray[typeIndex][posIndex].y;
         int z = this->vpr_.legalPosArray[typeIndex][posIndex].z;

         if( this->vpr_.gridArray[x][y].blocks[z] == EMPTY )
         {
            originPoint.Set( x, y, z );
            break;
         }
      }
   }

   if(( typeIndex != EMPTY ) &&
      ( this->vpr_.freeLocationArray[typeIndex] >= 0 ) &&
      ( !originPoint.IsValid( )))
   {
      // Handle case where we failed to find placement location
      // (due to exceeding the max number of random placement attempts)
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      printHandler.Warning( "Failed initial relative macro placement for block \"%s\" after %u random tries.\n",
                            TIO_PSZ_STR( pszBlockName ),
                            TRP_FIND_RANDOM_ORIGIN_POINT_MAX_ATTEMPTS );
      printHandler.Info( "%sAttempting to find available location based on exhaustive search of all free locations.\n",
                         TIO_PREFIX_WARNING_SPACE );

      // Perform an exhaustive search for 1st free location
      for( posIndex = 0; posIndex < this->vpr_.freeLocationArray[typeIndex]; ++posIndex )
      {
         int x = this->vpr_.legalPosArray[typeIndex][posIndex].x;
         int y = this->vpr_.legalPosArray[typeIndex][posIndex].y;
         int z = this->vpr_.legalPosArray[typeIndex][posIndex].z;

         if( this->vpr_.gridArray[x][y].blocks[z] == EMPTY )
         {
            originPoint.Set( x, y, z );
            break;
         }
      }
   }

   if(( typeIndex != EMPTY ) &&
      ( this->vpr_.freeLocationArray[typeIndex] >= 0 ) &&
      ( !originPoint.IsValid( )))
   {
      // Handle case where we still failed to find placement location
      // (even after an exhaustive iteration over all free locations)
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      printHandler.Error( "Initial relative placement failed!\n" );
      printHandler.Info( "%sCould not place block \"%s\", no free locations of type \"%s\".\n",
                         TIO_PREFIX_ERROR_SPACE,
                         TIO_PSZ_STR( pszBlockName ),
                         TIO_PSZ_STR( this->vpr_.typeArray[typeIndex].name ));
   }

   if(( typeIndex != EMPTY ) &&
      ( this->vpr_.freeLocationArray[typeIndex] >= 0 ) &&
      ( originPoint.IsValid( )))
   {
      // Update VPR's 'free_locations' and 'legal_pos' structures to prevent
      // randomly finding same origin point again - IFF rotate not enabled
      // (for further reference, see VPR's initial_place_blocks() function)
      if( !this->placeOptions_.rotateEnable )
      {
         int lastIndex = this->vpr_.freeLocationArray[typeIndex] - 1;
	 if( lastIndex >= 0 )
	 {
            this->vpr_.legalPosArray[typeIndex][posIndex] = this->vpr_.legalPosArray[typeIndex][lastIndex];
	 }
         --this->vpr_.freeLocationArray[typeIndex];
      }
   }
   return( originPoint );
}

//===========================================================================//
// Method         : FindRandomRelativeNodeIndex_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
size_t TRP_RelativePlaceHandler_c::FindRandomRelativeNodeIndex_(
      const TRP_RelativeMacro_c& relativeMacro ) const
{
   size_t relativeNodeIndex = TCT_Rand( static_cast<size_t>( 0 ), 
                                        relativeMacro.GetLength( ) - 1 );
   return( relativeNodeIndex );
}

//===========================================================================//
// Method         : FindRelativeMacroIndex_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
size_t TRP_RelativePlaceHandler_c::FindRelativeMacroIndex_(
      const TGO_Point_c& point ) const
{
   size_t relativeMacroIndex = TRP_RELATIVE_MACRO_UNDEFINED;

   if( this->IsWithinGrid_( point ))
   {
      // Find relative block based on VPR's grid array point
      const TRP_RelativeBlock_c* prelativeBlock = this->FindRelativeBlock_( point );

      // Find relative macro index, if any, based on relative block
      if( prelativeBlock )
      {
         relativeMacroIndex = prelativeBlock->GetRelativeMacroIndex( );
      }
   }
   return( relativeMacroIndex );
}

//===========================================================================//
// Method         : FindRelativeNodeIndex_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
size_t TRP_RelativePlaceHandler_c::FindRelativeNodeIndex_(
      const TGO_Point_c& point ) const
{
   size_t relativeNodeIndex = TRP_RELATIVE_NODE_UNDEFINED;

   if( this->IsWithinGrid_( point ))
   {
      // Find relative block based on VPR's grid array point
      const TRP_RelativeBlock_c* prelativeBlock = this->FindRelativeBlock_( point );

      // Find relative node index, if any, based on relative block
      if( prelativeBlock )
      {
         relativeNodeIndex = prelativeBlock->GetRelativeNodeIndex( );
      }
   }
   return( relativeNodeIndex );
}

//===========================================================================//
// Method         : FindRelativeMacro_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TRP_RelativeMacro_c* TRP_RelativePlaceHandler_c::FindRelativeMacro_(
      const TGO_Point_c& point ) const
{
   TRP_RelativeMacro_c* prelativeMacro = 0;

   if( this->IsWithinGrid_( point ))
   {
      // Find relative macro index based on VPR's grid array point
      size_t relativeMacroIndex = this->FindRelativeMacroIndex_( point );

      // Find relative macro, if any, based on relative macro index
      prelativeMacro = this->relativeMacroList_[relativeMacroIndex];
   }
   return( prelativeMacro );
}

//===========================================================================//
// Method         : FindRelativeNode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TRP_RelativeNode_c* TRP_RelativePlaceHandler_c::FindRelativeNode_(
      const TGO_Point_c& point ) const
{
   TRP_RelativeNode_c* prelativeNode = 0;

   if( this->IsWithinGrid_( point ))
   {
      // Find relative macro and node indices based on VPR's grid array point
      size_t relativeMacroIndex = this->FindRelativeMacroIndex_( point );
      size_t relativeNodeIndex = this->FindRelativeNodeIndex_( point );

      // Find relative macro, if any, based on relative macro index
      const TRP_RelativeMacro_c* prelativeMacro = 0;
      prelativeMacro = this->relativeMacroList_[relativeMacroIndex];
      if( prelativeMacro )
      {
         prelativeNode = (*prelativeMacro)[relativeNodeIndex];
      }
   }
   return( prelativeNode );
}

//===========================================================================//
// Method         : FindRelativeBlock_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
const TRP_RelativeBlock_c* TRP_RelativePlaceHandler_c::FindRelativeBlock_(
      const TGO_Point_c& point ) const
{
   const TRP_RelativeBlock_c* prelativeBlock = 0;

   if( this->IsWithinGrid_( point ))
   {
      // Look up VPR block name based on VPR's grid array
      const char* pszBlockName = this->FindGridPointBlockName_( point );

      // Find relative block, if any, based on VPR's block name
      prelativeBlock = this->relativeBlockList_.Find( pszBlockName );
   }
   return( prelativeBlock );
}

//===========================================================================//
// Method         : FindGridPointBlockName_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
const char* TRP_RelativePlaceHandler_c::FindGridPointBlockName_(
      const TGO_Point_c& point ) const
{
   const char* pszBlockName = 0;

   int blockIndex = this->FindGridPointBlockIndex_( point );
   if( blockIndex != EMPTY )
   {
      pszBlockName = this->vpr_.blockArray[blockIndex].name;
   }
   return( pszBlockName );
}

//===========================================================================//
// Method         : FindGridPointBlockIndex_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
int TRP_RelativePlaceHandler_c::FindGridPointBlockIndex_(
      const TGO_Point_c& point ) const
{
   int blockIndex = EMPTY;

   if( this->IsWithinGrid_( point ))
   {
      int x = point.x;
      int y = point.y;
      int z = point.z;
      blockIndex = this->vpr_.gridArray[x][y].blocks[z];
   }
   return( blockIndex );
}

//===========================================================================//
// Method         : FindGridPointType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
const t_type_descriptor* TRP_RelativePlaceHandler_c::FindGridPointType_(
      const TGO_Point_c& point ) const
{
   const t_type_descriptor* vpr_type = 0;

   if( this->IsWithinGrid_( point ))
   {
      int x = point.x;
      int y = point.y;
      vpr_type = this->vpr_.gridArray[x][y].type;
   }
   return( vpr_type );
}

//===========================================================================//
// Method         : IsEmptyGridPoint_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::IsEmptyGridPoint_(
      const TGO_Point_c& point ) const
{
   bool isEmpty = false;

   if( this->IsWithinGrid_( point ))
   {
      int x = point.x;
      int y = point.y;
      int z = point.z;
      isEmpty = ( this->vpr_.gridArray[x][y].blocks[z] == EMPTY ?
                  true : false );
   }
   return( isEmpty );
}

//===========================================================================//
// Method         : IsWithinGrid_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::IsWithinGrid_(
      const TGO_Point_c& point ) const
{
   bool isWithin = ( point.x >= 0 && point.x <= this->vpr_.nx &&
                     point.y >= 0 && point.y <= this->vpr_.ny ?
                     true : false );
   return( isWithin );
}

//===========================================================================//
// Method         : DecideAntiSide_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TC_SideMode_t TRP_RelativePlaceHandler_c::DecideAntiSide_(
      TC_SideMode_t side ) const
{
   TC_SideMode_t antiSide = TC_SIDE_UNDEFINED;

   switch( side )
   {
   case TC_SIDE_LEFT:  antiSide = TC_SIDE_RIGHT; break;
   case TC_SIDE_RIGHT: antiSide = TC_SIDE_LEFT;  break;
   case TC_SIDE_LOWER: antiSide = TC_SIDE_UPPER; break;
   case TC_SIDE_UPPER: antiSide = TC_SIDE_LOWER; break;
   default:                                      break;
   }
   return( antiSide );
}

//===========================================================================//
// Method         : ShowMissingBlockNameError_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::ShowMissingBlockNameError_(
      const char* pszBlockName ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   bool ok = printHandler.Error( "Invalid relative placement constraint!\n" );
   printHandler.Info( "%sBlock \"%s\" was not found in VPR's block list.\n",
                      TIO_PREFIX_ERROR_SPACE,
                      TIO_PSZ_STR( pszBlockName ));
   return( ok );
}

//===========================================================================//
// Method         : ShowInvalidConstraintError_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativePlaceHandler_c::ShowInvalidConstraintError_(
      const TRP_RelativeBlock_c& fromBlock,
      const TRP_RelativeBlock_c& toBlock,
            TC_SideMode_t        side,
      const string&              srExistingFromBlockName,
      const string&              srExistingToBlockName ) const
{
   const char* pszFromBlockName = fromBlock.GetName( );
   const char* pszToBlockName = toBlock.GetName( );

   string srSide;
   TC_ExtractStringSideMode( side, &srSide );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   bool ok = printHandler.Error( "Invalid relative placement constraint!\n" );
   if( srExistingFromBlockName.length( ) &&
       srExistingToBlockName.length( ))
   {
      printHandler.Info( "%sConstraint already exists between \"%s\" and \"%s\".\n",
                         TIO_PREFIX_ERROR_SPACE,
                         TIO_SR_STR( srExistingFromBlockName ),
                         TIO_SR_STR( srExistingToBlockName ));
   }
   printHandler.Info( "%sIgnoring constraint from \"%s\" to \"%s\" on side '%s'.\n",
                      TIO_PREFIX_ERROR_SPACE,
                      TIO_PSZ_STR( pszFromBlockName ),
                      TIO_PSZ_STR( pszToBlockName ),
                      TIO_SR_STR( srSide ));
   return( ok );
}
