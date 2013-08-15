//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_FabricOptions class.
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TOS_FABRIC_OPTIONS_H
#define TOS_FABRIC_OPTIONS_H

#include <cstdio>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/12/13 jeffr : Original
//===========================================================================//
class TOS_FabricOptions_c
{
public:

   TOS_FabricOptions_c( void );
   TOS_FabricOptions_c( bool blocks_override_,
                        bool switchBoxes_override_,
                        bool connectionBlocks_override_,
                        bool channels_override_ );
   ~TOS_FabricOptions_c( void );

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

public:

   class TOS_Blocks_c
   {
   public:

      bool override;       // Enables applying override block constraints, if any

   } blocks;

   class TOS_SwitchBoxes_c
   {
   public:

      bool override;       // Enables applying override switch box constraints, if any

   } switchBoxes;

   class TOS_ConnectionBlocks_c
   {
   public:

      bool override;       // Enables applying override connection blocks constraints, if any

   } connectionBlocks;

   class TOS_Channels_c
   {
   public:

      bool override;       // Enables applying override channel constraints, if any

   } channels;
};

#endif 
