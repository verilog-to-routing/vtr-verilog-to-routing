//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_PhysicalBlock class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - SetDims, SetOrigin
//           - SetUsage
//           - SetPhysicalBlockParent
//           - SetModeParent
//           - GetUsage
//           - GetPhysicalBlockParent
//           - GetModeParent
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
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

#ifndef TAS_PHYSICAL_BLOCK_H
#define TAS_PHYSICAL_BLOCK_H

#include <cstdio>
#include <string>
using namespace std;

#include "TGS_Point.h"

#include "TLO_Typedefs.h"
#include "TLO_Port.h"

#include "TAS_Typedefs.h"
#include "TAS_Mode.h"
#include "TAS_Interconnect.h"
#include "TAS_ConnectionFc.h"
#include "TAS_PinAssign.h"
#include "TAS_GridAssign.h"
#include "TAS_TimingDelayLists.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAS_PhysicalBlock_c
{
public:

   TAS_PhysicalBlock_c( void );
   TAS_PhysicalBlock_c( const string& srName );
   TAS_PhysicalBlock_c( const char* pszName );
   TAS_PhysicalBlock_c( const TAS_PhysicalBlock_c& physicalBlock );
   ~TAS_PhysicalBlock_c( void );

   TAS_PhysicalBlock_c& operator=( const TAS_PhysicalBlock_c& physicalBlock );
   bool operator==( const TAS_PhysicalBlock_c& physicalBlock ) const;
   bool operator!=( const TAS_PhysicalBlock_c& physicalBlock ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0,
               size_t hierLevel = 0,
               TAS_ModeList_t* pmodeList = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   void SetDims( const TGS_FloatDims_t& dims );
   void SetOrigin( const TGS_Point_c& origin );

   void SetUsage( TAS_UsageMode_t usage );

   void SetPhysicalBlockParent( const TAS_PhysicalBlock_c* pphysicalBlock );
   void SetModeParent( const TAS_Mode_c* pmode );

   TAS_UsageMode_t GetUsage( void ) const;

   const TAS_PhysicalBlock_c* GetPhysicalBlockParent( void ) const;
   const TAS_Mode_c* GetModeParent( void ) const;

   bool IsValid( void ) const;

public:

   string srName;       // Physical block name

   unsigned int width;  // Defines block width in tile grid units
   unsigned int height; // Defines block height in tile grid units
   unsigned int capacity;
                        // Defines block capacity (for IOs)
   unsigned int numPB;  // Defines number of cluster or leaf (BLE) nodes
                        // A "cluster" is assumed to contain 1+ <pb_type>
                        // A "leaf" is assumed to contain 0 <pb_type>

   TAS_PhysicalBlockModelType_t modelType;
                        // Selects STANDARD|CUSTOM (ie. BLIF) model type
   string srModelName;  // Used to define either a "standard" model 
                        // (eg. class ".names", ".latch", ".input", or ".output") 
                        // or a "custom" model (eg. BLIF ".subckt user_block_A").  

   TAS_ClassType_t classType;  
                        // Selects pre-defined class type 
                        // (eg. "lut", "flipflop", or "memory")

   TAS_ConnectionFc_c fcIn;  // Defines connection box flexibility 
   TAS_ConnectionFc_c fcOut; // (ie. #of wires in channel connected to output pin)

   TAS_ModeNameList_t modeNameList;   
                             // Defines physical block's internal mode list
   TAS_ModeList_t modeList;  // List of hierarchical modes (ie. <mode>...)

   TAS_PhysicalBlockList_t physicalBlockList;
                             // List of physical blocks (ie. <pb_type>...)
   TAS_InterconnectList_t interconnectList; 
                             // Describes intra-block net connectivity

   TLO_PortList_t portList;  // List of input, output, and/or clock ports

   TAS_TimingDelayLists_c timingDelayLists;
                             // Defines max, clock setup, and clock-to-Q delays

   TAS_PinAssignPatternType_t pinAssignPattern; 
                             // Selects pattern: SPREAD|CUSTOM 
   TAS_PinAssignList_t pinAssignList; 
                             // Defines pin side/offset assignments 
                             // Valid when pin assign pattern = CUSTOM
                             // Applies to top-level blocks only
   TAS_GridAssignList_t gridAssignList; 
                             // Defines block column assignments (optional)
                             // Applies to top-level blocks only

   class TAS_PhysicalBlockSorted_c
   {
   public:

      TAS_ModeSortedList_t          modeList; 
      TAS_PhysicalBlockSortedList_t physicalBlockList;
   } sorted;

private:

   TGS_FloatDims_t dims_;    // Defines width/height dimensions (dx,dy)
   TGS_Point_c     origin_;  // Defines reference origin (x,y)

   TAS_UsageMode_t usage_;   // Defines usage mode (physical block or input/output)

   TAS_PhysicalBlock_c* pphysicalBlockParent_;
   TAS_Mode_c*          pmodeParent_; 

private:

   enum TAS_DefCapacity_e 
   { 
      TAS_MODE_LIST_DEF_CAPACITY = 4,
      TAS_MODE_NAME_LIST_DEF_CAPACITY = 16,
      TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY = 64,
      TAS_INTERCONNECT_LIST_DEF_CAPACITY = 64,
      TAS_PORT_LIST_DEF_CAPACITY = 64,
      TAS_PIN_ASSIGN_LIST_DEF_CAPACITY = 1,
      TAS_GRID_ASSIGN_LIST_DEF_CAPACITY = 1
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TAS_PhysicalBlock_c::SetName( 
      const string& srName_ )
{
   this->srName = srName_;
}

//===========================================================================//
inline void TAS_PhysicalBlock_c::SetName( 
      const char* pszName )
{
   this->srName = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TAS_PhysicalBlock_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName ));
}

//===========================================================================//
inline const char* TAS_PhysicalBlock_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName ));
}

//===========================================================================//
inline void TAS_PhysicalBlock_c::SetDims(
      const TGS_FloatDims_t& dims )
{
   this->dims_ = dims;
}

//===========================================================================//
inline void TAS_PhysicalBlock_c::SetOrigin(
      const TGS_Point_c& origin )
{
   this->origin_ = origin;
}

//===========================================================================//
inline void TAS_PhysicalBlock_c::SetUsage(
      const TAS_UsageMode_t usage )
{
   this->usage_ = usage;
}

//===========================================================================//
inline void TAS_PhysicalBlock_c::SetPhysicalBlockParent( 
      const TAS_PhysicalBlock_c* pphysicalBlock )
{
   this->pphysicalBlockParent_ = const_cast< TAS_PhysicalBlock_c* >( pphysicalBlock );
}

//===========================================================================//
inline void TAS_PhysicalBlock_c::SetModeParent( 
      const TAS_Mode_c* pmode )
{
   this->pmodeParent_ = const_cast< TAS_Mode_c* >( pmode );
}

//===========================================================================//
inline TAS_UsageMode_t TAS_PhysicalBlock_c::GetUsage(
      void ) const
{
   return( this->usage_ );
}

//===========================================================================//
inline const TAS_PhysicalBlock_c* TAS_PhysicalBlock_c::GetPhysicalBlockParent( 
      void ) const
{
   return( this->pphysicalBlockParent_ );
}

//===========================================================================//
inline const TAS_Mode_c* TAS_PhysicalBlock_c::GetModeParent( 
      void ) const
{
   return( this->pmodeParent_ );
}

//===========================================================================//
inline bool TAS_PhysicalBlock_c::IsValid( 
      void ) const
{
   return( this->srName.length( ) ? true : false );
}

#endif
