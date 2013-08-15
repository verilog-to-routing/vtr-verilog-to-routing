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
