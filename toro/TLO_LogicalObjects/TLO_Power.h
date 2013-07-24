//===========================================================================//
// Purpose : Declaration and inline definitions for a TLO_Power class.
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

#ifndef TLO_POWER_H
#define TLO_POWER_H

#include <cstdio>
using namespace std;

#include "TLO_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
class TLO_Power_c
{
public:

   TLO_Power_c( void );
   TLO_Power_c( TLO_PowerType_t type,
                double cap,
                double relativeLength,
                double absoluteLength );
   TLO_Power_c( TLO_PowerType_t type,
                double size );
   TLO_Power_c( bool initialized,
                double energyPerToggle,
                bool scaledByStaticProb,
                bool scaledByStaticProb_n );
   TLO_Power_c( const TLO_Power_c& power );
   ~TLO_Power_c( void );

   TLO_Power_c& operator=( const TLO_Power_c& power );
   bool operator==( const TLO_Power_c& power ) const;
   bool operator!=( const TLO_Power_c& power ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void Clear( void );

   void SetWire( TLO_PowerType_t type,
                 double cap,
                 double relativeLength,
                 double absoluteLength );
   void SetBuffer( TLO_PowerType_t type,
                   double size );
   void SetPinToggle( bool initialized,
                      double energyPerToggle,
                      bool scaledByStaticProb,
                      bool scaledByStaticProb_n );

   void GetWire( TLO_PowerType_t* ptype,
                 double* pcap,
                 double* prelativeLength,
                 double* pabsoluteLength ) const;
   void GetBuffer( TLO_PowerType_t* ptype,
                   double* psize ) const;
   void GetPinToggle( bool* pinitialized,
                      double* penergyPerToggle,
                      bool* pscaledByStaticProb,
                      bool* pscaledByStaticProb_n ) const;

   bool IsValidWire( void ) const;
   bool IsValidBuffer( void ) const;
   bool IsValidPinToggle( void ) const;

   bool IsValid( void ) const;

private:

   // Wire-related properties
   class TLO_PowerWire_c
   {
   public:

      TLO_PowerType_t type;
      double          cap;
      double          relativeLength;
      double          absoluteLength;
   } wire_;

   // Buffer-related properties
   class TLO_PowerBuffer_c
   {
   public:

      TLO_PowerType_t type;
      double          size;
   } buffer_;

   // Pin-toggle properties
   class TLO_PowerPinToggle_c
   {
   public:

      bool            initialized;
      double          energyPerToggle;
      bool            scaledByStaticProb;
      bool            scaledByStaticProb_n;
   } pinToggle_;
};

#endif
