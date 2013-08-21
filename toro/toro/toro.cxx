//===========================================================================//
//     _______  _______  ____    _______         ___    __  ____    ____ 
//    /__  __/ / ___  / /__  \  / ___  /         | |   / / /__  \  /__  \
//      / /   / /  / /  ___) / / /  / /   _____  | |  / /  ___) / ___)  /
//     / /   / /  / / /   __/ / /  / /   /____/  | | / / /  ___/ /   __/
//    / /   / /__/ / / /\ \  / /__/ /            | |/ / / /     / /\ \ 
//   /_/   /______/ /_/ /_/ /______/             |___/ /_/     /_/ /_/
// 
//===========================================================================//
//
// Purpose : Main for 'Toro* - The Next Generation VPR'.  This function
//           is responsible for capturing command-line arguments, then 
//           invoking the master input-processing-output object.
//
//           Functions include:
//           - main
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

#include "TIO_SkinHandler.h"
#include "TIO_PrintHandler.h"

#include "TCL_CommandLine.h"

#include "TM_Master.h"

//===========================================================================//
// Function       : main
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
int main( int argc, char *argv[] )
{
   bool ok = true;

   // Initialize a skin handler 'singleton' based on "VPR" or "toro*" mode
   TIO_SkinHandler_c::NewInstance( );
   TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );
   // skinHandler.Set( TIO_SkinHandler_c::TIO_SKIN_VPR );
   skinHandler.Set( TIO_SkinHandler_c::TIO_SKIN_TORO );

   // Initialize a print handler 'singleton' using default standard output
   TIO_PrintHandler_c::NewInstance( );
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.SetStdioOutput( stdout );

   // Capture and remember any user command-line arguments
   TCL_CommandLine_c commandLine( argc, argv, &ok );
   if( ok )
   {
      // Invoke the master tool and see where it takes us today!
      TM_Master_c master( commandLine, &ok );

      // All done, time to tell user we are "Gettin' the heck outta Dodge"...
      printHandler.Info( "Exiting...\n" );
   }
   TIO_PrintHandler_c::DeleteInstance( ); // So long, and thanks for the fish!
   TIO_SkinHandler_c::DeleteInstance( );  // Yes, Elvis has left the building!

   return( ok ? 0 : 1 );
}
