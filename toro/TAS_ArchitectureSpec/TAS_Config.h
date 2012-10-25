//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_Config class.
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

#ifndef TAS_CONFIG_H
#define TAS_CONFIG_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Typedefs.h"

#include "TGS_Typedefs.h"

#include "TAS_Typedefs.h"
#include "TAS_ChannelWidth.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAS_Config_c
{
public:

   TAS_Config_c( void );
   TAS_Config_c( const TAS_Config_c& config );
   ~TAS_Config_c( void );

   TAS_Config_c& operator=( const TAS_Config_c& config );
   bool operator==( const TAS_Config_c& config ) const;
   bool operator!=( const TAS_Config_c& config ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   class TAS_ConfigLayout_c
   {
   public:

      TAS_ArraySizeMode_t sizeMode;
                                 // Defines array size mode (auto vs. manual)
      class TAS_ConfigLayoutAutoSize_c
      {
      public:

         double aspectRatio;     // Defines auto size mode's aspect ratio 
                                 // (ie. width/height ratio)
      } autoSize;

      class TAS_ConfigLayoutManualSize_c
      {
      public:

         TGS_IntDims_t gridDims; // Defines manual size mode's width & height
 	                         // (based on tile grid units)
      } manualSize;
   } layout;

   class TAS_ConfigDevice_c
   {
   public:

      class TAS_ConfigDeviceSwitchBoxes_c
      {
      public:
         TAS_SwitchBoxModelType_t modelType; 
                                 // Defines switch box model type 
                                 // (eg. "Wilton", "subset", or "universal")
         unsigned int fs;        // Defines 'Fs', the switch box flexibility
                             	 // (ie. #of wires that connect to switch box)
      } switchBoxes;

      class TAS_ConfigDeviceConnectionBoxes_c
      {
      public:

         double capInput;        // Buffer input pin C (farads) 
 	                         // from routing track to a connection box
         double delayInput;      // Buffer delay (seconds)
 	                         // from routing track to a connection box
      } connectionBoxes;

      class TAS_ConfigDeviceSegments_c
      {
      public:
         TAS_SegmentDirType_t dirType; 
                                 // Defines segment dirction type
                                 // (eg. "unidirectional" or "bidirectional"
      } segments;

      class TAS_ConfigDeviceChannelWidth_c
      {
      public:
         TAS_ChannelWidth_c io;  // Defines channel width distributions
         TAS_ChannelWidth_c x;   // "
         TAS_ChannelWidth_c y;   // "

      } channelWidth;

      class TAS_ConfigDeviceAreaModel_c
      {
      public:

         double resMinWidthNMOS; // R (ohms) for min-width NMOS transistor
         double resMinWidthPMOS; // R (ohms) for min-width PMOS transistor
         double sizeInputPinMux; // Size for ipin mux transistors
         double areaGridTile;    // Estimated area needed for *all* blocks

      } areaModel;

   } device;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TAS_Config_c::IsValid( 
      void ) const
{
   return( true );
}

#endif
