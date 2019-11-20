//===========================================================================//
// Purpose : Template version for a (regexp) TCT_NameList class.
//
//           Inline methods include:
//           - SetCapacity
//           - GetCapacity
//           - GetLength
//           - Clear
//           - IsMember
//           - IsValid
//
//           Public methods include:
//           - TCT_NameList_c, ~TCT_NameList_c
//           - operator=
//           - operator==, operator!=
//           - operator[]
//           - Print
//           - ExtractString
//           - Add, Delete
//           - Find
//           - FindIndex
//           - FindName
//           - ApplyRegExp
//
//           Private methods include:
//           - Search_
//           - ShowMessageInvalidRegExpName_
//           - ShowMessageMissingRegExpName_
//
//===========================================================================//

#ifndef TCT_NAME_LIST_H
#define TCT_NAME_LIST_H

#include <stdio.h>
#include <limits.h>

#include <vector>
#include <iterator>
#include <algorithm>

#include <string>
using namespace std;

#include "TIO_PrintHandler.h"

#include "TCT_RegExpIter.h"

#include "TC_Name.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > class TCT_NameList_c
{
public:

   TCT_NameList_c( size_t capacity = TC_DEFAULT_CAPACITY );
   TCT_NameList_c( const TCT_NameList_c& nameList );
   ~TCT_NameList_c( void );

   TCT_NameList_c& operator=( const TCT_NameList_c& nameList );

   bool operator==( const TCT_NameList_c& nameList ) const;
   bool operator!=( const TCT_NameList_c& nameList ) const;

   T* operator[]( size_t index );
   T* operator[]( size_t index ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrData,
                       size_t maxLen = SIZE_MAX,
                       bool quotedString = true ) const;

   void SetCapacity( size_t capacity );
   size_t GetCapacity( void ) const;

   size_t GetLength( void ) const;

   void Add( const char* pszName );
   void Add( const string& srName );
   void Add( const T& data );
   void Add( const TCT_NameList_c& nameList );

   void Delete( const T& data );
   void Delete( size_t index );

   void Clear( void );

   bool Find( const char* pszName, T* pdata ) const;
   bool Find( const string& srName, T* pdata ) const;
   bool Find( const T& data, T* pdata ) const;

   size_t FindIndex( const char* pszName ) const;
   size_t FindIndex( const string& srName ) const;
   size_t FindIndex( const T& data ) const;

   const char* FindName( size_t index ) const;

   bool ApplyRegExp( const TCT_NameList_c& nameList,
                     bool isShowWarningEnabled = false,
                     bool isShowErrorEnabled = false,
                     const char* pszShowRegExpType = "name" );
   bool ApplyRegExp( TCT_NameList_c* pnameList,
                     bool isShowWarningEnabled = false,
                     bool isShowErrorEnabled = false,
                     const char* pszShowRegExpType = "name" );

   bool MatchRegExp( const TCT_NameList_c& nameList ) const;
   bool MatchRegExp( const char * pszName ) const;
   bool MatchRegExp( const string& srName ) const;

   bool IsMember( const char* pszName ) const;
   bool IsMember( const string& srName ) const;
   bool IsMember( const T& data ) const;

   bool IsValid( void ) const;

private:

   bool Search_( const T& data, 
                 T* pdata = 0 ) const;

   bool ShowMessageInvalidRegExpName_( const string& srRegExpName,
                                       bool isShowWarningEnabled,
                                       bool isShowErrorEnabled,
                                       const char* pszShowRegExpType ) const;
   bool ShowMessageMissingRegExpName_( const string& srRegExpName,
                                       bool isShowWarningEnabled,
                                       bool isShowErrorEnabled,
                                       const char* pszShowRegExpType ) const;

private:
   
   size_t capacity_;
   std::vector< T > vector_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > inline void TCT_NameList_c< T >::SetCapacity(
      size_t capacity )
{
   this->capacity_ = capacity;
   this->vector_.reserve( capacity );
}

//===========================================================================//
template< class T > inline size_t TCT_NameList_c< T >::GetCapacity(
      void ) const
{
   return( TCT_Max( this->capacity_, this->vector_.size( )));
}

//===========================================================================//
template< class T > inline size_t TCT_NameList_c< T >::GetLength( 
      void ) const
{
   return( this->vector_.size( ));
}

//===========================================================================//
template< class T > inline void TCT_NameList_c< T >::Clear( 
      void )
{
   this->vector_.clear( );
}

//===========================================================================//
template< class T > inline bool TCT_NameList_c< T >::IsMember(
      const T& data ) const
{
   return( this->Search_( data ) ? true : false );
}

//===========================================================================//
template< class T > inline bool TCT_NameList_c< T >::IsMember(
      const string& srName ) const
{
   T data( srName );
   return( this->IsMember( data ));
}

//===========================================================================//
template< class T > inline bool TCT_NameList_c< T >::IsMember(
      const char* pszName ) const
{
   T data( pszName );
   return( this->IsMember( data ));
}

//===========================================================================//
template< class T > inline bool TCT_NameList_c< T >::IsValid( 
      void ) const
{
   return( this->GetLength( ) > 0 ? true : false );
}

//===========================================================================//
// Method         : TCT_NameList_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > TCT_NameList_c< T >::TCT_NameList_c( 
      size_t capacity )
      :
      capacity_( capacity )
{
}

//===========================================================================//
template< class T > TCT_NameList_c< T >::TCT_NameList_c( 
      const TCT_NameList_c& nameList )
      :
      capacity_( nameList.capacity_ ),
      vector_( nameList.vector_ )
{
}

//===========================================================================//
// Method         : ~TCT_NameList_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > TCT_NameList_c< T >::~TCT_NameList_c( 
      void )
{
   this->vector_.clear( );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > TCT_NameList_c< T >& TCT_NameList_c< T >::operator=( 
      const TCT_NameList_c& nameList )
{
   this->capacity_ = nameList.capacity_;
   this->vector_.operator=( nameList.vector_ );

   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_NameList_c< T >::operator==( 
      const TCT_NameList_c& nameList ) const
{
   bool isEqual = this->GetLength( ) == nameList.GetLength( ) ? 
                  true : false;
   if ( isEqual )
   {
      for ( size_t i = 0; i < this->GetLength( ); ++i )
      {
         isEqual = *this->operator[]( i ) == *nameList.operator[]( i ) ?
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
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_NameList_c< T >::operator!=( 
      const TCT_NameList_c& nameList ) const
{
   return( !this->operator==( nameList ) ? true : false );
}

//===========================================================================//
// Method         : operator[]
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_NameList_c< T >::operator[]( 
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
template< class T > T* TCT_NameList_c< T >::operator[]( 
      size_t index ) const
{
   T* pdata = const_cast< TCT_NameList_c< T >* >( this )->operator[]( index );

   return( pdata );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_NameList_c< T >::Print( 
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
// 05/01/12 jeffr : Original
//===========================================================================//
template<class T> void TCT_NameList_c< T >::ExtractString(
      string* psrData,
      size_t  maxLen,
      bool    quotedString ) const
{
   if ( psrData )
   {
      *psrData = "";

      for ( size_t i = 0; i < this->GetLength( ); ++i )
      {
         const T& data = *this->operator[]( i );
         const string& srDataString = data.GetName( );

	 if ( quotedString )
	 {
            *psrData += "\"";
	 }
         *psrData += srDataString;
	 if ( quotedString )
	 {
            *psrData += "\"";
	 }
         *psrData += ( i < this->GetLength( ) - 1 ? " " : "" );

         if ( psrData->length( ) >= maxLen )
	 {
	    *psrData += "...";
	    break;
	 }
      }
   }
}

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_NameList_c< T >::Add( 
      const char* pszName )
{
   T data( pszName );
   this->Add( data );
}

//===========================================================================//
template< class T > void TCT_NameList_c< T >::Add( 
      const string& srName )
{
   T data( srName );
   this->Add( data );
}

//===========================================================================//
template< class T > void TCT_NameList_c< T >::Add( 
      const T& data )
{
   this->vector_.push_back( data );
}

//===========================================================================//
template< class T > void TCT_NameList_c< T >::Add( 
      const TCT_NameList_c& nameList )
{
   for ( size_t i = 0; i < nameList.GetLength( ); ++i )
   {
      const T& data = *nameList.operator[]( i );
      if ( data.IsValid( ))  
      {
	 this->Add( data );
      }
   }
}

//===========================================================================//
// Method         : Delete
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_NameList_c< T >::Delete( 
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
template< class T > void TCT_NameList_c< T >::Delete( 
      size_t index )
{
   const T& data = *this->operator[]( index );
   this->Delete( data );
}

//===========================================================================//
// Method         : Find
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_NameList_c< T >::Find(
      const T& data,
            T* pdata ) const
{
   bool found = false;

   found = this->Search_( data, pdata );

   return( found );
}

//===========================================================================//
template< class T > bool TCT_NameList_c< T >::Find(
      const string& srName,
            T*      pdata ) const
{
   T data( srName );
   return( this->Find( data, pdata ));
}

//===========================================================================//
template< class T > bool TCT_NameList_c< T >::Find(
      const char* pszName,
            T*    pdata ) const
{
   T data( pszName );
   return( this->Find( data, pdata ));
}

//===========================================================================//
// Method         : FindIndex
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > size_t TCT_NameList_c< T >::FindIndex( 
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
template< class T > size_t TCT_NameList_c< T >::FindIndex( 
      const string& srName ) const
{
   T data( srName );
   return( this->FindIndex( data ));
}

//===========================================================================//
template< class T > size_t TCT_NameList_c< T >::FindIndex( 
      const char* pszName ) const
{
   T data( pszName );
   return( this->FindIndex( data ));
}

//===========================================================================//
// Method         : FindName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > const char* TCT_NameList_c< T >::FindName( 
      size_t index ) const
{
   return( this->operator[]( index ) ? 
           this->operator[]( index )->GetName( ) : 0 );
}

//===========================================================================//
// Method         : ApplyRegExp
// Purpose        : Match all regular expression patterns in this name list
//                  object with name elements from the given name list.  Any
//                  matching names elements are added to this name list.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_NameList_c< T >::ApplyRegExp( 
      const TCT_NameList_c& nameList,
            bool            isShowWarningEnabled,
            bool            isShowErrorEnabled,
      const char*           pszShowRegExpType )
{
   bool ok = true;

   // Make local copy of this name list to be pattern matched against 
   TCT_NameList_c< TC_Name_c > thisList( *this );
   this->Clear( );

   // Iterate for each name element in list, apply regular expression matching
   for ( size_t i = 0; i < thisList.GetLength( ); ++i )
   {
      const string& srRegExpName = thisList.FindName( i );

      TCT_RegExpIter_c< TCT_NameList_c< T > > regExpNameIter;
      ok = regExpNameIter.Init( srRegExpName, nameList );
      if ( !ok )
         break;

      if ( regExpNameIter.HasRegExp( ))
      {
         // Iterate over given list and add matching name elements
         size_t matchCount = 0;  
         while ( matchCount < SIZE_MAX )
	 {
            size_t matchIndex = regExpNameIter.Next( );
            if ( matchIndex == SIZE_MAX )
               break;
            
            // Found next matching name element in the test list
            ++matchCount;

            // Add next matching name element to this list
            const T& matchData = *nameList.operator[]( matchIndex );
            this->Add( matchData );
         }

         if ( matchCount == 0 )
	 {
            ok = this->ShowMessageInvalidRegExpName_( srRegExpName, 
                                                      isShowWarningEnabled,
                                                      isShowErrorEnabled,
                                                      pszShowRegExpType );
            if ( !ok )
              break;
         }
      }
      else 
      {
         if ( nameList.IsMember( srRegExpName ))
	 {
            this->Add( srRegExpName );
	 }
         else
	 {
            ok = this->ShowMessageMissingRegExpName_( srRegExpName, 
                                                      isShowWarningEnabled,
                                                      isShowErrorEnabled,
                                                      pszShowRegExpType );
            if ( !ok )
              break;
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : ApplyRegExp
// Purpose        : Match all regular expression patterns in the given name 
//                  list with name elements from this name list object.  Any 
//                  matching names elements are added to the given name list.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_NameList_c< T >::ApplyRegExp( 
            TCT_NameList_c* pnameList,
            bool            isShowWarningEnabled,
            bool            isShowErrorEnabled,
      const char*           pszShowRegExpType )
{
   bool ok = true;

   // Iterate for each name element in list, apply regular expression matching
   size_t i = 0;
   while ( i < pnameList->GetLength( ))
   {
      const string& srRegExpName = pnameList->FindName( i );

      TCT_RegExpIter_c< TCT_NameList_c< T > > regExpThisIter;
      ok = regExpThisIter.Init( srRegExpName, *this );
      if ( !ok )
         break;

      if ( this->IsValid( ) && 
          regExpThisIter.HasRegExp( ))
      {
         // Iterate over given list and add matching name elements
         size_t matchCount = 0;  
         while ( matchCount < SIZE_MAX )
	 {
            size_t matchIndex = regExpThisIter.Next( );
            if ( matchIndex == SIZE_MAX )
               break;
            
            // Found next matching name element in the test list
            ++matchCount;

            // Add next matching name element to the given name list
            const T& matchData = *this->operator[]( matchIndex );
            pnameList->Add( matchData );
         }

         if ( matchCount == 0 )
	 {
            ok = this->ShowMessageInvalidRegExpName_( srRegExpName, 
                                                      isShowWarningEnabled,
                                                      isShowErrorEnabled,
                                                      pszShowRegExpType );
            if ( !ok )
              break;
         }

         // Delete regular expression matched from the given list when done
         pnameList->Delete( i );
      }
      else 
      {
         if ( this->IsMember( srRegExpName ))
	 {
            ++i;
	 }
         else
	 {
            pnameList->Delete( i );

            ok = this->ShowMessageMissingRegExpName_( srRegExpName, 
                                                      isShowWarningEnabled,
                                                      isShowErrorEnabled,
                                                      pszShowRegExpType );
            if ( !ok )
              break;
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : MatchRegExp
// Purpose        : Match all regular expression patterns in the given name 
//                  list with name elements from this name list object.  
//                  Returns true if any pattern matches are found.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_NameList_c< T >::MatchRegExp( 
      const TCT_NameList_c& nameList ) const
{
   bool ok = false;

   // Iterate for each name element in list, apply regular expression matching
   for ( size_t i = 0; i < this->GetLength( ); ++i )
   {
      const string& srRegExpName = this->FindName( i );

      TCT_RegExpIter_c< TCT_NameList_c< T > > regExpNameIter;
      if ( !regExpNameIter.Init( srRegExpName, nameList ))
         break;

      if ( regExpNameIter.HasRegExp( ))
      {
         size_t matchCount = 0;  
         while ( matchCount < SIZE_MAX )
	 {
            size_t matchIndex = regExpNameIter.Next( );
            if ( matchIndex == SIZE_MAX )
               break;
            
            ++matchCount;
         }

         if ( matchCount > 0 )
	 {
            ok = true;
	    break;
	 }
      }
      else 
      {
         if ( nameList.IsMember( srRegExpName ))
	 {
            ok = true;
	    break;
	 }
      }
   }
   return( ok );
}

//===========================================================================//
template< class T > bool TCT_NameList_c< T >::MatchRegExp( 
      const char* pszName ) const
{
   string srName( pszName ? pszName : "" );

   return( this->MatchRegExp( srName ));
}

//===========================================================================//
template< class T > bool TCT_NameList_c< T >::MatchRegExp( 
      const string& srName ) const
{
   TCT_NameList_c nameList( 1 );
   TC_Name_c name( srName );
   nameList.Add( name );

   return( this->MatchRegExp( nameList ));
}

//===========================================================================//
// Method         : Search_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_NameList_c< T >::Search_(
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

//===========================================================================//
// Method         : ShowMessageInvalidRegExpName_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_NameList_c< T >::ShowMessageInvalidRegExpName_(
      const string& srRegExpName,
            bool    isShowWarningEnabled,
            bool    isShowErrorEnabled,
      const char*   pszShowRegExpType ) const
{
   TIO_PrintHandler_c& messageHandler = TIO_PrintHandler_c::GetInstance( );

   if ( isShowWarningEnabled )
   {
      messageHandler.Warning( "Invalid name '%s', no pattern match found in %s list.\n",
                              TIO_SR_STR( srRegExpName ),
                              TIO_PSZ_STR( pszShowRegExpType ));
   }
   else if ( isShowErrorEnabled )
   {
      messageHandler.Error( "Invalid name '%s', no pattern match found in %s list.\n",
                            TIO_SR_STR( srRegExpName ),
                            TIO_PSZ_STR( pszShowRegExpType ));
   }
   return( messageHandler.IsWithinMaxCount( ));
}

//===========================================================================//
// Method         : ShowMessageMissingRegExpName_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_NameList_c< T >::ShowMessageMissingRegExpName_(
      const string& srRegExpName,
            bool    isShowWarningEnabled,
            bool    isShowErrorEnabled,
      const char*   pszShowRegExpType ) const
{
   TIO_PrintHandler_c& messageHandler = TIO_PrintHandler_c::GetInstance( );

   if ( isShowWarningEnabled )
   {
      messageHandler.Warning( "Missing name '%s', no match found in %s list.\n",
                              TIO_SR_STR( srRegExpName ),
                              TIO_PSZ_STR( pszShowRegExpType ));
   }
   else if ( isShowErrorEnabled )
   {
      messageHandler.Error( "Missing name '%s', no match found in %s list.\n",
                            TIO_SR_STR( srRegExpName ),
                            TIO_PSZ_STR( pszShowRegExpType ));
   }
   return( messageHandler.IsWithinMaxCount( ));
}

#endif 
