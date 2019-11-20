//===========================================================================//
// Purpose : Template version for a TCT_OrderedQueue ordered queue class.
//
//           Inline methods include:
//           - GetLength
//           - IsValid
//
//           Public methods include:
//           - TCT_OrderedQueue_c, ~TCT_OrderedQueue_c
//           - operator==, operator!=
//           - operator[]
//           - Print
//           - ExtractString
//           - Add
//           - Delete
//           - Clear
//           - Find
//           - IsMember
//
//           Private methods include:
//           - Search_
//
//===========================================================================//

#ifndef TCT_ORDERED_QUEUE_H
#define TCT_ORDERED_QUEUE_H

#include <stdio.h>

#include <deque>
#include <iterator>
#include <algorithm>

#include <string>
using namespace std;

#include "TIO_Typedefs.h"

#include "TC_Typedefs.h"
#include "TC_MinGrid.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > class TCT_OrderedQueue_c 
{
public:

   TCT_OrderedQueue_c( void );
   TCT_OrderedQueue_c( const TCT_OrderedQueue_c& orderedQueue );
   ~TCT_OrderedQueue_c( void );

   bool operator==( const TCT_OrderedQueue_c< T >& orderedQueue ) const;
   bool operator!=( const TCT_OrderedQueue_c< T >& orderedQueue ) const;

   T* operator[]( size_t index );
   T* operator[]( size_t index ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrData ) const;
   void ExtractString( TC_DataMode_t mode,
                       string* psrData,
                       size_t precision = SIZE_MAX ) const;

   size_t GetLength( void ) const;

   void Add( const T& data );

   void Delete( const T& data );

   void Clear( void );

   size_t FindIndex( const T& data ) const;

   bool IsMember( const T& data ) const;
   bool IsValid( void ) const;

private:

   bool Search_( const T& data, T* pdata = 0 ) const;

private:
   
   std::deque< T > list_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > inline size_t TCT_OrderedQueue_c< T >::GetLength( 
      void ) const
{
   return( this->list_.size( ));
}

//===========================================================================//
template< class T > inline bool TCT_OrderedQueue_c< T >::IsValid( 
      void ) const
{
   return( this->GetLength( ) > 0 ? true : false );
}

//===========================================================================//
// Method         : TCT_OrderedQueue_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_OrderedQueue_c< T >::TCT_OrderedQueue_c( 
      void )
{
}

//===========================================================================//
template< class T > TCT_OrderedQueue_c< T >::TCT_OrderedQueue_c( 
      const TCT_OrderedQueue_c& orderedQueue )
      :
      list_( orderedQueue.list_ )
{
}

//===========================================================================//
// Method         : ~TCT_OrderedQueue_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_OrderedQueue_c< T >::~TCT_OrderedQueue_c( 
      void )
{
   this->list_.clear( );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_OrderedQueue_c< T >::operator==(
      const TCT_OrderedQueue_c< T >& orderedQueue ) const
{
   bool isEqual = false;

   if (( this->GetLength( ) > 0 ) &&
      ( orderedQueue.GetLength( ) > 0 ) &&
      ( this->GetLength( ) == orderedQueue.GetLength( )))
   {
      for ( size_t i = 0; i < this->GetLength( ); ++i )
      {
         const T& thisData = *const_cast< TCT_OrderedQueue_c< T >* >( this )->operator[]( i );
         const T& listData = *const_cast< TCT_OrderedQueue_c< T >& >( orderedQueue ).operator[]( i );
         if ( thisData == listData )
         {
            isEqual = true;
            continue;
         }
         else
         {
            isEqual = false;
            break;
         }
      }
   }
   else if (( this->GetLength( ) == 0 ) &&
           ( orderedQueue.GetLength( ) == 0 ))
   {
      isEqual = true;
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
template< class T > bool TCT_OrderedQueue_c< T >::operator!=(
      const TCT_OrderedQueue_c< T >& orderedQueue ) const
{
   return( this->operator==( orderedQueue ) ? false : true );
}

//===========================================================================//
// Method         : operator[]
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_OrderedQueue_c< T >::operator[]( 
      size_t index )
{
   T* pdata = 0;
   if ( index < this->GetLength( ))
   {
      pdata = &this->list_.operator[]( index );
   }
   return( pdata );
}

//===========================================================================//
template< class T > T* TCT_OrderedQueue_c< T >::operator[]( 
      size_t index ) const
{
   T* pdata = 0;
   if ( index < this->GetLength( ))
   {
      pdata = const_cast< TCT_OrderedQueue_c< T >* >( this )->operator[]( index );
   }
   return( pdata );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedQueue_c< T >::Print( 
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
template<class T> void TCT_OrderedQueue_c< T >::ExtractString(
      string* psrData ) const
{
   if ( psrData )
   {
      *psrData = "";

      for ( size_t i = 0; i < this->GetLength( ); ++i )
      {
         const T& data = *this->operator[]( i );
         if ( data.IsValid( ))
         {
            string srData;
	    data.ExtractString( &srData );

            *psrData += srData;
	    *psrData += ( i + 1 == this->GetLength( ) ? "" : " " );
         }
      }
   }
}

//===========================================================================//
template<class T> void TCT_OrderedQueue_c< T >::ExtractString(
      TC_DataMode_t mode,
      string*       psrData,
      size_t        precision ) const
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
template< class T > void TCT_OrderedQueue_c< T >::Add( 
      const T& data )
{
   this->list_.push_back( data );
}

//===========================================================================//
// Method         : Delete
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedQueue_c< T >::Delete( 
      const T& data )
{
   typename std::deque< T >::iterator begin = this->list_.begin( );
   typename std::deque< T >::iterator end = this->list_.end( );
   typename std::deque< T >::iterator iter = std::find( begin, end, data );
   if ( iter != end )
   {
      this->list_.erase( iter );
   }
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedQueue_c< T >::Clear( 
      void )
{
   this->list_.clear( );
}

//===========================================================================//
// Method         : FindIndex
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > size_t TCT_OrderedQueue_c< T >::FindIndex( 
      const T& data ) const
{
   size_t index = string::npos;

   typename std::deque< T >::const_iterator begin = this->list_.begin( );
   typename std::deque< T >::const_iterator end = this->list_.end( );
   typename std::deque< T >::const_iterator iter = std::find( begin, end, data );
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
template< class T > bool TCT_OrderedQueue_c< T >::IsMember(
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
template< class T > bool TCT_OrderedQueue_c< T >::Search_(
      const T& data,
            T* pdata ) const
{
   bool found = false;

   typename std::deque< T >::const_iterator begin = this->list_.begin( );
   typename std::deque< T >::const_iterator end = this->list_.end( );
   typename std::deque< T >::const_iterator iter = std::find( begin, end, data );
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
