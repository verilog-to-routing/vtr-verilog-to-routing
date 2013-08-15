//===========================================================================//
// Purpose : Method definitions for the TFM_Segment class.
//
//           Public methods include:
//           - TFM_Segment_c, ~TFM_Segment_c
//           - operator=
//           - operator==, operator!=
//           - Print
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

#include "TC_MinGrid.h"

#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TFM_Segment.h"

//===========================================================================//
// Method         : TFM_Segment_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_Segment_c::TFM_Segment_c( 
      void )
{
   this->timing.res = 0.0;
   this->timing.cap = 0.0;
}

//===========================================================================//
TFM_Segment_c::TFM_Segment_c( 
      const string&      srName_,
      const TGS_Path_c&  path_,
            unsigned int index_ )
      :
      srName( srName_ ),
      path( path_ ),
      index( index_ )
{
   this->timing.res = 0.0;
   this->timing.cap = 0.0;
}

//===========================================================================//
TFM_Segment_c::TFM_Segment_c( 
      const char*        pszName,
      const TGS_Path_c&  path_,
            unsigned int index_ )
      :
      srName( TIO_PSZ_STR( pszName )),
      path( path_ ),
      index( index_ )
{
   this->timing.res = 0.0;
   this->timing.cap = 0.0;
}

//===========================================================================//
TFM_Segment_c::TFM_Segment_c( 
      const string& srName_ )
      :
      srName( srName_ )
{
   this->timing.res = 0.0;
   this->timing.cap = 0.0;
}

//===========================================================================//
TFM_Segment_c::TFM_Segment_c( 
      const char* pszName )
      :
      srName( TIO_PSZ_STR( pszName ))
{
   this->timing.res = 0.0;
   this->timing.cap = 0.0;
}

//===========================================================================//
TFM_Segment_c::TFM_Segment_c( 
      const TFM_Segment_c& segment )
      :
      srName( segment.srName ),
      path( segment.path ),
      index( segment.index )
{
   this->timing.res = segment.timing.res;
   this->timing.cap = segment.timing.cap;
}

//===========================================================================//
// Method         : ~TFM_Segment_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFM_Segment_c::~TFM_Segment_c( 
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
TFM_Segment_c& TFM_Segment_c::operator=( 
      const TFM_Segment_c& segment )
{
   if( &segment != this )
   {
      this->srName = segment.srName;
      this->path = segment.path;
      this->index = segment.index;
      this->timing.res = segment.timing.res;
      this->timing.cap = segment.timing.cap;
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
bool TFM_Segment_c::operator==( 
      const TFM_Segment_c& segment ) const
{
   return(( this->srName == segment.srName ) &&
          ( this->path == segment.path ) &&
          ( this->index == segment.index ) &&
          ( TCTF_IsEQ( this->timing.res, segment.timing.res )) &&
          ( TCTF_IsEQ( this->timing.cap, segment.timing.cap )) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFM_Segment_c::operator!=( 
      const TFM_Segment_c& segment ) const
{
   return( !this->operator==( segment ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TFM_Segment_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<segment name=\"%s\" index=\"%u\"",
                                        TIO_SR_STR( this->srName ),
                                        this->index );
   if( TCTF_IsGT( this->path.width, 0.0 ))
   {
      printHandler.Write( pfile, 0, " width=\"%0.*f\"", 
                                    precision, this->path.width );
   }
   printHandler.Write( pfile, 0, ">" );

   string srLine;
   this->path.line.ExtractString( &srLine );
   printHandler.Write( pfile, 0, " <line> %s </line>", TIO_SR_STR( srLine ));

   if( TCTF_IsGT( this->timing.res, 0.0 ) ||
       TCTF_IsGT( this->timing.cap, 0.0 ))
   {
      printHandler.Write( pfile, 0, "\n" );
      printHandler.Write( pfile, spaceLen + 3, " <timing res=\"%0.*f\" cap=\"%0.*f\"/>\n",
                                               precision, this->timing.res,
                                               precision, this->timing.cap );
      printHandler.Write( pfile, spaceLen, "" );
   }
   printHandler.Write( pfile, 0, " </segment>\n" );
}
