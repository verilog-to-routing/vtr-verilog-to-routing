//===========================================================================//
// Purpose : Declaration and inline definitions for the TFH_ConnectionBlock 
//           class.
//
//           Inline methods include:
//           - GetBlockOrigin
//           - GetPinIndex, GetPinName, GetPinSide
//           - GetConnectionPattern
//           - IsValid
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

#ifndef TFH_CONNECTION_BLOCK_H
#define TFH_CONNECTION_BLOCK_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Bit.h"

#include "TGO_Point.h"

#include "TFH_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
class TFH_ConnectionBlock_c
{
public:

   TFH_ConnectionBlock_c( void );
   TFH_ConnectionBlock_c( const TGO_Point_c& vpr_blockOrigin,
                          int vpr_pinIndex );
   TFH_ConnectionBlock_c( const TGO_Point_c& vpr_blockOrigin,
                          int vpr_pinIndex,
                          const string& srPinName,
                          TC_SideMode_t pinSide,
                          const TFH_BitPattern_t& connectionPattern );
   TFH_ConnectionBlock_c( const TGO_Point_c& vpr_blockOrigin,
                          int vpr_pinIndex,
                          const char* pszPinName,
                          TC_SideMode_t pinSide,
                          const TFH_BitPattern_t& connectionPattern );
   TFH_ConnectionBlock_c( const TFH_ConnectionBlock_c& connectionBlock );
   ~TFH_ConnectionBlock_c( void );

   TFH_ConnectionBlock_c& operator=( const TFH_ConnectionBlock_c& connectionBlock );
   bool operator<( const TFH_ConnectionBlock_c& connectionBlock ) const;
   bool operator==( const TFH_ConnectionBlock_c& connectionBlock ) const;
   bool operator!=( const TFH_ConnectionBlock_c& connectionBlock ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   const TGO_Point_c& GetBlockOrigin( void ) const;
   int GetPinIndex( void ) const;
   const char* GetPinName( void ) const;
   TC_SideMode_t GetPinSide( void ) const;
   const TFH_BitPattern_t& GetConnectionPattern( void ) const;

   bool IsValid( void ) const;

private:

   TGO_Point_c vpr_blockOrigin_;// VPR's grid array block origin coordinate

   int vpr_pinIndex_;           // Specifies pin index, as defined by VPR
   string srPinName_;           // Specifies connection box's pin name
   TC_SideMode_t pinSide_;      // Specifies pin side (LEFT|RIGHT|LOWER|UPPER)

   TFH_BitPattern_t connectionPattern_; 
                                // Describes connection box pattern
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
inline const TGO_Point_c& TFH_ConnectionBlock_c::GetBlockOrigin( 
      void ) const
{
   return( this->vpr_blockOrigin_ );
}

//===========================================================================//
inline int TFH_ConnectionBlock_c::GetPinIndex( 
      void ) const
{
   return( this->vpr_pinIndex_ );
}

//===========================================================================//
inline const char* TFH_ConnectionBlock_c::GetPinName( 
      void ) const
{
   return( TIO_SR_STR( this->srPinName_ ));
}

//===========================================================================//
inline TC_SideMode_t TFH_ConnectionBlock_c::GetPinSide( 
      void ) const
{
   return( this->pinSide_ );
}

//===========================================================================//
inline const TFH_BitPattern_t& TFH_ConnectionBlock_c::GetConnectionPattern( 
      void ) const
{
   return( this->connectionPattern_ );
}

//===========================================================================//
inline bool TFH_ConnectionBlock_c::IsValid( 
      void ) const
{
   return( this->vpr_blockOrigin_.IsValid( ) ? true : false );
}

#endif
