//===========================================================================//
// Purpose : Template version for a TCT_SortedVector sorted vector class.
//
//           Inline methods include:
//           - SetCapacity
//           - GetCapacity
//           - GetLength
//           - IsValid
//
//           Public methods include:
//           - TCT_SortedVector_c, ~TCT_SortedVector_c
//           - operator=
//           - operator==, operator!=
//           - operator[]
//           - Print
//           - ExtractString
//           - Add
//           - Replace
//           - Delete
//           - Clear
//           - Find
//           - FindIndex
//           - IsMember
//
//           Private methods include:
//           - Search_
//           - Sort_
//
//===========================================================================//

#ifndef TCT_SORTED_VECTOR_H
#define TCT_SORTED_VECTOR_H

#include <stdio.h>

#include <vector>
#include <iterator>
#include <algorithm>

#include <string>
using namespace std;

#include "TIO_Typedefs.h"

#include "TC_Typedefs.h"
#include "TCT_Generic.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > class TCT_SortedVector_c
{
public:

   TCT_SortedVector_c( size_t capacity = TC_DEFAULT_CAPACITY );
   TCT_SortedVector_c( const TCT_SortedVector_c& sortedVector );
   ~TCT_SortedVector_c( void );

   TCT_SortedVector_c& operator=( const TCT_SortedVector_c& sortedVector );

   bool operator==( const TCT_SortedVector_c& sortedVector ) const;
   bool operator!=( const TCT_SortedVector_c& sortedVector ) const;

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
   void Add( const TCT_SortedVector_c< T >& sortedVector );

   void Replace( const T& data );

   void Delete( const T& data );

   void Clear( void );

   bool Find( const T& data, T* pdata ) const;
   size_t FindIndex( const T& data ) const;

   bool IsMember( const T& data ) const;
   bool IsValid( void ) const;

private:

   bool Search_( const T& data, T* pdata = 0 ) const;
   void Sort_( void ) const;
   
private:
   
   bool isSorted_;
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
template< class T > inline void TCT_SortedVector_c< T >::SetCapacity(
      size_t capacity )
{
   this->capacity_ = capacity;
   this->vector_.reserve( capacity );
}

//===========================================================================//
template< class T > inline size_t TCT_SortedVector_c< T >::GetCapacity(
      void ) const
{
   return( TCT_Max( this->capacity_, this->vector_.size( )));
}

//===========================================================================//
template< class T > inline size_t TCT_SortedVector_c< T >::GetLength( 
      void ) const
{
   return( this->vector_.size( ));
}

//===========================================================================//
template< class T > inline bool TCT_SortedVector_c< T >::IsValid( 
      void ) const
{
   return( this->GetLength( ) > 0 ? true : false );
}

//===========================================================================//
// Method         : TCT_SortedVector_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_SortedVector_c< T >::TCT_SortedVector_c( 
      size_t capacity )
      :
      isSorted_( false ),
      capacity_( capacity )
{
   this->vector_.reserve( capacity );
}

//===========================================================================//
template< class T > TCT_SortedVector_c< T >::TCT_SortedVector_c( 
      const TCT_SortedVector_c& sortedVector )
      :
      isSorted_( sortedVector.isSorted_ ),
      capacity_( sortedVector.GetCapacity( )),
      vector_( sortedVector.vector_ )
{
}

//===========================================================================//
// Method         : ~TCT_SortedVector_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_SortedVector_c< T >::~TCT_SortedVector_c( 
      void )
{
   this->vector_.clear( );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_SortedVector_c< T >& TCT_SortedVector_c< T >::operator=( 
      const TCT_SortedVector_c& sortedVector )
{
   this->isSorted_ = sortedVector.isSorted_;
   this->capacity_ = sortedVector.capacity_;
   this->vector_.operator=( sortedVector.vector_ );

   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_SortedVector_c< T >::operator==( 
      const TCT_SortedVector_c& sortedVector ) const
{
   bool isEqual = this->GetLength( ) == sortedVector.GetLength( ) ? 
                  true : false;
   if ( isEqual )
   {
      for ( size_t i = 0; i < this->GetLength( ); ++i )
      {
         isEqual = *this->operator[]( i ) == *sortedVector.operator[]( i ) ?
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
template< class T > bool TCT_SortedVector_c< T >::operator!=( 
      const TCT_SortedVector_c& sortedVector ) const
{
   return( !this->operator==( sortedVector ) ? true : false );
}

//===========================================================================//
// Method         : operator[]
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_SortedVector_c< T >::operator[]( 
      size_t index )
{
   T* pdata = 0;

   if ( index < this->GetLength( ))
   {
      if ( !isSorted_ )
      {
	this->Sort_( );
      }
      pdata = &this->vector_.operator[]( index );
   }
   return( pdata );
}

//===========================================================================//
template< class T > T* TCT_SortedVector_c< T >::operator[]( 
      size_t index ) const
{
   T* pdata = const_cast< TCT_SortedVector_c< T >* >( this )->operator[]( index );

   return( pdata );
}

//===========================================================================//
template< class T > T* TCT_SortedVector_c< T >::operator[]( 
      const T& data )
{
   size_t index = this->FindIndex( data );
   return( index != string::npos ? this->operator[]( index ) : 0 );
}   

//===========================================================================//
template< class T > T* TCT_SortedVector_c< T >::operator[]( 
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
template< class T > void TCT_SortedVector_c< T >::Print( 
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
template<class T> void TCT_SortedVector_c< T >::ExtractString(
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
            iDataValue = *reinterpret_cast< int* >( this->operator[]( i ));
            sprintf( szDataString, "%d", iDataValue );
            break;

         case TC_DATA_UINT:
            uiDataValue = *reinterpret_cast< unsigned int* >( this->operator[]( i ));
            sprintf( szDataString, "%u", uiDataValue );
            break;

         case TC_DATA_LONG:
            lDataValue = *reinterpret_cast< long* >( this->operator[]( i ));
            sprintf( szDataString, "%ld", lDataValue );
            break;

         case TC_DATA_ULONG:
            ulDataValue = *reinterpret_cast< unsigned long* >( this->operator[]( i ));
            sprintf( szDataString, "%lu", ulDataValue );
            break;

         case TC_DATA_SIZE:
            sDataValue = *reinterpret_cast< size_t* >( this->operator[]( i ));
            sprintf( szDataString, "%lu", sDataValue );
            break;

         case TC_DATA_FLOAT:
            fDataValue = *reinterpret_cast< double* >( this->operator[]( i ));
            sprintf( szDataString, "%0.*f", precision, fDataValue );
            break;

         case TC_DATA_EXP:
            eDataValue = *reinterpret_cast< double* >( this->operator[]( i ));
            sprintf( szDataString, "%0.*e", precision + 1, eDataValue );
            break;

         case TC_DATA_STRING:
            srDataValue = *reinterpret_cast< string* >( this->operator[]( i ));
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
template< class T > void TCT_SortedVector_c< T >::Add( 
      const T& data )
{
   this->vector_.push_back( data );

   this->isSorted_ = false;
}

//===========================================================================//
template< class T > void TCT_SortedVector_c< T >::Add( 
      const TCT_SortedVector_c< T >& sortedVector )
{
   for ( size_t i = 0; i < sortedVector.GetLength( ); ++i )
   {
      const T& data = *sortedVector.operator[]( i );
      this->Add( data );
   }
}

//===========================================================================//
// Method         : Replace
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_SortedVector_c< T >::Replace( 
      const T& data )
{
   this->Delete( data );
   this->Add( data );
}

//===========================================================================//
// Method         : Delete
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_SortedVector_c< T >::Delete( 
      const T& data )
{
   if ( !this->isSorted_ )
   {
      this->Sort_( );
   }

   typename std::vector< T >::iterator begin = this->vector_.begin( );
   typename std::vector< T >::iterator end = this->vector_.end( );
   if ( std::binary_search( begin, end, data ))
   {
      typename std::vector< T >::iterator iter = std::lower_bound( begin, end, data );

      this->vector_.erase( iter );
   }
   this->isSorted_ = false;
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_SortedVector_c< T >::Clear( 
      void )
{
   this->vector_.clear( );

   this->isSorted_ = false;
}

//===========================================================================//
// Method         : Find
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_SortedVector_c< T >::Find(
      const T& data,
            T* pdata ) const
{
   bool found = false;

   if ( !this->isSorted_ )
   {
      this->Sort_( );
   }
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
template< class T > size_t TCT_SortedVector_c< T >::FindIndex( 
      const T& data ) const
{
   size_t index = string::npos;

   if ( !this->isSorted_ )
   {
      this->Sort_( );
   }

   typename std::vector< T >::const_iterator begin = this->vector_.begin( );
   typename std::vector< T >::const_iterator end = this->vector_.end( );
   if ( std::binary_search( begin, end, data ))
   {
      typename std::vector< T >::const_iterator iter = std::lower_bound( begin, end, data );

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
template< class T > bool TCT_SortedVector_c< T >::IsMember(
      const T& data ) const
{
   if ( !this->isSorted_ )
   {
      this->Sort_( );
   }
   return( this->Search_( data ) ? true : false );
}

//===========================================================================//
// Method         : Search_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_SortedVector_c< T >::Search_(
      const T& data,
            T* pdata ) const
{
   bool found = false;

   typename std::vector< T >::const_iterator begin = this->vector_.begin( );
   typename std::vector< T >::const_iterator end = this->vector_.end( );
   if ( std::binary_search( begin, end, data ))
   {
      found = true;

      if ( pdata )
      {
         typename std::vector< T >::const_iterator iter = std::lower_bound( begin, end, data );

         *pdata = *iter;
      }
   }
   return( found );
}

//===========================================================================//
// Method         : Sort_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_SortedVector_c< T >::Sort_(
      void ) const
{
   if ( !this->isSorted_ )
   {
      TCT_SortedVector_c* psortedVector = 0;
      psortedVector = const_cast< TCT_SortedVector_c* >( this );

      typename std::vector< T >::iterator begin = psortedVector->vector_.begin( );
      typename std::vector< T >::iterator end = psortedVector->vector_.end( );
      std::sort( begin, end );

      psortedVector->isSorted_ = true;
   }
}

#endif
