//===========================================================================//
// Purpose : Enums, typedefs, and defines for ANSI C++ compatibility.
//
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//

#ifndef TC_TYPEDEFS_H
#define TC_TYPEDEFS_H

#if defined( SUN8 )
   #define _XOPEN_SOURCE
#endif

//---------------------------------------------------------------------------//
// Define float constants as needed
//---------------------------------------------------------------------------//

#include <float.h>
#define TC_FLT_EPSILON ( static_cast< double >( FLT_RADIX * FLT_EPSILON ))
#define TC_FLT_MIN ( static_cast< double >( -1.0 * FLT_MAX + ( FLT_RADIX * FLT_EPSILON )))
#define TC_FLT_MAX ( static_cast< double >( 1.0 * FLT_MAX - ( FLT_RADIX * FLT_EPSILON )))

//---------------------------------------------------------------------------//
// Define limit constants as needed
//---------------------------------------------------------------------------//

#if defined( LINUX24 )
   #include <stdint.h>
   #ifndef SIZE_MAX
      #define SIZE_MAX UINT_MAX
   #endif
#endif

#if defined( LINUX24_64 )
   #include <stdint.h>
   #ifndef SIZE_MAX
      #define SIZE_MAX ULONG_MAX
   #endif
#endif

#include <stdlib.h>
#ifdef RAND_MAX
   #define TC_RAND_MAX RAND_MAX
#else
   #define TC_RAND_MAX INT_MAX
#endif

//---------------------------------------------------------------------------//
// Define math constants as needed
//---------------------------------------------------------------------------//

#include <math.h>
#ifndef M_SQRT2
   #define M_SQRT2 1.41421356237309504880
#endif
#define TC_SQRT2 ( M_SQRT2 )

//---------------------------------------------------------------------------//
// Define 'nothrow' keyword if not currently available (ie, make NOP for new) 
//---------------------------------------------------------------------------//

#define TC_NOTHROW (std::nothrow) // Set 'nothrow' behavior for default

//---------------------------------------------------------------------------//
// Define common mode constants and typedefs
//---------------------------------------------------------------------------//

enum TC_BitMode_e
{
   TC_BIT_UNDEFINED = 0,
   TC_BIT_TRUE,
   TC_BIT_FALSE,
   TC_BIT_DONT_CARE
};
typedef enum TC_BitMode_e TC_BitMode_t;

enum TC_DataMode_e
{
   TC_DATA_UNDEFINED = 0,
   TC_DATA_INT,
   TC_DATA_UINT,
   TC_DATA_LONG,
   TC_DATA_ULONG,
   TC_DATA_SIZE,
   TC_DATA_FLOAT,
   TC_DATA_EXP,
   TC_DATA_STRING
};
typedef enum TC_DataMode_e TC_DataMode_t;

enum TC_SideMode_e
{
   TC_SIDE_UNDEFINED = 0,
   TC_SIDE_LEFT,
   TC_SIDE_RIGHT,
   TC_SIDE_LOWER,
   TC_SIDE_UPPER,
   TC_SIDE_BOTTOM,
   TC_SIDE_TOP,
   TC_SIDE_PREV,
   TC_SIDE_NEXT
};
typedef TC_SideMode_e TC_SideMode_t;

enum TC_TypeMode_e
{
   TC_TYPE_UNDEFINED = 0,
   TC_TYPE_INPUT,
   TC_TYPE_OUTPUT,
   TC_TYPE_SIGNAL,
   TC_TYPE_CLOCK,
   TC_TYPE_RESET,
   TC_TYPE_POWER
};
typedef enum TC_TypeMode_e TC_TypeMode_t;

//---------------------------------------------------------------------------//
// Define common dims constants as needed
//---------------------------------------------------------------------------//

#include "TCT_Dims.h"

typedef TCT_Dims_c< unsigned int > TC_UIntDims_t;
typedef TCT_Dims_c< double > TC_FloatDims_t;

//---------------------------------------------------------------------------//
// Define common list constants as needed
//---------------------------------------------------------------------------//

#define TC_DEFAULT_CAPACITY ( static_cast< size_t >( 64 ))

#endif 
