//===========================================================================//
// Purpose : Method definitions for the TOS_ExecuteOptions class.
//
//           Public methods include:
//           - TOS_ExecuteOptions_c, ~TOS_ExecuteOptions_c
//           - Print
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

#include "TOS_StringUtils.h"
#include "TOS_ExecuteOptions.h"

//===========================================================================//
// Method         : TOS_ExecuteOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_ExecuteOptions_c::TOS_ExecuteOptions_c( 
      void )
      :
      maxWarningCount( 0 ),
      maxErrorCount( 0 ),
      toolMask( 0 )
{
   this->tiClay.resyncNets = true;
   this->tiClay.freeNets = true;
}

//===========================================================================//
TOS_ExecuteOptions_c::TOS_ExecuteOptions_c( 
      unsigned long maxWarningCount_,
      unsigned long maxErrorCount_,
      int           toolMask_ )
      :
      maxWarningCount( maxWarningCount_ ),
      maxErrorCount( maxErrorCount_ ),
      toolMask( toolMask_ )
{
   this->tiClay.resyncNets = true;
   this->tiClay.freeNets = true;
}

//===========================================================================//
// Method         : ~TOS_ExecuteOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_ExecuteOptions_c::~TOS_ExecuteOptions_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_ExecuteOptions_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srToolMask;
   TOS_ExtractStringExecuteToolMask( this->toolMask, &srToolMask );

   printHandler.Write( pfile, spaceLen, "HALT_MAX_WARNINGS          = %ld\n", static_cast<long>( this->maxWarningCount ));
   printHandler.Write( pfile, spaceLen, "HALT_MAX_ERRORS            = %ld\n", static_cast<long>( this->maxErrorCount ));

   printHandler.Write( pfile, spaceLen, "\n" );
   printHandler.Write( pfile, spaceLen, "EXECUTE_MODE               = { %s}\n", TIO_SR_STR( srToolMask ));

   printHandler.Write( pfile, spaceLen, "\n" );
   printHandler.Write( pfile, spaceLen, "CLAY_RESYNC_VPR_NETS       = %s\n", TIO_BOOL_STR( this->tiClay.resyncNets ));
   printHandler.Write( pfile, spaceLen, "CLAY_FREE_VPR_NETS         = %s\n", TIO_BOOL_STR( this->tiClay.freeNets ));
}
