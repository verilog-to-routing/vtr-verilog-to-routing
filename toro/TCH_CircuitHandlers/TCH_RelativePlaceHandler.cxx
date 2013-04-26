//===========================================================================//
// Purpose : Method definitions for the TCH_RelativePlaceHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - Configure
//           - Set, Reset
//           - InitialPlace
//           - Place
//           - IsCandidate
//           - IsValid
//
//           Protected methods include:
//           - TCH_RelativePlaceHandler_c, ~TCH_RelativePlaceHandler_c
//
//           Private methods include:
//           - AddSideConstraint_
//           - NewSideConstraint_
//           - MergeSideConstraints_
//           - ExistingSideConstraint_
//           - HasExistingSideConstraint_
//           - IsAvailableSideConstraint_
//           - ResetRelativeMacroList_
//           - ResetRelativeBlockList_
//           - InitialPlaceMacros_
//           - InitialPlaceMacroNodes_
//           - InitialPlaceMacroUpdate_
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
//           - DecideAntiSide_
//           - ShowIllegalRelativeMacroWarning_
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
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TGO_StringUtils.h"

#include "TCH_Typedefs.h"
#include "TCH_RelativeMove.h"
#include "TCH_RelativePlaceHandler.h"

// Initialize the relative place handler "singleton" class, as needed
TCH_RelativePlaceHandler_c* TCH_RelativePlaceHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TCH_RelativePlaceHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativePlaceHandler_c::TCH_RelativePlaceHandler_c( 
      void )
      :
      relativeMacroList_( TCH_RELATIVE_MACRO_LIST_DEF_CAPACITY ),
      relativeBlockList_( TCH_RELATIVE_BLOCK_LIST_DEF_CAPACITY )
{
   this->placeOptions_.rotateEnable = false;
   this->placeOptions_.maxPlaceRetryCt = 0;
   this->placeOptions_.maxMacroRetryCt = 0;

   pinstance_ = 0;
}

//===========================================================================//
// Method         : ~TCH_RelativePlaceHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativePlaceHandler_c::~TCH_RelativePlaceHandler_c( 
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
void TCH_RelativePlaceHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TCH_RelativePlaceHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativePlaceHandler_c::DeleteInstance( 
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
TCH_RelativePlaceHandler_c& TCH_RelativePlaceHandler_c::GetInstance(
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
bool TCH_RelativePlaceHandler_c::HasInstance(
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
bool TCH_RelativePlaceHandler_c::Configure(
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

   // Followed up clearing any existing relative block and macro list data
   this->relativeMacroList_.Clear( );
   this->relativeBlockList_.Clear( );

   // Continue by setting relative block and macro list estimated capacities
   int toro_circuitBlockCount = static_cast< int >( toro_circuitBlockList.GetLength( ));
   this->relativeMacroList_.SetCapacity( toro_circuitBlockCount );
   this->relativeBlockList_.SetCapacity( toro_circuitBlockCount );

   // Initialize the local relative block list based on Toro's block list
   for( size_t i = 0; i < toro_circuitBlockList.GetLength( ); ++i )
   {
      const TPO_Inst_c& toro_circuitBlock = *toro_circuitBlockList[i];
      const char* pszBlockName = toro_circuitBlock.GetName( );

      TCH_RelativeBlock_c relativeBlock( pszBlockName );
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
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativePlaceHandler_c::Set(
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
   // Set local reference to VPR's grid array
   this->vpr_data_.Init( vpr_gridArray, vpr_nx, vpr_ny, 
                         vpr_blockArray, vpr_blockCount,
                         vpr_typeArray, vpr_typeCount,
                         vpr_freeLocationArray, vpr_legalPosArray );
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
bool TCH_RelativePlaceHandler_c::Reset(
      void )
{
   bool ok = true;

   // Reset local VPR's data placement information
   this->vpr_data_.Reset( );

   // Reset placement and type information for relative macro and block lists
   ok = this->ResetRelativeBlockList_( this->vpr_data_.vpr_blockArray,
                                       this->vpr_data_.vpr_blockCount,
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
bool TCH_RelativePlaceHandler_c::InitialPlace(
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

   // Initialize local VPR grid array and block list structures
   this->Set( vpr_gridArray, vpr_nx, vpr_ny,
              vpr_blockArray, vpr_blockCount,
              vpr_typeArray, vpr_typeCount,
              vpr_freeLocationArray, vpr_legalPosArray );

   // At least one relative placement constraint has been defined
   bool placedAllMacros = true;

   // Define how many times we can retry when searching for initial placement
   size_t maxPlaceCt = this->placeOptions_.maxPlaceRetryCt;
   size_t tryPlaceCt = 0;

   while( tryPlaceCt < maxPlaceCt )
   {
      // Reset VPR grid array, and relative macro and block lists, then try place
      ok = this->Reset( );
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

   if( ok && !placedAllMacros ) // Failed to comply - the Force is weak here
   {
      ok = this->Reset( );

      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Error( "Failed to find initial placement based on relative placement constraints.\n" );
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
bool TCH_RelativePlaceHandler_c::Place( 
      int x1, int y1, int z1,
      int x2, int y2, int z2,
      t_pl_blocks_to_be_moved& vpr_blocksAffected )
{
   TGO_Point_c fromPoint( x1, y1, z1 );
   TGO_Point_c toPoint( x2, y2, z2 );

   return( this->Place( fromPoint, toPoint, vpr_blocksAffected ));
}

//===========================================================================//
bool TCH_RelativePlaceHandler_c::Place( 
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
      TCH_RelativeMoveList_t relativeMoveList;
      ok = this->PlaceSwapMacros_( fromPoint, toPoint,
                                   &relativeMoveList );
      if( ok )
      {
         // Load VPR's "blocks_affected" array based on relative move list
         for( size_t i = 0; i < relativeMoveList.GetLength( ); ++i )
         {
            const TCH_RelativeMove_c& relativeMove = *relativeMoveList[i];
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
bool TCH_RelativePlaceHandler_c::IsCandidate( 
      int x1, int y1, int z1,
      int x2, int y2, int z2 ) const
{
   TGO_Point_c fromPoint( x1, y1, z1 );
   TGO_Point_c toPoint( x2, y2, z2 );

   return( this->IsCandidate( fromPoint, toPoint ));
}

//===========================================================================//
bool TCH_RelativePlaceHandler_c::IsCandidate( 
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
bool TCH_RelativePlaceHandler_c::IsValid(
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
bool TCH_RelativePlaceHandler_c::AddSideConstraint_(
      const char*         pszFromBlockName,
      const char*         pszToBlockName,
            TC_SideMode_t side,
            size_t        relativeMacroIndex )
{
   bool ok = true;

   // Look up "from" and "to" relative macro indices based on given block names
   TCH_RelativeBlock_c* pfromBlock = this->relativeBlockList_.Find( pszFromBlockName );
   TCH_RelativeBlock_c* ptoBlock = this->relativeBlockList_.Find( pszToBlockName );

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
bool TCH_RelativePlaceHandler_c::AddSideConstraint_(
      TCH_RelativeBlock_c* pfromBlock,
      TCH_RelativeBlock_c* ptoBlock,
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
void TCH_RelativePlaceHandler_c::NewSideConstraint_(
      TCH_RelativeBlock_c* pfromBlock,
      TCH_RelativeBlock_c* ptoBlock,
      TC_SideMode_t        side,
      size_t               relativeMacroIndex )
{
   // First, add new relative macro (or use existing, per optional parameter)
   if( relativeMacroIndex == TCH_RELATIVE_MACRO_UNDEFINED )
   {
      relativeMacroIndex = this->relativeMacroList_.GetLength( );

      TCH_RelativeMacro_c relativeMacro;
      this->relativeMacroList_.Add( relativeMacro );
   }
   TCH_RelativeMacro_c* prelativeMacro = this->relativeMacroList_[relativeMacroIndex];

   // Next, add new relative nodes based on given block names and side
   const char* pszFromBlockName = pfromBlock->GetName( );
   const char* pszToBlockName = ptoBlock->GetName( );
   size_t fromNodeIndex = TCH_RELATIVE_NODE_UNDEFINED;
   size_t toNodeIndex = TCH_RELATIVE_NODE_UNDEFINED;
   prelativeMacro->Add( pszFromBlockName, pszToBlockName, side,
                        &fromNodeIndex, &toNodeIndex );

   // Finally, update relative block macro and node indices
   pfromBlock->SetRelativeMacroIndex( relativeMacroIndex );
   pfromBlock->SetRelativeNodeIndex( fromNodeIndex );

   ptoBlock->SetRelativeMacroIndex( relativeMacroIndex );
   ptoBlock->SetRelativeNodeIndex( toNodeIndex );
}

//===========================================================================//
void TCH_RelativePlaceHandler_c::NewSideConstraint_(
      const TCH_RelativeBlock_c& fromBlock,
      const TCH_RelativeBlock_c& toBlock,
            TC_SideMode_t        side )
{
   size_t relativeMacroIndex = fromBlock.GetRelativeMacroIndex( );
   size_t fromNodeIndex = fromBlock.GetRelativeNodeIndex( );
   size_t toNodeIndex = toBlock.GetRelativeNodeIndex( );

   // First, find existing relative macro
   TCH_RelativeMacro_c* prelativeMacro = this->relativeMacroList_[relativeMacroIndex];

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
bool TCH_RelativePlaceHandler_c::MergeSideConstraints_(
      TCH_RelativeBlock_c* pfromBlock,
      TCH_RelativeBlock_c* ptoBlock,
      TC_SideMode_t        side )
{
   bool ok = true;

   size_t fromMacroIndex = pfromBlock->GetRelativeMacroIndex( );
   size_t toMacroIndex = ptoBlock->GetRelativeMacroIndex( );

   TCH_RelativeMacro_c& toMacro = *this->relativeMacroList_[toMacroIndex];

   // Start by clearing all block indices asso. with existing "to" relative macro 
   for( size_t i = 0; i < toMacro.GetLength( ); ++i )
   {
      const TCH_RelativeNode_c& toNode = *toMacro[i];

      const char* pszToBlockName = toNode.GetBlockName( );
      TCH_RelativeBlock_c* ptoBlock_ = this->relativeBlockList_.Find( pszToBlockName );
      ptoBlock_->Reset( );
   }

   // Then, iterate the "to" relative macro...
   // Recursively add side constraints to existing "from" relative macro
   for( size_t i = 0; i < toMacro.GetLength( ); ++i )
   {
      const TCH_RelativeNode_c& toNode = *toMacro[i];

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
bool TCH_RelativePlaceHandler_c::MergeSideConstraints_(
            size_t               fromMacroIndex,
      const TCH_RelativeMacro_c& toMacro,
      const TCH_RelativeNode_c&  toNode,
            TC_SideMode_t        side )
{
   bool ok = true;

   if( toNode.HasSideIndex( side ))
   {
      const char* pszToBlockName = toNode.GetBlockName( );

      size_t neighborNodeIndex = toNode.GetSideIndex( side );
      const TCH_RelativeNode_c& neighborNode = *toMacro[neighborNodeIndex];
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
bool TCH_RelativePlaceHandler_c::ExistingSideConstraint_(
      const TCH_RelativeBlock_c& fromBlock,
            TCH_RelativeBlock_c* ptoBlock,
            TC_SideMode_t        side,
            string*              psrFromBlockName,
            string*              psrToBlockName )
{
   bool ok = true;

   // Find existing relative macro and node based on "from" block's indices
   size_t relativeMacroIndex = fromBlock.GetRelativeMacroIndex( );
   size_t fromNodeIndex = fromBlock.GetRelativeNodeIndex( );

   TCH_RelativeMacro_c* prelativeMacro = this->relativeMacroList_[relativeMacroIndex];
   const TCH_RelativeNode_c& fromNode = *prelativeMacro->Find( fromNodeIndex );

   if( !fromNode.HasSideIndex( side ))
   {
      // Side is available on existing macro "from" node, now add a "to" node

      // Add new "to" relative node
      const char* pszToBlockName = ptoBlock->GetName( );
      size_t toNodeIndex = TCH_RELATIVE_NODE_UNDEFINED;
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
      const TCH_RelativeNode_c* ptoNode = prelativeMacro->Find( toNodeIndex );
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
bool TCH_RelativePlaceHandler_c::HasExistingSideConstraint_(
      const TCH_RelativeBlock_c& fromBlock,
      const TCH_RelativeBlock_c& toBlock,
            TC_SideMode_t        side,
            string*              psrFromBlockName,
            string*              psrToBlockName ) const
{
   bool isExisting = false;

   size_t fromMacroIndex = fromBlock.GetRelativeMacroIndex( );
   size_t fromNodeIndex = fromBlock.GetRelativeNodeIndex( );

   size_t toMacroIndex = toBlock.GetRelativeMacroIndex( );
   size_t toNodeIndex = toBlock.GetRelativeNodeIndex( );

   if(( fromMacroIndex != TCH_RELATIVE_MACRO_UNDEFINED ) &&
      ( toMacroIndex != TCH_RELATIVE_MACRO_UNDEFINED ))
   {
      if(( fromMacroIndex == toMacroIndex ) &&
         ( fromNodeIndex != toNodeIndex ))
      {
         const TCH_RelativeMacro_c& relativeMacro = *this->relativeMacroList_[fromMacroIndex];
         const TCH_RelativeNode_c& fromNode = *relativeMacro[fromNodeIndex];
         const TCH_RelativeNode_c& toNode = *relativeMacro[toNodeIndex];

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
            const TCH_RelativeNode_c* pfromNode = relativeMacro[fromNodeIndex];
            *psrFromBlockName = ( pfromNode ? pfromNode->GetBlockName( ) : "" );
         }
         if( psrToBlockName )
         {
            toNodeIndex = ( fromNode.HasSideIndex( antiSide ) ? 
                            fromNode.GetSideIndex( antiSide ) : toNodeIndex );
            const TCH_RelativeNode_c* ptoNode = relativeMacro[toNodeIndex];
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
bool TCH_RelativePlaceHandler_c::IsAvailableSideConstraint_(
      const TCH_RelativeBlock_c& fromBlock,
      const TCH_RelativeBlock_c& toBlock,
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

   if(( fromMacroIndex != TCH_RELATIVE_MACRO_UNDEFINED ) &&
      ( toMacroIndex != TCH_RELATIVE_MACRO_UNDEFINED ))
   {
      if( fromMacroIndex != toMacroIndex )
      {
         const TCH_RelativeMacro_c& fromMacro = *this->relativeMacroList_[fromMacroIndex];
         const TCH_RelativeMacro_c& toMacro = *this->relativeMacroList_[toMacroIndex];

         const TCH_RelativeNode_c& fromNode = *fromMacro[fromNodeIndex];
         const TCH_RelativeNode_c& toNode = *toMacro[toNodeIndex];

         TC_SideMode_t antiSide = this->DecideAntiSide_( side );
         if(( fromNode.GetSideIndex( side ) == TCH_RELATIVE_NODE_UNDEFINED ) &&
            ( toNode.GetSideIndex( antiSide ) == TCH_RELATIVE_NODE_UNDEFINED ))
         {
            isAvailable = true;
         }

         if( pfromAvailable )
         {
            *pfromAvailable = ( fromNode.GetSideIndex( side ) == TCH_RELATIVE_NODE_UNDEFINED ? 
                                true : false );
         }
         if( ptoAvailable )
         {
            *ptoAvailable = ( toNode.GetSideIndex( side ) == TCH_RELATIVE_NODE_UNDEFINED ? 
                              true : false );
         }

         if( psrFromBlockName )
         {
            fromNodeIndex = toNode.GetSideIndex( antiSide );
            const TCH_RelativeNode_c* pfromNode = fromMacro.Find( fromNodeIndex );
            *psrFromBlockName = ( pfromNode ? pfromNode->GetBlockName( ) : "" );
         }
         if( psrToBlockName )
         {
            toNodeIndex = fromNode.GetSideIndex( side );
            const TCH_RelativeNode_c* ptoNode = toMacro.Find( toNodeIndex );
            *psrToBlockName = ( ptoNode ? ptoNode->GetBlockName( ) : "" );
         }
      }
   }
   return( isAvailable );
}

//===========================================================================//
// Method         : ResetRelativeMacroList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativePlaceHandler_c::ResetRelativeMacroList_(
      const TCH_RelativeBlockList_t& relativeBlockList,
            TCH_RelativeMacroList_t* prelativeMacroList )
{
   // Reset all macro node 'vpr_type' fields based on relative block list
   for( size_t i = 0; i < prelativeMacroList->GetLength( ); ++i )
   {
      TCH_RelativeMacro_c* prelativeMacro = (*prelativeMacroList)[i];
      for( size_t j = 0; j < prelativeMacro->GetLength( ); ++j )
      {
         TCH_RelativeNode_c* prelativeNode = (*prelativeMacro)[j];

         TGO_Point_c vpr_gridPoint;
         prelativeNode->SetVPR_GridPoint( vpr_gridPoint );

         const char* pszBlockName = prelativeNode->GetBlockName( );
         const TCH_RelativeBlock_c* prelativeBlock = relativeBlockList.Find( pszBlockName );
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
bool TCH_RelativePlaceHandler_c::ResetRelativeBlockList_(
      t_block*                 vpr_blockArray,
      int                      vpr_blockCount,
      TCH_RelativeBlockList_t* prelativeBlockList )
{
   bool ok = true;

   // Initialize relative block list based on given VPR block array types
   for( int blockIndex = 0; blockIndex < vpr_blockCount; ++blockIndex )
   {
      const char* pszBlockName = vpr_blockArray[blockIndex].name;
      TCH_RelativeBlock_c* prelativeBlock = prelativeBlockList->Find( pszBlockName );
      if( !prelativeBlock )
         continue;

      int vpr_index = blockIndex;
      prelativeBlock->SetVPR_Index( vpr_index );

      const t_type_descriptor* vpr_type = vpr_blockArray[blockIndex].type;
      prelativeBlock->SetVPR_Type( vpr_type );
   }

   // Validate relative block list based on local VPR block name list
   TCH_RelativeBlockList_t vpr_blockList( vpr_blockCount );
   for( int blockIndex = 0; blockIndex < vpr_blockCount; ++blockIndex )
   {
      const char* pszBlockName = vpr_blockArray[blockIndex].name;
      vpr_blockList.Add( pszBlockName );
   }

   for( size_t i = 0; i < prelativeBlockList->GetLength( ); ++i )
   {
      const TCH_RelativeBlock_c& relativeBlock = *(*prelativeBlockList)[i];
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
bool TCH_RelativePlaceHandler_c::InitialPlaceMacros_(
      bool* pplacedAllMacros )
{
   bool ok = true;

   bool foundAllPlacements = true;

   for( size_t i = 0; i < this->relativeMacroList_.GetLength( ); ++i )
   {
      TCH_RelativeMacro_c* prelativeMacro = this->relativeMacroList_[i];

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
      ok = printHandler.Warning( "Failed initial relative placement based on relative placement constraints.\n" );
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
bool TCH_RelativePlaceHandler_c::InitialPlaceMacroNodes_(
      TCH_RelativeMacro_c* prelativeMacro,
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
         this->InitialPlaceMacroUpdate_( *prelativeMacro,
                                         this->relativeBlockList_ );
         *pfoundPlacement = true;
      }
      else
      {
         ok = this->ShowIllegalRelativeMacroWarning_( *prelativeMacro,
                                                      relativeNodeIndex, 
                                                      origin, rotate );
      }
   }
   else if( origin.IsValid( ))
   {
      *pvalidPlacement = true;
   }
   return( ok );
}

//===========================================================================//
// Method         : InitialPlaceMacroUpdate_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativePlaceHandler_c::InitialPlaceMacroUpdate_(
      const TCH_RelativeMacro_c&     relativeMacro,
      const TCH_RelativeBlockList_t& relativeBlockList )
{
   for( size_t i = 0; i < relativeMacro.GetLength( ); ++i )
   {
      const TCH_RelativeNode_c& relativeNode = *relativeMacro[i];
      const TGO_Point_c& vpr_gridPoint = relativeNode.GetVPR_GridPoint( );

      const char* pszBlockName = relativeNode.GetBlockName( );
      const TCH_RelativeBlock_c& relativeBlock = *relativeBlockList.Find( pszBlockName );
      int vpr_blockIndex = relativeBlock.GetVPR_Index( );

      this->vpr_data_.Set( vpr_gridPoint, vpr_blockIndex );
   }
}

//===========================================================================//
// Method         : InitialPlaceMacroResetCoords_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativePlaceHandler_c::InitialPlaceMacroResetCoords_(
      void )
{
   TCH_RotateMaskHash_t* protateMaskHash = &this->initialPlaceMacroCoords_;

   protateMaskHash->Clear( );
}

//===========================================================================//
// Method         : InitialPlaceMacroIsLegal_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_RelativePlaceHandler_c::InitialPlaceMacroIsLegal_(
            size_t           relativeNodeIndex,
      const TGO_Point_c&     origin,
            TGO_RotateMode_t rotate ) const
{
   bool isLegal = true;

   TCH_RotateMaskHash_t* protateMaskHash = const_cast< TCH_RotateMaskHash_t* >( &this->initialPlaceMacroCoords_ );

   TCH_RotateMaskKey_c rotateMaskKey( origin, relativeNodeIndex );
   TCH_RotateMaskValue_c* protateMaskValue;
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
         TCH_RotateMaskValue_c rotateMaskValue( this->placeOptions_.rotateEnable );

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
bool TCH_RelativePlaceHandler_c::InitialPlaceMacroIsOpen_(
      const TCH_RelativeMacro_c& relativeMacro,
            size_t               relativeNodeIndex,
      const TGO_Point_c&         origin,
            TGO_RotateMode_t     rotate ) const
{
   bool isOpen = false;

   TCH_RelativeMacro_c relativeMacro_( relativeMacro );
   relativeMacro_.Set( relativeNodeIndex, origin, rotate );

   for( size_t i = 0; i < relativeMacro_.GetLength( ); ++i )
   {
      const TCH_RelativeNode_c& relativeNode = *relativeMacro_[i];

      // Query VPR's grid array to test (1) open && (2) matching type ptr
      const TGO_Point_c& gridPoint = relativeNode.GetVPR_GridPoint( );
      const t_type_descriptor* vpr_type = relativeNode.GetVPR_Type( );
      if( this->vpr_data_.IsOpenGridPoint( gridPoint, vpr_type ))
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
bool TCH_RelativePlaceHandler_c::InitialPlaceMacroIsOpen_(
            size_t           relativeMacroIndex,
            size_t           relativeNodeIndex,
      const TGO_Point_c&     origin,
            TGO_RotateMode_t rotate ) const
{
   bool isOpen = false;

   const TCH_RelativeMacro_c* prelativeMacro = this->relativeMacroList_[relativeMacroIndex];
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
void TCH_RelativePlaceHandler_c::PlaceResetMacros_(
      void )
{
   for( size_t i = 0; i < this->relativeMacroList_.GetLength( ); ++i )
   {
      TCH_RelativeMacro_c* prelativeMacro = this->relativeMacroList_[i];
      for( size_t j = 0; j < prelativeMacro->GetLength( ); ++j )
      {
         TCH_RelativeNode_c* prelativeNode = (*prelativeMacro)[j];

         const char* pszBlockName = prelativeNode->GetBlockName( );
         const TCH_RelativeBlock_c& relativeBlock = *this->relativeBlockList_.Find( pszBlockName );

         const t_block* vpr_blockArray = this->vpr_data_.vpr_blockArray;
         int vpr_index = relativeBlock.GetVPR_Index( );
         TGO_Point_c vpr_gridPoint( vpr_blockArray[vpr_index].x,
                                    vpr_blockArray[vpr_index].y,
                                    vpr_blockArray[vpr_index].z );

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
bool TCH_RelativePlaceHandler_c::PlaceSwapMacros_(
      const TGO_Point_c&            fromPoint,
      const TGO_Point_c&            toPoint,
            TCH_RelativeMoveList_t* prelativeMoveList )
{
   TCH_RelativeMoveList_t fromToMoveList;
   TCH_RelativeMoveList_t toFromMoveList;

   bool foundPlaceSwap = false;

   // Iterate attempt to swap "from" => "to" based available (optional) rotation
   this->PlaceMacroResetRotate_( TCH_PLACE_MACRO_ROTATE_TO );
   while( this->PlaceMacroIsValidRotate_( TCH_PLACE_MACRO_ROTATE_TO ))
   {
      // Select a random rotate mode (if enabled)
      TGO_RotateMode_t toRotate = this->FindRandomRotateMode_( );

      if( this->PlaceMacroIsLegalRotate_( TCH_PLACE_MACRO_ROTATE_TO, toRotate ))
      {
         fromToMoveList.Clear( );

         // Query if "to" is available based on "from" origin and "to" transform
         // (and return list of "from->to" moves corresponding to availability)
         if( this->PlaceMacroIsAvailableCoord_( fromPoint, toPoint, toRotate,
                                                &fromToMoveList ))
         {
            // Iterate attempt to swap "to" => "from" based available rotation
            this->PlaceMacroResetRotate_( TCH_PLACE_MACRO_ROTATE_FROM );
            while( this->PlaceMacroIsValidRotate_( TCH_PLACE_MACRO_ROTATE_FROM ))
            {
               // Select a random rotate mode (if enabled)
               TGO_RotateMode_t fromRotate = this->FindRandomRotateMode_( );
               if( this->PlaceMacroIsLegalRotate_( TCH_PLACE_MACRO_ROTATE_FROM, fromRotate ))
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
void TCH_RelativePlaceHandler_c::PlaceSwapUpdate_( 
      const TCH_RelativeMove_c&      relativeMove,
            t_pl_blocks_to_be_moved& vpr_blocksAffected ) const
{
   const TGO_Point_c& fromPoint = relativeMove.GetFromPoint( );
   const TGO_Point_c& toPoint = relativeMove.GetToPoint( );

   // Update relative node grid points per from/to points, as needed
   TCH_RelativeNode_c* pfromNode = this->FindRelativeNode_( fromPoint );
   if( pfromNode )
   {
      pfromNode->SetVPR_GridPoint( toPoint );
   }
   TCH_RelativeNode_c* ptoNode = this->FindRelativeNode_( toPoint );
   if( ptoNode )
   {
      ptoNode->SetVPR_GridPoint( fromPoint );
   }

   // Find VPR's global "block" array indices per from/to points
   int fromBlockIndex = this->vpr_data_.FindGridPointBlockIndex( fromPoint );
   bool fromEmpty = relativeMove.GetFromEmpty( );
   bool toEmpty = relativeMove.GetToEmpty( );

   // Update VPR's global "block" array using locally cached reference
   t_block* vpr_blockArray = this->vpr_data_.vpr_blockArray;
   vpr_blockArray[fromBlockIndex].x = toPoint.x;
   vpr_blockArray[fromBlockIndex].y = toPoint.y;
   vpr_blockArray[fromBlockIndex].z = toPoint.z;

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
bool TCH_RelativePlaceHandler_c::PlaceMacroIsRelativeCoord_(
      const TGO_Point_c& point ) const
{
   bool isMacro = false;

   // Find relative block based on VPR's grid array point
   const TCH_RelativeBlock_c* prelativeBlock = this->FindRelativeBlock_( point );

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
bool TCH_RelativePlaceHandler_c::PlaceMacroIsAvailableCoord_(
      const TGO_Point_c&            fromOrigin,
      const TGO_Point_c&            toOrigin,
            TGO_RotateMode_t        toRotate,
            TCH_RelativeMoveList_t* prelativeMoveList ) const
{
   bool isAvailable = true;

   size_t fromMacroIndex = this->FindRelativeMacroIndex_( fromOrigin );
   size_t toMacroIndex = this->FindRelativeMacroIndex_( toOrigin );

   TCH_RelativeMacro_c fromMacro_;
   const TCH_RelativeMacro_c* pfromMacro = this->FindRelativeMacro_( fromOrigin );
   if( !pfromMacro )
   {
      fromMacro_.Add( );
      fromMacro_.Set( 0, fromOrigin ); 
      pfromMacro = &fromMacro_;
   }

   TCH_RelativeMoveList_t relativeMoveList( 2 * pfromMacro->GetLength( ));

   for( size_t i = 0; i < pfromMacro->GetLength( ); ++i )
   {
      const TCH_RelativeNode_c& fromNode = *(*pfromMacro)[i];
      TGO_Point_c fromPoint = fromNode.GetVPR_GridPoint( );

      // Apply transform to relative node's grid point (based on origin point)
      TGO_Transform_c toTransform( toOrigin, toRotate, fromOrigin, fromPoint );
      TGO_Point_c toPoint;
      toTransform.Apply( toOrigin, &toPoint );

      // Validate that transformed point is still valid wrt. VPR's grid array
      if( !this->vpr_data_.IsWithinGridArray( toPoint ))
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

      const t_type_descriptor* vpr_fromType = this->vpr_data_.FindGridPointType( fromPoint );
      const t_type_descriptor* vpr_toType = this->vpr_data_.FindGridPointType( toPoint );
      if( vpr_fromType != vpr_toType )
      {
         // VPR's grid types do not match => location is *not* available
         isAvailable = false;
         break;
      }

      if( this->vpr_data_.IsEmptyGridPoint( fromPoint ))
      {
         // VPR's grid location is empty => really not much we can do today
         continue;
      }

      if( this->vpr_data_.IsEmptyGridPoint( toPoint ))
      {
         // VPR's grid location is empty => it is available

         // Add move "from" relative macro node -> "to" empty block location
         TCH_RelativeMove_c fromMove( fromMacroIndex, fromPoint, toPoint );
         relativeMoveList.Add( fromMove );
         continue;
      }

      const TCH_RelativeBlock_c* pfromBlock = this->FindRelativeBlock_( fromPoint );
      if( !pfromBlock ||
          !pfromBlock->HasRelativeMacroIndex( ))
      {
         // VPR's grid block name is not asso. with a relative macro => not available
         continue;
      }
      
      const TCH_RelativeBlock_c* ptoBlock = this->FindRelativeBlock_( toPoint );
      if( !ptoBlock ||
          !ptoBlock->HasRelativeMacroIndex( ))
      {
         // VPR's grid block name is not asso. with a relative macro => it is available

         // Add move "from" relative macro node -> "to" single block location,
         // and add move "to" single block -> "from" relative macro node
         // (since "to" block location not a member of a relative macro)
         TCH_RelativeMove_c fromMove( fromMacroIndex, fromPoint, toPoint );
         TCH_RelativeMove_c toMove( toPoint, fromMacroIndex, fromPoint );
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
         TCH_RelativeMove_c fromMove( fromMacroIndex, fromPoint, toMacroIndex, toPoint );
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
void TCH_RelativePlaceHandler_c::PlaceMacroResetRotate_(
      TCH_PlaceMacroRotateMode_t mode )
{
   TCH_RotateMaskValue_c* protateMaskValue = ( mode == TCH_PLACE_MACRO_ROTATE_FROM ?
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
bool TCH_RelativePlaceHandler_c::PlaceMacroIsLegalRotate_( 
      TCH_PlaceMacroRotateMode_t mode,
      TGO_RotateMode_t           rotate ) const
{
   bool isLegal = true;

   const TCH_RotateMaskValue_c* pplaceMacroRotate = ( mode == TCH_PLACE_MACRO_ROTATE_FROM ?
                                                      &this->fromPlaceMacroRotate_ :
                                                      &this->toPlaceMacroRotate_ );
   TCH_RotateMaskValue_c* protateMaskValue = const_cast< TCH_RotateMaskValue_c* >( pplaceMacroRotate );
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
bool TCH_RelativePlaceHandler_c::PlaceMacroIsValidRotate_( 
      TCH_PlaceMacroRotateMode_t mode ) const
{
   const TCH_RotateMaskValue_c* pplaceMacroRotate = ( mode == TCH_PLACE_MACRO_ROTATE_FROM ?
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
void TCH_RelativePlaceHandler_c::PlaceMacroUpdateMoveList_(
      TCH_RelativeMoveList_t* prelativeMoveList ) const
{
   for( size_t i = 0; i < prelativeMoveList->GetLength( ); ++i )
   {
      TCH_RelativeMove_c* prelativeMove = (*prelativeMoveList)[i];
      const TGO_Point_c& fromPoint = prelativeMove->GetFromPoint( );
      const TGO_Point_c& toPoint = prelativeMove->GetToPoint( );

      bool fromEmpty = true;
      bool toEmpty = true;

      for( size_t j = 0; j < prelativeMoveList->GetLength( ); ++j )
      {
         const TCH_RelativeMove_c& relativeMove = *(*prelativeMoveList)[j];
         
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
bool TCH_RelativePlaceHandler_c::PlaceMacroIsLegalMoveList_(
      const TCH_RelativeMoveList_t& fromToMoveList, 
      const TCH_RelativeMoveList_t& toFromMoveList ) const
{
   bool isLegal = true;
   for( size_t i = 0; i < toFromMoveList.GetLength( ); ++i )
   {
      const TCH_RelativeMove_c& toFromMove = *toFromMoveList[i];
      for( size_t j = 0; j < fromToMoveList.GetLength( ); ++j )
      {
         const TCH_RelativeMove_c& fromToMove = *fromToMoveList[j];
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
TGO_RotateMode_t TCH_RelativePlaceHandler_c::FindRandomRotateMode_(
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
TGO_Point_c TCH_RelativePlaceHandler_c::FindRandomOriginPoint_(
      const TCH_RelativeMacro_c& relativeMacro,
            size_t               relativeNodeIndex ) const
{
   const TCH_RelativeNodeList_t& relativeNodeList = relativeMacro.GetRelativeNodeList( );
   const TCH_RelativeNode_c& relativeNode = *relativeNodeList[relativeNodeIndex];

   return( this->FindRandomOriginPoint_( relativeNode ));
}

//===========================================================================//
TGO_Point_c TCH_RelativePlaceHandler_c::FindRandomOriginPoint_(
      const TCH_RelativeNode_c& relativeNode ) const
{
   const char* pszBlockName = relativeNode.GetBlockName( );

   return( this->FindRandomOriginPoint_( pszBlockName ));
}

//===========================================================================//
TGO_Point_c TCH_RelativePlaceHandler_c::FindRandomOriginPoint_(
      const char* pszBlockName ) const
{
   TGO_Point_c origin;

   const TCH_RelativeBlock_c& relativeBlock = *this->relativeBlockList_.Find( pszBlockName );
   const t_type_descriptor* vpr_type = relativeBlock.GetVPR_Type( );
   int typeIndex = ( vpr_type ? vpr_type->index : EMPTY );
   int posIndex = 0;

   t_grid_tile** vpr_gridArray = this->vpr_data_.vpr_gridArray;
   int* vpr_freeLocationArray = this->vpr_data_.vpr_freeLocationArray;
   t_legal_pos** vpr_legalPosArray = this->vpr_data_.vpr_legalPosArray;

   if(( typeIndex != EMPTY ) &&
      ( typeIndex != INVALID ) &&
      ( vpr_freeLocationArray[typeIndex] >= 0 ))
   {
      // Attempt to select a random (and available) point from within VPR's grid array
      // (based on VPR's internal 'free_locations' and 'legal_pos' structures)
      for( size_t i = 0; i < TCH_FIND_RANDOM_ORIGIN_POINT_MAX_ATTEMPTS; ++i )
      {
         posIndex = TCT_Rand( 0, vpr_freeLocationArray[typeIndex] - 1 );
         int x = vpr_legalPosArray[typeIndex][posIndex].x;
         int y = vpr_legalPosArray[typeIndex][posIndex].y;
         int z = vpr_legalPosArray[typeIndex][posIndex].z;

         if( vpr_gridArray[x][y].blocks[z] == EMPTY )
         {
            origin.Set( x, y, z );
            break;
         }
      }
   }

   if(( typeIndex != EMPTY ) &&
      ( typeIndex != INVALID ) &&
      ( vpr_freeLocationArray[typeIndex] >= 0 ) &&
      ( !origin.IsValid( )))
   {
      // Handle case where we failed to find placement location
      // (due to exceeding the max number of random placement attempts)
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      printHandler.Warning( "Failed initial relative macro placement for block \"%s\" after %u random tries.\n",
                            TIO_PSZ_STR( pszBlockName ),
                            TCH_FIND_RANDOM_ORIGIN_POINT_MAX_ATTEMPTS );
      printHandler.Info( "%sAttempting to find available location based on exhaustive search of all free locations.\n",
                         TIO_PREFIX_WARNING_SPACE );

      // Perform an exhaustive search for 1st free location
      for( posIndex = 0; posIndex < vpr_freeLocationArray[typeIndex]; ++posIndex )
      {
         int x = vpr_legalPosArray[typeIndex][posIndex].x;
         int y = vpr_legalPosArray[typeIndex][posIndex].y;
         int z = vpr_legalPosArray[typeIndex][posIndex].z;

         if( vpr_gridArray[x][y].blocks[z] == EMPTY )
         {
            origin.Set( x, y, z );
            break;
         }
      }
   }

   if(( typeIndex != EMPTY ) &&
      ( typeIndex != INVALID ) &&
      ( vpr_freeLocationArray[typeIndex] >= 0 ) &&
      ( !origin.IsValid( )))
   {
      // Handle case where we still failed to find placement location
      // (even after an exhaustive iteration over all free locations)
      const t_type_descriptor* vpr_typeArray = this->vpr_data_.vpr_typeArray;

      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      printHandler.Error( "Initial relative placement failed!\n" );
      printHandler.Info( "%sCould not place block \"%s\", no free locations of type \"%s\".\n",
                         TIO_PREFIX_ERROR_SPACE,
                         TIO_PSZ_STR( pszBlockName ),
                         TIO_PSZ_STR( vpr_typeArray[typeIndex].name ));
   }

   if(( typeIndex != EMPTY ) &&
      ( typeIndex != INVALID ) &&
      ( vpr_freeLocationArray[typeIndex] >= 0 ) &&
      ( origin.IsValid( )))
   {
      // Update VPR's 'free_locations' and 'legal_pos' structures to prevent
      // randomly finding same origin point again - IFF rotate not enabled
      // (for further reference, see VPR's initial_place_blocks() function)
      if( !this->placeOptions_.rotateEnable )
      {
         int lastIndex = vpr_freeLocationArray[typeIndex] - 1;
         if( lastIndex >= 0 )
         {
            vpr_legalPosArray[typeIndex][posIndex] = vpr_legalPosArray[typeIndex][lastIndex];
         }
         --vpr_freeLocationArray[typeIndex];
      }
   }
   return( origin );
}

//===========================================================================//
// Method         : FindRandomRelativeNodeIndex_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
size_t TCH_RelativePlaceHandler_c::FindRandomRelativeNodeIndex_(
      const TCH_RelativeMacro_c& relativeMacro ) const
{
   size_t relativeNodeIndex = TCT_Rand( static_cast< size_t >( 0 ), 
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
size_t TCH_RelativePlaceHandler_c::FindRelativeMacroIndex_(
      const TGO_Point_c& point ) const
{
   size_t relativeMacroIndex = TCH_RELATIVE_MACRO_UNDEFINED;

   if( this->vpr_data_.IsWithinGridArray( point ))
   {
      // Find relative block based on VPR's grid array point
      const TCH_RelativeBlock_c* prelativeBlock = this->FindRelativeBlock_( point );

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
size_t TCH_RelativePlaceHandler_c::FindRelativeNodeIndex_(
      const TGO_Point_c& point ) const
{
   size_t relativeNodeIndex = TCH_RELATIVE_NODE_UNDEFINED;

   if( this->vpr_data_.IsWithinGridArray( point ))
   {
      // Find relative block based on VPR's grid array point
      const TCH_RelativeBlock_c* prelativeBlock = this->FindRelativeBlock_( point );

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
TCH_RelativeMacro_c* TCH_RelativePlaceHandler_c::FindRelativeMacro_(
      const TGO_Point_c& point ) const
{
   TCH_RelativeMacro_c* prelativeMacro = 0;

   if( this->vpr_data_.IsWithinGridArray( point ))
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
TCH_RelativeNode_c* TCH_RelativePlaceHandler_c::FindRelativeNode_(
      const TGO_Point_c& point ) const
{
   TCH_RelativeNode_c* prelativeNode = 0;

   if( this->vpr_data_.IsWithinGridArray( point ))
   {
      // Find relative macro and node indices based on VPR's grid array point
      size_t relativeMacroIndex = this->FindRelativeMacroIndex_( point );
      size_t relativeNodeIndex = this->FindRelativeNodeIndex_( point );

      // Find relative macro, if any, based on relative macro index
      const TCH_RelativeMacro_c* prelativeMacro = 0;
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
const TCH_RelativeBlock_c* TCH_RelativePlaceHandler_c::FindRelativeBlock_(
      const TGO_Point_c& point ) const
{
   const TCH_RelativeBlock_c* prelativeBlock = 0;

   if( this->vpr_data_.IsWithinGridArray( point ))
   {
      // Look up VPR block name based on VPR's grid array
      const char* pszBlockName = this->vpr_data_.FindGridPointBlockName( point );

      // Find relative block, if any, based on VPR's block name
      prelativeBlock = this->relativeBlockList_.Find( pszBlockName );
   }
   return( prelativeBlock );
}

//===========================================================================//
// Method         : DecideAntiSide_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TC_SideMode_t TCH_RelativePlaceHandler_c::DecideAntiSide_(
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
// Method         : ShowIllegalRelativeMacroWarning_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_RelativePlaceHandler_c::ShowIllegalRelativeMacroWarning_(
      const TCH_RelativeMacro_c& relativeMacro,
            size_t               relativeNodeIndex,
      const TGO_Point_c&         origin,
            TGO_RotateMode_t     rotate ) const
{
   const TCH_RelativeNode_c& relativeNode = *relativeMacro.Find( relativeNodeIndex );
   const char* pszBlockName = relativeNode.GetBlockName( );

   string srOrigin;
   origin.ExtractString( &srOrigin );

   string srRotate;
   TGO_ExtractStringRotateMode( rotate, &srRotate );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   bool ok = printHandler.Warning( "Failed relative macro placement based on reference block \"%s\" at origin (%s) and rotate %s.\n",
                                   TIO_PSZ_STR( pszBlockName ),
                                   TIO_SR_STR( srOrigin ),
                                   TIO_SR_STR( srRotate ));
   return( ok );
}

//===========================================================================//
// Method         : ShowMissingBlockNameError_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_RelativePlaceHandler_c::ShowMissingBlockNameError_(
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
bool TCH_RelativePlaceHandler_c::ShowInvalidConstraintError_(
      const TCH_RelativeBlock_c& fromBlock,
      const TCH_RelativeBlock_c& toBlock,
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
