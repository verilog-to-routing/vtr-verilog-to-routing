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
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
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
   printHandler.Write( pfile, spaceLen, "<fabric name=\"%s\">\n",
                                        TIO_SR_STR( this->srName ));
   printHandler.Write( pfile, spaceLen, "\n" );

   spaceLen += 3;

   this->config.Print( pfile, spaceLen );
   printHandler.Write( pfile, spaceLen, "\n" );

   if( this->inputOutputList.IsValid( ))
   {
      this->inputOutputList.Print( pfile, spaceLen );
      printHandler.Write( pfile, spaceLen, "\n" );
   }
   if( this->physicalBlockList.IsValid( ))
   {
      this->physicalBlockList.Print( pfile, spaceLen );
      printHandler.Write( pfile, spaceLen, "\n" );
   }
   if( this->switchBoxList.IsValid( ))
   {
      this->switchBoxList.Print( pfile, spaceLen );
      printHandler.Write( pfile, spaceLen, "\n" );
   }
   if( this->channelList.IsValid( ))
   {
      this->channelList.Print( pfile, spaceLen );
      printHandler.Write( pfile, spaceLen, "\n" );
   }
   if( this->segmentList.IsValid( ))
   {
      this->segmentList.Print( pfile, spaceLen );
      printHandler.Write( pfile, spaceLen, "\n" );
   }
   spaceLen -= 3;
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
   this->config.ioPolygon.Reset( );

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
   return(( this->config.IsValid( ) ||
          ( this->physicalBlockList.IsValid( ) ||
            this->inputOutputList.IsValid( ) ||
            this->switchBoxList.IsValid( ) ||
            this->channelList.IsValid( ) ||
            this->segmentList.IsValid( ))) ? 
          true : false );
}
