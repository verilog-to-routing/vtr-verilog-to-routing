//===========================================================================//
// Purpose : Template version for a TCT_OrderedVector ordered vector class.
//
//           Inline methods include:
//           - SetCapacity
//           - GetCapacity
//           - GetLength
//           - IsValid
//
//           Public methods include:
//           - TCT_OrderedVector_c, ~TCT_OrderedVector_c
//           - operator=
//           - operator==, operator!=
//           - operator[]
//           - Print
//           - ExtractString
//           - Add
//           - Insert
//           - Replace
//           - Delete
//           - Remove
//           - Clear
//           - Reverse
//           - Find
//           - FindIndex
//           - IsMember
//
//           Private methods include:
//           - Search_
//
//===========================================================================//

#ifndef TCT_ORDERED_VECTOR_H
#define TCT_ORDERED_VECTOR_H

#include <stdio.h>

#include <vector>
#include <iterator>
#include <algorithm>

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
template< class T > class TCT_OrderedVector_c
{
public:

   TCT_OrderedVector_c( size_t capacity = TC_DEFAULT_CAPACITY );
   TCT_OrderedVector_c( const TCT_OrderedVector_c& orderedVector );
   ~TCT_OrderedVector_c( void );

   TCT_OrderedVector_c& operator=( const TCT_OrderedVector_c& orderedVector );

   bool operator==( const TCT_OrderedVector_c& orderedVector ) const;
   bool operator!=( const TCT_OrderedVector_c& orderedVector ) const;

   T* operator[]( size_t index );
   T* operator[]( size_t index ) const;

   T* operator[]( const T& data );
   T* operator[]( const T& data ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( TC_DataMode_t mode,
                       string* psrData,
                       size_t precision = SIZE_MAX,
                       size_t maxLen = INT_MAX ) const;

   void SetCapacity( size_t capacity );
   size_t GetCapacity( void ) const;

   size_t GetLength( void ) const;

   void Add( const T& data );
   void Add( const TCT_OrderedVector_c< T >& orderedVector );

   void Insert( size_t index, const T& data );

   void Replace( const T& data );
   void Replace( size_t index, const T& data );

   void Delete( const T& data );
   void Delete( size_t index );

   void Remove( const T& data );

   T* At( size_t index );
   T* At( size_t index ) const;

   void Clear( void );
  
   void Reverse( void );

   bool Find( const T& data, T* pdata ) const;
   size_t FindIndex( const T& data ) const;

   bool IsMember( const T& data ) const;
   bool IsValid( void ) const;

private:

   bool Search_( const T& data, T* pdata = 0 ) const;

private:
   
   size_t capacity_;
   std::vector< T > vector_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > inline void TCT_OrderedVector_c< T >::SetCapacity(
      size_t capacity )
{
   this->capacity_ = capacity;
   this->vector_.reserve( capacity );
}

//===========================================================================//
template< class T > inline size_t TCT_OrderedVector_c< T >::GetCapacity(
      void ) const
{
   return( TCT_Max( this->capacity_, this->vector_.size( )));
}

//===========================================================================//
template< class T > inline size_t TCT_OrderedVector_c< T >::GetLength( 
      void ) const
{
   return( this->vector_.size( ));
}

//===========================================================================//
template< class T > inline bool TCT_OrderedVector_c< T >::IsValid( 
      void ) const
{
   return( this->GetLength( ) > 0 ? true : false );
}

//===========================================================================//
// Method         : TCT_OrderedVector_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_OrderedVector_c< T >::TCT_OrderedVector_c( 
      size_t capacity )
      :
      capacity_( capacity )
{
   this->vector_.reserve( capacity );
}

//===========================================================================//
template< class T > TCT_OrderedVector_c< T >::TCT_OrderedVector_c( 
      const TCT_OrderedVector_c& orderedVector )
      :
      capacity_( orderedVector.capacity_ ),
      vector_( orderedVector.vector_ )
{
}

//===========================================================================//
// Method         : ~TCT_OrderedVector_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_OrderedVector_c< T >::~TCT_OrderedVector_c( 
      void )
{
   this->vector_.clear( );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_OrderedVector_c< T >& TCT_OrderedVector_c< T >::operator=( 
      const TCT_OrderedVector_c& orderedVector )
{
   this->capacity_ = orderedVector.capacity_;
   this->vector_.operator=( orderedVector.vector_ );

   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_OrderedVector_c< T >::operator==( 
      const TCT_OrderedVector_c& orderedVector ) const
{
   bool isEqual = this->GetLength( ) == orderedVector.GetLength( ) ? 
                  true : false;
   if ( isEqual )
   {
      for ( size_t i = 0; i < this->GetLength( ); ++i )
      {
         isEqual = *this->operator[]( i ) == *orderedVector.operator[]( i ) ?
                   true : false;
         if ( !isEqual )
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
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_OrderedVector_c< T >::operator!=( 
      const TCT_OrderedVector_c& orderedVector ) const
{
   return( !this->operator==( orderedVector ) ? true : false );
}

//===========================================================================//
// Method         : operator[]
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_OrderedVector_c< T >::operator[]( 
      size_t index )
{
   T* pdata = 0;

   if ( index < this->GetLength( ))
   {
      pdata = &this->vector_.operator[]( index );
   }
   return( pdata );
}

//===========================================================================//
template< class T > T* TCT_OrderedVector_c< T >::operator[]( 
      size_t index ) const
{
   T* pdata = const_cast< TCT_OrderedVector_c< T >* >( this )->operator[]( index );

   return( pdata );
}

//===========================================================================//
template< class T > T* TCT_OrderedVector_c< T >::operator[]( 
      const T& data )
{
   size_t index = this->FindIndex( data );
   return( index != string::npos ? this->operator[]( index ) : 0 );
}   

//===========================================================================//
template< class T > T* TCT_OrderedVector_c< T >::operator[]( 
      const T& data ) const
{
   size_t index = this->FindIndex( data );
   return( index != string::npos ? this->operator[]( index ) : 0 );
}   

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedVector_c< T >::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   for ( size_t i = 0; i < this->GetLength( ); ++i )
   {
      const T& data = *this->operator[]( i );
      if ( data.IsValid( ))  
      {
         data.Print( pfile, spaceLen );
      }
   }
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedVector_c< T >::ExtractString(
      TC_DataMode_t mode,
      string*       psrData,
      size_t        precision,
      size_t        maxLen ) const
{
   if ( psrData )
   {
      *psrData = "";

      if ( precision == SIZE_MAX )
      {
         TC_MinGrid_c& minGrid = TC_MinGrid_c::GetInstance( );
         precision = minGrid.GetPrecision( );
      }

      for ( size_t i = 0; i < this->GetLength( ); ++i )
      {
         int iDataValue;
         unsigned int uiDataValue;
         long lDataValue;
         unsigned long ulDataValue;
         size_t sDataValue;
         double fDataValue;
         double eDataValue;
         string srDataValue;

         char szDataString[ TIO_FORMAT_STRING_LEN_DATA ];
         memset( szDataString, 0, sizeof( szDataString ));

         switch( mode )
	 {
         case TC_DATA_INT:
            iDataValue = *reinterpret_cast< int* >( this->At( i ));
            sprintf( szDataString, "%d", iDataValue );
            break;

         case TC_DATA_UINT:
            uiDataValue = *reinterpret_cast< unsigned int* >( this->At( i ));
            sprintf( szDataString, "%u", uiDataValue );
            break;

         case TC_DATA_LONG:
            lDataValue = *reinterpret_cast< long* >( this->At( i ));
            sprintf( szDataString, "%ld", lDataValue );
            break;

         case TC_DATA_ULONG:
            ulDataValue = *reinterpret_cast< unsigned long* >( this->At( i ));
            sprintf( szDataString, "%lu", ulDataValue );
            break;

         case TC_DATA_SIZE:
            sDataValue = *reinterpret_cast< size_t* >( this->At( i ));
            sprintf( szDataString, "%lu", sDataValue );
            break;

         case TC_DATA_FLOAT:
            fDataValue = *reinterpret_cast< double* >( this->At( i ));
            sprintf( szDataString, "%0.*f", precision, fDataValue );
            break;

         case TC_DATA_EXP:
            eDataValue = *reinterpret_cast< double* >( this->At( i ));
            sprintf( szDataString, "%0.*e", precision + 1, eDataValue );
            break;

         case TC_DATA_STRING:
            srDataValue = *reinterpret_cast< string* >( this->At( i ));
            sprintf( szDataString, "%.*s", sizeof( szDataString ) - 1, srDataValue.data( ));
            break;

         case TC_DATA_UNDEFINED:
            sprintf( szDataString, "?" );
            break;
	 }

	 size_t lenDataString = TCT_Max( strlen( szDataString ), strlen( "..." ));
         if ( psrData->length( ) + lenDataString >= maxLen )
	 {
	    *psrData += "...";
	    break;
         }

         *psrData += szDataString;
         *psrData += ( i < this->GetLength( ) - 1 ? " " : "" );
      }
   }
}

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedVector_c< T >::Add( 
      const T& data )
{
   this->vector_.push_back( data );
}

//===========================================================================//
template< class T > void TCT_OrderedVector_c< T >::Add( 
      const TCT_OrderedVector_c< T >& orderedVector )
{
   for ( size_t i = 0; i < orderedVector.GetLength( ); ++i )
   {
      const T& data = *orderedVector.operator[]( i );
      this->Add( data );
   }
}

//===========================================================================//
// Method         : Insert
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedVector_c< T >::Insert( 
	    size_t index,
      const T&     data )
{
   for ( size_t i = this->GetLength( ); i < index; ++i )
   {
      T data_;
      this->Add( data_ );
   }
   this->Add( data );
}

//===========================================================================//
// Method         : Replace
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedVector_c< T >::Replace( 
      const T& data )
{
   size_t index = this->FindIndex( data );
   if ( index != string::npos )
   {
      this->Replace( index, data );
   }
}

//===========================================================================//
template< class T > void TCT_OrderedVector_c< T >::Replace( 
            size_t index,
      const T&     data )
{
   const T* pdata = this->operator[]( index );
   if ( pdata )
   {
      typename std::vector< T >::iterator begin = this->vector_.begin( );
      typename std::vector< T >::iterator end = this->vector_.end( );
      typename std::vector< T >::iterator iter = std::find( begin, end, *pdata );
      if ( iter != end )
      {
         this->vector_.erase( iter );
         this->vector_.insert( iter, data );
      }
   }
}

//===========================================================================//
// Method         : Delete
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedVector_c< T >::Delete( 
      const T& data )
{
   typename std::vector< T >::iterator begin = this->vector_.begin( );
   typename std::vector< T >::iterator end = this->vector_.end( );
   typename std::vector< T >::iterator iter = std::find( begin, end, data );
   if ( iter != end )
   {
      this->vector_.erase( iter );
   }
}

//===========================================================================//
template< class T > void TCT_OrderedVector_c< T >::Delete( 
      size_t index )
{
   const T* pdata = this->operator[]( index );
   if ( pdata )
   {
      typename std::vector< T >::iterator begin = this->vector_.begin( );
      typename std::vector< T >::iterator end = this->vector_.end( );
      typename std::vector< T >::iterator iter = std::find( begin, end, *pdata );
      if ( iter != end )
      {
         this->vector_.erase( iter );
      }
   }
}

//===========================================================================//
// Method         : Remove
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedVector_c< T >::Remove( 
      const T& data )
{
   typename std::vector< T >::iterator begin = this->vector_.begin( );
   typename std::vector< T >::iterator end = this->vector_.end( );
   typename std::vector< T >::iterator iter = std::find( begin, end, data );
   if ( iter != end )
   {
      this->vector_.erase( iter );
   }
}

//===========================================================================//
// Method         : At
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_OrderedVector_c< T >::At( 
      size_t index )
{
   T* pdata = 0;

   if ( index < this->GetLength( ))
   {
      pdata = &this->vector_.operator[]( index );
   }
   return( pdata );
}

//===========================================================================//
template< class T > T* TCT_OrderedVector_c< T >::At( 
      size_t index ) const
{
   T* pdata = const_cast< TCT_OrderedVector_c< T >* >( this )->At( index );

   return( pdata );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedVector_c< T >::Clear( 
      void )
{
   this->vector_.clear( );
}

//===========================================================================//
// Method         : Reverse
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedVector_c< T >::Reverse( 
      void )
{
   typename std::vector< T >::iterator begin = this->vector_.begin( );
   typename std::vector< T >::iterator end = this->vector_.end( );
   std::reverse( begin, end );
}

//===========================================================================//
// Method         : Find
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_OrderedVector_c< T >::Find(
      const T& data,
            T* pdata ) const
{
   bool found = false;

   found = this->Search_( data, pdata );

   return( found );
}

//===========================================================================//
// Method         : FindIndex
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > size_t TCT_OrderedVector_c< T >::FindIndex( 
      const T& data ) const
{
   size_t index = string::npos;

   typename std::vector< T >::const_iterator begin = this->vector_.begin( );
   typename std::vector< T >::const_iterator end = this->vector_.end( );
   typename std::vector< T >::const_iterator iter = std::find( begin, end, data );
   if ( iter != end )
   {
      index = 0;
      #if defined( SUN8 ) || defined( SUN10 ) || defined( LINUX24 )
         std::distance( begin, iter, index );
      #elif defined( LINUX24_64 )
         index = std::distance( begin, iter );
      #else
         index = std::distance( begin, iter );
      #endif
   }
   return( index );
}

//===========================================================================//
// Method         : IsMember
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_OrderedVector_c< T >::IsMember(
      const T& data ) const
{
   return( this->Search_( data ) ? true : false );
}

//===========================================================================//
// Method         : Search_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_OrderedVector_c< T >::Search_(
      const T& data,
            T* pdata ) const
{
   bool found = false;

   typename std::vector< T >::const_iterator begin = this->vector_.begin( );
   typename std::vector< T >::const_iterator end = this->vector_.end( );
   typename std::vector< T >::const_iterator iter = std::find( begin, end, data );
   if ( iter != end )
   {
      found = true;

      if ( pdata )
      {
         *pdata = *iter;
      }
   }
   return( found );
}

#endif
