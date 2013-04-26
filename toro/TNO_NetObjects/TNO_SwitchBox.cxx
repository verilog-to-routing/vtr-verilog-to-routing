//===========================================================================//
//Purpose : Method definitions for the TNO_SwitchBox class.
//
//           Public methods include:
//           - TNO_SwitchBox_c, ~TNO_SwitchBox_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
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

#include "TC_StringUtils.h"

#include "TNO_SwitchBox.h"

//===========================================================================//
// Method         : TNO_SwitchBox_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_SwitchBox_c::TNO_SwitchBox_c( 
      void )
{
} 

//===========================================================================//
TNO_SwitchBox_c::TNO_SwitchBox_c( 
      const string& srName )
      :
      srName_( srName )
{
} 

//===========================================================================//
TNO_SwitchBox_c::TNO_SwitchBox_c( 
      const char* pszName )
      :
      srName_( TIO_PSZ_STR( pszName ))
{
} 

//===========================================================================//
TNO_SwitchBox_c::TNO_SwitchBox_c( 
      const string&         srName,
      const TC_SideIndex_c& input,
      const TC_SideIndex_c& output )
      :
      srName_( srName ),
      input_( input ),
      output_( output )
{
} 

//===========================================================================//
TNO_SwitchBox_c::TNO_SwitchBox_c( 
      const char*           pszName,
      const TC_SideIndex_c& input,
      const TC_SideIndex_c& output )
      :
      srName_( TIO_PSZ_STR( pszName )),
      input_( input ),
      output_( output )
{
} 

//===========================================================================//
TNO_SwitchBox_c::TNO_SwitchBox_c( 
      const string&       srName,
            TC_SideMode_t inputSide,
            size_t        inputIndex,
            TC_SideMode_t outputSide,
            size_t        outputIndex )
      :
      srName_( srName ),
      input_( inputSide, inputIndex ),
      output_( outputSide, outputIndex )
{
} 

//===========================================================================//
TNO_SwitchBox_c::TNO_SwitchBox_c( 
      const char*         pszName,
            TC_SideMode_t inputSide,
            size_t        inputIndex,
            TC_SideMode_t outputSide,
            size_t        outputIndex )
      :
      srName_( TIO_PSZ_STR( pszName )),
      input_( inputSide, inputIndex ),
      output_( outputSide, outputIndex )
{
} 

//===========================================================================//
TNO_SwitchBox_c::TNO_SwitchBox_c( 
      const TNO_SwitchBox_c& switchBox )
      :
      srName_( switchBox.srName_ ),
      input_( switchBox.input_ ),
      output_( switchBox.output_ )
{
} 

//===========================================================================//
// Method         : ~TNO_SwitchBox_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_SwitchBox_c::~TNO_SwitchBox_c( 
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
TNO_SwitchBox_c& TNO_SwitchBox_c::operator=( 
      const TNO_SwitchBox_c& switchBox )
{
   if( &switchBox != this )
   {
      this->srName_ = switchBox.srName_;
      this->input_ = switchBox.input_;
      this->output_ = switchBox.output_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_SwitchBox_c::operator<( 
      const TNO_SwitchBox_c& switchBox ) const
{
   return(( TC_CompareStrings( this->srName_, switchBox.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_SwitchBox_c::operator==( 
      const TNO_SwitchBox_c& switchBox ) const
{
   return(( this->srName_ == switchBox.srName_ ) &&
          ( this->input_ == switchBox.input_ ) &&
          ( this->output_ == switchBox.output_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_SwitchBox_c::operator!=( 
      const TNO_SwitchBox_c& switchBox ) const
{
   return( !this->operator==( switchBox ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_SwitchBox_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srSwitchBox;
   this->ExtractString( &srSwitchBox );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srSwitchBox ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_SwitchBox_c::ExtractString( 
      string* psrSwitchBox ) const
{
   if( psrSwitchBox )
   {
      if( this->IsValid( ))
      {
         string srInput, srOutput;
         this->input_.ExtractString( &srInput );
         this->output_.ExtractString( &srOutput );

         *psrSwitchBox = "<sb ";
         *psrSwitchBox += "name=\"";
         *psrSwitchBox += this->srName_;
         *psrSwitchBox += "\"> ";
         *psrSwitchBox += "<sides>";
         *psrSwitchBox += " ";
         *psrSwitchBox += srInput;
         *psrSwitchBox += " ";
         *psrSwitchBox += srOutput;
         *psrSwitchBox += " ";
         *psrSwitchBox += "</sides> ";
         *psrSwitchBox += "</sb>";
      }
      else
      {
         *psrSwitchBox = "?";
      }
   }
} 

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_SwitchBox_c::Clear( 
      void )
{
   this->srName_ = "";
   this->input_.Clear( );
   this->output_.Clear( );
}
