//===========================================================================//
// Purpose : Template version of a TCT_RangeOrderedVector range ordered 
//           vector template class.  This ordered vector is defined with a 
//           limited min/max (possibly non-zero) range.
//
//           Inline methods include:
//           - SetCapacity
//           - GetMin, GetMax
//           - GetLength
//           - Insert
//           - Clear
//           - IsValid
//
//           Public methods include:
//           - TCT_RangeOrderedVector_c, ~TCT_RangeOrderedVector_c
//           - operator=
//           - operator==, operator!=
//           - operator[]
//           - Print
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
//                                                                           //
// This program is free software; you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation; version 3 of the License, or any later version. //
//                                                                           //
// This program is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY //
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   //
// for more details.                                                         //
//                                                                           //
// You should have received a copy of the GNU General Public License along   //
// with this program; if not, see <http://www.gnu.org/licenses>.             //
//---------------------------------------------------------------------------//

#ifndef TCT_RANGE_ORDERED_VECTOR_H
#define TCT_RANGE_ORDERED_VECTOR_H

#include <cstdio>
#include <string>
using namespace std;

#include "TCT_Generic.h"
#include "TCT_Range.h"
#include "TCT_OrderedVector.h"

#include "TC_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > class TCT_RangeOrderedVector_c
      :
      private TCT_OrderedVector_c< T >
{
public:

   TCT_RangeOrderedVector_c( size_t n = 0 );
   TCT_RangeOrderedVector_c( size_t n,
                             size_t m );
   TCT_RangeOrderedVector_c( TCT_Range_c< size_t > range );
   TCT_RangeOrderedVector_c( TCT_Range_c< int > range );
   TCT_RangeOrderedVector_c( const TCT_RangeOrderedVector_c& rangeOrderedVector );
   ~TCT_RangeOrderedVector_c( void );

   TCT_RangeOrderedVector_c< T >& operator=( const TCT_RangeOrderedVector_c& rangeOrderedVector );

   bool operator==( const TCT_RangeOrderedVector_c& rangeOrderedVector ) const;
   bool operator!=( const TCT_RangeOrderedVector_c& rangeOrderedVector ) const;

   T* operator[]( size_t i );
   T* operator[]( size_t i ) const;

   T* operator[]( const T& data );
   T* operator[]( const T& data ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetCapacity( size_t capacity );
   void SetCapacity( TCT_Range_c< size_t > range );
   void SetCapacity( TCT_Range_c< int > range );

   size_t GetMin( void ) const;
   size_t GetMax( void ) const;

   size_t GetLength( void ) const;

   void Insert( size_t i, 
                const T& data );

   void Clear( void );

   bool IsValid( void ) const;

private:

   TCT_Range_c< size_t > range_; // Track min/max range indices for vector
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > inline void TCT_RangeOrderedVector_c< T >::SetCapacity( 
      size_t capacity )
{
   this->range_.Set( 0, capacity - 1 );
   TCT_OrderedVector_c< T >::SetCapacity( this->range_.FindLength( ) + 1 );
}

//===========================================================================//
template< class T > inline void TCT_RangeOrderedVector_c< T >::SetCapacity( 
      TCT_Range_c< size_t > range )
{
   this->range_.Set( range.i, range.j );
   TCT_OrderedVector_c< T >::SetCapacity( this->range_.FindLength( ) + 1 );
}

//===========================================================================//
template< class T > inline void TCT_RangeOrderedVector_c< T >::SetCapacity( 
      TCT_Range_c< int > range )
{
   this->range_.Set( range.i, range.j );
   TCT_OrderedVector_c< T >::SetCapacity( this->range_.FindLength( ) + 1 );
}

//===========================================================================//
template< class T > inline size_t TCT_RangeOrderedVector_c< T >::GetMin( 
      void ) const
{
   return( this->range_.FindMin( ));
}

//===========================================================================//
template< class T > inline size_t TCT_RangeOrderedVector_c< T >::GetMax( 
      void ) const
{
   return( this->range_.FindMax( ));
}

//===========================================================================//
template< class T > inline size_t TCT_RangeOrderedVector_c< T >::GetLength( 
      void ) const
{
   return( TCT_OrderedVector_c< T >::GetLength( ));
}

//===========================================================================//
template< class T > inline void TCT_RangeOrderedVector_c< T >::Insert( 
            size_t i,
      const T&     data )
{
   TCT_OrderedVector_c< T >::Insert( i, data );
}

//===========================================================================//
template< class T > inline void TCT_RangeOrderedVector_c< T >::Clear( 
      void )
{
   TCT_OrderedVector_c< T >::Clear( );
}

//===========================================================================//
template< class T > inline bool TCT_RangeOrderedVector_c< T >::IsValid( 
      void ) const
{
   return( this->GetLength( ) > 0 ? true : false );
}

//===========================================================================//
// Method         : TCT_RangeOrderedVector_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_RangeOrderedVector_c< T >::TCT_RangeOrderedVector_c( 
      size_t n )
      :
      TCT_OrderedVector_c< T >( n + 1 ),
      range_( 0, n )
{
}

//===========================================================================//
template< class T > TCT_RangeOrderedVector_c< T >::TCT_RangeOrderedVector_c( 
      size_t n,
      size_t m )
      :
      TCT_OrderedVector_c< T >( TCT_Max( n, m ) - TCT_Min( n, m ) + 1 ),
      range_( TCT_Min( n, m ), TCT_Max( n, m ))
{
}

//===========================================================================//
template< class T > TCT_RangeOrderedVector_c< T >::TCT_RangeOrderedVector_c( 
      TCT_Range_c< size_t > range )
      :
      TCT_OrderedVector_c< T >( range.FindLength( ) + 1 ),
      range_( range.FindMin( ), range.FindMax( ))
{
}

//===========================================================================//
template< class T > TCT_RangeOrderedVector_c< T >::TCT_RangeOrderedVector_c( 
      TCT_Range_c< int > range )
      :
      range_( range.FindMin( ), range.FindMax( ))
{
   TCT_OrderedVector_c< T >::SetCapacity( this->range_.FindLength( ) + 1 );
}

//===========================================================================//
template< class T > TCT_RangeOrderedVector_c< T >::TCT_RangeOrderedVector_c( 
      const TCT_RangeOrderedVector_c& rangeOrderedVector )
      :
      TCT_OrderedVector_c< T >( rangeOrderedVector ),
      range_( rangeOrderedVector.range_ )
{
}

//===========================================================================//
// Method         : ~TCT_RangeOrderedVector_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_RangeOrderedVector_c< T >::~TCT_RangeOrderedVector_c( 
      void )
{
   TCT_OrderedVector_c< T >::Clear( );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > inline TCT_RangeOrderedVector_c< T >& TCT_RangeOrderedVector_c< T >::operator=( 
      const TCT_RangeOrderedVector_c& rangeOrderedVector )
{
   if( &rangeOrderedVector != this )
   {
      this->range_ = rangeOrderedVector.range_;
      TCT_OrderedVector_c< T >::operator=( rangeOrderedVector );
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_RangeOrderedVector_c< T >::operator==( 
      const TCT_RangeOrderedVector_c& rangeOrderedVector ) const
{
   bool isEqual = this->GetLength( ) == rangeOrderedVector.GetLength( ) ? 
                  true : false;
   if( isEqual )
   {
      for( size_t i = 0; i < this->GetLength( ); ++i )
      {
         isEqual = *this->operator[]( i ) == *rangeOrderedVector.operator[]( i ) ?
                   true : false;
         if( !isEqual )
            break;
      }
   }
   return( isEqual );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_RangeOrderedVector_c< T >::operator!=( 
      const TCT_RangeOrderedVector_c& rangeOrderedVector ) const
{
   return( !this->operator==( rangeOrderedVector ) ? true : false );
}

//===========================================================================//
// Method         : operator[]
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > inline T* TCT_RangeOrderedVector_c< T >::operator[]( 
      size_t i )
{
   T* p = 0;

   if( this->range_.IsWithin( i ))
   {
      i -= this->range_.FindMin( );
      p = TCT_OrderedVector_c< T >::operator[]( i );
   }
   return( p );
}

//===========================================================================//
template< class T > inline T* TCT_RangeOrderedVector_c< T >::operator[]( 
      size_t i ) const
{
   T* p = 0;

   if( this->range_.IsWithin( i ))
   {
      i -= this->range_.FindMin( );
      p = TCT_OrderedVector_c< T >::operator[]( i );
   }
   return( p );
}

//===========================================================================//
template< class T > T* TCT_RangeOrderedVector_c< T >::operator[]( 
      const T& data )
{
   return( TCT_OrderedVector_c< T >::operator[]( data ));
}   

//===========================================================================//
template< class T > T* TCT_RangeOrderedVector_c< T >::operator[]( 
      const T& data ) const
{
   return( TCT_OrderedVector_c< T >::operator[]( data ));
}   

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_RangeOrderedVector_c< T >::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   for( size_t i = this->GetMin( ); i <= this->GetMax( ); ++i )
   {
      const T& data = *this->operator[]( i );
      data.Print( pfile, spaceLen );
   }
}

#endif 
