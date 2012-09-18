//===========================================================================//
// Purpose : Method definitions for the TFM_Pin class.
//
//           Public methods include:
//           - TFM_Pin_c, ~TFM_Pin_c
//           - operator=
//           - operator==, operator!=
//           - Print
//
//===========================================================================//

#include "TC_MinGrid.h"
#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TFM_Pin.h"

//===========================================================================//
// Method         : TFM_Pin_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TFM_Pin_c::TFM_Pin_c( 
      void )
      :
      side( TC_SIDE_UNDEFINED ),
      offset( 0.0 ),
      width( 0.0 ),
      slice( 0 )
{
}

//===========================================================================//
TFM_Pin_c::TFM_Pin_c( 
      const string&       srName,
	    TC_SideMode_t side_,
	    double        offset_,
	    double        width_,
	    unsigned int  slice_ )
      :
      TPO_Pin_t( srName ),
      side( side_ ),
      offset( offset_ ),
      width( width_ ),
      slice( slice_ )
{
}

//===========================================================================//
TFM_Pin_c::TFM_Pin_c( 
      const char*         pszName,
	    TC_SideMode_t side_,
	    double        offset_,
	    double        width_,
	    unsigned int  slice_ )
      :
      TPO_Pin_t( pszName ),
      side( side_ ),
      offset( offset_ ),
      width( width_ ),
      slice( slice_ )
{
}

//===========================================================================//
TFM_Pin_c::TFM_Pin_c( 
      const TFM_Pin_c& pin )
      :
      TPO_Pin_t( pin ),
      side( pin.side ),
      offset( pin.offset ),
      width( pin.width ),
      slice( pin.slice ),
      connectionPattern( pin.connectionPattern )
{
}

//===========================================================================//
// Method         : ~TFM_Pin_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TFM_Pin_c::~TFM_Pin_c( 
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
TFM_Pin_c& TFM_Pin_c::operator=( 
      const TFM_Pin_c& pin )
{
   if( &pin != this )
   {
      TPO_Pin_t::operator=( pin );
      this->side = pin.side;
      this->offset = pin.offset;
      this->width = pin.width;
      this->slice = pin.slice;
      this->connectionPattern = pin.connectionPattern;
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
bool TFM_Pin_c::operator==( 
      const TFM_Pin_c& pin ) const
{
   return( TPO_Pin_t::operator==( pin ) ? true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TFM_Pin_c::operator!=( 
      const TFM_Pin_c& pin ) const
{
   return( !this->operator==( pin ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TFM_Pin_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<pin \"%s\" ",
                                        TIO_PSZ_STR( TPO_Pin_t::GetName( )));
   if( this->side != TC_SIDE_UNDEFINED )
   {
      string srSide;
      TC_ExtractStringSideMode( this->side, &srSide );
      printHandler.Write( pfile, 0, "side = %s offset = %0.*f width = %0.*f slice = %u > ", 
                                    TIO_SR_STR( srSide ),
			            precision, this->offset,
                                    precision, this->width,
                                    this->slice );
   }
   if( this->connectionPattern.IsValid( ))
   {
      string srConnectionBoxPattern;
      this->connectionPattern.ExtractString( &srConnectionBoxPattern );
      printHandler.Write( pfile, 0, "<cb %s > ",
 			            TIO_SR_STR( srConnectionBoxPattern ));
   }
   printHandler.Write( pfile, 0, "</pin>\n" );
}
