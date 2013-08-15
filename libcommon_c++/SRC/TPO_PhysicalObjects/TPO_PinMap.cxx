//===========================================================================//
// Purpose : Method definitions for the TPO_PinMap class.
//
//           Public methods include:
//           - TPO_PinMap_c, ~TPO_PinMap_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - Set
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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TPO_PinMap.h"

//===========================================================================//
// Method         : TPO_PinMap_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_PinMap_c::TPO_PinMap_c( 
      void )
      :
      type_( TC_TYPE_UNDEFINED )
{
} 

//===========================================================================//
TPO_PinMap_c::TPO_PinMap_c( 
      const string&       srInstPinName,
      const string&       srCellPinName,
            TC_TypeMode_t type )
      :
      srInstPinName_( srInstPinName ),
      srCellPinName_( srCellPinName ),
      type_( type )
{
} 

//===========================================================================//
TPO_PinMap_c::TPO_PinMap_c( 
      const char*         pszInstPinName,
      const char*         pszCellPinName,
            TC_TypeMode_t type )
      :
      srInstPinName_( TIO_PSZ_STR( pszInstPinName )),
      srCellPinName_( TIO_PSZ_STR( pszCellPinName )),
      type_( type )
{
} 

//===========================================================================//
TPO_PinMap_c::TPO_PinMap_c( 
      const TPO_PinMap_c& pinMap )
      :
      srInstPinName_( pinMap.srInstPinName_ ),
      srCellPinName_( pinMap.srCellPinName_ ),
      type_( pinMap.type_ )
{
} 

//===========================================================================//
// Method         : ~TPO_PinMap_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_PinMap_c::~TPO_PinMap_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_PinMap_c& TPO_PinMap_c::operator=( 
      const TPO_PinMap_c& pinMap )
{
   if( &pinMap != this )
   {
      this->srInstPinName_ = pinMap.srInstPinName_;
      this->srCellPinName_ = pinMap.srCellPinName_;
      this->type_ = pinMap.type_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_PinMap_c::operator<( 
      const TPO_PinMap_c& pinMap ) const
{
   return(( TC_CompareStrings( this->srInstPinName_, pinMap.srInstPinName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_PinMap_c::operator==( 
      const TPO_PinMap_c& pinMap ) const
{
   return(( this->srInstPinName_ == pinMap.srInstPinName_ ) &&
          ( this->srCellPinName_ == pinMap.srCellPinName_ ) &&
          ( this->type_ == pinMap.type_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_PinMap_c::operator!=( 
      const TPO_PinMap_c& pinMap ) const
{
   return( !this->operator==( pinMap ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_PinMap_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srType;
   TC_ExtractStringTypeMode( this->type_, &srType );
   printHandler.Write( pfile, spaceLen, "\"%s\"=\"%s\" %s\n",
                                        TIO_SR_STR( this->srInstPinName_ ),
                                        TIO_SR_STR( this->srCellPinName_ ),
                                        TIO_SR_STR( srType ));
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_PinMap_c::Set(
      const string&       srInstPinName,
      const string&       srCellPinName,
            TC_TypeMode_t type )
{
   this->srInstPinName_ = srInstPinName;
   this->srCellPinName_ = srCellPinName;
   this->type_ = type;
} 

//===========================================================================//
void TPO_PinMap_c::Set(
      const char*         pszInstPinName,
      const char*         pszCellPinName,
            TC_TypeMode_t type )
{
   this->srInstPinName_ = TIO_PSZ_STR( pszInstPinName );
   this->srCellPinName_ = TIO_PSZ_STR( pszCellPinName );
   this->type_ = type;
} 
