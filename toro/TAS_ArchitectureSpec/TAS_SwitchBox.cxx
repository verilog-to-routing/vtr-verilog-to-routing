//===========================================================================//
// Purpose : Method definitions for the TAS_SwitchBox class.
//
//           Public methods include:
//           - TAS_SwitchBox_c, ~TAS_SwitchBox_c
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

#include "TAS_SwitchBox.h"

//===========================================================================//
// Method         : TAS_SwitchBox_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_SwitchBox_c::TAS_SwitchBox_c( 
      void )
      :
      model( TAS_SWITCH_BOX_MODEL_UNDEFINED ),
      type( TAS_SWITCH_BOX_UNDEFINED ),
      fs( 0 )
{
   this->timing.res = 0.0;
   this->timing.capInput = 0.0;
   this->timing.capOutput = 0.0;
   this->timing.delay = 0.0;

   this->area.buffer = 0.0;
   this->area.muxTransistor = 0.0;
}

//===========================================================================//
TAS_SwitchBox_c::TAS_SwitchBox_c( 
      const string& srName_ )
      :
      srName( srName_ ),
      model( TAS_SWITCH_BOX_MODEL_UNDEFINED ),
      type( TAS_SWITCH_BOX_UNDEFINED ),
      fs( 0 )
{
   this->timing.res = 0.0;
   this->timing.capInput = 0.0;
   this->timing.capOutput = 0.0;
   this->timing.delay = 0.0;

   this->area.buffer = 0.0;
   this->area.muxTransistor = 0.0;
}

//===========================================================================//
TAS_SwitchBox_c::TAS_SwitchBox_c( 
      const char* pszName )
      :
      srName( TIO_PSZ_STR( pszName )),
      model( TAS_SWITCH_BOX_MODEL_UNDEFINED ),
      type( TAS_SWITCH_BOX_UNDEFINED ),
      fs( 0 )
{
   this->timing.res = 0.0;
   this->timing.capInput = 0.0;
   this->timing.capOutput = 0.0;
   this->timing.delay = 0.0;

   this->area.buffer = 0.0;
   this->area.muxTransistor = 0.0;
}

//===========================================================================//
TAS_SwitchBox_c::TAS_SwitchBox_c( 
      const TAS_SwitchBox_c& switchBox )
      :
      srName( switchBox.srName ),
      model( switchBox.model ),
      type( switchBox.type ),
      fs( switchBox.fs )
{
   this->timing.res = switchBox.timing.res;
   this->timing.capInput = switchBox.timing.capInput;
   this->timing.capOutput = switchBox.timing.capOutput;
   this->timing.delay = switchBox.timing.delay;

   this->area.buffer = switchBox.area.buffer;
   this->area.muxTransistor = switchBox.area.muxTransistor;

   this->dims_ = switchBox.dims_;
   this->origin_ = switchBox.origin_;

   this->mapTable_ = switchBox.mapTable_;
}

//===========================================================================//
// Method         : ~TAS_SwitchBox_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_SwitchBox_c::~TAS_SwitchBox_c( 
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
TAS_SwitchBox_c& TAS_SwitchBox_c::operator=( 
      const TAS_SwitchBox_c& switchBox )
{
   if( &switchBox != this )
   {
      this->srName = switchBox.srName;
      this->model = switchBox.model;
      this->type = switchBox.type;
      this->fs = switchBox.fs;
      this->timing.res = switchBox.timing.res;
      this->timing.capInput = switchBox.timing.capInput;
      this->timing.capOutput = switchBox.timing.capOutput;
      this->timing.delay = switchBox.timing.delay;
      this->area.buffer = switchBox.area.buffer;
      this->area.muxTransistor = switchBox.area.muxTransistor;
      this->dims_ = switchBox.dims_;
      this->origin_ = switchBox.origin_;
      this->mapTable_ = switchBox.mapTable_;
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
bool TAS_SwitchBox_c::operator==( 
      const TAS_SwitchBox_c& switchBox ) const
{
   return(( this->srName == switchBox.srName ) ? true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_SwitchBox_c::operator!=( 
      const TAS_SwitchBox_c& switchBox ) const
{
   return( !this->operator==( switchBox ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_SwitchBox_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<switchbox name=\"%s\"", 
                                        TIO_SR_STR( this->srName ));

   if( this->model != TAS_SWITCH_BOX_MODEL_UNDEFINED )
   {
      string srModel;
      TAS_ExtractStringSwitchBoxModelType( this->model, &srModel );
      printHandler.Write( pfile, 0, " model=\"%s\"", 
                                    TIO_SR_STR( srModel ));
   }

   if( this->type != TAS_SWITCH_BOX_UNDEFINED )
   {
      string srType;
      TAS_ExtractStringSwitchBoxType( this->type, &srType );
      printHandler.Write( pfile, 0, " type=\"%s\"", 
                                    TIO_SR_STR( srType ));
   }

   if( this->fs != UINT_MAX )
   {
      printHandler.Write( pfile, 0, " fs=\"%u\"", this->fs );
   }
   else
   {
      printHandler.Write( pfile, 0, " fs=\"full\"" );
   }

   if( this->dims_.IsValid( ) || this->origin_.IsValid( ))
   {
      printHandler.Write( pfile, 0, "\n           " ); 

      if( this->dims_.IsValid( ))
      {
         printHandler.Write( pfile, 0, "<size> %0.*f %0.*f </size>", 
                                       precision, this->dims_.dx, 
                                       precision, this->dims_.dy );
      }
      if( this->origin_.IsValid( ))
      {
         printHandler.Write( pfile, 0, "<origin> %0.*f %0.*f </origin>", 
                                       precision, this->origin_.x, 
                                       precision, this->origin_.y );
      }
   }
   printHandler.Write( pfile, 0, ">\n" );
   spaceLen += 3;
   
   if( TCTF_IsGT( this->timing.res, 0.0 ) ||
       TCTF_IsGT( this->timing.capInput, 0.0 ) ||
       TCTF_IsGT( this->timing.capOutput, 0.0 ) ||
       TCTF_IsGT( this->timing.delay, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "<timing\n" );
      spaceLen += 3;

      if( TCTF_IsGT( this->timing.res, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "res=\"%0.*f\"\n", 
                                              precision, this->timing.res );
      }
      if( TCTF_IsGT( this->timing.capInput, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "cap_in=\"%0.*e\"\n", 
                                              precision + 1, this->timing.capInput );
      }
      if( TCTF_IsGT( this->timing.capOutput, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "cap_out=\"%0.*e\"\n", 
                                              precision + 1, this->timing.capOutput );
      }
      if( TCTF_IsGT( this->timing.delay, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "delay=\"%0.*e\"\n", 
                                              precision + 1, this->timing.delay );
      }

      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "/>\n" );
   }

   if( this->mapTable_.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<mapping>\n" );
      spaceLen += 3;

      for( size_t i = 0; i < this->mapTable_.GetLength( ); ++i )
      {
         printHandler.Write( pfile, spaceLen, "" );

         const TAS_MapList_t& mapList = *this->mapTable_[i];
         for( size_t j = 0; j < mapList.GetLength( ); ++j )
         {
            TAS_SideIndex_t& sideIndex = *mapList[j];
            string srSideIndex;
            sideIndex.ExtractString( &srSideIndex, 1 );
            printHandler.Write( pfile, 0, "%s ", TIO_SR_STR( srSideIndex ));
         }
         printHandler.Write( pfile, 0, "\n" );
      }
      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "</mapping>\n" );
   }

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</switchbox>\n" );
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_SwitchBox_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_SwitchBox_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srType;
   TAS_ExtractStringSwitchBoxType( this->type, &srType );

   printHandler.Write( pfile, spaceLen, "<switch type=\"%s\" name=\"%s\" R=\"%0.*f\" Cin=\"%0.*e\" Cout=\"%0.*e\" Tdel=\"%0.*e\" buf_size=\"%0.*f\" mux_trans_size=\"%0.*f\"/>\n",
                                        TIO_SR_STR( srType ),
                                        TIO_SR_STR( this->srName ),
                                        precision, this->timing.res,
                                        precision + 1, this->timing.capInput,
                                        precision + 1, this->timing.capOutput,
                                        precision + 1, this->timing.delay,
                                        precision, this->area.buffer,
                                        precision, this->area.muxTransistor );
}
