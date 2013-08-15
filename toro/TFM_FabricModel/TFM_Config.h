//===========================================================================//
// Purpose : Declaration and inline definitions for a TFM_Config class.
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

#ifndef TFM_CONFIG_H
#define TFM_CONFIG_H

#include <cstdio>
using namespace std;

#include "TGS_Region.h"
#include "TGO_Polygon.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
class TFM_Config_c
{
public:

   TFM_Config_c( void );
   TFM_Config_c( const TFM_Config_c& config );
   ~TFM_Config_c( void );

   TFM_Config_c& operator=( const TFM_Config_c& config );
   bool operator==( const TFM_Config_c& config ) const;
   bool operator!=( const TFM_Config_c& config ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   bool IsValid( void ) const;

public:

   TGS_Region_c  fabricRegion;  // Fabric floorplan region boundary
   TGO_Polygon_c ioPolygon;     // Fabric floorplan polygon boundary (for IOs)
};

#endif
