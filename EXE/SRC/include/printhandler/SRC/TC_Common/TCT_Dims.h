//===========================================================================//
// Purpose : Template version for a TCT_Dims_c dimensions (width, height) 
//           class.
//
//           Inline methods include:
//           - TCT_Dims_c, ~TCT_Dims_c
//           - Set, Reset
//           - FindMin, FindMax
//           - FindArea
//           - HasArea
//           - IsValid
//
//           Public methods include:
//           - operator=
//           - operator==, operator!=
//           - ExtractString
//
//===========================================================================//

#ifndef TCT_DIMS_H
#define TCT_DIMS_H

#include <stdio.h>
#include <limits.h>

#include <string>
using namespace std;

#include "TIO_Typedefs.h"

#include "TCT_Generic.h"

#include "TC_Typedefs.h"
#include "TC_MinGrid.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > class TCT_Dims_c
{
public:

   TCT_Dims_c( T width = static_cast< T >( INT_MAX ), 
               T height = static_cast< T >( INT_MAX ));
   TCT_Dims_c( const TCT_Dims_c< T >& dims );
   ~TCT_Dims_c( void );

   TCT_Dims_c< T >& operator=( const TCT_Dims_c< T >& dims );
   bool operator==( const TCT_Dims_c< T >& dims ) const;
   bool operator!=( const TCT_Dims_c< T >& dims ) const;

   void ExtractString( TC_DataMode_t mode,
                       string* psrData,
                       size_t precision = SIZE_MAX ) const;

   void Set( const TCT_Dims_c< T >& dims );
   void Set( T i, T j );
   void Reset( void );

   T FindMin( void ) const;
   T FindMax( void ) const;
   T FindArea( void ) const;

   bool HasArea( void ) const;

   bool IsValid( void ) const;

public:

   T width;
   T height;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > inline TCT_Dims_c< T >::TCT_Dims_c( 
      T width_,
      T height_ )
      :
      width( width_ ),
      height( height_ )
{
}

//===========================================================================//
template< class T > inline TCT_Dims_c< T >::TCT_Dims_c( 
      const TCT_Dims_c< T >& dims )
      :
      width( dims.width ),
      height( dims.height )
{
}

//===========================================================================//
template< class T > inline TCT_Dims_c< T >::~TCT_Dims_c( 
      void )
{
}

//===========================================================================//
template< class T > inline void TCT_Dims_c< T >::Set(
      const TCT_Dims_c< T >& dims )
{
   this->width = dims.width;
   this->height = dims.height;
}

//===========================================================================//
template< class T > inline void TCT_Dims_c< T >::Set(
      T width_,
      T height_ )
{
   this->width = width_;
   this->height = height_;
}

//===========================================================================//
template< class T > inline void TCT_Dims_c< T >::Reset(
      void )
{
   this->width = static_cast< T >( INT_MAX );
   this->height = static_cast< T >( INT_MAX );
}

//===========================================================================//
template< class T > inline T TCT_Dims_c< T >::FindMin( 
      void ) const
{
   return( TCT_Min( this->width, this->height ));
}

//===========================================================================//
template< class T > inline T TCT_Dims_c< T >::FindMax( 
      void ) const
{
   return( TCT_Max( this->width, this->height ));
}

//===========================================================================//
template< class T > inline T TCT_Dims_c< T >::FindArea( 
      void ) const
{
   return( this->width * this->height );
}

//===========================================================================//
template< class T > inline bool TCT_Dims_c< T >::HasArea( 
      void ) const
{
   return(( TCTF_IsGT( this->width, static_cast< T >( 0 ))) &&
          ( TCTF_IsGT( this->height, static_cast< T >( 0 ))) ?
          true : false );
}

//===========================================================================//
template< class T > inline bool TCT_Dims_c< T >::IsValid( 
      void ) const
{
   return(( TCTF_IsLT( this->width, static_cast< T >( INT_MAX ))) &&
          ( TCTF_IsLT( this->height, static_cast< T >( INT_MAX ))) ?
          true : false );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_Dims_c< T >& TCT_Dims_c< T >::operator=( 
      const TCT_Dims_c< T >& dims )
{
   if ( &dims != this )
   {
      this->width = dims.width;
      this->height = dims.height;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_Dims_c< T >::operator==( 
      const TCT_Dims_c< T >& dims ) const
{
   return(( TCTF_IsEQ( this->width, dims.width )) &&
          ( TCTF_IsEQ( this->height, dims.height )) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_Dims_c< T >::operator!=( 
      const TCT_Dims_c< T >& dims ) const
{
   return(( !this->operator==( dims )) ? 
          true : false );
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template<class T> void TCT_Dims_c< T >::ExtractString(
      TC_DataMode_t mode,
      string*       psrData,
      size_t        precision ) const
{
	int i;
   if ( psrData )
   {
      if ( precision == SIZE_MAX )
      {
         TC_MinGrid_c& minGrid = TC_MinGrid_c::GetInstance( );
         precision = minGrid.GetPrecision( );
      }

      TCT_Dims_c< T >* pdims = const_cast< TCT_Dims_c< T >* >( this );

      char szData[ TIO_FORMAT_STRING_LEN_DATA ];
      for (i = 0; i < TIO_FORMAT_STRING_LEN_DATA; i++) {
		  szData[i] = (char)0;
	  }

      switch( mode )
      {
      case TC_DATA_INT:
         sprintf( szData, "%d %d", 
                          *reinterpret_cast< int* >( &pdims->width ),
                          *reinterpret_cast< int* >( &pdims->height ));
         break;

      case TC_DATA_UINT:
         sprintf( szData, "%u %u", 
		          *reinterpret_cast< unsigned int* >( &pdims->width ),
                          *reinterpret_cast< unsigned int* >( &pdims->height ));
         break;

      case TC_DATA_LONG:
         sprintf( szData, "%ld %ld", 
		          *reinterpret_cast< long* >( &pdims->width ),
                          *reinterpret_cast< long* >( &pdims->height ));
         break;

      case TC_DATA_ULONG:
         sprintf( szData, "%lu %lu", 
		          *reinterpret_cast< unsigned long* >( &pdims->width ),
                          *reinterpret_cast< unsigned long* >( &pdims->height ));
         break;

      case TC_DATA_SIZE:
         sprintf( szData, "%lu %lu", 
		          *reinterpret_cast< size_t* >( &pdims->width ),
                          *reinterpret_cast< size_t* >( &pdims->height ));
         break;

      case TC_DATA_FLOAT:
         sprintf( szData, "%0.*f %0.*f", 
		          precision, *reinterpret_cast< double* >( &pdims->width ),
                          precision, *reinterpret_cast< double* >( &pdims->height ));
         break;

      case TC_DATA_EXP:
         sprintf( szData, "%0.*e %0.*e", 
		          precision + 1, *reinterpret_cast< double* >( &pdims->width ),
                          precision + 1, *reinterpret_cast< double* >( &pdims->height ));
         break;

      case TC_DATA_UNDEFINED:
         sprintf( szData, "? ?" );
         break;
      }  

      *psrData = szData;
   }
}

#endif 
