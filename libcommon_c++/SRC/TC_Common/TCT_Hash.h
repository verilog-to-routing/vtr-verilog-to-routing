//===========================================================================//
// Purpose : Template version for a TCT_Hash hash class.
//
//           Inline methods include:
//           - GetLength
//           - IsValid
//
//           Public methods include:
//           - TCT_Hash_c, ~TCT_Hash_c
//           - operator=
//           - operator==, operator!=
//           - Print
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

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TCT_HASH_H
#define TCT_HASH_H

#include <cstdio>
#include <map>
#include <iterator>
using namespace std;

#include "TIO_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > class TCT_Hash_c
{
public:

   TCT_Hash_c( void );
   TCT_Hash_c( const TCT_Hash_c& hash );
   ~TCT_Hash_c( void );

   TCT_Hash_c& operator=( const TCT_Hash_c& hash );

   bool operator==( const TCT_Hash_c& hash ) const;
   bool operator!=( const TCT_Hash_c& hash ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   size_t GetLength( void ) const;

   void Add( const Key& key, const T& value );
   void Add( const TCT_Hash_c< Key, T >& hash );

   void Delete( const Key& key );

   void Clear( void );
  
   bool Find( const Key& key, T** ppvalue ) const;

   bool IsMember( const Key& key ) const;
   bool IsValid( void ) const;

private:

   bool Search_( const Key& key, T** ppvalue = 0 ) const;

private:
   
   std::map< Key, T > map_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > inline size_t TCT_Hash_c< Key, T >::GetLength( 
      void ) const
{
   return( this->map_.size( ));
}

//===========================================================================//
template< class Key, class T > inline bool TCT_Hash_c< Key, T >::IsValid( 
      void ) const
{
   return( this->GetLength( ) > 0 ? true : false );
}

//===========================================================================//
// Method         : TCT_Hash_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > TCT_Hash_c< Key, T >::TCT_Hash_c( 
      void )
{
}

//===========================================================================//
template< class Key, class T > TCT_Hash_c< Key, T >::TCT_Hash_c( 
      const TCT_Hash_c& hash )
      :
      map_( hash.map_ )
{
}

//===========================================================================//
// Method         : ~TCT_Hash_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > TCT_Hash_c< Key, T >::~TCT_Hash_c( 
      void )
{
   this->map_.clear( );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > TCT_Hash_c< Key, T >& TCT_Hash_c< Key, T >::operator=( 
      const TCT_Hash_c& hash )
{
   this->map_.operator=( hash.map_ );

   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > bool TCT_Hash_c< Key, T >::operator==( 
      const TCT_Hash_c& hash ) const
{
   bool isEqual = this->GetLength( ) == hash.GetLength( ) ? 
                  true : false;
   if( isEqual )
   {
      typename std::map< Key, T >::const_iterator begin = this->map_.begin( );
      typename std::map< Key, T >::const_iterator end = this->map_.end( );
      for( typename std::map< Key, T >::const_iterator p = begin; p != end; ++p )
      {
         T value;
         isEqual = ( hash.Find( p->first, &value ) && ( p->second == value ) ? 
                     true : false );
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
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > bool TCT_Hash_c< Key, T >::operator!=( 
      const TCT_Hash_c& hash ) const
{
   return( !this->operator==( hash ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > void TCT_Hash_c< Key, T >::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   typename std::map< Key, T >::const_iterator begin = this->map_.begin( );
   typename std::map< Key, T >::const_iterator end = this->map_.end( );
   for( typename std::map< Key, T >::const_iterator p = begin; p != end; ++p )
   {
      p->first.Print( pfile, spaceLen );
      p->second.Print( pfile, spaceLen + 3 );
   }
}

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > void TCT_Hash_c< Key, T >::Add( 
      const Key& key,
      const T&   value )
{
   pair< Key, T > pair_( key, value );
   this->map_.insert( pair_ );
}

//===========================================================================//
template< class Key, class T > void TCT_Hash_c< Key, T >::Add( 
      const TCT_Hash_c< Key, T >& hash )
{
   typename std::map< Key, T >::iterator begin = this->map_.begin( );
   typename std::map< Key, T >::iterator end = this->map_.end( );
   for( typename std::map< Key, T >::iterator p = begin; p != end; ++p )
   {
      const Key& key = p->first;
      const T& value = p->second;
      this->Add( key, value );
   }
}

//===========================================================================//
// Method         : Delete
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > void TCT_Hash_c< Key, T >::Delete( 
      const Key& key )
{
   typename std::map< Key, T >::iterator end = this->map_.end( );
   typename std::map< Key, T >::iterator iter = this->map_.find( key );
   if( iter != end )
   {
      this->map_.erase( iter );
   }
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > void TCT_Hash_c< Key, T >::Clear( 
      void )
{
   this->map_.clear( );
}

//===========================================================================//
// Method         : Find
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > bool TCT_Hash_c< Key, T >::Find(
      const Key& key,
            T**  ppvalue ) const
{
   bool found = false;

   found = this->Search_( key, ppvalue );

   return( found );
}

//===========================================================================//
// Method         : IsMember
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > bool TCT_Hash_c< Key, T >::IsMember(
      const Key& key ) const
{
   return( this->Search_( key ) ? true : false );
}

//===========================================================================//
// Method         : Search_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
template< class Key, class T > bool TCT_Hash_c< Key, T >::Search_(
      const Key& key,
            T**  ppvalue ) const
{
   bool found = false;

   typename std::map< Key, T >::const_iterator end = this->map_.end( );
   typename std::map< Key, T >::const_iterator iter = this->map_.find( key );
   if( iter != end )
   {
      found = true;

      if( ppvalue )
      {
         *ppvalue = const_cast< T* >( &iter->second );
      }
   }
   return( found );
}

#endif
