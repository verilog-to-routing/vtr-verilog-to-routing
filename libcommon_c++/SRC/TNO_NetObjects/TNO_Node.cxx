//===========================================================================//
//Purpose : Method definitions for the TNO_Node class.
//
//           Public methods include:
//           - TNO_Node_c, ~TNO_Node_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - Set
//           - Clear
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
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

#include "TNO_StringUtils.h"
#include "TNO_Node.h"

//===========================================================================//
// Method         : TNO_Node_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_Node_c::TNO_Node_c( 
      void )
      :
      type_( TNO_NODE_UNDEFINED )
{
} 

//===========================================================================//
TNO_Node_c::TNO_Node_c( 
      const TNO_InstPin_c& instPin )
      :
      type_( TNO_NODE_INST_PIN ),
      instPin_( instPin )
{
} 

//===========================================================================//
TNO_Node_c::TNO_Node_c( 
      const TNO_Segment_c& segment )
      :
      type_( TNO_NODE_SEGMENT ),
      segment_( segment )
{
} 

//===========================================================================//
TNO_Node_c::TNO_Node_c( 
      const TNO_SwitchBox_c& switchBox )
      :
      type_( TNO_NODE_SWITCH_BOX ),
      switchBox_( switchBox )
{
} 

//===========================================================================//
TNO_Node_c::TNO_Node_c( 
      const TNO_Node_c& node )
      :
      type_( node.type_ ),
      instPin_( node.instPin_ ),
      segment_( node.segment_ ),
      switchBox_( node.switchBox_ )
{
} 

//===========================================================================//
// Method         : ~TNO_Node_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_Node_c::~TNO_Node_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_Node_c& TNO_Node_c::operator=( 
      const TNO_Node_c& node )
{
   if( &node != this )
   {
      this->type_ = node.type_;
      this->instPin_ = node.instPin_;
      this->segment_ = node.segment_;
      this->switchBox_ = node.switchBox_;
   }
   return( *this ); 
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_Node_c::operator==( 
      const TNO_Node_c& node ) const
{
   return(( this->type_ == node.type_ ) &&
          ( this->instPin_ == node.instPin_ ) &&
          ( this->segment_ == node.segment_ ) &&
          ( this->switchBox_ == node.switchBox_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_Node_c::operator!=( 
      const TNO_Node_c& node ) const
{
   return( !this->operator==( node ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Node_c::Print( 
      FILE*  pfile,
      size_t spaceLen,
      bool   verbose ) const
{
   if( verbose )
   {
      string srType;
      TNO_ExtractStringNodeType( this->type_, &srType );

      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      printHandler.Write( pfile, spaceLen, "type=\"%s\"\n", TIO_SR_STR( srType ));
   }

   switch( this->type_ )
   {
   case TNO_NODE_INST_PIN: 
      this->instPin_.Print( pfile, spaceLen + 3 );
      break;
   case TNO_NODE_SEGMENT: 
      this->segment_.Print( pfile, spaceLen + 3 );
      break;
   case TNO_NODE_SWITCH_BOX:
      this->switchBox_.Print( pfile, spaceLen + 3 );
      break;
   default:
      break;
   }
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Node_c::Set( 
      const TNO_InstPin_c& instPin )
{
   this->Clear( );

   this->type_ = TNO_NODE_INST_PIN;
   this->instPin_ = instPin;
} 

//===========================================================================//
void TNO_Node_c::Set( 
      const TNO_Segment_c& segment )
{
   this->Clear( );

   this->type_ = TNO_NODE_SEGMENT;
   this->segment_ = segment;
} 

//===========================================================================//
void TNO_Node_c::Set( 
      const TNO_SwitchBox_c& switchBox )
{
   this->Clear( );

   this->type_ = TNO_NODE_SWITCH_BOX;
   this->switchBox_ = switchBox;
} 

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_Node_c::Clear( 
      void )
{
   this->type_ = TNO_NODE_UNDEFINED;

   this->instPin_.Clear( );
   this->segment_.Clear( );
   this->switchBox_.Clear( );
} 
