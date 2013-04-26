//===========================================================================//
// Purpose : Declaration and inline definitions for a TFV_FabricData class.
//
//           Inline methods include:
//           - SetDataType, SetName, SetMasterName, SetOrigin
//           - SetTrackIndex, SetTrackHorzCount, SetTrackVertCount
//           - SetSliceCount, SetSliceCapacity
//           - SetTiming
//           - InitMapTable
//           - AddMap
//           - AddPin
//           - AddConnection
//           - GetDataType, GetName, GetMasterName, GetOrigin
//           - GetTrackIndex, GetTrackHorzCount, GetTrackVertCount
//           - GetSliceCount, GetSliceCapacity
//           - GetTiming
//           - GetMapTable
//           - GetPinList
//           - GetConnectionList
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

#ifndef TFV_FABRIC_DATA_H
#define TFV_FABRIC_DATA_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Typedefs.h"
#include "TC_SideIndex.h"
#include "TC_MapTable.h"

#include "TGO_Point.h"

#include "TFV_Typedefs.h"
#include "TFV_FabricPin.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
class TFV_FabricData_c 
{
public:

   TFV_FabricData_c( void );
   TFV_FabricData_c( TFV_DataType_t dataType ); 
   TFV_FabricData_c( const TFV_FabricData_c& fabricData );
   ~TFV_FabricData_c( void );

   TFV_FabricData_c& operator=( const TFV_FabricData_c& fabricData );
   bool operator==( const TFV_FabricData_c& fabricData ) const;
   bool operator!=( const TFV_FabricData_c& fabricData ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;
   void ExtractString( string* psrDataString ) const;

   void SetDataType( TFV_DataType_t dataType );
   void SetName( const string& srName );
   void SetName( const char* pszName );
   void SetMasterName( const string& srMasterName );
   void SetMasterName( const char* pszMasterName );
   void SetOrigin( const TGO_Point_c& origin );
   void SetTrackIndex( unsigned int index );
   void SetTrackHorzCount( unsigned int horzCount );
   void SetTrackVertCount( unsigned int vertCount );
   void SetSliceCount( unsigned int count );
   void SetSliceCapacity( unsigned int capacity );
   void SetTiming( double res, 
                   double capInput,
                   double capOutput,
                   double delay );

   void InitMapTable( unsigned int horzWidth,
                      unsigned int vertWidth );
   void AddMap( TC_SideMode_t side1, unsigned int index1,
                TC_SideMode_t side2, unsigned int index2 );
   void AddMap( const TC_MapTable_c& mapTable );

   void AddPin( const TFV_FabricPin_c& pin );
   void AddPin( const TFV_FabricPinList_t& pinList );
   void AddConnection( const TFV_Connection_t& connection );
   void AddConnection( const TFV_ConnectionList_t& connectionList );

   TFV_DataType_t GetDataType( void ) const;
   const char* GetName( void ) const;
   const char* GetMasterName( void ) const;
   const TGO_Point_c& GetOrigin( void ) const;

   unsigned int GetTrackIndex( void ) const;
   unsigned int GetTrackHorzCount( void ) const;
   unsigned int GetTrackVertCount( void ) const;
   unsigned int GetSliceCount( void ) const;
   unsigned int GetSliceCapacity( void ) const;
   void GetTiming( double* pres, 
                   double* pcapInput,
                   double* pcapOutput,
                   double* pdelay ) const;
   const TC_MapTable_c& GetMapTable( void ) const;
   const TFV_FabricPinList_t& GetPinList( void ) const;
   const TFV_ConnectionList_t& GetConnectionList( void ) const;

   double CalcTrack( const TGS_Region_c& region,
                     const TC_SideIndex_c& sideIndex ) const;
   double CalcTrack( const TGS_Region_c& region,
                     TC_SideMode_t side,
                     unsigned int index ) const;
   double CalcTrack( const TGS_Region_c& region,
                     TGS_OrientMode_t orient,
                     unsigned int index ) const;

   TGS_Point_c CalcPoint( const TGS_Region_c& region,
                          const TFV_FabricPin_c& pin ) const;
   TGS_Point_c CalcPoint( const TGS_Region_c& region,
                          TC_SideMode_t side,
                          double delta = 0.0,
                          double width = 0.0 ) const;

   bool IsValid( void ) const;

private:

   TFV_DataType_t dataType_;     // Defines fabric data type
                                 // (eg. PB, IO, SB, CB, channel, or segment)

   string srName_;               // Define fabric data name
   string srMasterName_;         // Optional fabric master 
                                 // (specifically for PB, IO, and SB objects)
   TGO_Point_c origin_;          // Define fabric origin point
                                 // (specifically for PB, IO, and SB objects)

   TFV_FabricPinList_t pinList_; // Fabric data pin list
                                 // (specifically for PB and IO objects)
   TFV_ConnectionList_t connectionList_;
                                 // Fabric data connection list
                                 // (specifically for CB objects)
   TC_MapTable_c mapTable_;      // Fabric data side map table
                                 // (specifically for SB objects)
   class TFV_FabricDataTrack_c
   {
   public:

      unsigned int index;        // Fabric data track index
      unsigned int horzCount;    // Fabric data track count (horizontal)
      unsigned int vertCount;    // Fabric data track count (vertical)
   } track_;                     // (specifically for channels and segments)

   class TFV_FabricDataSlice_c
   {
   public:

      unsigned int count;       // Fabric data slice count (total #of slices)
      unsigned int capacity;    // Fabric data slice capacity (pins per slice)
   } slice_;                    // (specifically for IO objects)

   class TFV_FabricDataTiming_c
   {
   public:

      double res;                // Fabric data resistance (ohms)
      double capInput;           // Fabric data input capacitance (farads)
      double capOutput;          // Fabric data output capacitance (farads)
      double delay;              // Fabric data intrinsic delay (seconds)
   } timing_;                    // (specifically for PB, IO, and SB objects)

private:

   enum TFV_DefCapacity_e 
   { 
      TFV_PIN_LIST_DEF_CAPACITY = 64,
      TFV_CONNECTION_LIST_DEF_CAPACITY = 16
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
inline void TFV_FabricData_c::SetDataType(
      TFV_DataType_t dataType )
{
   this->dataType_ = dataType;
}

//===========================================================================//
inline void TFV_FabricData_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TFV_FabricData_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline void TFV_FabricData_c::SetMasterName( 
      const string& srMasterName )
{
   this->srMasterName_ = srMasterName;
}

//===========================================================================//
inline void TFV_FabricData_c::SetMasterName( 
      const char* pszMasterName )
{
   this->srMasterName_ = TIO_PSZ_STR( pszMasterName );
}

//===========================================================================//
inline void TFV_FabricData_c::SetOrigin(
      const TGO_Point_c& origin )
{
   this->origin_ = origin;
}

//===========================================================================//
inline void TFV_FabricData_c::SetTrackIndex(
      unsigned int index )
{
   this->track_.index = index;
}

//===========================================================================//
inline void TFV_FabricData_c::SetTrackHorzCount(
      unsigned int horzCount )
{
   this->track_.horzCount = horzCount;
}

//===========================================================================//
inline void TFV_FabricData_c::SetTrackVertCount(
      unsigned int vertCount )
{
   this->track_.vertCount = vertCount;
}

//===========================================================================//
inline void TFV_FabricData_c::SetSliceCount(
      unsigned int count )
{
   this->slice_.count = count;
}

//===========================================================================//
inline void TFV_FabricData_c::SetSliceCapacity(
      unsigned int capacity )
{
   this->slice_.capacity = capacity;
}

//===========================================================================//
inline void TFV_FabricData_c::SetTiming( 
      double res, 
      double capInput,
      double capOutput,
      double delay )
{
   this->timing_.res = res;
   this->timing_.capInput = capInput;
   this->timing_.capOutput = capOutput;
   this->timing_.delay = delay;
}

//===========================================================================//
inline void TFV_FabricData_c::InitMapTable(
      unsigned int horzWidth,
      unsigned int vertWidth )
{
   this->mapTable_.Init( horzWidth, vertWidth );
}

//===========================================================================//
inline void TFV_FabricData_c::AddMap(
      TC_SideMode_t side1, 
      unsigned int  index1,
      TC_SideMode_t side2, 
      unsigned int  index2 )
{
   this->mapTable_.Add( side1, index1, side2, index2 );
}

//===========================================================================//
inline void TFV_FabricData_c::AddMap(
      const TC_MapTable_c& mapTable )
{
   this->mapTable_.Add( mapTable );
}

//===========================================================================//
inline void TFV_FabricData_c::AddPin(
      const TFV_FabricPin_c& pin )
{
   this->pinList_.Add( pin );
}

//===========================================================================//
inline void TFV_FabricData_c::AddPin(
      const TFV_FabricPinList_t& pinList )
{
   this->pinList_.Add( pinList );
}

//===========================================================================//
inline void TFV_FabricData_c::AddConnection(
      const TFV_Connection_t& connection )
{
   this->connectionList_.Add( connection );
}

//===========================================================================//
inline void TFV_FabricData_c::AddConnection(
      const TFV_ConnectionList_t& connectionList )
{
   this->connectionList_.Add( connectionList );
}

//===========================================================================//
inline TFV_DataType_t TFV_FabricData_c::GetDataType( 
      void ) const
{
   return( this->dataType_ );
}

//===========================================================================//
inline const char* TFV_FabricData_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TFV_FabricData_c::GetMasterName( 
      void ) const
{
   return( TIO_SR_STR( this->srMasterName_ ));
}

//===========================================================================//
inline const TGO_Point_c& TFV_FabricData_c::GetOrigin(
      void ) const
{
   return( this->origin_ );
}

//===========================================================================//
inline unsigned int TFV_FabricData_c::GetTrackIndex( 
      void ) const
{
   return( this->track_.index );
}

//===========================================================================//
inline unsigned int TFV_FabricData_c::GetSliceCount( 
      void ) const
{
   return( this->slice_.count );
}

//===========================================================================//
inline unsigned int TFV_FabricData_c::GetSliceCapacity( 
      void ) const
{
   return( this->slice_.capacity );
}

//===========================================================================//
inline unsigned int TFV_FabricData_c::GetTrackHorzCount( 
      void ) const
{
   return( this->track_.horzCount );
}

//===========================================================================//
inline unsigned int TFV_FabricData_c::GetTrackVertCount( 
      void ) const
{
   return( this->track_.vertCount );
}

//===========================================================================//
inline void TFV_FabricData_c::GetTiming( 
      double* pres, 
      double* pcapInput,
      double* pcapOutput,
      double* pdelay ) const
{
   *pres = this->timing_.res;
   *pcapInput = this->timing_.capInput;
   *pcapOutput = this->timing_.capOutput;
   *pdelay = this->timing_.delay;
}

//===========================================================================//
inline const TC_MapTable_c& TFV_FabricData_c::GetMapTable( 
      void ) const
{
   return( this->mapTable_ );
}

//===========================================================================//
inline const TFV_FabricPinList_t& TFV_FabricData_c::GetPinList(
      void ) const
{
   return( this->pinList_ );
}

//===========================================================================//
inline const TFV_ConnectionList_t& TFV_FabricData_c::GetConnectionList(
      void ) const
{
   return( this->connectionList_ );
}

//===========================================================================//
inline bool TFV_FabricData_c::IsValid( 
      void ) const
{
   return( this->dataType_ != TFV_DATA_UNDEFINED ? true : false );
}

#endif
