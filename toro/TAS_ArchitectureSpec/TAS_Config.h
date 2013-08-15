//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_Config class.
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

#ifndef TAS_CONFIG_H
#define TAS_CONFIG_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Typedefs.h"

#include "TGS_Typedefs.h"

#include "TAS_Typedefs.h"
#include "TAS_ChannelWidth.h"
#include "TAS_Clock.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
// 07/17/13 jeffr : Added TAS_ConfigPower_c and TAS_ConfigClock_c support
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

   class TAS_ConfigPower_c
   {
   public:

      class TAS_ConfigPowerInterconnect_c
      {
      public:	
         double capWire;        // Local interconnect wire C (farads) 
         double factor;         // Local interconnect factor
      } interconnect;

      class TAS_ConfigPowerBuffers
      {
      public:
         double logicalEffortFactor; 

      } buffers;

      class TAS_ConfigPowerSRAM
      {
      public:
         double transistorsPerBit;

      } sram;

   } power;

   TAS_ClockList_t clockList;   // Optional block clock list constants

private:

   enum TAS_DefCapacity_e 
   { 
      TAS_CLOCK_LIST_DEF_CAPACITY = 1
   };
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
