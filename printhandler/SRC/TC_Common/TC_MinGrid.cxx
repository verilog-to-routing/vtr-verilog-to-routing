//===========================================================================//
// Purpose : Method definitions for a TC_MinGrid minimum grid class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance
//           - SetGrid
//           - IntToFloat, UnsignedIntToFloat, LongIntToFloat
//           - FloatToInt, FloatToUnsignedInt, FloatToLongInt
//           - SnapToGrid
//           - SnapToGridCeil
//           - IsOnGrid
//
//           Protected methods include:
//           - TC_MinGrid_c, ~TC_MinGrid_c
//
//===========================================================================//

#include "limits.h"
#include "math.h"

#include "TCT_Generic.h"

#include "TC_Typedefs.h"
#include "TC_MinGrid.h"

// Initialize the min grid "singleton" class, as needed
TC_MinGrid_c* TC_MinGrid_c::pinstance_ = 0;

//===========================================================================//
// Method         : TC_MinGrid_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TC_MinGrid_c::TC_MinGrid_c( 
      void )
      :   
      grid_( 0.1 ),
      units_( 10 ),
      magnitude_( 10 ),
      precision_( 1 )
{
   this->SetGrid( TC_MIN_GRID_DEF );
}

//===========================================================================//
// Method         : ~TC_MinGrid_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TC_MinGrid_c::~TC_MinGrid_c( 
      void )
{
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TC_MinGrid_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TC_MinGrid_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TC_MinGrid_c::DeleteInstance( 
      void )
{
   if ( pinstance_ != NULL )
   {
      delete pinstance_;
      pinstance_ = NULL;
   }
}

//===========================================================================//
// Method         : GetInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TC_MinGrid_c& TC_MinGrid_c::GetInstance( 
      void )
{
   if ( !pinstance_ )
   {
      NewInstance( );
   }
   return( *pinstance_ );
}

//===========================================================================//
// Method         : SetGrid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TC_MinGrid_c::SetGrid( 
      double grid )
{
   this->grid_ = grid;
   if ( this->grid_ > TC_FLT_EPSILON )
   {
      // Compute database units (integer) based on min grid value
      double units = ( 1.0 / this->grid_ ) + TC_FLT_EPSILON;
      this->units_ = static_cast< unsigned int >( units );

      // Compute format precision (integer) based on min grid value
      double precision = 10.0;
      while ( precision <= 1000000.0 + TC_FLT_EPSILON )
      {
         double f = this->grid_ * precision;
         double c = ceil( f - TC_FLT_EPSILON );
         if ( fabs( c - f ) < TC_FLT_EPSILON )
            break;

         precision *= 10.0;
      }
      precision = ceil( log10( precision ));
      this->precision_ = static_cast< unsigned int >( precision );

      // Compute database magnitude (integer) based on precision value
      this->magnitude_ = 1;
      for ( unsigned int n = 1; n <= this->precision_; ++n )  
      {
         this->magnitude_ *= 10;
      }
   }
}

//===========================================================================//
// Method         : IntToFloat
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
double TC_MinGrid_c::IntToFloat( 
      int i ) const
{
   double f;
   TCT_UnitToFloat( i, &f, this->units_ );

   return( f );
}

//===========================================================================//
// Method         : UnsignedIntToFloat
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
double TC_MinGrid_c::UnsignedIntToFloat( 
      unsigned int ui ) const
{
   double f;
   TCT_UnitToFloat( ui, &f, this->units_ );

   return( f );
}

//===========================================================================//
// Method         : LongIntToFloat
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
double TC_MinGrid_c::LongIntToFloat( 
      long int li ) const
{
   double f;
   TCT_UnitToFloat( li, &f, this->units_ );

   return( f );
}

//===========================================================================//
// Method         : FloatToInt
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
int TC_MinGrid_c::FloatToInt( 
      double f ) const
{
   int i;
   if ( f < static_cast< double >( INT_MIN ) - TC_FLT_EPSILON )
   {
      i = INT_MIN;
   } 
   else if ( f > static_cast< double >( INT_MAX ) - TC_FLT_EPSILON )
   {
      i = INT_MAX;
   } 
   else
   {
      TCT_FloatToUnit( f, &i, this->units_ );
   } 
   return( i );
}

//===========================================================================//
// Method         : FloatToUnsignedInt
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
unsigned int TC_MinGrid_c::FloatToUnsignedInt( 
      double f ) const
{
   unsigned int ui;
   if ( f < static_cast< double >( 0 ) - TC_FLT_EPSILON )
   {
      ui = UINT_MAX;
   } 
   else if ( f > static_cast< double >( UINT_MAX ) - TC_FLT_EPSILON )
   {
      ui = UINT_MAX;
   } 
   else
   {
      TCT_FloatToUnit( f, &ui, this->units_ );
   } 
   return( ui );
}

//===========================================================================//
// Method         : FloatToLongInt
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
long int TC_MinGrid_c::FloatToLongInt( 
      double f ) const
{
   long int li;
   if ( f < static_cast< double >( LONG_MIN ) - TC_FLT_EPSILON )
   {
      li = LONG_MIN;
   } 
   else if ( f > static_cast< double >( LONG_MAX ) - TC_FLT_EPSILON )
   {
      li = LONG_MAX;
   } 
   else
   {
      TCT_FloatToUnit( f, &li, this->units_ );
   } 
   return( li );
}

//===========================================================================//
// Method         : SnapToGrid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
double TC_MinGrid_c::SnapToGrid(
      double f ) const
{
   int i = this->FloatToInt( f );

   double fdelta = 0.0;
   if (( i == INT_MIN ) || ( i == INT_MAX ))
   {
      double fmin = static_cast< double >( INT_MIN );
      double fmax = static_cast< double >( INT_MAX );
      if ( TCTF_IsGT( f, fmin ) && TCTF_IsLT( f, fmax ))
      {
	 fdelta = floor( f );
         i = this->FloatToInt( f - fdelta );
      }
   }

   double fprime = 0.0;
   if ( i == INT_MIN )
   {
      fprime = static_cast< double >( INT_MIN );
   }
   else if ( i == INT_MAX )
   {
      fprime = static_cast< double >( INT_MAX );
   }
   else
   {
      fprime = this->IntToFloat( i ) + fdelta;
   }
   return( fprime );
}

//===========================================================================//
// Method         : SnapToGridCeil
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
double TC_MinGrid_c::SnapToGridCeil(
      double f ) const
{
   int i = static_cast< int >( ceil( f / this->grid_ - TC_FLT_EPSILON ));

   return( static_cast< double >( i ) * this->grid_ );
}

//===========================================================================//
// Method         : IsOnGrid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TC_MinGrid_c::IsOnGrid( 
      double  f, 
      double* pf ) const
{
   double fprime = this->SnapToGrid( f );

   if ( pf )
   {
      *pf = fprime;
   }
   return( fabs( f - fprime ) < TC_FLT_EPSILON ? true : false );
}
