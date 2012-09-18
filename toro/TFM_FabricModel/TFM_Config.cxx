//===========================================================================//
// Purpose : Method definitions for the TFM_Config class.
//
//           Public methods include:
//           - TFM_Config_c, ~TFM_Config_c
//           - operator=
//           - operator==, operator!=
//           - Print
//
//===========================================================================//

#if defined( SUN8 ) || defined( SUN10 )
   #include <math.h>
#endif

#include "TIO_PrintHandler.h"

#include "TFM_Config.h"

//===========================================================================//
// Method         : TFM_Config_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_Config_c::TFM_Config_c( 
      void )
{
} 

//===========================================================================//
TFM_Config_c::TFM_Config_c( 
      const TFM_Config_c& config )
      :
      fabricRegion( config.fabricRegion )
{
} 

//===========================================================================//
// Method         : ~TFM_Config_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_Config_c::~TFM_Config_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_Config_c& TFM_Config_c::operator=( 
      const TFM_Config_c& config )
{
   if( &config != this )
   {
      this->fabricRegion = config.fabricRegion;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFM_Config_c::operator==( 
      const TFM_Config_c& config ) const
{
   return(( this->fabricRegion == config.fabricRegion ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFM_Config_c::operator!=( 
      const TFM_Config_c& config ) const
{
   return( !this->operator==( config ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TFM_Config_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   if( this->fabricRegion.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<config>\n" );
      spaceLen += 3;

      string srFabricRegion;
      this->fabricRegion.ExtractString( &srFabricRegion );

      printHandler.Write( pfile, spaceLen, "<region %s >\n", 
                                           TIO_SR_STR( srFabricRegion ));
      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "</config>\n" );
   }
}
