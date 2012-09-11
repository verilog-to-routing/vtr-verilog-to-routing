//===========================================================================//
// Purpose : Method definitions for the (regexp) TC_Name class.
//
//           Public methods include:
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - ExtractString
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TC_StringUtils.h"
#include "TC_Name.h"

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TC_Name_c& TC_Name_c::operator=( 
      const TC_Name_c& name )
{
   if ( &name != this )
   {
      this->srName_ = name.srName_;
      this->value_ = name.value_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TC_Name_c::operator<( 
      const TC_Name_c& name ) const
{
   return(( TC_CompareStrings( this->srName_, name.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TC_Name_c::operator==( 
      const TC_Name_c& name ) const
{
   return(( this->srName_.length( )) && 
          ( name.srName_.length( )) &&
          ( this->srName_ == name.srName_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TC_Name_c::operator!=( 
      const TC_Name_c& name ) const
{
   return(( !this->operator==( name )) ?
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TC_Name_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srName;
   this->ExtractString( &srName );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srName ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TC_Name_c::ExtractString( 
      string* psrName ) const
{
   if ( psrName )
   {
      if ( this->IsValid( ))
      {
         *psrName = this->srName_;
      }
      else
      {
         *psrName = "?";
      }
   }
} 
