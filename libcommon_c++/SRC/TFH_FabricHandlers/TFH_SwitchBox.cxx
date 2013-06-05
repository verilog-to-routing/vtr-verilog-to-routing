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
