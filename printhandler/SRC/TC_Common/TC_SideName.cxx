//===========================================================================//
// Purpose : Method definitions for the TC_SideName class.
//
//           Public methods include:
//           - TC_SideName_c, ~TC_SideName_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TC_StringUtils.h"
#include "TC_SideName.h"

//===========================================================================//
// Method         : TC_SideName_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_SideName_c::TC_SideName_c( 
      void )
      :
      side_( TC_SIDE_UNDEFINED )
{
} 

//===========================================================================//
TC_SideName_c::TC_SideName_c( 
            TC_SideMode_t side,
      const string&       srName )
      :
      side_( side ),
      srName_( srName )
{
} 

//===========================================================================//
TC_SideName_c::TC_SideName_c( 
            TC_SideMode_t side,
      const char*         pszName )
      :
      side_( side ),
      srName_( pszName ? pszName : "" )
{
} 

//===========================================================================//
TC_SideName_c::TC_SideName_c( 
      const TC_SideName_c& sideName )
      :
      side_( sideName.side_ ),
      srName_( sideName.srName_ )
{
} 

//===========================================================================//
// Method         : ~TC_SideName_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_SideName_c::~TC_SideName_c( 
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
TC_SideName_c& TC_SideName_c::operator=( 
      const TC_SideName_c& sideName )
{
   if ( &sideName != this )
   {
      this->side_ = sideName.side_;
      this->srName_ = sideName.srName_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_SideName_c::operator==( 
      const TC_SideName_c& sideName ) const
{
   return(( this->side_ == sideName.side_ ) &&
          ( this->srName_ == sideName.srName_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_SideName_c::operator!=( 
      const TC_SideName_c& sideName ) const
{
   return( !this->operator==( sideName ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_SideName_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srSideName;
   this->ExtractString( &srSideName );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srSideName ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_SideName_c::ExtractString( 
      string* psrSideName ) const
{
   if ( psrSideName )
   {
      if ( this->IsValid( ))
      {
	 string srSide;
         TC_ExtractStringSideMode( this->side_, &srSide );

         *psrSideName = srSide;

	 if ( this->srName_.length( ))
	 {
            *psrSideName += " ";
            *psrSideName += this->srName_;
         }
      }
      else
      {
         *psrSideName = "?";
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
void TC_SideName_c::Set( 
            TC_SideMode_t side,
      const string&       srName )
{
   this->side_ = side;
   this->srName_ = srName;
} 

//===========================================================================//
void TC_SideName_c::Set( 
            TC_SideMode_t side,
      const char*         pszName )
{
   this->side_ = side;
   this->srName_ = ( pszName ? pszName : "" );
} 
