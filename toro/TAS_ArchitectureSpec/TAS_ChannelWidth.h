//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_ChannelWidth class
//
//           Inline methods include:
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

#ifndef TAS_CHANNEL_WIDTH_H
#define TAS_CHANNEL_WIDTH_H

#include <cstdio>
using namespace std;

#include "TAS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/15/12 jeffr : Original
//===========================================================================//
class TAS_ChannelWidth_c
{
public:

   TAS_ChannelWidth_c( void );
   TAS_ChannelWidth_c( const TAS_ChannelWidth_c& channelWidth );
   ~TAS_ChannelWidth_c( void );

   TAS_ChannelWidth_c& operator=( const TAS_ChannelWidth_c& channelWidth );
   bool operator==( const TAS_ChannelWidth_c& channelWidth ) const;
   bool operator!=( const TAS_ChannelWidth_c& channelWidth ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   TAS_ChannelUsageMode_e usageMode; // Selects channel usage mode (ie. IO, X, or Y)
   double width;                     // Defines channel width 

   TAS_ChannelDistrMode_e distrMode; // Selects distribution mode (eg. UNIFORM)
                                     // (aka. standard deviation for Gaussian)
   double peak;                      // Defines values (for Gaussian and pulse waveforms)
   double xpeak;                     // "
   double dc;                        // "
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/15/12 jeffr : Original
//===========================================================================//
inline bool TAS_ChannelWidth_c::IsValid( 
      void ) const
{
   return( this->usageMode != TAS_CHANNEL_USAGE_UNDEFINED ? true : false );
}

#endif
