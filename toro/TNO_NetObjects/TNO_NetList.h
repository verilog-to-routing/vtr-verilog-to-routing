//===========================================================================//
// Purpose : Declaration and inline definitions for a TNO_NetList_c class.
//
//           Inline methods include:
//           - SetCapacity
//           - ResetCapacity
//           - GetCapacity
//           - GetLength
//           - Add
//           - Delete
//           - Clear
//           - Find
//           - FindIndex
//           - FindName
//           - Uniquify
//           - IsMember
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
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

#ifndef TNO_NET_LIST_H
#define TNO_NET_LIST_H

#include "TCT_SortedNameDynamicVector.h"

#include "TNO_Net.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
class TNO_NetList_c 
      : 
      private TCT_SortedNameDynamicVector_c< TNO_Net_c >
{
public:

   TNO_NetList_c( size_t allocLen = 1,
                  size_t reallocLen = 1 );
   TNO_NetList_c( const TNO_NetList_c& netList );
   ~TNO_NetList_c( void );

   TNO_NetList_c& operator=( const TNO_NetList_c& netList );
   bool operator==( const TNO_NetList_c& netList ) const;
   bool operator!=( const TNO_NetList_c& netList ) const;

   TNO_Net_c* operator[]( size_t netIndex );
   TNO_Net_c* operator[]( size_t netIndex ) const;

   TNO_Net_c* operator[]( const TNO_Net_c& net );
   TNO_Net_c* operator[]( const TNO_Net_c& net ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   bool ExpandNameList( const TNO_NameList_t& netNameList,
                        TNO_NameList_t* pnetNameList,
                        TC_TypeMode_t netType,
                        bool isShowBusEnabled = true,
                        bool isShowWarningEnabled = true,
                        bool isShowErrorEnabled = false,
                        const char* pszShowRegExpType = 0 ) const;
   bool ExpandNameList( const TNO_NameList_t& netNameList,
                        TNO_NameList_t* pnetNameList,
                        bool isShowBusEnabled = true,
                        bool isShowWarningEnabled = true,
                        bool isShowErrorEnabled = false,
                        const char* pszShowRegExpType = 0 ) const;
   bool ExpandNameList( const string& srNetName,
                        TNO_NameList_t* pnetNameList,
                        bool isShowBusEnabled = true,
                        bool isShowWarningEnabled = true,
                        bool isShowErrorEnabled = false,
                        const char* pszShowRegExpType = 0 ) const;
   bool ExpandNameList( const char* pszNetName,
                        TNO_NameList_t* pnetNameList,
                        bool isShowBusEnabled = true,
                        bool isShowWarningEnabled = true,
                        bool isShowErrorEnabled = false,
                        const char* pszShowRegExpType = 0 ) const;

   void SetCapacity( size_t capacity );
   void SetCapacity( size_t allocLen,
                     size_t reallocLen );

   void ResetCapacity( size_t capacity );
   void ResetCapacity( size_t allocLen,
                       size_t reallocLen );

   size_t GetCapacity( void ) const;

   size_t GetLength( void ) const;

   TNO_Net_c* Add( const TNO_Net_c& net );
   TNO_Net_c* Add( const string& srNetName );
   TNO_Net_c* Add( const char* pszNetName );

   void Delete( const TNO_Net_c& net );
   void Delete( const string& srNetName );
   void Delete( const char* pszNetName );

   void Clear( void );

   TNO_Net_c* Find( const TNO_Net_c& net ) const;
   TNO_Net_c* Find( const string& srNetName ) const;
   TNO_Net_c* Find( const char* pszNetName ) const;

   size_t FindIndex( const TNO_Net_c& net ) const;
   size_t FindIndex( const string& srNetName ) const;
   size_t FindIndex( const char* pszNetName ) const;

   const char* FindName( size_t netIndex ) const;

   void Uniquify( void );
   
   bool IsRoutable( size_t netIndex ) const;
   bool IsRoutable( const TNO_Net_c& net ) const;
   bool IsRoutable( const string& srNetName ) const;
   bool IsRoutable( const char* pszNetName ) const;

   bool IsMember( const TNO_Net_c& net ) const;
   bool IsMember( const string& srNetName ) const;
   bool IsMember( const char* pszNetName ) const;

   bool IsValid( void ) const;

private:

   bool ExpandBusRegExp_( TNO_NameList_t* pnameList ) const;
   bool ApplyRegExp_( const TNO_NameList_t& nameList,
                      TNO_NameList_t* pnameList,
                      bool isShowWarningEnabled,
                      bool isShowErrorEnabled,
                      const char* pszShowRegExpType ) const;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
inline void TNO_NetList_c::SetCapacity( 
      size_t capacity )
{
   TCT_SortedNameDynamicVector_c< TNO_Net_c >::SetCapacity( capacity );
}

//===========================================================================//
inline void TNO_NetList_c::SetCapacity( 
      size_t allocLen,
      size_t reallocLen )
{
   TCT_SortedNameDynamicVector_c< TNO_Net_c >::SetCapacity( allocLen, reallocLen );
}

//===========================================================================//
inline void TNO_NetList_c::ResetCapacity( 
      size_t capacity )
{
   TCT_SortedNameDynamicVector_c< TNO_Net_c >::ResetCapacity( capacity );
}

//===========================================================================//
inline void TNO_NetList_c::ResetCapacity( 
      size_t allocLen,
      size_t reallocLen )
{
   TCT_SortedNameDynamicVector_c< TNO_Net_c >::ResetCapacity( allocLen, reallocLen );
}

//===========================================================================//
inline size_t TNO_NetList_c::GetCapacity( 
      void ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::GetCapacity( ));
}

//===========================================================================//
inline size_t TNO_NetList_c::GetLength( 
      void ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::GetLength( ));
}

//===========================================================================//
inline TNO_Net_c* TNO_NetList_c::Add( 
      const TNO_Net_c& net )
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::Add( net ));
}

//===========================================================================//
inline TNO_Net_c* TNO_NetList_c::Add( 
      const string& srNetName )
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::Add( srNetName ));
}

//===========================================================================//
inline TNO_Net_c* TNO_NetList_c::Add( 
      const char* pszNetName )
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::Add( pszNetName ));
}

//===========================================================================//
inline void TNO_NetList_c::Delete( 
      const TNO_Net_c& net )
{
   TCT_SortedNameDynamicVector_c< TNO_Net_c >::Delete( net );
}

//===========================================================================//
inline void TNO_NetList_c::Delete( 
      const string& srNetName )
{
   TCT_SortedNameDynamicVector_c< TNO_Net_c >::Delete( srNetName );
}

//===========================================================================//
inline void TNO_NetList_c::Delete( 
      const char* pszNetName )
{
   TCT_SortedNameDynamicVector_c< TNO_Net_c >::Delete( pszNetName );
}

//===========================================================================//
inline void TNO_NetList_c::Clear(
      void )
{
   TCT_SortedNameDynamicVector_c< TNO_Net_c >::Clear( );
}

//===========================================================================//
inline TNO_Net_c* TNO_NetList_c::Find( 
      const TNO_Net_c& net ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::Find( net ));
}

//===========================================================================//
inline TNO_Net_c* TNO_NetList_c::Find( 
      const string& srNetName ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::Find( srNetName ));
}

//===========================================================================//
inline TNO_Net_c* TNO_NetList_c::Find( 
      const char* pszNetName ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::Find( pszNetName ));
}

//===========================================================================//
inline size_t TNO_NetList_c::FindIndex( 
      const TNO_Net_c& net ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::FindIndex( net ));
}

//===========================================================================//
inline size_t TNO_NetList_c::FindIndex( 
      const string& srNetName ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::FindIndex( srNetName ));
}

//===========================================================================//
inline size_t TNO_NetList_c::FindIndex( 
      const char* pszNetName ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::FindIndex( pszNetName ));
}

//===========================================================================//
inline const char* TNO_NetList_c::FindName( 
      size_t netIndex ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::FindName( netIndex ));
}

//===========================================================================//
inline void TNO_NetList_c::Uniquify( 
      void )
{
   TCT_SortedNameDynamicVector_c< TNO_Net_c >::Uniquify( );
}

//===========================================================================//
inline bool TNO_NetList_c::IsMember( 
      const TNO_Net_c& net ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::IsMember( net ));
}

//===========================================================================//
inline bool TNO_NetList_c::IsMember( 
      const string& srNetName ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::IsMember( srNetName ));
}

//===========================================================================//
inline bool TNO_NetList_c::IsMember( 
      const char* pszNetName ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::IsMember( pszNetName ));
}

//===========================================================================//
inline bool TNO_NetList_c::IsValid( 
      void ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::IsValid( ));
}

#endif 
