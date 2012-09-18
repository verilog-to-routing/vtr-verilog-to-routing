//===========================================================================//
// Purpose : Methods for the 'TFP_FabricHandler_c' class.
//
//           Public methods include:
//           - TFP_FabricHandler_c, ~TFP_FabricHandler_c
//           - Init
//           - SyntaxError
//           - IsValid
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TFP_FabricHandler.h"

//===========================================================================//
// Method         : TFP_FabricHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TFP_FabricHandler_c::TFP_FabricHandler_c(
      TFM_FabricModel_c* pfabricModel )
      :
      pfabricModel_( 0 )
{
   if( pfabricModel )
   {
      this->Init( pfabricModel );
   }
}

//===========================================================================//
// Method         : ~TFP_FabricHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TFP_FabricHandler_c::~TFP_FabricHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
bool TFP_FabricHandler_c::Init( 
      TFM_FabricModel_c* pfabricModel )
{
   this->pfabricModel_ = pfabricModel;

   return( this->pfabricModel_ ? true : false );
}

//===========================================================================//
// Method         : SyntaxError
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
bool TFP_FabricHandler_c::SyntaxError( 
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
// Purpose        : Return true if this fabric handler was able to properly
//                  initialize a 'pfabricModel_' object.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
inline bool TFP_FabricHandler_c::IsValid( void ) const
{
   bool isValid = false;

   if( this->pfabricModel_ )
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
