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
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
//                                                                           //
// This program is free software; you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation; version 3 of the License, or any later version. //
//                                                                           //
// This program is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY //
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   //
// for more details.                                                         //
//                                                                           //
// You should have received a copy of the GNU General Public License along   //
// with this program; if not, see <http://www.gnu.org/licenses>.             //
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

   printHandler.Write( pfile, spaceLen, "<segment \"%s\" ", TIO_SR_STR( this->srName ));
   printHandler.Write( pfile, 0, "index = %u ", this->index );

   if( TCTF_IsGT( this->path.width, 0.0 ))
   {
      printHandler.Write( pfile, 0, "width = %0.*f ", precision, this->path.width );
   }
   printHandler.Write( pfile, 0, "> " );

   string srLine;
   this->path.line.ExtractString( &srLine );
   printHandler.Write( pfile, 0, "<line %s > ", TIO_SR_STR( srLine ));

   if( TCTF_IsGT( this->timing.res, 0.0 ) ||
       TCTF_IsGT( this->timing.cap, 0.0 ))
   {
      printHandler.Write( pfile, 0, "\n" );
      printHandler.Write( pfile, spaceLen + 3, "<timing res = %0.*f cap = %0.*f >\n",
                                               precision, this->timing.res,
                                               precision, this->timing.cap );
      printHandler.Write( pfile, spaceLen, "" );
   }
   printHandler.Write( pfile, 0, "</segment>\n" );
}
