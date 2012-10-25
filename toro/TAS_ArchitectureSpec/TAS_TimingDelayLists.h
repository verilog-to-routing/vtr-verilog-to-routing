//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_TimingDelayLists 
//           class.
//
//           Inline methods include:
//           - IsValid
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
                             // Defines common min/max delays from all pins
                             // asso. with the given input port to all pins
                             // asso. with the given output port
   TAS_TimingDelayList_t delayMatrixList;    
                             // Defines unique min/max delays from each pin 
                             // asso. with the given input port to each pin 
                             // asso. with the given output port
   TAS_TimingDelayList_t tSetupList;     
                             // Defines clock setup time for all pins 
                             // asso. with the given port
                             // Applies to sequential blocks (see flipflops)
   TAS_TimingDelayList_t tHoldList;     
                             // Defines clock hold time for all pins 
                             // asso. with the given port
                             // Applies to sequential blocks (see flipflops)
   TAS_TimingDelayList_t clockToQList;
                             // Defines clock-to-Q delay for all pins 
                             // asso. with the given output port 
                             // Applies to sequential blocks (see flipflops)
   TAS_TimingDelayList_t capList; 
                             // Defines common min/max caps from all pins
                             // asso. with the given input port to all pins
                             // asso. with the given output port
   TAS_TimingDelayList_t capMatrixList;    
                             // Defines unique min/max caps from each pin 
                             // asso. with the given input port to each pin 
                             // asso. with the given output port
   TAS_TimingDelayList_t packPatternList; 
                             // Defines pack pattern (for molecule support)

private:

   enum TAS_DefCapacity_e 
   { 
      TAS_DELAY_LIST_DEF_CAPACITY = 1,
      TAS_DELAY_MATRIX_LIST_DEF_CAPACITY = 1,
      TAS_T_SETUP_LIST_DEF_CAPACITY = 1,
      TAS_T_HOLD_LIST_DEF_CAPACITY = 1,
      TAS_CLOCK_TO_Q_LIST_DEF_CAPACITY = 1,
      TAS_CAP_LIST_DEF_CAPACITY = 1,
      TAS_CAP_MATRIX_LIST_DEF_CAPACITY = 1,
      TAS_PACK_PATTERN_LIST_DEF_CAPACITY = 1
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
           this->tSetupList.IsValid( ) ||
           this->tHoldList.IsValid( ) ||
           this->clockToQList.IsValid( ) ||
           this->capList.IsValid( ) ||
           this->capMatrixList.IsValid( ) ||
           this->packPatternList.IsValid( ) ?
           true : false );
}

#endif
