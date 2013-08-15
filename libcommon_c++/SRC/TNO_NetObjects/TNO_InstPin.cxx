//===========================================================================//
// Purpose : Method definitions for the TNO_InstPin class.
//
//           Public methods include:
//           - TNO_InstPin_c, ~TNO_InstPin_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set
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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TNO_Typedefs.h"
#include "TNO_InstPin.h"

//===========================================================================//
// Method         : TNO_InstPin_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TNO_InstPin_c::TNO_InstPin_c( 
      void )
      :
      portIndex_( TNO_PORT_INDEX_INVALID ),
      pinIndex_( TNO_PIN_INDEX_INVALID ),
      type_( TC_TYPE_UNDEFINED )
{
} 

//===========================================================================//
TNO_InstPin_c::TNO_InstPin_c( 
      const string&       srInstName,
      const string&       srPortName,
            size_t        portIndex,
      const string&       srPinName,
            size_t        pinIndex,
            TC_TypeMode_t type )
      :
      portIndex_( TNO_PORT_INDEX_INVALID ),
      pinIndex_( TNO_PIN_INDEX_INVALID ),
      type_( TC_TYPE_UNDEFINED )
{
   this->Set( srInstName, srPortName, portIndex, srPinName, pinIndex, type );
} 

//===========================================================================//
TNO_InstPin_c::TNO_InstPin_c( 
      const char*         pszInstName,
      const char*         pszPortName,
            size_t        portIndex,
      const char*         pszPinName,
            size_t        pinIndex,
            TC_TypeMode_t type )
      :
      portIndex_( TNO_PORT_INDEX_INVALID ),
      pinIndex_( TNO_PIN_INDEX_INVALID ),
      type_( TC_TYPE_UNDEFINED )
{
   this->Set( pszInstName, pszPortName, portIndex, pszPinName, pinIndex, type );
} 

//===========================================================================//
TNO_InstPin_c::TNO_InstPin_c( 
      const string&       srInstName,
      const string&       srPortName,
      const string&       srPinName,
            TC_TypeMode_t type )
      :
      portIndex_( TNO_PORT_INDEX_INVALID ),
      pinIndex_( TNO_PIN_INDEX_INVALID ),
      type_( TC_TYPE_UNDEFINED )
{
   this->Set( srInstName, srPortName, srPinName, type );
} 

//===========================================================================//
TNO_InstPin_c::TNO_InstPin_c( 
      const char*         pszInstName,
      const char*         pszPortName,
      const char*         pszPinName,
            TC_TypeMode_t type )
      :
      portIndex_( TNO_PORT_INDEX_INVALID ),
      pinIndex_( TNO_PIN_INDEX_INVALID ),
      type_( TC_TYPE_UNDEFINED )
{
   this->Set( pszInstName, pszPortName, pszPinName, type );
} 

//===========================================================================//
TNO_InstPin_c::TNO_InstPin_c( 
      const string&       srInstName,
      const string&       srPinName,
            TC_TypeMode_t type )
      :
      type_( TC_TYPE_UNDEFINED )
{
   this->Set( srInstName, srPinName, type );
} 

//===========================================================================//
TNO_InstPin_c::TNO_InstPin_c( 
      const char*         pszInstName,
      const char*         pszPinName,
            TC_TypeMode_t type )
      :
      type_( TC_TYPE_UNDEFINED )
{
   this->Set( pszInstName, pszPinName, type );
} 

//===========================================================================//
TNO_InstPin_c::TNO_InstPin_c( 
      const TNO_InstPin_c& instPin )
      :
      srName_( instPin.srName_ ),
      srInstName_( instPin.srInstName_ ),
      srPortName_( instPin.srPortName_ ),
      srPinName_( instPin.srPinName_ ),
      portIndex_( instPin.portIndex_ ),
      pinIndex_( instPin.pinIndex_ ),
      type_( instPin.type_ )
{
} 

//===========================================================================//
// Method         : ~TNO_InstPin_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TNO_InstPin_c::~TNO_InstPin_c( 
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
TNO_InstPin_c& TNO_InstPin_c::operator=( 
      const TNO_InstPin_c& instPin )
{
   if( &instPin != this )
   {
      this->srName_ = instPin.srName_;
      this->srInstName_ = instPin.srInstName_;
      this->srPortName_ = instPin.srPortName_;
      this->srPinName_ = instPin.srPinName_;
      this->portIndex_ = instPin.portIndex_;
      this->pinIndex_ = instPin.pinIndex_;
      this->type_ = instPin.type_;
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
bool TNO_InstPin_c::operator<( 
      const TNO_InstPin_c& instPin ) const
{
   return(( TC_CompareStrings( this->srName_, instPin.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TNO_InstPin_c::operator==( 
      const TNO_InstPin_c& instPin ) const
{
   return(( this->srName_ == instPin.srName_ ) &&
          ( this->srInstName_ == instPin.srInstName_ ) &&
          ( this->srPortName_ == instPin.srPortName_ ) &&
          ( this->srPinName_ == instPin.srPinName_ ) &&
          ( this->portIndex_ == instPin.portIndex_ ) &&
          ( this->pinIndex_ == instPin.pinIndex_ ) &&
          ( this->type_ == instPin.type_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TNO_InstPin_c::operator!=( 
      const TNO_InstPin_c& instPin ) const
{
   return( !this->operator==( instPin ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TNO_InstPin_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srInstPin;
   this->ExtractString( &srInstPin );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srInstPin ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TNO_InstPin_c::ExtractString( 
      string* psrInstPin ) const
{
   if( psrInstPin )
   {
      if( this->IsValid( ))
      {
         *psrInstPin = "<pin";
         if( this->srInstName_.length( ))
         {
            *psrInstPin += psrInstPin->length( ) ? " " : "";
            *psrInstPin += "inst=\"";
            *psrInstPin += this->srInstName_;
            *psrInstPin += "\"";
         }
         if( this->srPortName_.length( ))
         {
            string srPortNameIndex;
            TC_FormatStringNameIndex( this->srPortName_, this->portIndex_, 
                                      &srPortNameIndex );

            *psrInstPin += psrInstPin->length( ) ? " " : "";
            *psrInstPin += "port=\"";
            *psrInstPin += srPortNameIndex;
            *psrInstPin += "\"";
         }
         if( this->srPinName_.length( ))
         {
            string srPinNameIndex;
            TC_FormatStringNameIndex( this->srPinName_, this->pinIndex_, 
                                      &srPinNameIndex );

            *psrInstPin += psrInstPin->length( ) ? " " : "";
            *psrInstPin += "pin=\"";
            *psrInstPin += srPinNameIndex;
            *psrInstPin += "\"";
         }

         if( this->type_ != TC_TYPE_UNDEFINED )
         {
            string srType;
            TC_ExtractStringTypeMode( this->type_, &srType );

            *psrInstPin += psrInstPin->length( ) ? " " : "";
            *psrInstPin += "type=\"";
            *psrInstPin += srType;
            *psrInstPin += "\"";
         }
         *psrInstPin += "/>";
      }
      else
      {
         *psrInstPin = "?";
      }
   }
} 

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TNO_InstPin_c::Set( 
      const string&       srInstName,
      const string&       srPortName,
            size_t        portIndex,
      const string&       srPinName,
            size_t        pinIndex,
            TC_TypeMode_t type )
{
   const char* pszInstName = ( srInstName.length( ) ? srInstName.data( ) : 0 );
   const char* pszPortName = ( srPortName.length( ) ? srPortName.data( ) : 0 );
   const char* pszPinName = ( srPinName.length( ) ? srPinName.data( ) : 0 );

   this->Set( pszInstName, pszPortName, portIndex, pszPinName, pinIndex, type );
} 

//===========================================================================//
void TNO_InstPin_c::Set( 
      const char*         pszInstName,
      const char*         pszPortName,
            size_t        portIndex,
      const char*         pszPinName,
            size_t        pinIndex,
            TC_TypeMode_t type )
{
   this->srInstName_ = TIO_PSZ_STR( pszInstName );
   this->srPortName_ = TIO_PSZ_STR( pszPortName );
   this->srPinName_ = TIO_PSZ_STR( pszPinName );
   this->portIndex_ = portIndex;
   this->pinIndex_ = pinIndex;
   this->type_ = type;

   this->srName_ = "";
   this->srName_ += this->srInstName_;
   if( pszPortName ) 
   {
      string srPortNameIndex;
      TC_FormatStringNameIndex( this->srPortName_, this->portIndex_, 
                                &srPortNameIndex );
      this->srName_ += "|";
      this->srName_ += srPortNameIndex;
   }
   if( pszPinName ) 
   {
      string srPinNameIndex;
      TC_FormatStringNameIndex( this->srPinName_, this->pinIndex_, 
                                &srPinNameIndex );
      this->srName_ += "|";
      this->srName_ += srPinNameIndex;
   }
} 

//===========================================================================//
void TNO_InstPin_c::Set( 
      const string&       srInstName,
      const string&       srPortName,
      const string&       srPinName,
            TC_TypeMode_t type )
{
   const char* pszInstName = ( srInstName.length( ) ? srInstName.data( ) : 0 );
   const char* pszPortName = ( srPortName.length( ) ? srPortName.data( ) : 0 );
   const char* pszPinName = ( srPinName.length( ) ? srPinName.data( ) : 0 );

   this->Set( pszInstName, pszPortName, pszPinName, type );
} 

//===========================================================================//
void TNO_InstPin_c::Set( 
      const char*         pszInstName,
      const char*         pszPortName,
      const char*         pszPinName,
            TC_TypeMode_t type )
{
   string srPortName( "" );
   string srPinName( "" );
   size_t portIndex = TNO_PORT_INDEX_INVALID;
   size_t pinIndex = TNO_PIN_INDEX_INVALID;

   TC_ParseStringNameIndex( pszPortName, &srPortName, &portIndex );
   TC_ParseStringNameIndex( pszPinName, &srPinName, &pinIndex );

   pszPortName = ( srPortName.length( ) ? srPortName.data( ) : 0 );
   pszPinName = ( srPinName.length( ) ? srPinName.data( ) : 0 );

   this->Set( pszInstName, pszPortName, portIndex, pszPinName, pinIndex, type );
} 

//===========================================================================//
void TNO_InstPin_c::Set( 
      const string&       srInstName,
      const string&       srPinName,
            TC_TypeMode_t type )
{
   const char* pszInstName = ( srInstName.length( ) ? srInstName.data( ) : 0 );
   const char* pszPinName = ( srPinName.length( ) ? srPinName.data( ) : 0 );

   this->Set( pszInstName, pszPinName, type );
} 

//===========================================================================//
void TNO_InstPin_c::Set( 
      const char*         pszInstName,
      const char*         pszPinName,
            TC_TypeMode_t type )
{
   const char* pszPortName = 0;

   this->Set( pszInstName, pszPortName, pszPinName, type );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TNO_InstPin_c::Clear( 
      void )
{
   this->srInstName_ = "";
   this->srPortName_ = "";
   this->srPinName_ = "";
   this->portIndex_ = TNO_PORT_INDEX_INVALID;
   this->pinIndex_ = TNO_PIN_INDEX_INVALID;
   this->type_ = TC_TYPE_UNDEFINED;
}
