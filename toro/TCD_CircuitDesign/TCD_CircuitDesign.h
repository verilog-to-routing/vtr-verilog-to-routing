//===========================================================================//
// Purpose : Declaration and inline definitions for a TCD_CircuitDesign 
//           class.
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

#ifndef TCD_CIRCUIT_DESIGN_H
#define TCD_CIRCUIT_DESIGN_H

#include <cstdio>
#include <string>
using namespace std;

#include "TPO_Typedefs.h"
#include "TPO_Inst.h"
#include "TPO_PlaceRegions.h"

#include "TLO_Typedefs.h"
#include "TLO_Cell.h"

#include "TNO_Typedefs.h"
#include "TNO_NetList.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TCD_CircuitDesign_c
{
public:

   TCD_CircuitDesign_c( void );
   TCD_CircuitDesign_c( const TCD_CircuitDesign_c& circuitDesign );
   ~TCD_CircuitDesign_c( void );

   TCD_CircuitDesign_c& operator=( const TCD_CircuitDesign_c& circuitDesign );
   bool operator==( const TCD_CircuitDesign_c& circuitDesign ) const;
   bool operator!=( const TCD_CircuitDesign_c& circuitDesign ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintBLIF( void ) const;
   void PrintBLIF( FILE* pfile, size_t spaceLen = 0 ) const;

   bool InitDefaults( const string& srDefaultBaseName );
   bool InitValidate( void );

   bool IsLegal( void ) const;
   bool IsValid( void ) const;

private:

   void InitDefaultsNetList_( const TPO_InstList_t& instList,
                              const TPO_PortList_t& portList,
                              const TLO_CellList_t& cellList,
                              TNO_NetList_c* pnetList ) const;
   void InitDefaultsNetNameList_( const TPO_InstList_t& instList,
                                  const TPO_PortList_t& portList,
                                  TNO_NetList_c* pnetList,
                                  TNO_NameList_t* pnetNameList ) const;

   bool InitValidateNetList_( TNO_NetList_c* pnetList ) const;
   bool InitValidateInstList_( const TPO_InstList_t& instList,
                               const TLO_CellList_t& cellList ) const;

public:

   string srName;   // Define design's top-level name

   TPO_InstList_t blockList; // Define design's physical block list

   TPO_PortList_t portList;  // Define design's input/output port list
   TPO_NameList_t portNameList;

   TPO_InstList_t instList;  // Define design's primatives (based on BLIF)
   TPO_NameList_t instNameList;

   TLO_CellList_t cellList;  // Define design's non-standard cell list
                             // (see BLIF ".subckt")
   TNO_NetList_c  netList;   // Define design's net list
   TNO_NameList_t netNameList;
   TNO_NameList_t netOrderList;

   TPO_PlaceRegionsList_t placeRegionsList;
                             // Optional design's region placement constrants

private:

   enum TCD_DefCapacity_e 
   { 
      TCD_BLOCK_LIST_DEF_CAPACITY = 64,
      TCD_PORT_LIST_DEF_CAPACITY = 64,
      TCD_PORT_NAME_LIST_DEF_CAPACITY = 64,
      TCD_INST_LIST_DEF_CAPACITY = 64,
      TCD_INST_NAME_LIST_DEF_CAPACITY = 64,
      TCD_CELL_LIST_DEF_CAPACITY = 64,
      TCD_NET_LIST_DEF_CAPACITY = 1024,
      TCD_NET_NAME_LIST_DEF_CAPACITY = 1024,
      TCD_NET_ORDER_LIST_DEF_CAPACITY = 1024,
      TCD_PLACE_REGIONS_LIST_DEF_CAPACITY = 1
   };
};

#endif
