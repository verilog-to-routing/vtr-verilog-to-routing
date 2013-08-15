//===========================================================================//
// Purpose : Method definitions for the TIO_SkinHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - Set
//
//           Protected methods include:
//           - TIO_SkinHandler_c, ~TIO_SkinHandler_c
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

#if defined( SUN8 ) || defined( SUN10 )
   #include <time.h>
#endif

#include "TC_Typedefs.h"

#include "TIO_StringText.h"

#include "TIO_SkinHandler.h"

// Initialize the user execute "singleton" class, as needed
TIO_SkinHandler_c* TIO_SkinHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TIO_SkinHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TIO_SkinHandler_c::TIO_SkinHandler_c( 
      void )
      :
      skinMode_( TIO_SkinHandler_c::TIO_SKIN_UNDEFINED )
{
   pinstance_ = 0;
}

//===========================================================================//
// Method         : TIO_SkinHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TIO_SkinHandler_c::~TIO_SkinHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_SkinHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TIO_SkinHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_SkinHandler_c::DeleteInstance( 
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
// 05/01/12 jeffr : Original
//===========================================================================//
TIO_SkinHandler_c& TIO_SkinHandler_c::GetInstance(
      void )
{
   if( !pinstance_ )
   {
      NewInstance( );
   }
   return( *pinstance_ );
}

//===========================================================================//
// Method         : HasInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_SkinHandler_c::HasInstance(
      void )
{
   return( pinstance_ ? true : false );
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_SkinHandler_c::Set(
      int skinMode )
{
   this->skinMode_ = skinMode;

   switch( this->skinMode_ )
   {
   case TIO_SkinHandler_c::TIO_SKIN_VPR:

      this->srProgramName_ = TIO_SZ_VPR_PROGRAM_NAME;
      this->srProgramTitle_ = TIO_SZ_VPR_PROGRAM_TITLE;
      this->srBinaryName_ = TIO_SZ_VPR_BINARY_NAME;
      this->srSourceName_ = TIO_SZ_VPR_SOURCE_NAME;
      this->srCopyright_ = TIO_SZ_VPR_COPYRIGHT;
      break;

   case TIO_SkinHandler_c::TIO_SKIN_TORO:

      this->srProgramName_ = TIO_SZ_TORO_PROGRAM_NAME;
      this->srProgramTitle_ = TIO_SZ_TORO_PROGRAM_TITLE;
      this->srBinaryName_ = TIO_SZ_TORO_BINARY_NAME;
      this->srSourceName_ = TIO_SZ_TORO_SOURCE_NAME;
      this->srCopyright_ = TIO_SZ_TORO_COPYRIGHT;
      break;
   }
}
