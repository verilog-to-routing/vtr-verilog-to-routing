//===========================================================================//
// Purpose : Method definitions for the TAS_ConnectionFc class.
//
//           Public methods include:
//           - TAS_ConnectionFc_c, ~TAS_ConnectionFc_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
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

#include "TC_MinGrid.h"
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TAS_StringUtils.h"
#include "TAS_ConnectionFc.h"

//===========================================================================//
// Method         : TAS_ConnectionFc_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TAS_ConnectionFc_c::TAS_ConnectionFc_c( 
      void )
      :
      type( TAS_CONNECTION_BOX_UNDEFINED ),
      percent( 0.0 ),
      absolute( 0 ),
      dir_( TC_TYPE_UNDEFINED )
{
}

//===========================================================================//
TAS_ConnectionFc_c::TAS_ConnectionFc_c( 
      const TAS_ConnectionFc_c& connectionFc )
      :
      type( connectionFc.type ),
      percent( connectionFc.percent ),
      absolute( connectionFc.absolute ),
      srName( connectionFc.srName ),
      dir_( connectionFc.dir_ )
{
}

//===========================================================================//
// Method         : ~TAS_ConnectionFc_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TAS_ConnectionFc_c::~TAS_ConnectionFc_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TAS_ConnectionFc_c& TAS_ConnectionFc_c::operator=( 
      const TAS_ConnectionFc_c& connectionFc )
{
   if( &connectionFc != this )
   {
      this->type = connectionFc.type;
      this->percent = connectionFc.percent;
      this->absolute = connectionFc.absolute;
      this->srName = connectionFc.srName;
      this->dir_ = connectionFc.dir_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TAS_ConnectionFc_c::operator==( 
      const TAS_ConnectionFc_c& connectionFc ) const
{
   return(( this->type == connectionFc.type ) &&
          ( TCTF_IsEQ( this->percent, connectionFc.percent )) &&
          ( this->absolute == connectionFc.absolute ) &&
	  ( this->srName == connectionFc.srName ) &&
          ( this->dir_ == connectionFc.dir_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TAS_ConnectionFc_c::operator!=( 
      const TAS_ConnectionFc_c& connectionFc ) const
{
   return( !this->operator==( connectionFc ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TAS_ConnectionFc_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   const char* pszFc = "";
   switch( this->dir_ )
   {
   case TC_TYPE_INPUT:  pszFc = "fc_in";  break;
   case TC_TYPE_OUTPUT: pszFc = "fc_out"; break;
   default:             pszFc = "fc";     break;
   }

   if( this->srName.length( ))
   {
      printHandler.Write( pfile, spaceLen, "name=\"%s\" ", 
                                           TIO_SR_STR( this->srName ));
   }

   if( this->type == TAS_CONNECTION_BOX_FRACTION )
   {
      printHandler.Write( pfile, spaceLen, "%s=\"%0.*f\"", 
                                           TIO_PSZ_STR( pszFc ), 
                                           precision, this->percent );
   }
   else if( this->type == TAS_CONNECTION_BOX_ABSOLUTE )
   {
      printHandler.Write( pfile, spaceLen, "%s=\"%u\"", 
                                           TIO_PSZ_STR( pszFc ),
                                           this->absolute );
   }
   else if( this->type == TAS_CONNECTION_BOX_FULL )
   {
      printHandler.Write( pfile, spaceLen, "%s=\"full\"", 
                                           TIO_PSZ_STR( pszFc ));
   }
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TAS_ConnectionFc_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_ConnectionFc_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   const char* pszFc = "";
   switch( this->dir_ )
   {
   case TC_TYPE_INPUT:  pszFc = "default_in";  break;
   case TC_TYPE_OUTPUT: pszFc = "default_out"; break;
   default:             pszFc = "fc";          break;
   }

   string srFcType;
   TAS_ExtractStringConnectionBoxType( this->type, &srFcType );

   if( this->srName.length( ))
   {
      printHandler.Write( pfile, spaceLen, "<pin name=\"%s\" ",
                                           TIO_SR_STR( this->srName ));
   }

   if( this->type == TAS_CONNECTION_BOX_FRACTION )
   {
      printHandler.Write( pfile, 0, "%s_type=\"%s\" %s_val=\"%0.*f\"",
                                    TIO_PSZ_STR( pszFc ),
                                    TIO_SR_STR( srFcType ),
                                    TIO_PSZ_STR( pszFc ),
                                    precision, this->percent );
   }
   else if( this->type == TAS_CONNECTION_BOX_ABSOLUTE )
   {
      printHandler.Write( pfile, 0, "%s_type=\"%s\" %s_val=\"%u\"",
                                    TIO_PSZ_STR( pszFc ),
                                    TIO_SR_STR( srFcType ),
                                    TIO_PSZ_STR( pszFc ),
                                    this->absolute );
   }
   else if( this->type == TAS_CONNECTION_BOX_FULL )
   {
      printHandler.Write( pfile, 0, "%s_type=\"%s\"",
                                    TIO_PSZ_STR( pszFc ),
                                    TIO_SR_STR( srFcType ));
   }

   if( this->srName.length( ))
   {
      printHandler.Write( pfile, 0, "/>\n" );
   }
}
