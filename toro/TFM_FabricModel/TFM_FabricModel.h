//===========================================================================//
// Purpose : Declaration and inline definitions for a TFM_FabricModel class.
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

#ifndef TFM_FABRIC_MODEL_H
#define TFM_FABRIC_MODEL_H

#include <cstdio>
#include <string>
using namespace std;

#include "TFV_FabricView.h"

#include "TFM_Typedefs.h"
#include "TFM_Config.h"
#include "TFM_Block.h"
#include "TFM_Channel.h"
#include "TFM_Segment.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
class TFM_FabricModel_c
{
public:

   TFM_FabricModel_c( void );
   TFM_FabricModel_c( const TFM_FabricModel_c& fabricModel );
   ~TFM_FabricModel_c( void );

   TFM_FabricModel_c& operator=( const TFM_FabricModel_c& fabricModel );
   bool operator==( const TFM_FabricModel_c& fabricModel ) const;
   bool operator!=( const TFM_FabricModel_c& fabricModel ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void Clear( void );

   TFV_FabricView_c* GetFabricView( void );
   TFV_FabricView_c* GetFabricView( void ) const;

   bool IsValid( void ) const;

public:

   string srName;

   TFM_Config_c            config;
   TFM_PhysicalBlockList_t physicalBlockList;
   TFM_InputOutputList_t   inputOutputList;
   TFM_SwitchBoxList_t     switchBoxList;
   TFM_ChannelList_t       channelList;
   TFM_SegmentList_t       segmentList;

private:

   TFV_FabricView_c fabricView_;

private:

   enum TFM_DefCapacity_e 
   { 
      TFM_PHYSICAL_BLOCK_LIST_DEF_CAPACITY = 256,
      TFM_INPUT_OUTPUT_LIST_DEF_CAPACITY   = 256,
      TFM_SWITCH_BOX_LIST_DEF_CAPACITY     = 256,
      TFM_CHANNEL_LIST_DEF_CAPACITY        = 256,
      TFM_SEGMENT_LIST_DEF_CAPACITY        = 1024
   };
};

#endif
