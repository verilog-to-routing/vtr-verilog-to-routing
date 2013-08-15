//===========================================================================//
// Purpose : Declaration and inline definitions for a TGS_ArrayGridIter 
//           iterator class.
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

#ifndef TGS_ARRAY_GRID_ITER_H
#define TGS_ARRAY_GRID_ITER_H

#include "TGS_Typedefs.h"
#include "TGS_ArrayGrid.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
class TGS_ArrayGridIter_c 
{
public:

   TGS_ArrayGridIter_c( const TGS_ArrayGrid_c* parrayGrid = 0,
                        const TGS_Region_c* parrayRegion = 0 );
   TGS_ArrayGridIter_c( const TGS_ArrayGrid_c& arrayGrid,
                        const TGS_Region_c& arrayRegion );
   ~TGS_ArrayGridIter_c( void );

   void Init( const TGS_ArrayGrid_c* parrayGrid,
              const TGS_Region_c* parrayRegion );
   void Init( const TGS_ArrayGrid_c& arrayGrid,
              const TGS_Region_c& arrayRegion );

   bool Next( double* px = 0,
              double* py = 0 );

   void Reset( void );

   bool IsValid( void ) const;

private:

   TGS_ArrayGrid_c* parrayGrid_;  // Ptr to array grid being iterated
   TGS_Region_c     arrayRegion_; // Initial iterate array grid region

   TGS_FloatDims_t  pitch_;       // Cached iterate array grid pitch dims

   double x_;                     // Current iterated array grid indices
   double y_;                     // "
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/25/12 jeffr : Original
//===========================================================================//
inline bool TGS_ArrayGridIter_c::IsValid( 
      void ) const
{
   return( this->parrayGrid_ ? true : false );
}

#endif
