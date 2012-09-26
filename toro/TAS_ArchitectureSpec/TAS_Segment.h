//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_Segment class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

#ifndef TAS_SEGMENT_H
#define TAS_SEGMENT_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Bit.h"

#include "TAS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAS_Segment_c
{
public:

   TAS_Segment_c( void );
   TAS_Segment_c( const TAS_Segment_c& segment );
   ~TAS_Segment_c( void );

   TAS_Segment_c& operator=( const TAS_Segment_c& segment );
   bool operator==( const TAS_Segment_c& segment ) const;
   bool operator!=( const TAS_Segment_c& segment ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   unsigned int length; // Defines number of blocks spanned by track segment
                        // Use UINT_MAX to define "longline" segments that 
                        // span the entire FPGA fabric

   TAS_SegmentDirType_t dirType; 
                        // Selects direction: UNDIRECTIONAL|BIDIRECTIONAL 
                        // Assumes all track segments have same direction
   double trackFreq;    // Defines routing track frequency 
                        // Based on percentage w.r.t. all track frequencies
   double trackPercent; // Defines routing track percentage 
                        // Percentage is defined after all segments are loaded

   TAS_BitPattern_t sbPattern; // Describes switch box population pattern
   TAS_BitPattern_t cbPattern; // Describes connection box population pattern

   string srMuxSwitchName;     // Maps segment to a switch box name
                               // Applies to UNIDIRECTIONAL segments only
   string srWireSwitchName;    // Maps segment to a switch box name
                               // Applies to BIDIRECTIONAL segments only
   string srOutputSwitchName;  // Maps segment to a switch box name
                               // Applies to BIDIRECTIONAL segments only
   class TAS_SegmentTiming_c
   {
   public:

      double res;       // Segment resistance (ohms) per block length
      double cap;       // Segment capacitance (farads) per block length
   } timing;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TAS_Segment_c::IsValid( 
      void ) const
{
   return( this->length > 0 ? true : false );
}

#endif
