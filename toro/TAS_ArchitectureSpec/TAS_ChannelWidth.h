//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_ChannelWidth class
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

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
