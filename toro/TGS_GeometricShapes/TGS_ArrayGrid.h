//===========================================================================//
// Purpose : Declaration and inline definitions for TGS_ArrayGrid class.
//
//           Public methods include:
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

#ifndef TGS_ARRAY_GRID_H
#define TGS_ARRAY_GRID_H

#include <cstdio>
using namespace std;

#if defined( SUN8 ) || defined( SUN10 )
   #include <time.h>
#endif

#include "TC_Typedefs.h"

#include "TCT_Generic.h"
#include "TCT_Dims.h"

#include "TGS_Region.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
class TGS_ArrayGrid_c
{
public:

   TGS_ArrayGrid_c( void );
   TGS_ArrayGrid_c( const TGS_FloatDims_t& pitchDims );
   TGS_ArrayGrid_c( const TGS_FloatDims_t& pitchDims,
                    const TGS_FloatDims_t& offsetDims );
   TGS_ArrayGrid_c( double xPitch, double yPitch );
   TGS_ArrayGrid_c( double xPitch, double yPitch,
                    double xOffset, double yOffset );
   ~TGS_ArrayGrid_c( void );

   TGS_ArrayGrid_c& operator=( const TGS_ArrayGrid_c& arrayGrid );
   bool operator==( const TGS_ArrayGrid_c& arrayGrid ) const;
   bool operator!=( const TGS_ArrayGrid_c& arrayGrid ) const;

   void Init( const TGS_FloatDims_t& pitchDims );
   void Init( const TGS_FloatDims_t& pitchDims,
              const TGS_FloatDims_t& offsetDims );
   void Init( double xPitch, double yPitch );
   void Init( double xPitch, double yPitch,
              double xOffset, double yOffset );

   void GetPitch( TGS_FloatDims_t* ppitchDims ) const;
   void GetPitch( double* pxPitch, double* pyPitch ) const;
   void GetOffset( TGS_FloatDims_t* poffsetDims ) const;
   void GetOffset( double* pxOffset, double* pyOffset ) const;

   void FindCount( const TGS_Region_c& region,
                   TGS_IntDims_t* pcount,
                   TGS_ArraySnapMode_t snapMode = TGS_ARRAY_SNAP_WITHIN ) const;
   void FindCount( const TGS_Region_c& region,
                   int* pxCount, int* pyCount,
                   TGS_ArraySnapMode_t snapMode = TGS_ARRAY_SNAP_WITHIN ) const;

   void SnapToGrid( const TGS_Point_c& point,
                    TGS_Point_c* ppoint,
                    TGS_CornerMode_t cornerMode = TGS_CORNER_UNDEFINED ) const;
   void SnapToGrid( const TGS_Region_c& region,
                    TGS_Region_c* pregion,
                    TGS_ArraySnapMode_t snapMode = TGS_ARRAY_SNAP_WITHIN ) const;
   void SnapToGrid( const TGS_Region_c& region,
                    const TGS_Point_c& point,
                    TGS_Point_c* ppoint,
                    TGS_ArraySnapMode_t snapMode = TGS_ARRAY_SNAP_WITHIN ) const;

   void SnapToPitch( const TGS_Point_c& point,
                     TGS_Point_c* ppoint ) const;
   void SnapToPitch( const TGS_Region_c& region,
                     TGS_Region_c* pregion,
                     TGS_ArraySnapMode_t snapMode = TGS_ARRAY_SNAP_WITHIN ) const;

   bool IsValid( void ) const;

private:

   double SnapDims_( double f, 
                     double pitch = 0.0,
                     double offset = 0.0 ) const;

private:

   TGS_FloatDims_t pitchDims_;
   TGS_FloatDims_t offsetDims_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
inline bool TGS_ArrayGrid_c::IsValid(
      void ) const
{
   return( TCTF_IsGT( this->pitchDims_.dx, 0.0 ) &&
           TCTF_IsGT( this->pitchDims_.dy, 0.0 ) ?
           true : false );
}

#endif
