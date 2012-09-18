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
      const TNO_SwitchBox_c& switchBox )
      :
      srName_( switchBox.srName_ ),
      inputPin_( switchBox.inputPin_ ),
      outputPin_( switchBox.outputPin_ )
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
      this->inputPin_ = switchBox.inputPin_;
      this->outputPin_ = switchBox.outputPin_;
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
          ( this->inputPin_ == switchBox.inputPin_ ) &&
          ( this->outputPin_ == switchBox.outputPin_ ) ?
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
         string srInputPin, srOutputPin;
         this->inputPin_.ExtractString( &srInputPin );
         this->outputPin_.ExtractString( &srOutputPin );

         *psrSwitchBox = "\"";
         *psrSwitchBox += this->srName_;
         *psrSwitchBox += "\" ";
         *psrSwitchBox += srInputPin;
         *psrSwitchBox += " ";
         *psrSwitchBox += srOutputPin;
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
   this->inputPin_.Clear( );
   this->outputPin_.Clear( );
}
