//===========================================================================//
//Purpose : Declaration and inline definitions for a TLO_Cell class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - SetMasterName
//           - SetPortList
//           - SetPinList
//           - SetTimingInputPinCap
//           - SetTimingInputPinDelay
//           - GetMasterName
//           - GetPortList
//           - GetPinList
//           - GetSource
//           - GetTimingInputPinCap
//           - GetTimingInputPinDelay
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

#ifndef TLO_CELL_H
#define TLO_CELL_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_NameType.h"

#include "TLO_Typedefs.h"
#include "TLO_Port.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TLO_Cell_c
{
public:

   TLO_Cell_c( void );
   TLO_Cell_c( const string& srName,
               TLO_CellSource_t source = TLO_CELL_SOURCE_UNDEFINED );
   TLO_Cell_c( const char* pszName,
               TLO_CellSource_t source = TLO_CELL_SOURCE_UNDEFINED );
   TLO_Cell_c( const TLO_Cell_c& cell );
   ~TLO_Cell_c( void );

   TLO_Cell_c& operator=( const TLO_Cell_c& cell );
   bool operator<( const TLO_Cell_c& cell ) const;
   bool operator==( const TLO_Cell_c& cell ) const;
   bool operator!=( const TLO_Cell_c& cell ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;
   void PrintBLIF( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   void SetMasterName( const string& srMasterName );
   void SetMasterName( const char* pszMasterName );
   void SetPortList( const TLO_PortList_t& portList );
   void SetPinList( const TLO_PinList_t& pinList );
   void SetSource( TLO_CellSource_t source );
   void SetTimingInputPinCap( double inputPinCap );
   void SetTimingInputPinDelay( double inputPinDelay );

   const char* GetMasterName( void ) const;
   const TLO_PortList_t& GetPortList( void ) const;
   const TLO_PinList_t& GetPinList( void ) const;
   TLO_CellSource_t GetSource( void ) const;
   double GetTimingInputPinCap( void ) const;
   double GetTimingInputPinDelay( void ) const;

   void AddPort( const string& srName, 
                 TC_TypeMode_t type );
   void AddPort( const char* pszName,
                 TC_TypeMode_t type );
   void AddPort( const TLO_Port_c& port );
   void AddPin( const TLO_Pin_t& pin );

   size_t FindPortCount( TC_TypeMode_t type ) const;

   bool HasPortType( TC_TypeMode_t type ) const;

   bool IsValid( void ) const;

private:

   string srName_;              // Defines cell name
   string srMasterName_;        // Defines cell master (architecture) name

   TLO_PortList_t portList_;    // List of cell ports (sorted by name)
   TLO_PinList_t  pinList_;     // List of cell pins (sorted by name)

   TLO_CellSource_t source_;    // Defines cell source (optional)
                                // For example: "auto_defined" vs. "user_defined"

   class TLO_CellTiming_c
   {
   public:

      double inputPinCap;       // Input pin capacitance (fF)
      double inputPinDelay;     // Input pin delay (ps)
   } timing_;

private:

   enum TLO_DefCapacity_e 
   { 
      TLO_PORT_LIST_DEF_CAPACITY = 64,
      TLO_PIN_LIST_DEF_CAPACITY = 64
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TLO_Cell_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TLO_Cell_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TLO_Cell_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TLO_Cell_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline void TLO_Cell_c::SetMasterName( 
      const string& srMasterName )
{
   this->srMasterName_ = srMasterName;
}

//===========================================================================//
inline void TLO_Cell_c::SetMasterName( 
      const char* pszMasterName )
{
   this->srMasterName_ = TIO_PSZ_STR( pszMasterName );
}

//===========================================================================//
inline void TLO_Cell_c::SetPortList( 
      const TLO_PortList_t& portList )
{
   this->portList_ = portList;
}

//===========================================================================//
inline void TLO_Cell_c::SetPinList( 
      const TLO_PinList_t& pinList )
{
   this->pinList_ = pinList;
}

//===========================================================================//
inline void TLO_Cell_c::SetSource( 
      TLO_CellSource_t source )
{
   this->source_ = source;
}

//===========================================================================//
inline void TLO_Cell_c::SetTimingInputPinCap(
      double inputPinCap )
{
   this->timing_.inputPinCap = inputPinCap;
}

//===========================================================================//
inline void TLO_Cell_c::SetTimingInputPinDelay(
      double inputPinDelay )
{
   this->timing_.inputPinDelay = inputPinDelay;
}

//===========================================================================//
inline const char* TLO_Cell_c::GetMasterName( 
      void ) const
{
   return( TIO_SR_STR( this->srMasterName_ ));
}

//===========================================================================//
inline const TLO_PortList_t& TLO_Cell_c::GetPortList( 
      void ) const
{
   return( this->portList_ );
}

//===========================================================================//
inline const TLO_PinList_t& TLO_Cell_c::GetPinList( 
      void ) const
{
   return( this->pinList_ );
}

//===========================================================================//
inline TLO_CellSource_t TLO_Cell_c::GetSource( 
      void ) const
{
   return( this->source_ );
}

//===========================================================================//
inline double TLO_Cell_c::GetTimingInputPinCap(
      void ) const
{
   return( this->timing_.inputPinCap );
}

//===========================================================================//
inline double TLO_Cell_c::GetTimingInputPinDelay(
      void ) const
{
   return( this->timing_.inputPinDelay );
}

//===========================================================================//
inline bool TLO_Cell_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
