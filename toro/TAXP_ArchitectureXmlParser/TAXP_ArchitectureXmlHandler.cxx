//===========================================================================//
// Purpose : Methods for the 'TAXP_ArchitectureXmlHandler_c' class.
//
//           Public methods include:
//           - TAXP_ArchitectureXmlHandler_c, ~TAXP_ArchitectureXmlHandler_c
//           - Init
//           - SyntaxError
//           - IsValid
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TAXP_ArchitectureXmlHandler.h"

//===========================================================================//
// Method         : TAXP_ArchitectureXmlHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAXP_ArchitectureXmlHandler_c::TAXP_ArchitectureXmlHandler_c(
      TAS_ArchitectureSpec_c* parchitectureSpec )
      :
      parchitectureSpec_( 0 )
{
   if( parchitectureSpec )
   {
      this->Init( parchitectureSpec );
   }
}

//===========================================================================//
// Method         : ~TAXP_ArchitectureXmlHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAXP_ArchitectureXmlHandler_c::~TAXP_ArchitectureXmlHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAXP_ArchitectureXmlHandler_c::Init( 
      TAS_ArchitectureSpec_c* parchitectureSpec )
{
   this->parchitectureSpec_ = parchitectureSpec;

   return( this->parchitectureSpec_ ? true : false );
}

//===========================================================================//
// Method         : SyntaxError
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAXP_ArchitectureXmlHandler_c::SyntaxError( 
            unsigned int lineNum, 
      const string&      srFileName,
      const char*        pszMessageText )
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   bool ok = printHandler.Error( "Syntax error %s.\n"
                                 "%sSee file '%s', line %d.\n",
                                 TIO_PSZ_STR( pszMessageText ),
                                 TIO_PREFIX_ERROR_SPACE,
                                 TIO_SR_STR( srFileName ), lineNum );
   return( ok );
}

//===========================================================================//
// Method         : IsValid
// Purpose        : Return true if this architecture XML handler was able to 
//                  properly initialize a 'parchitectureSpec_' object.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TAXP_ArchitectureXmlHandler_c::IsValid( void ) const
{
   bool isValid = false;

   if( this->parchitectureSpec_ )
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      if( printHandler.IsWithinMaxErrorCount( ) &&
          printHandler.IsWithinMaxWarningCount( ))
      {
         isValid = true;
      }
   }
   return( isValid );
}
