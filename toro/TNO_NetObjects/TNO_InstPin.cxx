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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

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
      type_( TC_TYPE_UNDEFINED )
{
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
      srPinName_( instPin.srPinName_ ),
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
      this->srPinName_ = instPin.srPinName_;
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
          ( this->srPinName_ == instPin.srPinName_ ) &&
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
         *psrInstPin = "\"";
         *psrInstPin += this->srInstName_;
         *psrInstPin += "\" \"";
         *psrInstPin += this->srPinName_;
         *psrInstPin += "\"";

	 if( this->type_ != TC_TYPE_UNDEFINED )
	 {
   	    string srType;
            TC_ExtractStringTypeMode( this->type_, &srType );

            *psrInstPin += " ";
            *psrInstPin += srType;
         }
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
      const string&       srPinName,
            TC_TypeMode_t type )
{
   this->srInstName_ = srInstName;
   this->srPinName_ = srPinName;

   this->srName_ = "";
   this->srName_ += this->srInstName_;
   this->srName_ += "|";
   this->srName_ += this->srPinName_;

   this->type_ = type;
} 

//===========================================================================//
void TNO_InstPin_c::Set( 
      const char*         pszInstName,
      const char*         pszPinName,
            TC_TypeMode_t type )
{
   this->srInstName_ = TIO_PSZ_STR( pszInstName );
   this->srPinName_ = TIO_PSZ_STR( pszPinName );

   this->srName_ = "";
   this->srName_ += this->srInstName_;
   this->srName_ += "|";
   this->srName_ += this->srPinName_;

   this->type_ = type;
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
   this->srPinName_ = "";
   this->type_ = TC_TYPE_UNDEFINED;
}
