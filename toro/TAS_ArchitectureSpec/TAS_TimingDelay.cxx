//===========================================================================//
// Purpose : Method definitions for the TAS_TimingDelay class.
//
//           Public methods include:
//           - TAS_TimingDelay_c, ~TAS_TimingDelay_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
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

#include "TC_MinGrid.h"
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TAS_StringUtils.h"

#include "TAS_TimingDelay.h"

//===========================================================================//
// Method         : TAS_TimingDelay_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_TimingDelay_c::TAS_TimingDelay_c( 
      void )
      :
      mode( TAS_TIMING_MODE_UNDEFINED ),
      type( TAS_TIMING_TYPE_UNDEFINED ),
      valueMin( 0.0 ),
      valueMax( 0.0 ),
      valueNom( 0.0 ),
      valueMatrix( TAS_TIMING_VALUE_MATRIX_DEF_CAPACITY, 
                   TAS_TIMING_VALUE_MATRIX_DEF_CAPACITY )
{
}

//===========================================================================//
TAS_TimingDelay_c::TAS_TimingDelay_c( 
      const TAS_TimingDelay_c& timingDelay )
      :
      mode( timingDelay.mode ),
      type( timingDelay.type ),
      valueMin( timingDelay.valueMin ),
      valueMax( timingDelay.valueMax ),
      valueNom( timingDelay.valueNom ),
      valueMatrix( timingDelay.valueMatrix ),
      srInputPortName( timingDelay.srInputPortName ),
      srOutputPortName( timingDelay.srOutputPortName ),
      srClockPortName( timingDelay.srClockPortName ),
      srPackPatternName( timingDelay.srPackPatternName )
{
}

//===========================================================================//
// Method         : ~TAS_TimingDelay_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_TimingDelay_c::~TAS_TimingDelay_c( 
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
TAS_TimingDelay_c& TAS_TimingDelay_c::operator=( 
      const TAS_TimingDelay_c& timingDelay )
{
   if( &timingDelay != this )
   {
      this->mode = timingDelay.mode;
      this->type = timingDelay.type;
      this->valueMin = timingDelay.valueMin;
      this->valueMax = timingDelay.valueMax;
      this->valueNom = timingDelay.valueNom;
      this->valueMatrix = timingDelay.valueMatrix;
      this->srInputPortName = timingDelay.srInputPortName;
      this->srOutputPortName = timingDelay.srOutputPortName;
      this->srClockPortName = timingDelay.srClockPortName;
      this->srPackPatternName = timingDelay.srPackPatternName;
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
bool TAS_TimingDelay_c::operator==( 
      const TAS_TimingDelay_c& timingDelay ) const
{
   return(( this->mode == timingDelay.mode ) &&
          ( this->type == timingDelay.type ) &&
          ( TCTF_IsEQ( this->valueMin, timingDelay.valueMin )) &&
          ( TCTF_IsEQ( this->valueMax, timingDelay.valueMax )) &&
          ( TCTF_IsEQ( this->valueNom, timingDelay.valueNom )) &&
          ( this->valueMatrix == timingDelay.valueMatrix ) &&
          ( this->srInputPortName == timingDelay.srInputPortName ) &&
          ( this->srOutputPortName == timingDelay.srOutputPortName ) &&
          ( this->srClockPortName == timingDelay.srClockPortName ) &&
          ( this->srPackPatternName == timingDelay.srPackPatternName ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_TimingDelay_c::operator!=( 
      const TAS_TimingDelay_c& timingDelay ) const
{
   return( !this->operator==( timingDelay ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_TimingDelay_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<timing " );

   string srMode;
   TAS_ExtractStringTimingMode( this->mode, &srMode );
   printHandler.Write( pfile, 0, "%s ", TIO_SR_STR( srMode ));

   string srType;
   TAS_ExtractStringTimingType( this->type, &srType );

   if( this->mode == TAS_TIMING_MODE_DELAY_CONSTANT )
   {
      printHandler.Write( pfile, 0, "input = \"%s\" ", TIO_SR_STR( this->srInputPortName ));
      printHandler.Write( pfile, 0, "output = \"%s\" ", TIO_SR_STR( this->srOutputPortName ));
      if( TCTF_IsGT( this->valueMin, 0.0 ))
      {
         printHandler.Write( pfile, 0, "min = %0.*e ", precision + 1, this->valueMin );
      }
      if( TCTF_IsGT( this->valueMax, 0.0 ))
      {
         printHandler.Write( pfile, 0, "max = %0.*e ", precision + 1, this->valueMax );
      }
      printHandler.Write( pfile, 0, "/>\n" );
   }
   else if( this->mode == TAS_TIMING_MODE_DELAY_MATRIX )
   {
      printHandler.Write( pfile, 0, "input = \"%s\" ", TIO_SR_STR( this->srInputPortName ));
      printHandler.Write( pfile, 0, "output = \"%s\" ", TIO_SR_STR( this->srOutputPortName ));
      printHandler.Write( pfile, 0, "\n" );
      spaceLen += 3;

      string srTimingDelayMatrix;
      this->valueMatrix.ExtractString( TC_DATA_EXP, &srTimingDelayMatrix, 
                                       4, SIZE_MAX, spaceLen + 6, 0 );
      printHandler.Write( pfile, spaceLen, "%s = %s", TIO_SR_STR( srType ), 
                                                      TIO_SR_STR( srTimingDelayMatrix ));
      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "/>\n" );
   }
   else if( this->mode == TAS_TIMING_MODE_T_SETUP )
   {
      printHandler.Write( pfile, 0, "input = \"%s\" ", TIO_SR_STR( this->srInputPortName ));
      printHandler.Write( pfile, 0, "clock = \"%s\" ", TIO_SR_STR( this->srClockPortName ));
      printHandler.Write( pfile, 0, "value = %0.*e ", precision + 1, this->valueNom );
      printHandler.Write( pfile, 0, "/>\n" );
   }
   else if( this->mode == TAS_TIMING_MODE_T_HOLD )
   {
      printHandler.Write( pfile, 0, "input = \"%s\" ", TIO_SR_STR( this->srInputPortName ));
      printHandler.Write( pfile, 0, "clock = \"%s\" ", TIO_SR_STR( this->srClockPortName ));
      printHandler.Write( pfile, 0, "value = %0.*e ", precision + 1, this->valueNom );
      printHandler.Write( pfile, 0, "/>\n" );
   }
   else if( this->mode == TAS_TIMING_MODE_CLOCK_TO_Q )
   {
      printHandler.Write( pfile, 0, "output = \"%s\" ", TIO_SR_STR( this->srOutputPortName ));
      printHandler.Write( pfile, 0, "clock = \"%s\" ", TIO_SR_STR( this->srClockPortName ));
      if( TCTF_IsGT( this->valueMin, 0.0 ))
      {
         printHandler.Write( pfile, 0, "min = %0.*e ", precision + 1, this->valueMin );
      }
      if( TCTF_IsGT( this->valueMax, 0.0 ))
      {
         printHandler.Write( pfile, 0, "max = %0.*e ", precision + 1, this->valueMax );
      }
      printHandler.Write( pfile, 0, "/>\n" );
   }
   else if( this->mode == TAS_TIMING_MODE_CAP_CONSTANT )
   {
      printHandler.Write( pfile, 0, "input = \"%s\" ", TIO_SR_STR( this->srInputPortName ));
      printHandler.Write( pfile, 0, "output = \"%s\" ", TIO_SR_STR( this->srOutputPortName ));
      printHandler.Write( pfile, 0, "value = %0.*f ", precision, this->valueNom );
      printHandler.Write( pfile, 0, "/>\n" );
   }
   else if( this->mode == TAS_TIMING_MODE_CAP_MATRIX )
   {
      printHandler.Write( pfile, 0, "input = \"%s\" ", TIO_SR_STR( this->srInputPortName ));
      printHandler.Write( pfile, 0, "output = \"%s\" ", TIO_SR_STR( this->srOutputPortName ));
      printHandler.Write( pfile, 0, "\n" );
      spaceLen += 3;

      string srTimingDelayMatrix;
      this->valueMatrix.ExtractString( TC_DATA_FLOAT, &srTimingDelayMatrix, 
                                       4, SIZE_MAX, spaceLen + 8, 0 );
      printHandler.Write( pfile, spaceLen, "value = %s", TIO_SR_STR( srTimingDelayMatrix ));

      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "/>\n" );
   }
   else if( this->mode == TAS_TIMING_MODE_PACK_PATTERN )
   {
      printHandler.Write( pfile, 0, "input = \"%s\" ", TIO_SR_STR( this->srInputPortName ));
      printHandler.Write( pfile, 0, "output = \"%s\" ", TIO_SR_STR( this->srOutputPortName ));
      printHandler.Write( pfile, 0, "name = \"%s\" ", TIO_SR_STR( this->srPackPatternName ));
      printHandler.Write( pfile, 0, "/>\n" );
   }
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_TimingDelay_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_TimingDelay_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( ) + 1;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srType;
   TAS_ExtractStringTimingType( this->type, &srType );

   switch( this->mode )
   {
   case TAS_TIMING_MODE_DELAY_CONSTANT:
      if( TCTF_IsGT( this->valueMin, 0.0 ) && TCTF_IsGT( this->valueMax, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "<delay_constant min=\"%0.*e\" max=\"%0.*e\" in_port=\"%s\" out_port=\"%s\"/>\n",
                                              precision + 1, this->valueMin,
                                              precision + 1, this->valueMax,
                                              TIO_SR_STR( this->srInputPortName ),
                                              TIO_SR_STR( this->srOutputPortName ));
      }
      else if( TCTF_IsGT( this->valueMin, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "<delay_constant min=\"%0.*e\" in_port=\"%s\" out_port=\"%s\"/>\n",
                                              precision + 1, this->valueMin,
                                              TIO_SR_STR( this->srInputPortName ),
                                              TIO_SR_STR( this->srOutputPortName ));
      }
      else if( TCTF_IsGT( this->valueMax, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "<delay_constant max=\"%0.*e\" in_port=\"%s\" out_port=\"%s\"/>\n",
                                              precision + 1, this->valueMax,
                                              TIO_SR_STR( this->srInputPortName ),
                                              TIO_SR_STR( this->srOutputPortName ));
      }
      break;

   case TAS_TIMING_MODE_DELAY_MATRIX:
      printHandler.Write( pfile, spaceLen, "<delay_matrix type=\"%s\" in_port=\"%s\" out_port=\"%s\">\n",
                                           TIO_SR_STR( srType ),
                                           TIO_SR_STR( this->srInputPortName ),
                                           TIO_SR_STR( this->srOutputPortName ));

      for( size_t j = 0; j < this->valueMatrix.GetHeight( ); ++j )
      {
         printHandler.Write( pfile, spaceLen + 3, "" );
         for( size_t i = 0; i < this->valueMatrix.GetWidth( ); ++i )
         {
            printHandler.Write( pfile, 0, "%0.*e%s",
                                          precision + 1, this->valueMatrix[i][j],
                                          i + 1 == this->valueMatrix.GetWidth( ) ? "" : " " );
         }
         printHandler.Write( pfile, 0, "\n" );
      }
      printHandler.Write( pfile, spaceLen, "</delay_matrix>\n" );
      break;

   case TAS_TIMING_MODE_T_SETUP:
      printHandler.Write( pfile, spaceLen, "<T_setup value=\"%0.*e\" port=\"%s\" clock=\"%s\"/>\n",
                                           precision + 1, this->valueNom,
                                           TIO_SR_STR( this->srInputPortName ),
                                           TIO_SR_STR( this->srClockPortName ));
      break;

   case TAS_TIMING_MODE_T_HOLD:
      printHandler.Write( pfile, spaceLen, "<T_hold value=\"%0.*e\" port=\"%s\" clock=\"%s\"/>\n",
                                           precision + 1, this->valueNom,
                                           TIO_SR_STR( this->srInputPortName ),
                                           TIO_SR_STR( this->srClockPortName ));
      break;

   case TAS_TIMING_MODE_CLOCK_TO_Q:
      if( TCTF_IsGT( this->valueMin, 0.0 ) && TCTF_IsGT( this->valueMax, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "<T_clock_to_Q min=\"%0.*e\" max=\"%0.*e\" port=\"%s\" clock=\"%s\"/>\n",
                                              precision + 1, this->valueMin,
                                              precision + 1, this->valueMax,
                                              TIO_SR_STR( this->srOutputPortName ),
                                              TIO_SR_STR( this->srClockPortName ));
      }
      else if( TCTF_IsGT( this->valueMin, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "<T_clock_to_Q min=\"%0.*e\" port=\"%s\" clock=\"%s\"/>\n",
                                              precision + 1, this->valueMin,
                                              TIO_SR_STR( this->srOutputPortName ),
                                              TIO_SR_STR( this->srClockPortName ));
      }
      else if( TCTF_IsGT( this->valueMax, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "<T_clock_to_Q max=\"%0.*e\" port=\"%s\" clock=\"%s\"/>\n",
                                              precision + 1, this->valueMax,
                                              TIO_SR_STR( this->srOutputPortName ),
                                              TIO_SR_STR( this->srClockPortName ));
      }
      break;

   case TAS_TIMING_MODE_CAP_CONSTANT:
      printHandler.Write( pfile, spaceLen, "<cap_constant C=\"%0.*f\" in_port=\"%s\" out_port=\"%s\"/>\n",
                                           precision, this->valueNom,
                                           TIO_SR_STR( this->srInputPortName ),
                                           TIO_SR_STR( this->srOutputPortName ));
      break;

   case TAS_TIMING_MODE_CAP_MATRIX:
      printHandler.Write( pfile, spaceLen, "<cap_matrix in_port=\"%s\" out_port=\"%s\">\n",
                                           TIO_SR_STR( this->srInputPortName ),
                                           TIO_SR_STR( this->srOutputPortName ));

      for( size_t j = 0; j < this->valueMatrix.GetHeight( ); ++j )
      {
         printHandler.Write( pfile, spaceLen + 3, "" );
         for( size_t i = 0; i < this->valueMatrix.GetWidth( ); ++i )
         {
            printHandler.Write( pfile, 0, "%0.*f%s",
                                          precision, this->valueMatrix[i][j],
                                          i + 1 == this->valueMatrix.GetWidth( ) ? "" : " " );
         }
         printHandler.Write( pfile, 0, "\n" );
      }
      printHandler.Write( pfile, spaceLen, "</cap_matrix>\n" );
      break;

   case TAS_TIMING_MODE_PACK_PATTERN:
      printHandler.Write( pfile, spaceLen, "<pack_pattern name=\"%s\" in_port=\"%s\" out_port=\"%s\"/>\n",
                                           TIO_SR_STR( this->srPackPatternName ),
                                           TIO_SR_STR( this->srInputPortName ),
                                           TIO_SR_STR( this->srOutputPortName ));
      break;

   case TAS_TIMING_MODE_UNDEFINED:
      break;
   }
}
