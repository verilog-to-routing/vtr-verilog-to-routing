//===========================================================================//
// Purpose : Method definitions for the TAS_ArchitectureSpec class.
//
//           Public methods include:
//           - TAS_ArchitectureSpec_c, ~TAS_ArchitectureSpec_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
//           - InitDefaults
//           - InitValidate
//           - IsValid
//
//           Private methods include:
//           - InitDefaultsConfig_
//           - InitDefaultsBlockHier_
//           - InitDefaultsParentLinks_
//           - InitDefaultsCellList_
//           - InitValidateConfig_
//           - InitValidatePhysicalBlockList_
//           - InitValidatePhysicalBlock_
//           - InitValidateModeList_
//           - InitValidateMode_
//           - InitValidatePinAssignList_
//           - InitValidateGridAssignList_
//           - InitValidateSegmentList_
//           - InitValidateSegment_
//           - InitValidateCellList_
//           - InitValidateCell_
//           - HasPhysicalBlockUsage_
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#include "TIO_PrintHandler.h"

#include "TAS_StringUtils.h"
#include "TAS_ArchitectureSpec.h"

//===========================================================================//
// Method         : TAS_ArchitectureSpec_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_ArchitectureSpec_c::TAS_ArchitectureSpec_c( 
      void )
      :
      physicalBlockList( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY ),
      modeList( TAS_MODE_LIST_DEF_CAPACITY ),
      switchBoxList( TAS_SWITCH_BOX_LIST_DEF_CAPACITY ),
      segmentList( TAS_SEGMENT_LIST_DEF_CAPACITY ),
      cellList( TAS_CELL_LIST_DEF_CAPACITY )
{
   this->sorted.physicalBlockList.SetCapacity( TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY );
   this->sorted.modeList.SetCapacity( TAS_MODE_LIST_DEF_CAPACITY );
   this->sorted.switchBoxList.SetCapacity( TAS_SWITCH_BOX_LIST_DEF_CAPACITY );
} 

//===========================================================================//
TAS_ArchitectureSpec_c::TAS_ArchitectureSpec_c( 
      const TAS_ArchitectureSpec_c& architectureSpec )
      :
      srName( architectureSpec.srName ),
      config( architectureSpec.config ),
      physicalBlockList( architectureSpec.physicalBlockList ),
      modeList( architectureSpec.modeList ),
      switchBoxList( architectureSpec.switchBoxList ),
      segmentList( architectureSpec.segmentList ),
      cellList( architectureSpec.cellList )
{
   this->sorted.physicalBlockList = architectureSpec.sorted.physicalBlockList;
   this->sorted.modeList = architectureSpec.sorted.modeList;
   this->sorted.switchBoxList = architectureSpec.sorted.switchBoxList;
} 

//===========================================================================//
// Method         : ~TAS_ArchitectureSpec_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_ArchitectureSpec_c::~TAS_ArchitectureSpec_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_ArchitectureSpec_c& TAS_ArchitectureSpec_c::operator=( 
      const TAS_ArchitectureSpec_c& architectureSpec )
{
   if( &architectureSpec != this )
   {
      this->srName = architectureSpec.srName;
      this->config = architectureSpec.config;
      this->physicalBlockList = architectureSpec.physicalBlockList;
      this->modeList = architectureSpec.modeList;
      this->switchBoxList = architectureSpec.switchBoxList;
      this->segmentList = architectureSpec.segmentList;
      this->cellList = architectureSpec.cellList;
      this->sorted.physicalBlockList = architectureSpec.sorted.physicalBlockList;
      this->sorted.modeList = architectureSpec.sorted.modeList;
      this->sorted.switchBoxList = architectureSpec.sorted.switchBoxList;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::operator==( 
      const TAS_ArchitectureSpec_c& architectureSpec ) const
{
   return(( this->srName == architectureSpec.srName ) &&
          ( this->config == architectureSpec.config ) &&
          ( this->physicalBlockList == architectureSpec.physicalBlockList ) &&
          ( this->modeList == architectureSpec.modeList ) &&
          ( this->switchBoxList == architectureSpec.switchBoxList ) &&
          ( this->segmentList == architectureSpec.segmentList ) &&
          ( this->cellList == architectureSpec.cellList ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::operator!=( 
      const TAS_ArchitectureSpec_c& architectureSpec ) const
{
   return( !this->operator==( architectureSpec ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_ArchitectureSpec_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "<architecture \"%s\" >\n",
                                        TIO_SR_STR( this->srName ));
   spaceLen += 3;

   printHandler.Write( pfile, spaceLen, "\n" );
   this->config.Print( pfile, spaceLen );

   if( this->physicalBlockList.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "\n" );

      TAS_ModeList_t modeList_;
      for( size_t i = 0; i < this->physicalBlockList.GetLength( ); ++i )
      {
         if( this->physicalBlockList[i]->GetUsage( ) == TAS_USAGE_INPUT_OUTPUT )
         {
            this->physicalBlockList[i]->Print( pfile, spaceLen,
                                               0, &modeList_ );
         }
      }
      for( size_t i = 0; i < this->physicalBlockList.GetLength( ); ++i )
      {
         if( this->physicalBlockList[i]->GetUsage( ) == TAS_USAGE_PHYSICAL_BLOCK )
         {
            this->physicalBlockList[i]->Print( pfile, spaceLen,
                                               0, &modeList_ );
         }
      }
      for( size_t i = 0; i < this->physicalBlockList.GetLength( ); ++i )
      {
         if(( this->physicalBlockList[i]->GetUsage( ) != TAS_USAGE_INPUT_OUTPUT ) &&
            ( this->physicalBlockList[i]->GetUsage( ) != TAS_USAGE_PHYSICAL_BLOCK ))
         {
            this->physicalBlockList[i]->Print( pfile, spaceLen,
                                               0, &modeList_ );
         }
      }
   }

   if( this->switchBoxList.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "\n" );
      this->switchBoxList.Print( pfile, spaceLen );
   }
   if( this->segmentList.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "\n" );
      this->segmentList.Print( pfile, spaceLen );
   }

   if( this->cellList.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "\n" );
      for( size_t i = 0; i < this->cellList.GetLength( ); ++i )
      {
         if( this->cellList[i]->GetSource( ) == TLO_CELL_SOURCE_STANDARD )
            continue;
         this->cellList[i]->Print( pfile, spaceLen );
      }
   }

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</architecture>\n" );
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_ArchitectureSpec_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_ArchitectureSpec_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<architecture>\n" );
   spaceLen += 3;

   if( this->config.IsValid( ))
   {
      this->config.PrintXML( pfile, spaceLen );
   }
   if( this->cellList.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<models>\n" );
      for( size_t i = 0; i < this->cellList.GetLength( ); ++i )
      {
         if( this->cellList[i]->GetSource( ) == TLO_CELL_SOURCE_STANDARD )
            continue;

         this->cellList[i]->PrintXML( pfile, spaceLen + 3 );
      }
      printHandler.Write( pfile, spaceLen, "</models>\n" );
   }
   if( this->physicalBlockList.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<complexblocklist>\n" );
      for( size_t i = 0; i < this->physicalBlockList.GetLength( ); ++i )
      {
         this->physicalBlockList[i]->PrintXML( pfile, spaceLen + 3 );
      }
      printHandler.Write( pfile, spaceLen, "</complexblocklist>\n" );
   }
   if( this->switchBoxList.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<switchlist>\n" );
      for( size_t i = 0; i < this->switchBoxList.GetLength( ); ++i )
      {
         this->switchBoxList[i]->PrintXML( pfile, spaceLen + 3 );
      }
      printHandler.Write( pfile, spaceLen, "</switchlist>\n" );
   }
   if( this->segmentList.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<segmentlist>\n" );
      for( size_t i = 0; i < this->segmentList.GetLength( ); ++i )
      {
         this->segmentList[i]->PrintXML( pfile, spaceLen + 3 );
      }
      printHandler.Write( pfile, spaceLen, "</segmentlist>\n" );
   }

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</architecture>\n" );
}

//===========================================================================//
// Method         : InitDefaults
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitDefaults( 
      void )
{
   bool ok = true;

   // Set configuration defaults 
   this->InitDefaultsConfig_( &this->config );

   // Initialize all physical block hierachical links, if needed (non-XML)
   ok = this->InitDefaultsBlockHier_( );
   if( ok )
   {
      // Initialize all physical block and mode parent pointers
      for( size_t i = 0; i < this->physicalBlockList.GetLength( ); ++i )
      {
         this->InitDefaultsParentLinks_( this->physicalBlockList[i], 0, 0 );
      }

      // Add cell list defaults
      this->InitDefaultsCellList_( &this->cellList );
   }
   return( ok );
}

//===========================================================================//
// Method         : InitValidate
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitValidate( 
      void ) 
{
   bool ok = true;
   if( ok )
   {
      ok = this->InitValidateConfig_( );
   }
   if( ok )
   {
      this->sorted.physicalBlockList.SetCapacity( this->physicalBlockList.GetCapacity( ));
      for( size_t i = 0; i < this->physicalBlockList.GetLength( ); ++i )
      {
         this->sorted.physicalBlockList.Add( *this->physicalBlockList[i] );
      }

      // Validate unique physical block list names
      bool uniquifyShowWarning = true;
      bool uniquifyShowError = false;
      ok = this->sorted.physicalBlockList.Uniquify( uniquifyShowWarning, uniquifyShowError,
                                                    "pb_type list" );
   }
   if( ok )
   {
      ok = this->InitValidatePhysicalBlockList_( &this->physicalBlockList,
                                                 this->cellList, 0 );
   }
   if( ok )
   {
      this->sorted.modeList.SetCapacity( this->modeList.GetCapacity( ));
      for( size_t i = 0; i < this->modeList.GetLength( ); ++i )
      {
         this->sorted.modeList.Add( *this->modeList[i] );
      }

      // Validate unique mode list names
      bool uniquifyShowWarning = true;
      bool uniquifyShowError = false;
      ok = this->sorted.modeList.Uniquify( uniquifyShowWarning, uniquifyShowError,
                                           "mode list" );
   }
   if( ok )
   {
      this->sorted.switchBoxList.SetCapacity( this->switchBoxList.GetCapacity( ));
      for( size_t i = 0; i < this->switchBoxList.GetLength( ); ++i )
      {
         this->sorted.switchBoxList.Add( *this->switchBoxList[i] );
      }

      // Validate unique switch box list names
      bool uniquifyShowWarning = true;
      bool uniquifyShowError = false;
      ok = this->sorted.switchBoxList.Uniquify( uniquifyShowWarning, uniquifyShowError,
                                                "switch box list" );
   }
   if( ok )
   {
      ok = this->InitValidateSegmentList_( this->segmentList,
                                           this->sorted.switchBoxList );
   }
   if( ok )
   {
      ok = this->InitValidateCellList_( this->cellList );
   }
   return( ok );
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::IsValid(
      void ) const
{
   return(( this->physicalBlockList.IsValid( )) ||
          ( this->modeList.IsValid( )) ||
          ( this->switchBoxList.IsValid( )) ||
          ( this->segmentList.IsValid( )) ||
          ( this->cellList.IsValid( )) ?
          true : false );
}

//===========================================================================//
// Method         : InitDefaultsConfig_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_ArchitectureSpec_c::InitDefaultsConfig_( 
      TAS_Config_c* pconfig ) const
{
   // Set configuration defaults 
   // (ie. hard-coded switch box default parameters based on the VPR tool)
   if( pconfig->device.switchBoxes.modelType == TAS_SWITCH_BOX_MODEL_UNDEFINED )
   {
      pconfig->device.switchBoxes.modelType = TAS_SWITCH_BOX_MODEL_WILTON;
   }
   if( pconfig->device.switchBoxes.fs == 0 )
   {
      pconfig->device.switchBoxes.fs = 3;
   }

   // Set channel distribution width defaults (for non-XML interface)
   if( pconfig->device.channelWidth.io.usageMode == TAS_CHANNEL_USAGE_UNDEFINED )
   {
      pconfig->device.channelWidth.io.usageMode = TAS_CHANNEL_USAGE_IO;
      pconfig->device.channelWidth.io.width = 1.0;
   }
   if( pconfig->device.channelWidth.x.usageMode == TAS_CHANNEL_USAGE_UNDEFINED )
   {
      pconfig->device.channelWidth.x.usageMode = TAS_CHANNEL_USAGE_X;
      pconfig->device.channelWidth.x.distrMode = TAS_CHANNEL_DISTR_UNIFORM;
      pconfig->device.channelWidth.x.peak = 1.0;
   }
   if( pconfig->device.channelWidth.y.usageMode == TAS_CHANNEL_USAGE_UNDEFINED )
   {
      pconfig->device.channelWidth.y.usageMode = TAS_CHANNEL_USAGE_Y;
      pconfig->device.channelWidth.y.distrMode = TAS_CHANNEL_DISTR_UNIFORM;
      pconfig->device.channelWidth.y.peak = 1.0;
   }

   // Set layour auto size aspect ratio (for non-XML interface)
   if( pconfig->layout.sizeMode == TAS_ARRAY_SIZE_UNDEFINED )
   {
      pconfig->layout.sizeMode = TAS_ARRAY_SIZE_AUTO;
      pconfig->layout.autoSize.aspectRatio = 1.0;
   }
}

//===========================================================================//
// Method         : InitDefaultsBlockHier_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitDefaultsBlockHier_(
      void )
{
   bool ok = true;

   if( this->HasPhysicalBlockUsage_( ))
   {
      TAS_PhysicalBlockList_t physicalBlockList_ = this->physicalBlockList;
      this->physicalBlockList.Clear( );

      // Iterate local list as long as list has at least one more element
      while( physicalBlockList_.IsValid( )) 
      {
         // Get 1st (ie. next) physical block and delete from list
         const TAS_PhysicalBlock_c& physicalBlock = *physicalBlockList_[0];
         TAS_PhysicalBlock_c* pphysicalBlock = this->physicalBlockList.Add( physicalBlock );

         physicalBlockList_.Delete( 0 );

         // Recursively iterate to add mode and physical block hierarchy
         ok = this->InitDefaultsBlockHier_( &physicalBlockList_, pphysicalBlock );
         if( !ok ) 
            break;
      }
   }
   return( ok );
}

//===========================================================================//
bool TAS_ArchitectureSpec_c::InitDefaultsBlockHier_( 
      TAS_PhysicalBlockList_t* pphysicalBlockList,
      TAS_PhysicalBlock_c*     pphysicalBlock )
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   TAS_ModeNameList_t modeNameList = pphysicalBlock->modeNameList;
   pphysicalBlock->modeNameList.Clear( );

   for( size_t i = 0; i < modeNameList.GetLength( ); ++i )
   {
      const TC_Name_c& modeName = *modeNameList[i];
      TAS_Mode_c mode;
      if( this->modeList.Find( modeName.GetName( ), &mode ))
      {
         TAS_Mode_c* pmode_ = pphysicalBlock->modeList.Add( mode );

         for( size_t j = 0; j < pmode_->physicalBlockList.GetLength( ); ++j )
         {
            TAS_PhysicalBlock_c* pphysicalBlock_ = pmode_->physicalBlockList[j];
            TAS_PhysicalBlock_c physicalBlock;
            if( pphysicalBlockList->Find( *pphysicalBlock_, &physicalBlock ))
            {
               pphysicalBlockList->Delete( physicalBlock );
               pphysicalBlock_ = pmode_->physicalBlockList.Replace( physicalBlock );

               ok = InitDefaultsBlockHier_( pphysicalBlockList, pphysicalBlock_ );
               if( !ok )
                  break;
            }
            else
            {
               ok = printHandler.Error( "Invalid architecture specification!\n"
                                        "%sMissing physical block \"%s\" for mode \"%s\"\n",
                                        TIO_PREFIX_ERROR_SPACE,
                                        TIO_PSZ_STR( pphysicalBlock->GetName( )),
                                        TIO_PSZ_STR( pmode_->GetName( )));
               if( !ok )
                  break;
            }
         }
      }
      else
      {
         ok = printHandler.Error( "Invalid architecture specification!\n"
                                  "%sMissing mode \"%s\" for physical block \"%s\"\n",
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_PSZ_STR( modeName.GetName( )),
                                  TIO_PSZ_STR( pphysicalBlock->GetName( )));
         if( !ok )
            break;
      }
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : InitDefaultsParentLinks_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_ArchitectureSpec_c::InitDefaultsParentLinks_(
            TAS_PhysicalBlock_c* pphysicalBlock,
      const TAS_PhysicalBlock_c* pphysicalBlockParent,
      const TAS_Mode_c*          pmodeParent ) const
{
   // Initialize links to parent (either physical block or mode)
   pphysicalBlock->SetPhysicalBlockParent( pphysicalBlockParent );
   pphysicalBlock->SetModeParent( pmodeParent );

   // Apply recursion to initialize children to this physical block
   for( size_t i = 0; i < pphysicalBlock->physicalBlockList.GetLength( ); ++i )
   {
      this->InitDefaultsParentLinks_( pphysicalBlock->physicalBlockList[i], 
                                      pphysicalBlock, 0 );
   }
   for( size_t i = 0; i < pphysicalBlock->modeList.GetLength( ); ++i )
   {
      this->InitDefaultsParentLinks_( pphysicalBlock->modeList[i], 
                                      pphysicalBlock );
   }
}

//===========================================================================//
void TAS_ArchitectureSpec_c::InitDefaultsParentLinks_(
            TAS_Mode_c*          pmode,
      const TAS_PhysicalBlock_c* pphysicalBlockParent ) const
{
   // Initialize links to parent (either physical block or mode)
   pmode->SetPhysicalBlockParent( pphysicalBlockParent );

   // Apply recursion to initialize children to this physical block
   for( size_t i = 0; i < pmode->physicalBlockList.GetLength( ); ++i )
   {
      this->InitDefaultsParentLinks_( pmode->physicalBlockList[i], 
                                      0, pmode );
   }
}

//===========================================================================//
// Method         : InitDefaultsCellList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_ArchitectureSpec_c::InitDefaultsCellList_( 
      TAS_CellList_t* pcellList ) const
{
   // Add cell list defaults
   // (ie. hard-coded BLIF cells parameters based on the VPR tool)
   TAS_Cell_c inputCell( "input", TLO_CELL_SOURCE_STANDARD );
   inputCell.AddPort( "inpad", TC_TYPE_OUTPUT );

   TAS_Cell_c outputCell( "output", TLO_CELL_SOURCE_STANDARD );
   outputCell.AddPort( "outpad", TC_TYPE_INPUT );

   TAS_Cell_c flipflopCell( "latch", TLO_CELL_SOURCE_STANDARD );
   flipflopCell.AddPort( "D", TC_TYPE_INPUT );
   flipflopCell.AddPort( "clk", TC_TYPE_CLOCK );
   flipflopCell.AddPort( "Q", TC_TYPE_OUTPUT );

   TAS_Cell_c lutCell( "names", TLO_CELL_SOURCE_STANDARD );
   lutCell.AddPort( "in", TC_TYPE_INPUT );
   lutCell.AddPort( "out", TC_TYPE_OUTPUT );

   pcellList->Add( inputCell );
   pcellList->Add( outputCell );
   pcellList->Add( flipflopCell );
   pcellList->Add( lutCell );
}

//===========================================================================//
// Method         : InitValidateConfig_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitValidateConfig_(
      void )
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;
   if( ok && this->IsValid( ))
   {
      if( TCTF_IsGT( this->config.layout.autoSize.aspectRatio, 0.0 ) &&
          this->config.layout.manualSize.gridDims.IsValid( ))
      {
         ok = printHandler.Warning( "Ignoring conflicting layout auto=\"%0.*f\", width=\"%u\", and height=\"%u\" parameters.\n"
                                    "%sValid parameters limited to either \"auto\" or \"width\" and \"height\" values.\n"
                                    "%sDefaulting to auto=\"%0.*f\".\n",
                                    precision, this->config.layout.autoSize.aspectRatio,
                                    this->config.layout.manualSize.gridDims.width,
                                    this->config.layout.manualSize.gridDims.height,
                                    TIO_PREFIX_WARNING_SPACE,
                                    TIO_PREFIX_WARNING_SPACE,
                                    precision, this->config.layout.autoSize.aspectRatio );

         this->config.layout.manualSize.gridDims.Reset( );
      }
   }
   if( ok && this->IsValid( ))
   {
      if( TCTF_IsLT( this->config.layout.autoSize.aspectRatio, 0.0 ))
      {
         ok = printHandler.Warning( "Ignoring invalid layout auto=\"%0.*f\" parameter.\n"
                                    "%sValid aspect ratio must be greater than zero.\n"
                                    "%sUsing \"auto\" default.\n",
                                    precision, this->config.layout.autoSize.aspectRatio,
                                    TIO_PREFIX_WARNING_SPACE,
                                    TIO_PREFIX_WARNING_SPACE );

         this->config.layout.autoSize.aspectRatio = 0.0;
      }
   }
   if( ok && this->IsValid( ))
   {
      if( !this->config.device.channelWidth.io.IsValid( ) ||
          TCTF_IsEQ( this->config.device.channelWidth.io.width, 0.0 ))
      {
         ok = printHandler.Error( "Missing or invalid channel width distribution!\n"
                                  "%sExpected <io width=\"...\"/>\n",
                                  TIO_PREFIX_ERROR_SPACE );
      }
   }
   if( ok && this->IsValid( ))
   {
      if( !this->config.device.channelWidth.x.IsValid( ) ||
          this->config.device.channelWidth.x.distrMode == TAS_CHANNEL_DISTR_UNDEFINED )
      {
         ok = printHandler.Error( "Missing or invalid channel width distribution!\n"
                                  "%sExpected <x distr=\"...\"/>\n",
                                  TIO_PREFIX_ERROR_SPACE );
      }
   }
   if( ok && this->IsValid( ))
   {
      if( !this->config.device.channelWidth.y.IsValid( ) ||
          this->config.device.channelWidth.y.distrMode == TAS_CHANNEL_DISTR_UNDEFINED )
      {
         ok = printHandler.Error( "Missing or invalid channel width distribution!\n"
                                  "%sExpected <y distr=\"...\"/>\n",
                                  TIO_PREFIX_ERROR_SPACE );
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : InitValidatePhysicalBlockList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitValidatePhysicalBlockList_(
            TAS_PhysicalBlockList_t* pphysicalBlockList,
      const TAS_CellList_t&          cellList_, 
            size_t                   hierarchyLevel ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   // Validate each physical block (recursively)
   bool ok = true;
   for( size_t i = 0; i < pphysicalBlockList->GetLength( ); ++i )
   {
      ok = this->InitValidatePhysicalBlock_(( *pphysicalBlockList )[i],
                                             cellList_, 
                                             hierarchyLevel + 1 );
      if( !ok )
         break;
   }

   if( ok && pphysicalBlockList->IsValid( ) && ( hierarchyLevel == 0 ))
   {
      if( !this->sorted.physicalBlockList.IsMember( "io" ))
      {
         ok = printHandler.Error( "Invalid physical block list!\n"
                                  "%sNo \"io\" physical block defined\n",
                                  TIO_PREFIX_ERROR_SPACE );
      }
   }

   if( ok && pphysicalBlockList->IsValid( ) && ( hierarchyLevel == 0 ))
   {
      bool isGridFillDefined = false;
      for( size_t i = 0; i < pphysicalBlockList->GetLength( ); ++i )
      {
         const TAS_PhysicalBlock_c& physicalBlock = *( *pphysicalBlockList )[i];
         const TAS_GridAssignList_t& gridAssignList = physicalBlock.gridAssignList;
         for( size_t j = 0; j < gridAssignList.GetLength( ); ++j )
         {
            const TAS_GridAssign_c& gridAssign = *gridAssignList[j];
            if( gridAssign.distr == TAS_GRID_ASSIGN_DISTR_FILL )
            {
               isGridFillDefined = true;
            }
         }
      }

      if( !isGridFillDefined )
      {
         ok = printHandler.Error( "Invalid physical block list!\n"
                                  "%sNo grid <loc type=\"fill\".../> detected\n",
                                  TIO_PREFIX_ERROR_SPACE );
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : InitValidatePhysicalBlock_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitValidatePhysicalBlock_(
            TAS_PhysicalBlock_c* pphysicalBlock,
      const TAS_CellList_t&      cellList_,
            size_t               hierarchyLevel ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;
   if( ok )
   {
      // Validate physical block "custom" BLIF model defined in by circuit ".subckt" cell
      if(( pphysicalBlock->modelType == TAS_PHYSICAL_BLOCK_MODEL_CUSTOM ) &&
         ( !cellList_.IsMember( pphysicalBlock->srModelName )))
      {
         ok = printHandler.Error( "Invalid physical BLIF model name detected!\n"
                                  "%sModel name not found in circuit '.subckt' list\n"
                                  "%sSee <pb_type name=\"%s\" blif_model=\".subckt %s\">\n",
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_SR_STR( pphysicalBlock->srName ),
                                  TIO_SR_STR( pphysicalBlock->srModelName ));
      }
   }
   if( ok )
   {
      pphysicalBlock->sorted.modeList.SetCapacity( pphysicalBlock->modeList.GetCapacity( ));
      for( size_t i = 0; i < pphysicalBlock->modeList.GetLength( ); ++i )
      {
         pphysicalBlock->sorted.modeList.Add( *pphysicalBlock->modeList[i] );
      }

      // Validate unique mode list names
      bool uniquifyShowWarning = true;
      bool uniquifyShowError = false;
      ok = pphysicalBlock->sorted.modeList.Uniquify( uniquifyShowWarning, uniquifyShowError,
                                                     "mode list" );
   }
   if( ok )
   {
      pphysicalBlock->sorted.physicalBlockList.SetCapacity( pphysicalBlock->physicalBlockList.GetCapacity( ));
      for( size_t i = 0; i < pphysicalBlock->physicalBlockList.GetLength( ); ++i )
      {
         pphysicalBlock->sorted.physicalBlockList.Add( *pphysicalBlock->physicalBlockList[i] );
      }

      // Validate unique physical block list names
      bool uniquifyShowWarning = true;
      bool uniquifyShowError = false;
      ok = pphysicalBlock->sorted.physicalBlockList.Uniquify( uniquifyShowWarning, uniquifyShowError,
                                                              "pb_type list" );

   }
   if( ok )
   {
      // Validate each physical block (recursively)
      ok = this->InitValidatePhysicalBlockList_( &pphysicalBlock->physicalBlockList,
                                                 cellList_, 
                                                 hierarchyLevel );
   }
   if( ok )
   {
      // Validate unique mode list names
      bool uniquifyShowWarning = true;
      bool uniquifyShowError = false;
      ok = pphysicalBlock->sorted.modeList.Uniquify( uniquifyShowWarning, uniquifyShowError,
                                                     "mode list" );

   }
   if( ok )
   {
      ok = this->InitValidateModeList_( &pphysicalBlock->modeList,
                                        cellList_,
                                        hierarchyLevel );
   }
   if( ok )
   {
      if(( pphysicalBlock->fcOut.type != pphysicalBlock->fcIn.type ) &&
         ( pphysicalBlock->fcOut.type != TAS_CONNECTION_BOX_FULL ))
      {
         string srFcInType, srFcOutType;
         TAS_ExtractStringConnectionBoxType( pphysicalBlock->fcIn.type, &srFcInType );
         TAS_ExtractStringConnectionBoxType( pphysicalBlock->fcOut.type, &srFcOutType );
         ok = printHandler.Error( "Invalid physical block Fc_in and Fc_out types!\n"
                                  "%sFc types must match unless FC_out is \"full\"\n"
                                  "%sSee: <fc_in type=\"%s\">...</fc_in>\n"
                                  "%sSee: <fc_out type=\"%s\">...</fc_out>\n",
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_SR_STR( srFcInType ),
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_SR_STR( srFcOutType ));
      }
   }
   if( ok )
   {
      ok = this->InitValidatePinAssignList_( pphysicalBlock->pinAssignList,
                                             pphysicalBlock->height );
   }
   if( ok )
   {
      ok = this->InitValidateGridAssignList_( pphysicalBlock->gridAssignList );
   }

// TBD ???  <delay_matrix type="max" in_port="string" out_port="string"/> matrix </delay>
// TBD ???  // Validate: matrix (input) #of rows assumed
// TBD ???  // Validate: matrix (input) #of rows == #of pins asso. with input port
// TBD ???  // Validate: matrix (input) #of columns == #of pins asso. with output port

   return( ok );
}

//===========================================================================//
// Method         : InitValidateModeList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitValidateModeList_(
            TAS_ModeList_t* pmodeList_,
      const TAS_CellList_t& cellList_,
            size_t          hierarchyLevel ) const
{
   bool ok = true;

   for( size_t i = 0; i < pmodeList_->GetLength( ); ++i )
   {
      ok = this->InitValidateMode_(( *pmodeList_ )[i], 
                                    cellList_,
                                    hierarchyLevel + 1 );
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : InitValidateMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitValidateMode_(
            TAS_Mode_c*     pmode,
      const TAS_CellList_t& cellList_,
            size_t          hierarchyLevel ) const
{
   bool ok = true;
   if( ok )
   {
      pmode->sorted.physicalBlockList.SetCapacity( pmode->physicalBlockList.GetCapacity( ));
      for( size_t i = 0; i < pmode->physicalBlockList.GetLength( ); ++i )
      {
         pmode->sorted.physicalBlockList.Add( *pmode->physicalBlockList[i] );
      }

      // Validate unique physical block list names
      bool uniquifyShowWarning = true;
      bool uniquifyShowError = false;
      ok = pmode->sorted.physicalBlockList.Uniquify( uniquifyShowWarning, uniquifyShowError,
                                                     "pb_type list" );
   }
   if( ok )
   {
      // Validate each physical block (recursively)
      for( size_t i = 0; i < pmode->physicalBlockList.GetLength( ); ++i )
      {
         ok = this->InitValidatePhysicalBlock_( pmode->physicalBlockList[i],
                                                cellList_, 
                                                hierarchyLevel );
         if( !ok )
            break;
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : InitValidatePinAssignList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitValidatePinAssignList_(
      const TAS_PinAssignList_t& pinAssignList,
            unsigned int         height ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   // Validate physical block pin assignment offset < height
   if( height > 0 )
   {
      for( size_t j = 0; j < pinAssignList.GetLength( ); ++j )
      {
         const TAS_PinAssign_c& pinAssign = *pinAssignList[j];
         if( pinAssign.pattern != TAS_PIN_ASSIGN_PATTERN_CUSTOM )
            continue;

         if( pinAssign.offset == 0 )
            continue;

         if( pinAssign.offset >= height )
         {
            ok = printHandler.Error( "Invalid physical block pin location \"custom\" pattern detected!\n"
                                     "%sPin location offset (%u) >= height (%u)\n",
                                     TIO_PREFIX_ERROR_SPACE,
                                     pinAssign.offset,
                                     height );
            if( !ok )
               break;
         }
         if( pinAssign.side == TC_SIDE_UPPER &&
             pinAssign.offset != height - 1 )
         {
            ok = printHandler.Error( "Invalid physical block pin location \"custom\" pattern detected!\n"
                                     "%sPin location side \"top\" offset (%u) != height - 1 (%u)\n",
                                     TIO_PREFIX_ERROR_SPACE,
                                     pinAssign.offset,
                                     height - 1 );
            if( !ok )
               break;
         }
         if( pinAssign.side == TC_SIDE_LOWER )
         {
            ok = printHandler.Error( "Invalid physical block pin location \"custom\" pattern detected!\n"
                                     "%sPin location side \"bottom\" offset (%u) != 0\n",
                                     TIO_PREFIX_ERROR_SPACE,
                                     pinAssign.offset );
            if( !ok )
               break;
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : InitValidateGridAssignList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitValidateGridAssignList_(
      const TAS_GridAssignList_t& gridAssignList ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   size_t fillCount = 0;
   size_t perimeterCount = 0;

   for( size_t i = 0; i < gridAssignList.GetLength( ); ++i )
   {
      const TAS_GridAssign_c& gridAssign = *gridAssignList[i];

      switch( gridAssign.distr )
      {
      case TAS_GRID_ASSIGN_DISTR_SINGLE:
         if( gridAssign.singlePercent == 0 )
         {
            ok = printHandler.Error( "Missing physical block grid location property!\n"
                                     "%sGrid location type \"rel\" must include a 'pos' value\n",
                                     TIO_PREFIX_ERROR_SPACE );
         }
         break;
      case TAS_GRID_ASSIGN_DISTR_MULTIPLE:
         if( gridAssign.multipleStart == 0 )
         {
            ok = printHandler.Error( "Missing physical block grid location property!\n"
                                     "%sGrid location type \"col\" must include a 'start' value\n",
                                     TIO_PREFIX_ERROR_SPACE );
         }
         if( gridAssign.multipleRepeat == 0 )
         {
            ok = printHandler.Error( "Missing physical block grid location property!\n"
                                     "%sGrid location type \"col\" must include a 'repeat' value\n",
                                     TIO_PREFIX_ERROR_SPACE );
         }
         break;
      case TAS_GRID_ASSIGN_DISTR_FILL:
         ++fillCount;
         break;
      case TAS_GRID_ASSIGN_DISTR_PERIMETER:
         ++perimeterCount;
         break;
      case TAS_GRID_ASSIGN_DISTR_UNDEFINED:
         break;
      }
      if( !ok )
         break;
   }

   if( ok && ( fillCount > 1 ))
   {
      ok = printHandler.Error( "Invalid physical block grid location type!\n"
                               "%sGrid location type \"fill\" defined more than once\n",
                               TIO_PREFIX_ERROR_SPACE );
   }
   if( ok && ( perimeterCount > 1 ))
   {
      ok = printHandler.Error( "Invalid physical block grid location type!\n"
                               "%sGrid location type \"perimeter\" defined more than once\n",
                               TIO_PREFIX_ERROR_SPACE );
   }
   return( ok );
}

//===========================================================================//
// Method         : InitValidateSegmentList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitValidateSegmentList_( 
      const TAS_SegmentList_t&         segmentList_,
      const TAS_SwitchBoxSortedList_t& switchBoxSortedList_ ) const
{
   // Validate each segment
   bool ok = true;
   for( size_t i = 0; i < segmentList_.GetLength( ); ++i )
   {
      ok = this->InitValidateSegment_( *segmentList_[i],
                                       switchBoxSortedList_ );
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : InitValidateSegment_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitValidateSegment_( 
      const TAS_Segment_c&             segment,
      const TAS_SwitchBoxSortedList_t& switchBoxSortedList_ ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;
   if( ok )
   {
      // Validate segment mux name matches switch box based on multiplexor type
      if(( segment.dirType == TAS_SEGMENT_DIR_UNIDIRECTIONAL ) &&
         ( !segment.srMuxSwitchName.length( )))
      {
         ok = printHandler.Error( "Missing segment mux switch name!\n"
                                  "%sExpected <mux name=\"...\"/> when type=\"unidir\"\n",
                                  TIO_PREFIX_ERROR_SPACE );
      }
      if(( segment.srMuxSwitchName.length( )) &&
         ( !switchBoxSortedList_.Find( segment.srMuxSwitchName )))
      {
         ok = printHandler.Error( "Invalid segment mux switch name detected!\n"
                                  "%sNo matching name found in switch box list\n"
                                  "%sSee: <mux name=\"%s\"/>\n", 
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_SR_STR( segment.srMuxSwitchName ));
      }
   }
   if( ok )
   {
      // Validate segment wire switch name matches switch box based on buffer type
      if(( segment.dirType == TAS_SEGMENT_DIR_BIDIRECTIONAL ) &&
         ( !segment.srWireSwitchName.length( )))
      {
         ok = printHandler.Error( "Missing segment wire switch name!\n"
                                  "%sExpected <wire name=\"...\"/> when type=\"bidir\"\n",
                                  TIO_PREFIX_ERROR_SPACE );
      }
      if(( segment.srWireSwitchName.length( )) &&
         ( !switchBoxSortedList_.Find( segment.srWireSwitchName )))
      {
         ok = printHandler.Error( "Invalid segment wire switch name detected!\n"
                                  "%sNo matching name found in switch box list\n"
                                  "%sSee: <wire_switch name=\"%s\"/>\n", 
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_SR_STR( segment.srWireSwitchName ));
      }
   }
   if( ok )
   {
      // Validate segment output switch name matches switch box based on buffer type
      if(( segment.dirType == TAS_SEGMENT_DIR_BIDIRECTIONAL ) &&
         ( !segment.srOutputSwitchName.length( )))
      {
         ok = printHandler.Error( "Missing segment output switch name!\n"
                                  "%sExpected <opin name=\"...\"/> when type=\"bidir\"\n",
                                  TIO_PREFIX_ERROR_SPACE );
      }
      if(( segment.srOutputSwitchName.length( )) &&
         ( !switchBoxSortedList_.Find( segment.srOutputSwitchName )))
      {
         ok = printHandler.Error( "Invalid segment output switch name detected!\n"
                                  "%sNo matching name found in switch box list\n"
                                  "%sSee: <opin_switch name=\"%s\"/>\n", 
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_PREFIX_ERROR_SPACE,
                                  TIO_SR_STR( segment.srOutputSwitchName ));
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : InitValidateCellList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitValidateCellList_( 
      const TAS_CellList_t& cellList_ ) const
{
   // Validate each cell
   bool ok = true;
   for( size_t i = 0; i < cellList_.GetLength( ); ++i )
   {
      if( cellList_[i]->GetSource( ) == TLO_CELL_SOURCE_STANDARD )
         continue;

      ok = this->InitValidateCell_( *cellList_[i] );
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : InitValidateCell_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::InitValidateCell_( 
      const TAS_Cell_c& cell ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = true;

   size_t inputCount = 0;
   size_t outputCount = 0;

   const TLO_PortList_t& portList = cell.GetPortList( );
   for( size_t i = 0; i < portList.GetLength( ); ++i )
   {
      const TLO_Port_c& port = *portList[i];
      inputCount += (( port.GetType( ) == TC_TYPE_INPUT ) ? 1 : 0 );
      outputCount += (( port.GetType( ) == TC_TYPE_OUTPUT ) ? 1 : 0 );
   }

   if( ok && !inputCount )
   {
      ok = printHandler.Error( "Invalid model cell detected!\n"
                               "%sNo input ports defined for model \"%s\"\n",
                               TIO_PREFIX_ERROR_SPACE,
                               TIO_PSZ_STR( cell.GetName( )));
   }
   if( ok && !outputCount )
   {
      ok = printHandler.Error( "Invalid model cell detected!\n"
                               "%sNo output ports defined for model \"%s\"\n",
                               TIO_PREFIX_ERROR_SPACE,
                               TIO_PSZ_STR( cell.GetName( )));
   }
   return( ok );
}

//===========================================================================//
// Method         : HasPhysicalBlockUsage_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TAS_ArchitectureSpec_c::HasPhysicalBlockUsage_(
      void ) const
{
   bool hasUsage = false;

   for( size_t i = 0; i < this->physicalBlockList.GetLength( ); ++i )
   {
      const TAS_PhysicalBlock_c& physicalBlock = *this->physicalBlockList[i];
      if( physicalBlock.GetUsage( ) != TAS_USAGE_UNDEFINED )
      {
         hasUsage = true;
         break;
      }
   }
   return( hasUsage );
}
