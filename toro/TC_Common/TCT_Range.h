//===========================================================================//
// Purpose : Template version for a TCT_Range_c range (i, j) template class.
//
//           Inline methods include:
//           - TCT_Range_c, ~TCT_Range_c
//           - Set, Reset
//           - FindMin, FindMax
//           - FindLength
//           - IsValid
//
//           Public methods include:
//           - operator=
//           - operator==, operator!=
//           - ExtractString
//           - Normalize
//           - IsOverlapping
//           - IsWithin
//           - IsConnected
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

#ifndef TCT_RANGE_H
#define TCT_RANGE_H

#include <cstdio>
#include <climits>
#include <cstring>
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
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > class TCT_Range_c
{
public:

   TCT_Range_c( void );
   TCT_Range_c( T i, T j );
   TCT_Range_c( const TCT_Range_c< T >& range );
   ~TCT_Range_c( void );

   TCT_Range_c< T >& operator=( const TCT_Range_c< T >& range );
   bool operator==( const TCT_Range_c< T >& range ) const;
   bool operator!=( const TCT_Range_c< T >& range ) const;

   void ExtractString( TC_DataMode_t mode,
                       string* psrData,
                       size_t precision = SIZE_MAX ) const;

   void Set( const TCT_Range_c< T >& range );
   void Set( T i, T j );
   void Reset( void );

   void Normalize( void );
   void Normalize( T i, T j );

   T FindMin( void ) const;
   T FindMax( void ) const;

   T FindLength( void ) const;

   bool IsOverlapping( const TCT_Range_c< T >& range ) const;
   bool IsWithin( const TCT_Range_c< T >& range ) const;
   bool IsWithin( T r ) const;
   bool IsConnected( const TCT_Range_c< T >& range ) const;

   bool IsValid( void ) const;

public:

   T i;
   T j;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > inline TCT_Range_c< T >::TCT_Range_c( 
      void ) 
      :
      i( static_cast< T >( INT_MAX )), 
      j( static_cast< T >( INT_MIN ))
{
}

//===========================================================================//
template< class T > inline TCT_Range_c< T >::TCT_Range_c( 
      T i_, 
      T j_ )
      :
      i( i_ ),
      j( j_ )
{
}

//===========================================================================//
template< class T > inline TCT_Range_c< T >::TCT_Range_c( 
      const TCT_Range_c< T >& range )
      :
      i( range.i ),
      j( range.j )
{
}

//===========================================================================//
template< class T > inline TCT_Range_c< T >::~TCT_Range_c( 
      void )
{
}

//===========================================================================//
template< class T > inline void TCT_Range_c< T >::Set(
      const TCT_Range_c< T >& range )
{
   this->i = range.i;
   this->j = range.j;
}

//===========================================================================//
template< class T > inline void TCT_Range_c< T >::Set(
      T i_,
      T j_ )
{
   this->i = i_;
   this->j = j_;
}

//===========================================================================//
template< class T > inline void TCT_Range_c< T >::Reset(
      void )
{
   this->i = static_cast< T >( INT_MAX );
   this->j = static_cast< T >( INT_MIN );
}

//===========================================================================//
template< class T > inline T TCT_Range_c< T >::FindMin( 
      void ) const
{
   return( TCT_Min( this->i, this->j ));
}

//===========================================================================//
template< class T > inline T TCT_Range_c< T >::FindMax( 
      void ) const
{
   return( TCT_Max( this->i, this->j ));
}

//===========================================================================//
template< class T > inline T TCT_Range_c< T >::FindLength( 
      void ) const
{
   return( this->j - this->i );
}

//===========================================================================//
template< class T > inline bool TCT_Range_c< T >::IsValid( 
      void ) const
{
   return(( TCTF_IsLE( this->i, this->j )) ? true : false );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_Range_c< T >& TCT_Range_c< T >::operator=( 
      const TCT_Range_c< T >& range )
{
   if( &range != this )
   {
      this->i = range.i;
      this->j = range.j;
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
template< class T > bool TCT_Range_c< T >::operator==( 
      const TCT_Range_c< T >& range ) const
{
   return(( TCTF_IsEQ( this->i, range.i )) &&
          ( TCTF_IsEQ( this->j, range.j )) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_Range_c< T >::operator!=( 
      const TCT_Range_c< T >& range ) const
{
   return(( !this->operator==( range )) ? 
          true : false );
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template<class T> void TCT_Range_c< T >::ExtractString(
      TC_DataMode_t mode,
      string*       psrData,
      size_t        precision ) const
{
   if( psrData )
   {
      if( precision == SIZE_MAX )
      {
         precision = TC_MinGrid_c::GetInstance( ).GetPrecision( );
      }

      TCT_Range_c< T >* prange = const_cast< TCT_Range_c< T >* >( this );

      char szData[TIO_FORMAT_STRING_LEN_DATA];
      memset( szData, 0, sizeof( szData ));

      switch( mode )
      {
      case TC_DATA_INT:
         sprintf( szData, "%d %d", 
                          *reinterpret_cast< int* >( &prange->i ),
                          *reinterpret_cast< int* >( &prange->j ));
         break;

      case TC_DATA_UINT:
         sprintf( szData, "%u %u", 
		          *reinterpret_cast< unsigned int* >( &prange->i ),
                          *reinterpret_cast< unsigned int* >( &prange->j ));
         break;

      case TC_DATA_LONG:
         sprintf( szData, "%ld %ld", 
		          *reinterpret_cast< long* >( &prange->i ),
                          *reinterpret_cast< long* >( &prange->j ));
         break;

      case TC_DATA_ULONG:
         sprintf( szData, "%lu %lu", 
		          *reinterpret_cast< unsigned long* >( &prange->i ),
                          *reinterpret_cast< unsigned long* >( &prange->j ));
         break;

      case TC_DATA_SIZE:
         sprintf( szData, "%lu %lu", 
		          *reinterpret_cast< size_t* >( &prange->i ),
                          *reinterpret_cast< size_t* >( &prange->j ));
         break;

      case TC_DATA_FLOAT:
         sprintf( szData, "%0.*f %0.*f", 
		          precision, *reinterpret_cast< double* >( &prange->i ),
                          precision, *reinterpret_cast< double* >( &prange->j ));
         break;

      case TC_DATA_EXP:
         sprintf( szData, "%0.*e %0.*e", 
		          static_cast< int >( precision + 1 ), *reinterpret_cast< double* >( &prange->i ),
                          static_cast< int >( precision + 1 ), *reinterpret_cast< double* >( &prange->j ));
         break;

      case TC_DATA_STRING:
         break;

      case TC_DATA_UNDEFINED:
         sprintf( szData, "? ?" );
         break;
      }  

      *psrData = szData;
   }
}

//===========================================================================//
// Method         : Normalize
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_Range_c< T >::Normalize(
      void )
{
   T min = this->FindMin( );
   T max = this->FindMax( );

   this->Set( min, max );
}

//===========================================================================//
template< class T > void TCT_Range_c< T >::Normalize(
      T i_,
      T j_ )
{
   T min = this->FindMin( );
   T max = this->FindMax( );

   this->Set( TCT_Max( min, i_ ), TCT_Min( max, j_ ));
}

//===========================================================================//
// Method         : IsOverlapping
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_Range_c< T >::IsOverlapping(
      const TCT_Range_c< T >& range ) const
{
   T thisMin = TCT_Min( this->i, this->j );
   T thisMax = TCT_Max( this->i, this->j );
   T rangeMin = TCT_Min( range.i, range.j );
   T rangeMax = TCT_Max( range.i, range.j );

   return(( TCTF_IsLT( thisMin, rangeMax )) && 
          ( TCTF_IsGT( thisMax, rangeMin )) ? 
          true : false );
}

//===========================================================================//
// Method         : IsWithin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_Range_c< T >::IsWithin( 
      const TCT_Range_c< T >& range ) const
{
   return(( TCTF_IsGE( range.i, this->i )) && 
          ( TCTF_IsLE( range.j, this->j )) ? 
          true : false );
}

//===========================================================================//
template< class T > bool TCT_Range_c< T >::IsWithin( 
      T r ) const
{
   return(( TCTF_IsGE( r, this->i )) && 
          ( TCTF_IsLE( r, this->j )) ? 
          true : false );
}

//===========================================================================//
// Method         : IsConnected
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_Range_c< T >::IsConnected(
      const TCT_Range_c< T >& range ) const
{
  return( this->IsOverlapping( range ) ||
	  this->IsWithin( range ) ||
	  range.IsWithin( *this ) ?
          true : false );
}

#endif
