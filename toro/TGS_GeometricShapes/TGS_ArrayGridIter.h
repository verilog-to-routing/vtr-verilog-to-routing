//===========================================================================//
// Purpose : Declaration and inline definitions for a TGS_ArrayGridIter 
//           iterator class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

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
