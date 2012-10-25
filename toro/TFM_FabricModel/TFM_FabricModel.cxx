//===========================================================================//
// Purpose : Method definitions for the TFM_FabricModel class.
//
//           Public methods include:
//           - TFM_FabricModel_c, ~TFM_FabricModel_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - Clear
//           - GetFabricView
//           - IsValid
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

#include "TFM_FabricModel.h"

//===========================================================================//
// Method         : TFM_FabricModel_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_FabricModel_c::TFM_FabricModel_c( 
      void )
      :
      physicalBlockList( TFM_PHYSICAL_BLOCK_LIST_DEF_CAPACITY ),
      inputOutputList( TFM_INPUT_OUTPUT_LIST_DEF_CAPACITY ),
      switchBoxList( TFM_SWITCH_BOX_LIST_DEF_CAPACITY ),
      channelList( TFM_CHANNEL_LIST_DEF_CAPACITY ),
      segmentList( TFM_SEGMENT_LIST_DEF_CAPACITY )
{
} 

//===========================================================================//
TFM_FabricModel_c::TFM_FabricModel_c( 
      const TFM_FabricModel_c& fabricModel )
      :
      srName( fabricModel.srName ),
      config( fabricModel.config ),
      physicalBlockList( fabricModel.physicalBlockList ),
      inputOutputList( fabricModel.inputOutputList ),
      switchBoxList( fabricModel.switchBoxList ),
      channelList( fabricModel.channelList ),
      segmentList( fabricModel.segmentList ),
      fabricView_( fabricModel.fabricView_ )
{
} 

//===========================================================================//
// Method         : ~TFM_FabricModel_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_FabricModel_c::~TFM_FabricModel_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_FabricModel_c& TFM_FabricModel_c::operator=( 
      const TFM_FabricModel_c& fabricModel )
{
   if( &fabricModel != this )
   {
      this->srName = fabricModel.srName;
      this->config = fabricModel.config;
      this->physicalBlockList = fabricModel.physicalBlockList;
      this->inputOutputList = fabricModel.inputOutputList;
      this->switchBoxList = fabricModel.switchBoxList;
      this->channelList = fabricModel.channelList;
      this->segmentList = fabricModel.segmentList;
      this->fabricView_ = fabricModel.fabricView_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFM_FabricModel_c::operator==( 
      const TFM_FabricModel_c& fabricModel ) const
{
   return(( this->srName == fabricModel.srName ) &&
          ( this->config == fabricModel.config ) &&
          ( this->physicalBlockList == fabricModel.physicalBlockList ) &&
          ( this->inputOutputList == fabricModel.inputOutputList ) &&
          ( this->switchBoxList == fabricModel.switchBoxList ) &&
          ( this->channelList == fabricModel.channelList ) &&
          ( this->segmentList == fabricModel.segmentList ) &&
          ( this->fabricView_ == fabricModel.fabricView_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFM_FabricModel_c::operator!=( 
      const TFM_FabricModel_c& fabricModel ) const
{
   return( !this->operator==( fabricModel ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TFM_FabricModel_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "<fabric \"%s\" >\n",
                                        TIO_SR_STR( this->srName ));
   spaceLen += 3;

   printHandler.Write( pfile, spaceLen, "\n" );
   this->config.Print( pfile, spaceLen );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->inputOutputList.Print( pfile, spaceLen );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->physicalBlockList.Print( pfile, spaceLen );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->switchBoxList.Print( pfile, spaceLen );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->channelList.Print( pfile, spaceLen );

   printHandler.Write( pfile, spaceLen, "\n" );
   this->segmentList.Print( pfile, spaceLen );

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "\n" );
   printHandler.Write( pfile, spaceLen, "</fabric>\n" );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TFM_FabricModel_c::Clear(
      void )
{
   this->srName = "";

   this->config.fabricRegion.Reset( );

   this->physicalBlockList.Clear( );
   this->inputOutputList.Clear( );
   this->switchBoxList.Clear( );
   this->channelList.Clear( );
   this->segmentList.Clear( );
}

//===========================================================================//
// Method         : GetFabricView
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFV_FabricView_c* TFM_FabricModel_c::GetFabricView(
      void )
{
   return( &this->fabricView_ );
}

//===========================================================================//
TFV_FabricView_c* TFM_FabricModel_c::GetFabricView(
      void ) const
{
   return( const_cast< TFV_FabricView_c* >( &this->fabricView_ ));
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFM_FabricModel_c::IsValid(
      void ) const
{
   return(( this->config.IsValid( )) &&
          ( this->physicalBlockList.IsValid( )) &&
          ( this->inputOutputList.IsValid( )) &&
          ( this->switchBoxList.IsValid( )) &&
          ( this->channelList.IsValid( )) &&
          ( this->segmentList.IsValid( )) ?
	  true : false );
}
