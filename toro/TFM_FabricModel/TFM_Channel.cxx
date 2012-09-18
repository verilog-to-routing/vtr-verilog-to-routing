//===========================================================================//
// Purpose : Method definitions for the TFM_Channel class.
//
//           Public methods include:
//           - TFM_Channel_c, ~TFM_Channel_c
//           - operator=
//           - operator==, operator!=
//           - Print
//
//===========================================================================//

#if defined( SUN8 ) || defined( SUN10 )
   #include <math.h>
#endif

#include "TIO_PrintHandler.h"

#include "TGS_StringUtils.h"

#include "TFM_Channel.h"

//===========================================================================//
// Method         : TFM_Channel_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_Channel_c::TFM_Channel_c( 
      void )
      :
      orient( TGS_ORIENT_UNDEFINED ),
      count( 0 )
{
}

//===========================================================================//
TFM_Channel_c::TFM_Channel_c( 
      const string&          srName_,
      const TGS_Region_c&    region_,
            TGS_OrientMode_t orient_,
            unsigned int     count_ )

      :
      srName( srName_ ),
      region( region_ ),
      orient( orient_ ),
      count( count_ )
{
}

//===========================================================================//
TFM_Channel_c::TFM_Channel_c( 
      const char*            pszName,
      const TGS_Region_c&    region_,
            TGS_OrientMode_t orient_,
            unsigned int     count_ )
      :
      srName( TIO_PSZ_STR( pszName )),
      region( region_ ),
      orient( orient_ ),
      count( count_ )
{
}

//===========================================================================//
TFM_Channel_c::TFM_Channel_c( 
      const TFM_Channel_c& channel )
      :
      srName( channel.srName ),
      region( channel.region ),
      orient( channel.orient ),
      count( channel.count )
{
}

//===========================================================================//
// Method         : ~TFM_Channel_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_Channel_c::~TFM_Channel_c( 
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
TFM_Channel_c& TFM_Channel_c::operator=( 
      const TFM_Channel_c& channel )
{
   if( &channel != this )
   {
      this->srName = channel.srName;
      this->region = channel.region;
      this->orient = channel.orient;
      this->count = channel.count;
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
bool TFM_Channel_c::operator==( 
      const TFM_Channel_c& channel ) const
{
   return(( this->srName == channel.srName ) &&
          ( this->region == channel.region ) &&
          ( this->orient == channel.orient ) &&
          ( this->count == channel.count ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFM_Channel_c::operator!=( 
      const TFM_Channel_c& channel ) const
{
   return( !this->operator==( channel ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TFM_Channel_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<channel " );

   printHandler.Write( pfile, 0, "\"%s\" ", TIO_SR_STR( this->srName ));

   if( this->orient != TGS_ORIENT_UNDEFINED )
   {
      string srOrient;
      TGS_ExtractStringOrientMode( this->orient, &srOrient );
      printHandler.Write( pfile, 0, "orient = %s ", TIO_SR_STR( srOrient ));
   }
   if( this->count != 0 )
   {
      printHandler.Write( pfile, 0, "count = %d ", this->count );
   }
   printHandler.Write( pfile, 0, "> " );

   string srRegion;
   this->region.ExtractString( &srRegion );
   printHandler.Write( pfile, 0, "<region %s > ", TIO_SR_STR( srRegion ));

   printHandler.Write( pfile, 0, "</channel>\n" );
}
