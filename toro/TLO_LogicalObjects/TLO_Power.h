//===========================================================================//
// Purpose : Declaration and inline definitions for a TLO_Power class.
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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
