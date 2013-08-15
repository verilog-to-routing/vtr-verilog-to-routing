//===========================================================================//
// Purpose : Method definitions for the TFH_SwitchBox class.
//
//           Public methods include:
//           - TFH_SwitchBox_c, ~TFH_SwitchBox_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//
//===========================================================================//

#include "TC_StringUtils.h"

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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TFH_StringUtils.h"
#include "TFH_SwitchBox.h"

//===========================================================================//
// Method         : TFH_SwitchBox_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/14/13 jeffr : Original
//===========================================================================//
TFH_SwitchBox_c::TFH_SwitchBox_c( 
      void )
{
} 

//===========================================================================//
TFH_SwitchBox_c::TFH_SwitchBox_c( 
      int x,
      int y )
      :
      vpr_gridPoint_( x, y )
{
} 

//===========================================================================//
TFH_SwitchBox_c::TFH_SwitchBox_c( 
      const TGO_Point_c& vpr_gridPoint )
      :
      vpr_gridPoint_( vpr_gridPoint )
{
} 

//===========================================================================//
TFH_SwitchBox_c::TFH_SwitchBox_c( 
      const TGO_Point_c&   vpr_gridPoint,
      const string&        srSwitchBoxName,
      const TC_MapTable_c& mapTable )
      :
      vpr_gridPoint_( vpr_gridPoint ),
      srSwitchBoxName_( srSwitchBoxName ),
      mapTable_( mapTable )
{
} 

//===========================================================================//
TFH_SwitchBox_c::TFH_SwitchBox_c( 
      const TGO_Point_c&   vpr_gridPoint,
      const char*          pszSwitchBoxName,
      const TC_MapTable_c& mapTable )
      :
      vpr_gridPoint_( vpr_gridPoint ),
      srSwitchBoxName_( TIO_PSZ_STR( pszSwitchBoxName )),
      mapTable_( mapTable )
{
} 

//===========================================================================//
TFH_SwitchBox_c::TFH_SwitchBox_c( 
      const TFH_SwitchBox_c& switchBox )
      :
      vpr_gridPoint_( switchBox.vpr_gridPoint_ ),
      srSwitchBoxName_( switchBox.srSwitchBoxName_ ),
      mapTable_( switchBox.mapTable_ )
{
} 

//===========================================================================//
// Method         : ~TFH_SwitchBox_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/14/13 jeffr : Original
//===========================================================================//
TFH_SwitchBox_c::~TFH_SwitchBox_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/14/13 jeffr : Original
//===========================================================================//
TFH_SwitchBox_c& TFH_SwitchBox_c::operator=( 
      const TFH_SwitchBox_c& switchBox )
{
   if( &switchBox != this )
   {
      this->vpr_gridPoint_ = switchBox.vpr_gridPoint_;
      this->srSwitchBoxName_ = switchBox.srSwitchBoxName_;
      this->mapTable_ = switchBox.mapTable_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/14/13 jeffr : Original
//===========================================================================//
bool TFH_SwitchBox_c::operator<( 
      const TFH_SwitchBox_c& switchBox ) const
{
   return( this->vpr_gridPoint_.operator<( switchBox.vpr_gridPoint_ ));
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/14/13 jeffr : Original
//===========================================================================//
bool TFH_SwitchBox_c::operator==( 
      const TFH_SwitchBox_c& switchBox ) const
{
   return(( this->vpr_gridPoint_ == switchBox.vpr_gridPoint_ ) &&
          ( this->srSwitchBoxName_ == switchBox.srSwitchBoxName_ ) &&
          ( this->mapTable_ == switchBox.mapTable_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/14/13 jeffr : Original
//===========================================================================//
bool TFH_SwitchBox_c::operator!=( 
      const TFH_SwitchBox_c& switchBox ) const
{
   return( !this->operator==( switchBox ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/14/13 jeffr : Original
//===========================================================================//
void TFH_SwitchBox_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<grid" );

   if( this->vpr_gridPoint_.IsValid( ))
   {
      string srGridPoint;
      this->vpr_gridPoint_.ExtractString( &srGridPoint );
      printHandler.Write( pfile, 0, " point=(%s)", TIO_SR_STR( srGridPoint ));
   }
   if( this->srSwitchBoxName_.length( ))
   {
      printHandler.Write( pfile, 0, " switch_box_name=\"%s\"", TIO_SR_STR( this->srSwitchBoxName_ ));
   }
   printHandler.Write( pfile, 0, ">\n" );

   if( this->mapTable_.IsValid( ))
   {
      spaceLen += 3;
      printHandler.Write( pfile, spaceLen, " <map_table>\n" );
      this->mapTable_.Print( pfile, spaceLen+3 );
      printHandler.Write( pfile, spaceLen, " </map_table>\n" );
   }
} 
