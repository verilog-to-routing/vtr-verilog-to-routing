//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_ConnectionFc class.
//
//           Inline methods include:
//           - SetDir
//           - GetDir
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

#ifndef TAS_CONNECTION_FC_H
#define TAS_CONNECTION_FC_H

#include <cstdio>
using namespace std;

#include "TAS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
class TAS_ConnectionFc_c
{
public:

   TAS_ConnectionFc_c( void );
   TAS_ConnectionFc_c( const TAS_ConnectionFc_c& connectionFc );
   ~TAS_ConnectionFc_c( void );

   TAS_ConnectionFc_c& operator=( const TAS_ConnectionFc_c& connectionFc );
   bool operator==( const TAS_ConnectionFc_c& connectionFc ) const;
   bool operator!=( const TAS_ConnectionFc_c& connectionFc ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   TC_TypeMode_t GetDir( void ) const;
   void SetDir( TC_TypeMode_t dir );

   bool IsValid( void ) const;

public:

   TAS_ConnectionBoxType_t type; 
                          // Selects type: FRACTION|ABSOLUTE|FULL
                          // Defines number of routing tracks connected 
                          // from input pins via a connection box
   double percent;        // Defines percent of tracks each pin connects to
                          // Applies when connection box type = FRACTION
   unsigned int absolute; // Defines number of tracks that each pin connects
                          // Applies when connection box type = ABSOLUTE

   string srName;         // Optional pin name (to override specific pins)

private:

   TC_TypeMode_t dir_;    // Selects Fc_in vs. Fc_out mode
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
inline void TAS_ConnectionFc_c::SetDir( 
      TC_TypeMode_t dir )
{
   this->dir_ = dir;
}

//===========================================================================//
inline TC_TypeMode_t TAS_ConnectionFc_c::GetDir( 
      void ) const
{
   return( this->dir_ );
}

//===========================================================================//
inline bool TAS_ConnectionFc_c::IsValid( 
      void ) const
{
   return( this->type != TAS_CONNECTION_BOX_UNDEFINED ? true : false );
}

#endif
