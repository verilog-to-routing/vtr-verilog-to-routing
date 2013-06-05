//===========================================================================//
// Purpose : Method definitions for the TCH_PrePlacedHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - Configure
//           - Set, Reset
//           - InitialPlace
//           - IsValid
//
//           Protected methods include:
//           - TCH_PrePlacedHandler_c, ~TCH_PrePlacedHandler_c
//
//           Private methods include:
//           - ResetPrePlacedBlockList_
//           - InitialPlaceBlockList_
//           - InitialPlaceBlock_
//           - ShowIllegalBlockWarning_
//           - ShowMissingBlockNameError_
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
using namespace std;

#include "TIO_PrintHandler.h"

#include "TGO_StringUtils.h"

#include "TCH_Typedefs.h"
#include "TCH_PrePlacedHandler.h"

// Initialize the pre-placed handler "singleton" class, as needed
TCH_PrePlacedHandler_c* TCH_PrePlacedHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TCH_PrePlacedHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
TCH_PrePlacedHandler_c::TCH_PrePlacedHandler_c( 
      void )
      :
      prePlacedBlockList_( TCH_PREPLACED_BLOCK_LIST_DEF_CAPACITY )
{
   pinstance_ = 0;
}

//===========================================================================//
// Method         : ~TCH_PrePlacedHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
TCH_PrePlacedHandler_c::~TCH_PrePlacedHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
void TCH_PrePlacedHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TCH_PrePlacedHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
void TCH_PrePlacedHandler_c::DeleteInstance( 
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
// 01/18/13 jeffr : Original
//===========================================================================//
TCH_PrePlacedHandler_c& TCH_PrePlacedHandler_c::GetInstance(
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
// 01/18/13 jeffr : Original
//===========================================================================//
bool TCH_PrePlacedHandler_c::HasInstance(
      void )
{
   return( pinstance_ ? true : false );
}

//===========================================================================//
// Method         : Configure
// Description    : Configure a pre-placed handler (singleton) based on Toro's
//                  block list and any associated pre-placed options.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
void TCH_PrePlacedHandler_c::Configure(
      const TPO_InstList_t& toro_circuitBlockList ) // Defined by Toro's block list
{
   // Begin by first clearing any existing pre-placed block list data
   this->prePlacedBlockList_.Clear( );

   // Continue by setting pre-placed block estimated capacity
   int toro_circuitBlockCount = static_cast< int >( toro_circuitBlockList.GetLength( ));
   this->prePlacedBlockList_.SetCapacity( toro_circuitBlockCount );

   // Initialize the local pre-placed block list based on Toro's block list
   for( size_t i = 0; i < toro_circuitBlockList.GetLength( ); ++i )
   {
      const TPO_Inst_c& toro_circuitBlock = *toro_circuitBlockList[i];

      const char* pszBlockName = toro_circuitBlock.GetName( );

      TPO_StatusMode_t placeStatus = toro_circuitBlock.GetPlaceStatus( );
      TCH_PlaceStatusMode_t status = TCH_PLACE_STATUS_UNDEFINED;
      switch( placeStatus )
      {
      case TPO_STATUS_FLOAT:  status = TCH_PLACE_STATUS_FLOAT;  break;
      case TPO_STATUS_FIXED:  status = TCH_PLACE_STATUS_FIXED;  break;
      case TPO_STATUS_PLACED: status = TCH_PLACE_STATUS_PLACED; break;
      default:                                                  break;
      }
      if(( status != TCH_PLACE_STATUS_FIXED ) && ( status != TCH_PLACE_STATUS_PLACED ))
         continue;

      const TGO_Point_c& origin = toro_circuitBlock.GetPlaceOrigin( );
      if( !origin.IsValid( ))
         continue;

      TCH_PrePlacedBlock_c prePlacedBlock( pszBlockName, status, origin );
      this->prePlacedBlockList_.Add( prePlacedBlock );
   }
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
void TCH_PrePlacedHandler_c::Set(
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
   // Set local reference to VPR's grid array and asso. global data structures
   this->vpr_data_.Init( vpr_gridArray, vpr_nx, vpr_ny, 
                         vpr_blockArray, vpr_blockCount,
                         vpr_typeArray, vpr_typeCount,
                         vpr_freeLocationArray, vpr_legalPosArray );
}

//===========================================================================//
// Method         : Reset
// Description    : Resets the pre-placed handler in order to clear any
//                  existing placement information in VPR's grid array or
//                  in the local pre-placed block list.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
bool TCH_PrePlacedHandler_c::Reset(
      void )
{
   bool ok = true;

   // Reset local VPR's data placement information
   this->vpr_data_.Reset( );

   // Reset block index and type information for the pre-placed block list
   ok = this->ResetPrePlacedBlockList_( this->vpr_data_.vpr_blockArray,
                                        this->vpr_data_.vpr_blockCount,
                                        &this->prePlacedBlockList_ );
   return( ok );
}

//===========================================================================//
// Method         : InitialPlace
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
bool TCH_PrePlacedHandler_c::InitialPlace(
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

   // At least one pre-placed constraint has been defined
   bool placedAllBlocks = true;

   // Initialize local VPR grid array and block list structures
   this->Set( vpr_gridArray, vpr_nx, vpr_ny,
              vpr_blockArray, vpr_blockCount,
              vpr_typeArray, vpr_typeCount,
              vpr_freeLocationArray, vpr_legalPosArray );

   // Reset VPR grid array and pre-placed block list, then initialize place
   ok = this->Reset( );
   if( ok )
   {
      ok = this->InitialPlaceBlockList_( &placedAllBlocks );
   }
   return( ok );
}

//===========================================================================//
// Method         : IsValid
// Description    : Returns TRUE if at least one pre-placed block exists
//                  (ie. at least one placement constraint has been defined).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
bool TCH_PrePlacedHandler_c::IsValid(
      void ) const
{
   return( this->prePlacedBlockList_.IsValid( ) ? true : false );
}

//===========================================================================//
// Method         : ResetPrePlacedBlockList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
bool TCH_PrePlacedHandler_c::ResetPrePlacedBlockList_(
      t_block*                  vpr_blockArray,
      int                       vpr_blockCount,
      TCH_PrePlacedBlockList_t* pprePlacedBlockList )
{
   bool ok = true;

   // Reset existing pre-placed block list index and type data
   for( size_t i = 0; i < pprePlacedBlockList->GetLength( ); ++i )
   {
      TCH_PrePlacedBlock_c* pprePlacedBlock = (*pprePlacedBlockList)[i];
      pprePlacedBlock->SetVPR_Index( -1 );
      pprePlacedBlock->SetVPR_Type( 0 );
   }

   // Update pre-placed block list based on VPR block array index and type data
   for( int blockIndex = 0; blockIndex < vpr_blockCount; ++blockIndex )
   {
      const char* pszBlockName = vpr_blockArray[blockIndex].name;
      const t_type_descriptor* vpr_type = vpr_blockArray[blockIndex].type;
      int vpr_index = blockIndex;

      TCH_PrePlacedBlock_c* pprePlacedBlock = pprePlacedBlockList->Find( pszBlockName );
      if( pprePlacedBlock )
      {
         pprePlacedBlock->SetVPR_Type( vpr_type );
         pprePlacedBlock->SetVPR_Index( vpr_index );
      }
   }

   // Validate that all pre-placed blocks have been updated
   for( size_t i = 0; i < pprePlacedBlockList->GetLength( ); ++i )
   {
      const TCH_PrePlacedBlock_c& prePlacedBlock = *(*pprePlacedBlockList)[i];
      if( prePlacedBlock.GetVPR_Index( ) == -1 )
      {
         const char* pszBlockName = prePlacedBlock.GetName( );
         ok = this->ShowMissingBlockNameError_( pszBlockName );
         if( !ok )
            break;
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : InitialPlaceBlockList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
bool TCH_PrePlacedHandler_c::InitialPlaceBlockList_(
      bool* pplacedAllBlocks )
{
   bool ok = true;

   bool foundAllPlacements = true;

   for( size_t i = 0; i < this->prePlacedBlockList_.GetLength( ); ++i )
   {
      const TCH_PrePlacedBlock_c& prePlacedBlock = *this->prePlacedBlockList_[i];

      bool validPlacement = false;
      bool foundPlacement = false;
      ok = this->InitialPlaceBlock_( prePlacedBlock,
                                     &validPlacement, &foundPlacement );
      if( !ok )
         break;

      if( validPlacement && !foundPlacement )
      {
         foundAllPlacements = false;
      }
   }

   if( !foundAllPlacements ) // Failed to place at least one of the pre-placed blocks
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Error( "Failed to find initial placement for all pre-placed blocks.\n" );
   }

   if( *pplacedAllBlocks )
   {
      *pplacedAllBlocks = foundAllPlacements;
   }
   return( ok );
}

//===========================================================================//
// Method         : InitialPlaceBlock_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
bool TCH_PrePlacedHandler_c::InitialPlaceBlock_(
      const TCH_PrePlacedBlock_c& prePlacedBlock,
            bool*                 pvalidPlacement,
            bool*                 pfoundPlacement )
{
   bool ok = true;

   *pvalidPlacement = false;
   *pfoundPlacement = false;

   const TGO_Point_c& gridPoint = prePlacedBlock.GetOrigin( );
   int blockIndex = prePlacedBlock.GetVPR_Index( );
   TCH_PlaceStatusMode_t blockStatus = prePlacedBlock.GetStatus( );

   if( gridPoint.IsValid( ))
   {
      *pvalidPlacement = true;

      const t_block* vpr_blockArray = this->vpr_data_.vpr_blockArray;
      const t_type_descriptor* vpr_type = vpr_blockArray[blockIndex].type;
      if( this->vpr_data_.IsOpenGridPoint( gridPoint, vpr_type ))
      {
         // And, update VPR's grid and block arrays based on the pre-placed block
         this->vpr_data_.Set( gridPoint, blockIndex, blockStatus );

         *pfoundPlacement = true;
      }
      else
      {
         const char* pszBlockName = prePlacedBlock.GetBlockName( );
         ok = this->ShowIllegalBlockWarning_( pszBlockName, gridPoint );
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : ShowIllegalBlockWarning_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
bool TCH_PrePlacedHandler_c::ShowIllegalBlockWarning_(
      const char*        pszBlockName,
      const TGO_Point_c& origin ) const
{
   string srOrigin;
   origin.ExtractString( &srOrigin );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   bool ok = printHandler.Warning( "Failed to place pre-placed block \"%s\".\n",
                                    TIO_PSZ_STR( pszBlockName ));
   printHandler.Info( "%sGrid location at (%s) is not open or does not match type.\n",
                      TIO_PREFIX_WARNING_SPACE,
                      TIO_SR_STR( srOrigin ));
   return( ok );
}

//===========================================================================//
// Method         : ShowMissingBlockNameError_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/18/13 jeffr : Original
//===========================================================================//
bool TCH_PrePlacedHandler_c::ShowMissingBlockNameError_(
      const char* pszBlockName ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   bool ok = printHandler.Error( "Invalid pre-placed block \"%s\"!\n",
                                 TIO_PSZ_STR( pszBlockName ));
   printHandler.Info( "%sBlock was not found in VPR's block list.\n",
                      TIO_PREFIX_ERROR_SPACE );
   return( ok );
}
