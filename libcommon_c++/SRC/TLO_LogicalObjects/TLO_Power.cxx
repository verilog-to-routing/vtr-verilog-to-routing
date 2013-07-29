//===========================================================================//
// Purpose : Method definitions for the TLO_Power class.
//
//           Public methods include:
//           - TLO_Power_c, ~TLO_Power_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - Clear
//           - SetWire
//           - SetBuffer
//           - SetPinToggle
//           - GetWire
//           - GetBuffer
//           - GetPinToggle
//           - IsValidWire
//           - IsValidBuffer
//           - IsValidPinToggle
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#include "TIO_PrintHandler.h"

#include "TLO_StringUtils.h"
#include "TLO_Power.h"

//===========================================================================//
// Method         : TLO_Power_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
TLO_Power_c::TLO_Power_c( 
      void )
{
   this->Clear( );
} 

//===========================================================================//
TLO_Power_c::TLO_Power_c( 
      TLO_PowerType_t type,
      double          cap,
      double          relativeLength,
      double          absoluteLength )
{
   this->Clear( );
   this->SetWire( type, cap, relativeLength, absoluteLength );
}

//===========================================================================//
TLO_Power_c::TLO_Power_c( 
      TLO_PowerType_t type,
      double          size )
{
   this->Clear( );
   this->SetBuffer( type, size );
}

//===========================================================================//
TLO_Power_c::TLO_Power_c( 
      bool   initialized,
      double energyPerToggle,
      bool   scaledByStaticProb,
      bool   scaledByStaticProb_n )
{
   this->Clear( );
   this->SetPinToggle( initialized, energyPerToggle, scaledByStaticProb, scaledByStaticProb_n );
}

//===========================================================================//
TLO_Power_c::TLO_Power_c( 
      const TLO_Power_c& power )
{
   this->wire_.type = power.wire_.type;
   this->wire_.cap = power.wire_.cap;
   this->wire_.relativeLength = power.wire_.relativeLength;
   this->wire_.absoluteLength = power.wire_.absoluteLength;

   this->buffer_.type = power.buffer_.type;
   this->buffer_.size = power.buffer_.size;

   this->pinToggle_.initialized = power.pinToggle_.initialized;
   this->pinToggle_.energyPerToggle = power.pinToggle_.energyPerToggle;
   this->pinToggle_.scaledByStaticProb = power.pinToggle_.scaledByStaticProb;
   this->pinToggle_.scaledByStaticProb_n = power.pinToggle_.scaledByStaticProb_n;
} 

//===========================================================================//
// Method         : ~TLO_Power_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
TLO_Power_c::~TLO_Power_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
TLO_Power_c& TLO_Power_c::operator=( 
      const TLO_Power_c& power )
{
   if( &power != this )
   {
      this->wire_.type = power.wire_.type;
      this->wire_.cap = power.wire_.cap;
      this->wire_.relativeLength = power.wire_.relativeLength;
      this->wire_.absoluteLength = power.wire_.absoluteLength;

      this->buffer_.type = power.buffer_.type;
      this->buffer_.size = power.buffer_.size;

      this->pinToggle_.initialized = power.pinToggle_.initialized;
      this->pinToggle_.energyPerToggle = power.pinToggle_.energyPerToggle;
      this->pinToggle_.scaledByStaticProb = power.pinToggle_.scaledByStaticProb;
      this->pinToggle_.scaledByStaticProb_n = power.pinToggle_.scaledByStaticProb_n;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
bool TLO_Power_c::operator==( 
      const TLO_Power_c& power ) const
{
   return(( this->wire_.type == power.wire_.type ) &&
          ( TCTF_IsEQ( this->wire_.cap, power.wire_.cap )) &&
          ( TCTF_IsEQ( this->wire_.relativeLength, power.wire_.relativeLength )) &&
          ( TCTF_IsEQ( this->wire_.absoluteLength, power.wire_.absoluteLength )) &&
          ( this->buffer_.type == power.buffer_.type ) &&
          ( TCTF_IsEQ( this->buffer_.size, power.buffer_.size )) &&
          ( this->pinToggle_.initialized == power.pinToggle_.initialized ) &&
          ( TCTF_IsEQ( this->pinToggle_.energyPerToggle, power.pinToggle_.energyPerToggle )) &&
          ( this->pinToggle_.scaledByStaticProb == power.pinToggle_.scaledByStaticProb ) &&
          ( this->pinToggle_.scaledByStaticProb_n == power.pinToggle_.scaledByStaticProb_n ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
bool TLO_Power_c::operator!=( 
      const TLO_Power_c& power ) const
{
   return( !this->operator==( power ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TLO_Power_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   if( this->IsValid( ))
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      unsigned int precision = MinGrid.GetPrecision( );

      printHandler.Write( pfile, spaceLen, "<power" );

      if( this->IsValidWire( ))
      {
         string srType;
         TLO_ExtractStringPowerType( this->wire_.type, &srType );

         printHandler.Write( pfile, 0, "wire_type=\"%s\" cap=\"%0.*e\" relative_length=\"%0.*f\", absolute_length=\"%0.*f\"" ,
                                       TIO_SR_STR( srType ),
                                       precision + 1, this->wire_.cap,
                                       precision, this->wire_.relativeLength,
                                       precision, this->wire_.absoluteLength );
      }
      if( this->IsValidBuffer( ))
      {
         string srType;
         TLO_ExtractStringPowerType( this->buffer_.type, &srType );

         printHandler.Write( pfile, 0, "buffer_type=\"%s\" size=\"%0.*f\"", 
                                       TIO_SR_STR( srType ),
                                       precision, this->buffer_.size );
      }
      if( this->IsValidPinToggle( ))
      {
         printHandler.Write( pfile, 0, "energy_per_toggle=\"%0.*e\"%s%s", 
                                       precision + 1, this->pinToggle_.energyPerToggle,
                                       this->pinToggle_.scaledByStaticProb ? " scaled_by_static_prob" : "",
                                       this->pinToggle_.scaledByStaticProb_n ? " scaled_by_static_prob_n" : "" );
      }
      printHandler.Write( pfile, 0, "/>\n" );
   }
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TLO_Power_c::Clear( 
      void )
{
   this->wire_.type = TLO_POWER_TYPE_UNDEFINED;
   this->wire_.cap = 0.0;
   this->wire_.relativeLength = 0.0;
   this->wire_.absoluteLength = 0.0;

   this->buffer_.type = TLO_POWER_TYPE_UNDEFINED;
   this->buffer_.size = 0.0;

   this->pinToggle_.initialized = false;
   this->pinToggle_.energyPerToggle = 0.0;
   this->pinToggle_.scaledByStaticProb = false;
   this->pinToggle_.scaledByStaticProb_n = false;
} 

//===========================================================================//
// Method         : SetWire
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TLO_Power_c::SetWire( 
      TLO_PowerType_t type,
      double          cap,
      double          relativeLength,
      double          absoluteLength )
{
   this->wire_.type = type;
   this->wire_.cap = cap;
   this->wire_.relativeLength = relativeLength;
   this->wire_.absoluteLength = absoluteLength;
}

//===========================================================================//
// Method         : SetBuffer
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TLO_Power_c::SetBuffer( 
      TLO_PowerType_t type,
      double          size )
{
   this->buffer_.type = type;
   this->buffer_.size = size;
}

//===========================================================================//
// Method         : SetPinToggle
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TLO_Power_c::SetPinToggle(
      bool   initialized,
      double energyPerToggle,
      bool   scaledByStaticProb,
      bool   scaledByStaticProb_n )
{
   this->pinToggle_.initialized = initialized;
   this->pinToggle_.energyPerToggle = energyPerToggle;
   this->pinToggle_.scaledByStaticProb = scaledByStaticProb;
   this->pinToggle_.scaledByStaticProb_n = scaledByStaticProb_n;
}

//===========================================================================//
// Method         : GetWire
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TLO_Power_c::GetWire( 
      TLO_PowerType_t* ptype,
      double*          pcap,
      double*          prelativeLength,
      double*          pabsoluteLength ) const
{
   *ptype = this->wire_.type;
   *pcap = this->wire_.cap;
   *prelativeLength = this->wire_.relativeLength;
   *pabsoluteLength = this->wire_.absoluteLength;
}

//===========================================================================//
// Method         : GetBuffer
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TLO_Power_c::GetBuffer( 
      TLO_PowerType_t* ptype,
      double*          psize ) const
{
   *ptype = this->buffer_.type;
   *psize = this->buffer_.size;
}

//===========================================================================//
// Method         : GetPinToggle
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TLO_Power_c::GetPinToggle(
      bool*   pinitialized,
      double* penergyPerToggle,
      bool*   pscaledByStaticProb,
      bool*   pscaledByStaticProb_n ) const
{
   *pinitialized = this->pinToggle_.initialized;
   *penergyPerToggle = this->pinToggle_.energyPerToggle;
   *pscaledByStaticProb = this->pinToggle_.scaledByStaticProb;
   *pscaledByStaticProb_n = this->pinToggle_.scaledByStaticProb_n;
}

//===========================================================================//
// Method         : IsValidWire
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
bool TLO_Power_c::IsValidWire( 
      void ) const
{
   return( this->wire_.type != TLO_POWER_TYPE_UNDEFINED ? true : false );
}

//===========================================================================//
// Method         : IsValidBuffer
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
bool TLO_Power_c::IsValidBuffer( 
      void ) const
{
   return( this->buffer_.type != TLO_POWER_TYPE_UNDEFINED ? true : false );
}

//===========================================================================//
// Method         : IsValidPinToggle
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
bool TLO_Power_c::IsValidPinToggle(
      void ) const
{
   return( this->pinToggle_.initialized ? true : false );
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
bool TLO_Power_c::IsValid( 
      void ) const
{
   return( this->IsValidWire( ) ||
           this->IsValidBuffer( ) ||
           this->IsValidPinToggle( ) ?
           true : false );
}

