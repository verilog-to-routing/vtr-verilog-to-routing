//===========================================================================//
// Purpose : Method definitions for the TFM_Config class.
//
//           Public methods include:
//           - TFM_Config_c, ~TFM_Config_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
//---------------------------------------------------------------------------//

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
      fabricRegion( config.fabricRegion ),
      ioPolygon( config.ioPolygon )
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
      this->ioPolygon = config.ioPolygon;
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
   return(( this->fabricRegion == config.fabricRegion ) &&
          ( this->ioPolygon == config.ioPolygon ) ?
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

   if( this->fabricRegion.IsValid( ) ||
       this->ioPolygon.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<config>\n" );
      spaceLen += 3;

      if( this->fabricRegion.IsValid( ))
      {
         string srFabricRegion;
         this->fabricRegion.ExtractString( &srFabricRegion );

         printHandler.Write( pfile, spaceLen, "<region> %s </region>\n", 
                                              TIO_SR_STR( srFabricRegion ));
      }
      if( this->ioPolygon.IsValid( ))
      {
         string srIoPolygon;
         this->ioPolygon.ExtractString( &srIoPolygon );

         printHandler.Write( pfile, spaceLen, "<polygon> %s </polygon>\n", 
                                              TIO_SR_STR( srIoPolygon ));
      }
      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "</config>\n" );
   }
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFM_Config_c::IsValid( 
      void ) const
{
   return( this->fabricRegion.IsValid( ) ||
           this->ioPolygon.IsValid( ) ?
           true : false );
}
