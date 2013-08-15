//===========================================================================//
// Purpose : Method definitions for the TFS_FloorplanStore class.
//
//           Public methods include:
//           - TFS_FloorplanStore_c, ~TFS_FloorplanStore_c
//           - operator=
//           - operator==, operator!=
//           - Print
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

#include "TFS_FloorplanStore.h"

//===========================================================================//
// Method         : TFS_FloorplanStore_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFS_FloorplanStore_c::TFS_FloorplanStore_c( 
      void )
{
} 

//===========================================================================//
TFS_FloorplanStore_c::TFS_FloorplanStore_c( 
      const TFS_FloorplanStore_c& floorplanStore )
      :
      architectureSpec( floorplanStore.architectureSpec ),
      fabricModel( floorplanStore.fabricModel ),
      circuitDesign( floorplanStore.circuitDesign )
{
} 

//===========================================================================//
// Method         : ~TFS_FloorplanStore_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFS_FloorplanStore_c::~TFS_FloorplanStore_c( 
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
TFS_FloorplanStore_c& TFS_FloorplanStore_c::operator=( 
      const TFS_FloorplanStore_c& floorplanStore )
{
   if( &floorplanStore != this )
   {
      this->architectureSpec = floorplanStore.architectureSpec;
      this->fabricModel = floorplanStore.fabricModel;
      this->circuitDesign = floorplanStore.circuitDesign;
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
bool TFS_FloorplanStore_c::operator==( 
      const TFS_FloorplanStore_c& floorplanStore ) const
{
   return(( this->architectureSpec == floorplanStore.architectureSpec ) &&
          ( this->fabricModel == floorplanStore.fabricModel ) &&
          ( this->circuitDesign == floorplanStore.circuitDesign ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFS_FloorplanStore_c::operator!=( 
      const TFS_FloorplanStore_c& floorplanStore ) const
{
   return( !this->operator==( floorplanStore ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TFS_FloorplanStore_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   this->architectureSpec.Print( pfile, spaceLen );
   this->fabricModel.Print( pfile, spaceLen );
   this->circuitDesign.Print( pfile, spaceLen );
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFS_FloorplanStore_c::IsValid(
      void ) const
{
   return(( this->architectureSpec.IsValid( )) &&
          ( this->fabricModel.IsValid( )) &&
          ( this->circuitDesign.IsValid( )) ?
          true : false );
}
