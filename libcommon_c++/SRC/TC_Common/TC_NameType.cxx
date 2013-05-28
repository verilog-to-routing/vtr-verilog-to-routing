//===========================================================================//
// Purpose : Method definitions for the TC_NameType class.
//
//           Public methods include:
//           - TC_NameType_c, ~TC_NameType_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TC_StringUtils.h"
#include "TC_NameType.h"

//===========================================================================//
// Method         : TC_NameType_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_NameType_c::TC_NameType_c( 
      void )
      :
      type_( TC_TYPE_UNDEFINED )
{
} 

//===========================================================================//
TC_NameType_c::TC_NameType_c( 
      const string&       srName,
            TC_TypeMode_t type )
      :
      srName_( srName ),
      type_( type )
{
} 

//===========================================================================//
TC_NameType_c::TC_NameType_c( 
      const char*         pszName,
            TC_TypeMode_t type )
      :
      srName_( pszName ? pszName : "" ),
      type_( type )
{
} 

//===========================================================================//
TC_NameType_c::TC_NameType_c( 
      const TC_NameType_c& nameType )
      :
      srName_( nameType.srName_ ),
      type_( nameType.type_ )
{
} 

//===========================================================================//
// Method         : ~TC_NameType_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_NameType_c::~TC_NameType_c( 
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
TC_NameType_c& TC_NameType_c::operator=( 
      const TC_NameType_c& nameType )
{
   if ( &nameType != this )
   {
      this->srName_ = nameType.srName_;
      this->type_ = nameType.type_;
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
bool TC_NameType_c::operator<( 
      const TC_NameType_c& nameType ) const
{
   return(( TC_CompareStrings( this->srName_, nameType.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_NameType_c::operator==( 
      const TC_NameType_c& nameType ) const
{
   return(( this->srName_ == nameType.srName_ ) &&
          ( this->type_ == nameType.type_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_NameType_c::operator!=( 
      const TC_NameType_c& nameType ) const
{
   return( !this->operator==( nameType ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_NameType_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srNameType;
   this->ExtractString( &srNameType );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srNameType ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_NameType_c::ExtractString( 
      string* psrNameType ) const
{
   if ( psrNameType )
   {
      if ( this->IsValid( ))
      {
         *psrNameType = this->srName_;

	 if ( this->type_ != TC_TYPE_UNDEFINED )
	 {
   	    string srType;
            TC_ExtractStringTypeMode( this->type_, &srType );

            *psrNameType += " ";
            *psrNameType += srType;
         }
      }
      else
      {
         *psrNameType = "?";
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
void TC_NameType_c::Set( 
      const string&       srName,
            TC_TypeMode_t type )
{
   this->srName_ = srName;
   this->type_ = type;
} 

//===========================================================================//
void TC_NameType_c::Set( 
      const char*         pszName,
            TC_TypeMode_t type )
{
   this->srName_ = ( pszName ? pszName : "" );
   this->type_ = type;
} 
