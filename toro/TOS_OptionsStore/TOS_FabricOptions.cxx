//===========================================================================//
// Purpose : Method definitions for the TOS_FabricOptions class.
//
//           Public methods include:
//           - TOS_FabricOptions_c, ~TOS_FabricOptions_c
//           - Print
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

#include "TIO_PrintHandler.h"

#include "TOS_StringUtils.h"
#include "TOS_FabricOptions.h"

//===========================================================================//
// Method         : TOS_FabricOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/12/13 jeffr : Original
//===========================================================================//
TOS_FabricOptions_c::TOS_FabricOptions_c( 
      void )
{
   this->blocks.override = false;
   this->switchBoxes.override = false;
   this->connectionBlocks.override = false;
   this->channels.override = false;
}

//===========================================================================//
TOS_FabricOptions_c::TOS_FabricOptions_c( 
      bool blocks_override_,
      bool switchBoxes_override_,
      bool connectionBlocks_override_,
      bool channels_override_ )
{
   this->blocks.override = blocks_override_;
   this->switchBoxes.override = switchBoxes_override_;
   this->connectionBlocks.override = connectionBlocks_override_;
   this->channels.override = channels_override_;
}

//===========================================================================//
// Method         : ~TOS_FabricOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/12/13 jeffr : Original
//===========================================================================//
TOS_FabricOptions_c::~TOS_FabricOptions_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/12/13 jeffr : Original
//===========================================================================//
void TOS_FabricOptions_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "FABRIC_BLOCK_ENABLE        = %s\n", TIO_BOOL_STR( this->blocks.override ));
   printHandler.Write( pfile, spaceLen, "FABRIC_SB_ENABLE           = %s\n", TIO_BOOL_STR( this->connectionBlocks.override ));
   printHandler.Write( pfile, spaceLen, "FABRIC_CB_ENABLE           = %s\n", TIO_BOOL_STR( this->connectionBlocks.override ));
   printHandler.Write( pfile, spaceLen, "FABRIC_CHANNEL_ENABLE      = %s\n", TIO_BOOL_STR( this->channels.override ));
}
