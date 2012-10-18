//===========================================================================//
// Purpose : Supporting methods for the TI_Input pre-processor class.  
//           These methods support validating input options and data files.
//
//           Private methods include:
//           - ValidateOptionsStore_
//           - ValidateArchitectureSpec_
//           - ValidateFabricModel_
//           - ValidateCircuitDesign_
//
//===========================================================================//

#include "TI_Input.h"

//===========================================================================//
// Method         : ValidateOptionsStore_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::ValidateOptionsStore_( 
      void )
{
   bool ok = true;

   // This is only a stub... (currently looking for intereseting work to do)

   return( ok );
}

//===========================================================================//
// Method         : ValidateArchitectureSpec_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::ValidateArchitectureSpec_( 
      void )
{
   bool ok = true;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Info( "Validating architecture file ...\n" );

   ok = this->parchitectureSpec_->InitValidate( );

   return( ok );
}

//===========================================================================//
// Method         : ValidateFabricModel_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::ValidateFabricModel_( 
      void )
{
   bool ok = true;

   // This is only a stub... (currently looking for intereseting work to do)

   return( ok );
}

//===========================================================================//
// Method         : ValidateCircuitDesign_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TI_Input_c::ValidateCircuitDesign_( 
      void )
{
   bool ok = true;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Info( "Validating circuit file '%s'...\n",
                      TIO_SR_STR( this->pcircuitDesign_->srName ));

   ok = this->pcircuitDesign_->InitValidate( );

   return( ok );
}
