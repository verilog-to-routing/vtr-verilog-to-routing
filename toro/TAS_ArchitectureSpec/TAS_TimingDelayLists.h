//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_TimingDelayLists 
//           class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

#ifndef TAS_TIMING_DELAY_LISTS_H
#define TAS_TIMING_DELAY_LISTS_H

#include <cstdio>
using namespace std;

#include "TAS_TimingDelay.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAS_TimingDelayLists_c
{
public:

   TAS_TimingDelayLists_c( void );
   TAS_TimingDelayLists_c( const TAS_TimingDelayLists_c& timingDelayLists );
   ~TAS_TimingDelayLists_c( void );

   TAS_TimingDelayLists_c& operator=( const TAS_TimingDelayLists_c& timingDelayLists );
   bool operator==( const TAS_TimingDelayLists_c& timingDelayLists ) const;
   bool operator!=( const TAS_TimingDelayLists_c& timingDelayLists ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   TAS_TimingDelayList_t delayList; 
                             // Defines a common max delay from all pins
                             // asso. with the given input port to all pins
                             // asso. with the given output port
   TAS_TimingDelayList_t delayMatrixList;    
                             // Defines unique max delays from each pin 
                             // asso. with the given input port to each pin 
                             // asso. with the given output port
   TAS_TimingDelayList_t delayClockSetupList;     
                             // Defines clock setup time for all pins 
                             // asso. with the given port
                             // Applies to sequential blocks (see flipflops)
   TAS_TimingDelayList_t delayClockToQList;
                             // Defines clock-to-Q delay for all pins 
                             // asso. with the given output port 
                             // Applies to sequential blocks (see flipflops)
private:

   enum TAS_DefCapacity_e 
   { 
      TAS_DELAY_LIST_DEF_CAPACITY = 1,
      TAS_DELAY_MATRIX_LIST_DEF_CAPACITY = 1,
      TAS_DELAY_CLOCK_SETUP_LIST_DEF_CAPACITY = 1,
      TAS_DELAY_CLOCK_TO_Q_LIST_DEF_CAPACITY = 1
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
inline bool TAS_TimingDelayLists_c::IsValid( 
      void ) const
{
   return( this->delayList.IsValid( ) ||
           this->delayMatrixList.IsValid( ) ||
           this->delayClockSetupList.IsValid( ) ||
           this->delayClockToQList.IsValid( ) ?
           true : false );
}

#endif
