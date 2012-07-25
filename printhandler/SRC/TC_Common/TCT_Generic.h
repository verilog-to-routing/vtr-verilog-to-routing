//===========================================================================//
// Purpose : Template versions of generic 'min/max/compare' functions.
//
//           Inline functions include:
//           - TCT_Min, TCT_Max
//           - TCTF_IsEQ, TCTF_IsNEQ
//           - TCTF_IsLE, TCTF_IsLT
//           - TCTF_IsGE, TCTF_IsGT
//           - TCTF_IsZE, TCTF_IsNZE
//
//           Functions include:
//           - TCT_Rand
//           - TCT_FloatToUnit
//           - TCT_UnitToFloat
//
//===========================================================================//

#ifndef TCT_GENERIC_H
#define TCT_GENERIC_H

#include <stdio.h>
#include <math.h>

#if defined( SUN8 ) || defined( SUN10 )
   #include <time.h>
#endif

#include "TC_Typedefs.h"

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//

template< class T > T TCT_Min( T i, T j, T k );
template< class T > T TCT_Min( T i, T j );

template< class T > T TCT_Max( T i, T j, T k );
template< class T > T TCT_Max( T i, T j );

template< class T > bool TCTF_IsEQ( T n, T o );
template< class T > bool TCTF_IsNEQ( T n, T o );

template< class T > bool TCTF_IsLE( T n, T o );
template< class T > bool TCTF_IsLT( T n, T o );

template< class T > bool TCTF_IsGE( T n, T o );
template< class T > bool TCTF_IsGT( T n, T o );

template< class T > bool TCTF_IsZE( T n );
template< class T > bool TCTF_IsNZE( T n );

template< class T > T TCT_Rand( T i, T j, T units );
template< class T > T TCT_Rand( T i, T j );
template< class T > T TCT_Rand( T len );

template< class T > T TCT_FloatToUnit( double val, T* punit, 
                                       int dbUnits = 0 );
template< class T > double TCT_UnitToFloat( T val, double* pval, 
                                            int dbUnits = 0 );

//===========================================================================//
// Purpose        : Function inline defintions
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > inline T TCT_Min( T i, T j, T k )
{
   return( TCT_Min( TCT_Min( i, j ), k ));
}

//===========================================================================//
template< class T > inline T TCT_Min( T i, T j )
{
   return( i < j ? i : j );
}

//===========================================================================//
template< class T > inline T TCT_Max( T i, T j, T k )
{
   return( TCT_Max( TCT_Max( i, j ), k ));
}

//===========================================================================//
template< class T > inline T TCT_Max( T i, T j )
{
   return( i > j ? i : j );
}

//===========================================================================//
template< class T > inline bool TCTF_IsEQ( T n, T o )
{
   double f = static_cast< double >( n );
   double g = static_cast< double >( o );
   return( fabs( f - g ) <= TC_FLT_EPSILON ? true : false );
} 

//===========================================================================//
template< class T > inline bool TCTF_IsNEQ( T n, T o )
{
   double f = static_cast< double >( n );
   double g = static_cast< double >( o );
   return( fabs( f - g ) > TC_FLT_EPSILON ? true : false );
} 

//===========================================================================//
template< class T > inline bool TCTF_IsLE( T n, T o )
{
   double f = static_cast< double >( n );
   double g = static_cast< double >( o );
   return( f <= ( g + TC_FLT_EPSILON ) ? true : false );
} 

//===========================================================================//
template< class T > inline bool TCTF_IsLT( T n, T o )
{
   double f = static_cast< double >( n );
   double g = static_cast< double >( o );
   return( f < ( g - TC_FLT_EPSILON ) ? true : false );
} 

//===========================================================================//
template< class T > inline bool TCTF_IsGE( T n, T o )
{
   double f = static_cast< double >( n );
   double g = static_cast< double >( o );
   return( f >= ( g - TC_FLT_EPSILON ) ? true : false );
} 

//===========================================================================//
template< class T > inline bool TCTF_IsGT( T n, T o )
{
   double f = static_cast< double >( n );
   double g = static_cast< double >( o );
   return( f > ( g + TC_FLT_EPSILON ) ? true : false );
} 

//===========================================================================//
template< class T > inline bool TCTF_IsZE( T n )
{
   double f = static_cast< double >( n );
   return( fabs( f ) <= TC_FLT_EPSILON ? true : false );
} 

//===========================================================================//
template< class T > inline bool TCTF_IsNZE( T n )
{
   double f = static_cast< double >( n );
   return( fabs( f ) > TC_FLT_EPSILON ? true : false );
}

//===========================================================================//
// Function       : TCT_Rand
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > T TCT_Rand( T i, T j, T units )
{
   // Check units to prevent divide by zero error
   if ( TCTF_IsZE( static_cast< double >( units )))
   {
      units = static_cast< T >( 1 );
   }

   // Get next random number and max number 
   int r = rand( );
   int max = TC_RAND_MAX;

   // Normalize random number into double range [0.0-1.0]
   double r_normal = static_cast< double >( r ) / static_cast< double >( max );

   // Apply given unit to transform (i,j) into long values
   double i_double = static_cast< double >( i ) / static_cast< double >( units );
   double j_double = static_cast< double >( j ) / static_cast< double >( units );

   long i_long = static_cast< long >( i_double + TC_FLT_EPSILON );
   long j_long = static_cast< long >( j_double + TC_FLT_EPSILON );
   long o_long = j_long - i_long + 1;

   // Apply normalized random number to long value range for random value
   double r_double = static_cast< double >( o_long ) * r_normal;

   // Check and adjust random value for special case (ie. max random number)
   long r_long = static_cast< long >( r_double );
   r_long = ( r_long <= o_long - 1 ? r_long : o_long - 1 );

   // Apply given unit again to transform random value into (i,j) offset
   double o_double = static_cast< double >( r_long ) * 
                     static_cast< double >( units );
   o_double += ( units < static_cast< double >( 1.0 ) ? 0.0 : TC_FLT_EPSILON );

   // And return random value based on (i,j) interval and random offset
   return( i + static_cast< T >( o_double ));
}


//===========================================================================//
template< class T > T TCT_Rand( T i, T j )
{
   return( TCT_Rand( i, j, static_cast< T >( 1 )));
}


//===========================================================================//
template< class T > T TCT_Rand( T len )
{
   T i = static_cast< T >( 0 );
   T j = len - static_cast< T >( 1 );
   return( TCT_Rand( i, j, static_cast< T >( 1 )));
} 

//===========================================================================//
// Function       : TCT_FloatToUnit
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > T TCT_FloatToUnit( 
      double val, 
      T*     punit, 
      int    dbUnits )
{
   double units = static_cast< double >( dbUnits > 1 ? dbUnits : 1 );
   double epsilon = ( val >= -1.0E-4 ? 1.0E-4 : -1.0E-4 );
   T unit = static_cast< T >(( val + epsilon ) * units );
   if ( punit )
   {
      *punit = unit;
   }
   return( unit );
}

//===========================================================================//
// Function       : TCT_UnitToFloat
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > double TCT_UnitToFloat( 
      T       unit, 
      double* pval, 
      int     dbUnits )
{
   double units = static_cast< double >( dbUnits > 1 ? dbUnits : 1 );
   double val = static_cast< double >( unit ) / units;
   if ( pval )
   {
      *pval = val;
   }
   return( val );
}

#endif 
