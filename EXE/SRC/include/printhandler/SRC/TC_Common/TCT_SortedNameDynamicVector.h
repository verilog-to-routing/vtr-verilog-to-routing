//===========================================================================//
// Purpose : Template version for a TCT_SortedNameDynamicVector_c sorted name
//           dynamic vector class.  This sorted name vector class dynamically 
//           expands and shrinks total memory usage requirements based on the 
//           current vector length.
//
//           Inline methods include:
//           - TCT_SortedNameDynamicVector_c, ~TCT_SortedNameDynamicVector_c
//           - operator[]
//           - SetCapacity
//           - ResetCapacity
//           - GetCapacity
//           - GetLength
//           - Add
//           - Delete
//           - Clear
//           - IsValid
//
//           Public methods include:
//           - operator=
//           - operator==, operator!=
//           - Print
//           - Find
//           - FindIndex
//           - FindName
//           - Uniquify
//           - ApplyRegExp
//           - IsMember
//
//           Private methods include:
//           - Add_
//           - Delete_
//           - Clear_
//           - Find_
//           - FindIndex_
//           - Search_
//           - Sort_
//           - SortTagDuplicates_
//           - ShowMessageDuplicateNameList_
//           - ShowMessageInvalidRegExpName_
//           - ShowMessageMissingRegExpName_
//
//===========================================================================//

#ifndef TCT_SORTED_NAME_DYNAMIC_VECTOR_H
#define TCT_SORTED_NAME_DYNAMIC_VECTOR_H

#include <stdio.h>
#include <limits.h>

#include <string>
using namespace std;

#include "TIO_PrintHandler.h"

#include "TC_StringUtils.h"

#include "TCT_Generic.h"
#include "TCT_DynamicVector.h"
#include "TCT_BSearch.h"
#include "TCT_QSort.h"

#include "TC_Name.h"
#include "TCT_NameList.h"
#include "TCT_RegExpIter.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > class TCT_SortedNameDynamicVector_c
      :
      private TCT_DynamicVector_c< T >
{
public:

   TCT_SortedNameDynamicVector_c( size_t allocLen = 1,
                                  size_t reallocLen = 1 );
   TCT_SortedNameDynamicVector_c( const TCT_SortedNameDynamicVector_c< T >& sortedNameDynamicVector );
   ~TCT_SortedNameDynamicVector_c( void );

   TCT_SortedNameDynamicVector_c< T >& operator=( const TCT_SortedNameDynamicVector_c< T >& sortedNameDynamicVector );
   bool operator==( const TCT_SortedNameDynamicVector_c& sortedNameDynamicVector ) const;
   bool operator!=( const TCT_SortedNameDynamicVector_c& sortedNameDynamicVector ) const;

   T* operator[]( size_t i );
   T* operator[]( size_t i ) const;

   T* operator[]( const T& data );
   T* operator[]( const T& data ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetCapacity( size_t capacity );
   void SetCapacity( size_t allocLen,
                     size_t reallocLen );
   void ResetCapacity( size_t capacity );
   void ResetCapacity( size_t allocLen,
                       size_t reallocLen );
   size_t GetCapacity( void ) const;

   size_t GetLength( void ) const;

   T* Add( const T& data );
   T* Add( const string& srName );
   T* Add( const char* pszName );

   void Delete( const T& data );
   void Delete( const string& srName );
   void Delete( const char* pszName );

   void Clear( void );

   T* Find( const T& data ) const;
   T* Find( const string& srName ) const;
   T* Find( const char* pszName ) const;

   size_t FindIndex( const T& data ) const;
   size_t FindIndex( const string& srName ) const;
   size_t FindIndex( const char* pszName ) const;

   const char* FindName( size_t i ) const;

   bool Uniquify( bool isShowWarningEnabled = false,
                  bool isShowErrorEnabled = false,
                  const char* pszShowElemType = 0,
                  const char* pszShowElemName = 0,
                  const char* pszShowDataType = 0 );

   bool ApplyRegExp( const TCT_NameList_c< TC_Name_c >& nameList,
                     TCT_NameList_c< TC_Name_c >* pnameList,
                     bool isShowWarningEnabled = false,
                     bool isShowErrorEnabled = false,
                     const char* pszShowRegExpType = "vector" ) const;
   bool ApplyRegExp( TCT_NameList_c< TC_Name_c >* pnameList,
                     bool isShowWarningEnabled = false,
                     bool isShowErrorEnabled = false,
                     const char* pszShowRegExpType = "vector" ) const;

   bool IsMember( const T& data ) const;
   bool IsMember( const string& srName ) const;
   bool IsMember( const char* pszName ) const;

   bool IsValid( void ) const;

private:

   T* Add_( const T& data );

   void Delete_( const T& data );

   void Clear_( void );

   T* Find_( const T& data ) const;

   size_t FindIndex_( const T& data ) const;

   T* Search_( const T& data ) const;
   bool Sort_( bool uniquify = false,
               TCT_NameList_c< TC_Name_c >* pduplicateNameList = 0 );
   size_t SortTagDuplicates_( TCT_NameList_c< TC_Name_c >* pduplicateNameList = 0 );

   bool ShowMessageDuplicateNameList_( const TCT_NameList_c< TC_Name_c >& duplicateNameList,
                                       bool isShowWarningEnabled,
                                       bool isShowErrorEnabled,
                                       const char* pszShowElemType,
                                       const char* pszShowElemName,
                                       const char* pszShowDataType ) const;
   bool ShowMessageInvalidRegExpName_( const string& srRegExpName,
                                       bool isShowWarningEnabled,
                                       bool isShowErrorEnabled,
                                       const char* pszShowRegExpType ) const;
   bool ShowMessageMissingRegExpName_( const string& srRegExpName,
                                       bool isShowWarningEnabled,
                                       bool isShowErrorEnabled,
                                       const char* pszShowRegExpType ) const;
private:

   mutable bool isSorted_;   // Track if dynamic vector is currently sorted
   mutable T*   pdataMR_;    // Ptr to 'most-recent' search cached data
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > inline TCT_SortedNameDynamicVector_c< T >::TCT_SortedNameDynamicVector_c( 
      size_t allocLen,
      size_t reallocLen )
      :
      TCT_DynamicVector_c< T >( allocLen, reallocLen ),
      isSorted_( false ),
      pdataMR_( 0 )
{
}

//===========================================================================//
template< class T > inline TCT_SortedNameDynamicVector_c< T >::TCT_SortedNameDynamicVector_c( 
      const TCT_SortedNameDynamicVector_c< T >& sortedNameDynamicVector )
      :
      TCT_DynamicVector_c< T >( sortedNameDynamicVector ),
      isSorted_( false ),
      pdataMR_( 0 )
{
}

//===========================================================================//
template< class T > inline TCT_SortedNameDynamicVector_c< T >::~TCT_SortedNameDynamicVector_c( 
      void )
{
}

//===========================================================================//
template< class T > inline T* TCT_SortedNameDynamicVector_c< T >::operator[]( 
      size_t i )
{
   return( TCT_DynamicVector_c< T >::operator[]( i ));
}

//===========================================================================//
template< class T > inline T* TCT_SortedNameDynamicVector_c< T >::operator[]( 
      size_t i ) const
{
   return( TCT_DynamicVector_c< T >::operator[]( i ));
}

//===========================================================================//
template< class T > inline T* TCT_SortedNameDynamicVector_c< T >::operator[]( 
      const T& data )
{
   size_t i = this->FindIndex_( data );
   return( TCT_DynamicVector_c< T >::operator[]( i ));
}

//===========================================================================//
template< class T > inline T* TCT_SortedNameDynamicVector_c< T >::operator[]( 
      const T& data ) const
{
   size_t i = this->FindIndex_( data );
   return( TCT_DynamicVector_c< T >::operator[]( i ));
}

//===========================================================================//
template< class T > inline void TCT_SortedNameDynamicVector_c< T >::SetCapacity( 
      size_t capacity )
{
   TCT_DynamicVector_c< T >::SetCapacity( capacity );
}

//===========================================================================//
template< class T > inline void TCT_SortedNameDynamicVector_c< T >::SetCapacity( 
      size_t allocLen,
      size_t reallocLen )
{
   TCT_DynamicVector_c< T >::SetCapacity( allocLen, reallocLen );
}

//===========================================================================//
template< class T > inline void TCT_SortedNameDynamicVector_c< T >::ResetCapacity( 
      size_t capacity )
{
   TCT_DynamicVector_c< T >::ResetCapacity( capacity );
}

//===========================================================================//
template< class T > inline void TCT_SortedNameDynamicVector_c< T >::ResetCapacity( 
      size_t allocLen,
      size_t reallocLen )
{
   TCT_DynamicVector_c< T >::ResetCapacity( allocLen, reallocLen );
}

//===========================================================================//
template< class T > inline size_t TCT_SortedNameDynamicVector_c< T >::GetCapacity( 
      void ) const
{
   return( TCT_DynamicVector_c< T >::GetCapacity( ));
}

//===========================================================================//
template< class T > inline size_t TCT_SortedNameDynamicVector_c< T >::GetLength( 
      void ) const
{
   return( TCT_DynamicVector_c< T >::GetLength( ));
}

//===========================================================================//
template< class T > inline T* TCT_SortedNameDynamicVector_c< T >::Add( 
      const T& data )
{
   return( this->Add_( data ));
}

//===========================================================================//
template< class T > inline T* TCT_SortedNameDynamicVector_c< T >::Add( 
      const string& srName )
{
   T data( srName );
   return( this->Add_( data ));
}

//===========================================================================//
template< class T > inline T* TCT_SortedNameDynamicVector_c< T >::Add( 
      const char* pszName )
{
   T data( pszName );
   return( this->Add_( data ));
}

//===========================================================================//
template< class T > inline void TCT_SortedNameDynamicVector_c< T >::Delete( 
      const T& data )
{
   this->Delete_( data );
}

//===========================================================================//
template< class T > inline void TCT_SortedNameDynamicVector_c< T >::Delete( 
      const string& srName )
{
   T data( srName );
   this->Delete_( data );
}

//===========================================================================//
template< class T > inline void TCT_SortedNameDynamicVector_c< T >::Delete( 
      const char* pszName )
{
   T data( pszName );
   this->Delete_( data );
}

//===========================================================================//
template< class T > inline void TCT_SortedNameDynamicVector_c< T >::Clear( 
      void )
{
   this->Clear_( );
}

//===========================================================================//
template< class T > inline bool TCT_SortedNameDynamicVector_c< T >::IsValid( 
      void ) const
{
   return( this->GetLength( ) > 0 ? true : false );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > TCT_SortedNameDynamicVector_c< T >& TCT_SortedNameDynamicVector_c< T >::operator=( 
      const TCT_SortedNameDynamicVector_c< T >& sortedNameDynamicVector )
{
   if ( &sortedNameDynamicVector != this )
   {
      TCT_DynamicVector_c< T >::operator=( sortedNameDynamicVector );
      this->isSorted_ = sortedNameDynamicVector.isSorted_;
      this->pdataMR_ = sortedNameDynamicVector.pdataMR_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_SortedNameDynamicVector_c< T >::operator==( 
      const TCT_SortedNameDynamicVector_c& sortedNameDynamicVector ) const
{
   bool isEqual = this->GetLength( ) == sortedNameDynamicVector.GetLength( ) ? 
                  true : false;
   if ( isEqual )
   {
      for ( size_t i = 0; i < this->GetLength( ); ++i )
      {
         isEqual = *this->operator[]( i ) == *sortedNameDynamicVector.operator[]( i ) ?
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
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_SortedNameDynamicVector_c< T >::operator!=( 
      const TCT_SortedNameDynamicVector_c& sortedNameDynamicVector ) const
{
   return( !this->operator==( sortedNameDynamicVector ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_SortedNameDynamicVector_c< T >::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   for ( size_t i = 0; i < this->GetLength( ); ++i )
   {
      const T& data = *this->operator[]( i );
      data.Print( pfile, spaceLen );
   }
}

//===========================================================================//
// Method         : Find
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_SortedNameDynamicVector_c< T >::Find( 
      const T& data ) const
{
   return( this->Find_( data ));
}

//===========================================================================//
template< class T > T* TCT_SortedNameDynamicVector_c< T >::Find( 
      const string& srName ) const
{
   T data( srName );
   return( this->Find_( data ));
}

//===========================================================================//
template< class T > T* TCT_SortedNameDynamicVector_c< T >::Find( 
      const char* pszName ) const
{
   T data( pszName );
   return( this->Find_( data ));
}

//===========================================================================//
// Method         : FindIndex
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > size_t TCT_SortedNameDynamicVector_c< T >::FindIndex( 
      const T& data ) const
{
   return( this->FindIndex_( data ));
}

//===========================================================================//
template< class T > size_t TCT_SortedNameDynamicVector_c< T >::FindIndex( 
      const string& srName ) const
{
   T data( srName );
   return( this->FindIndex_( data ));
}

//===========================================================================//
template< class T > size_t TCT_SortedNameDynamicVector_c< T >::FindIndex( 
      const char* pszName ) const
{
   T data( pszName );
   return( this->FindIndex_( data ));
}

//===========================================================================//
// Method         : FindName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > const char* TCT_SortedNameDynamicVector_c< T >::FindName( 
      size_t i ) const
{
   return( this->operator[]( i ) ? this->operator[]( i )->GetName( ) : 0 );
}

//===========================================================================//
// Method         : Uniquify
// Purpose        : Uniquifies this list by finding and deleting any 
//                  duplicate entries (per 'name' field).  Optionally, 
//                  returns a list of deleted entries (per 'name' field)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_SortedNameDynamicVector_c< T >::Uniquify( 
            bool  isShowWarningEnabled,
            bool  isShowErrorEnabled,
      const char* pszShowElemType,
      const char* pszShowElemName,
      const char* pszShowDataType )
{
   bool ok = true;

   bool uniquify = ( isShowWarningEnabled || isShowErrorEnabled ? true : false );
   if ( uniquify )
   {
      TCT_NameList_c< TC_Name_c > duplicateNameList;
      this->isSorted_ = this->Sort_( uniquify, &duplicateNameList );

      if ( duplicateNameList.IsValid( ))
      {
         ok = this->ShowMessageDuplicateNameList_( duplicateNameList,
                                                   isShowWarningEnabled,
                                                   isShowErrorEnabled,
                                                   pszShowElemType,
                                                   pszShowElemName,
                                                   pszShowDataType );
      }
   }
   else
   {
      this->isSorted_ = this->Sort_( );
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
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_SortedNameDynamicVector_c< T >::ApplyRegExp( 
      const TCT_NameList_c< TC_Name_c >& nameList,
            TCT_NameList_c< TC_Name_c >* pnameList,
            bool  isShowWarningEnabled,
            bool  isShowErrorEnabled,
      const char* pszShowRegExpType ) const
{
   bool ok = true;

   // Iterate for each name element in list, apply regular expression matching
   for ( size_t i = 0; i < nameList.GetLength( ); ++i )
   {
      const string& srRegExpName = ( nameList.FindName( i ));

      TCT_RegExpIter_c< TCT_SortedNameDynamicVector_c< T > > thisRegExpIter;
      ok = thisRegExpIter.Init( srRegExpName, *this );
      if ( !ok )
         break;

      if ( this->IsValid( ) && 
          thisRegExpIter.HasRegExp( ))
      {
         // Iterate over given list and add matching name elements
         size_t matchCount = 0;  
         while ( matchCount < SIZE_MAX )
	 {
            size_t matchIndex = thisRegExpIter.Next( );
            if ( matchIndex == SIZE_MAX )
               break;
            
            // Found next matching name element in the test list
            ++matchCount;

            // Add next matching name element to the given name list
            const T& matchData = *this->operator[]( matchIndex );
            const char* pszMatchName = matchData.GetName( );
            pnameList->Add( pszMatchName );
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
         if ( this->IsMember( srRegExpName ))
	 {
            pnameList->Add( srRegExpName );
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
template< class T > bool TCT_SortedNameDynamicVector_c< T >::ApplyRegExp( 
            TCT_NameList_c< TC_Name_c >* pnameList,
            bool  isShowWarningEnabled,
            bool  isShowErrorEnabled,
      const char* pszShowRegExpType ) const
{
   // Make local copy of given name list to be pattern matched against 
   TCT_NameList_c< TC_Name_c > nameList( *pnameList );
   pnameList->Clear( );

   bool ok = this->ApplyRegExp( nameList, 
                                pnameList,
				isShowWarningEnabled,
				isShowErrorEnabled,
				pszShowRegExpType );
   return( ok );
}

//===========================================================================//
// Method         : IsMember
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_SortedNameDynamicVector_c< T >::IsMember( 
      const T& data ) const
{
   return( this->Find_( data ) ? true : false );
}

//===========================================================================//
template< class T > bool TCT_SortedNameDynamicVector_c< T >::IsMember( 
      const string& srName ) const
{
   T data( srName );
   return( this->Find_( data ) ? true : false );
}

//===========================================================================//
template< class T > bool TCT_SortedNameDynamicVector_c< T >::IsMember( 
      const char* pszName ) const
{
   T data( pszName );
   return( this->Find_( data ) ? true : false );
}

//===========================================================================//
// Method         : Add_
// Purpose        : Adds a new data element to this sorted name dynamic 
//                  vector.  The vector is automatically sorted, if needed
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_SortedNameDynamicVector_c< T >::Add_( 
      const T& data )
{
   // Add new data element to end of vector and expand vector, if needed
   // (assumes we will eventually sort vector prior to 1st vector access)
   T* pdata = TCT_DynamicVector_c< T >::Add( data );
   if ( pdata )
   {
      // Clear internal cached pointer to 'most-recent' search data
      this->pdataMR_ = 0;

      // Clear flag indicating dynamic vector may no longer be sorted
      this->isSorted_ = false;

      // Sort vector after 'n' add operations to delete redundant entries
      // (this helps to shrink overall vector size memory allocation)
      size_t addCount = TCT_DynamicVector_c< T >::addCount_;
      if (( addCount % ( 1000 * TCT_DynamicVector_c< T >::reallocLen_ )) == 0 )
      {
         this->isSorted_ = this->Sort_( );
         pdata = this->Find_( data );
      }
   }
   return( pdata );
}

//===========================================================================//
// Method         : Delete_
// Purpose        : Deletes an existing data element from this sorted name 
//                  dynamic vector.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_SortedNameDynamicVector_c< T >::Delete_( 
      const T& data )
{
   size_t i = this->FindIndex_( data );
   if ( i < SIZE_MAX )
   {
      TCT_DynamicVector_c< T >::Delete( i );

      // Clear internal cached pointer to 'most-recent' search data
      this->pdataMR_ = 0;
   }
}

//===========================================================================//
// Method         : Clear_
// Purpose        : Clears this sorted name dynamic vector.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_SortedNameDynamicVector_c< T >::Clear_( 
      void )
{
   TCT_DynamicVector_c< T >::Clear( );

   // Clear internal cached pointer to 'most-recent' search data
   this->pdataMR_ = 0;
}

//===========================================================================//
// Method         : Find_
// Purpose        : Returns a pointer to a dynamic vector data element based
//                  on the given data element.  The sorted vector is binary
//                  searched based on the given data element's name string.
//                  The vector is first automatically sorted, if needed.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_SortedNameDynamicVector_c< T >::Find_(
      const T& data ) const
{
   // Test if dynamic vector has been modified since last time it was sorted
   if ( !this->isSorted_ )
   {
      // Sort after casting away const'ness (in order to keep 'Find_' const)
      // (this cast may be removed if compiler supports "mutable" keyword)
      TCT_SortedNameDynamicVector_c* psortedNameDynamicVector = 0;
      psortedNameDynamicVector = const_cast< TCT_SortedNameDynamicVector_c* >( this );
      psortedNameDynamicVector->isSorted_ = psortedNameDynamicVector->Sort_( );

      // Clear internal cached pointer to 'most-recent' search data
      psortedNameDynamicVector->pdataMR_ = 0;
   }

   // Given a sorted vector, do a binary search based on given data
   return( this->Search_( data ));
}

//===========================================================================//
// Method         : FindIndex_
// Purpose        : Returns a sorted name dynamic vector index based on the 
//                  given data element.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > size_t TCT_SortedNameDynamicVector_c< T >::FindIndex_(
      const T& data ) const
{
   size_t i = SIZE_MAX;

   // Search for dynamic vector data element using given data element
   const T* pdata = this->Find_( data );
   if ( pdata )
   {
      // Use pointer math to compute actual index into dynamic vector
      const T* padata = TCT_DynamicVector_c< T >::padata_;
      size_t dataAddress = reinterpret_cast< size_t >( pdata );
      size_t padataAddress = reinterpret_cast< size_t >( padata );
      i = ( dataAddress - padataAddress ) / 
          TCT_Max( sizeof( T ), static_cast< size_t >( 1 ));
   }
   return( i );
}

//===========================================================================//
// Method         : Search_
// Purpose        : Returns a pointer to a dynamic vector data element based
//                  on the given data element.  The sorted vector is binary
//                  searched based on the given data element's name string.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_SortedNameDynamicVector_c< T >::Search_(
      const T& data ) const
{
   T* pdata = 0;

   // Given a sorted vector, do a binary search based on given data
   if ( TCT_DynamicVector_c< T >::curLen_ > 0 )
   {
      if ( this->pdataMR_ )
      {
	 // Attempt to use pointer to cached 'most-recent search data
	 const T& dataMR = *this->pdataMR_;
         int i = TCT_BSearchCompare( &dataMR, &data, 
                                     TC_CompareStrings );
         pdata = ( i == 0 ? this->pdataMR_ : 0 );
      }

      if ( !pdata )
      {
	 // Hi-Ho, Hi-Ho... its off to (binary-search) work we go...
         pdata = TCT_BSearch( data, 
                              TCT_DynamicVector_c< T >::padata_, 
                              TCT_DynamicVector_c< T >::curLen_,
                              TC_CompareStrings );

	 // Don't forget to update pointer to cached 'most-recent search data
         // (this cast may be removed if compiler supports "mutable" keyword)
         TCT_SortedNameDynamicVector_c* psortedNameDynamicVector = 0;
         psortedNameDynamicVector = const_cast< TCT_SortedNameDynamicVector_c* >( this );
         psortedNameDynamicVector->pdataMR_ = pdata;
      }
   }
   return( pdata );
}

//===========================================================================//
// Method         : Sort_
// Purpose        : Sorts this dynamic vector based on data element name 
//                  strings.  Any duplicate name entries are identified,
//                  tagged, and deleted from this sorted vector.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_SortedNameDynamicVector_c< T >::Sort_(
      bool uniquify,
      TCT_NameList_c< TC_Name_c >* pduplicateNameList )
{
   bool ok = true;

   // Assume dynamic vector has been modified since last sort, re-sort now
   if ( TCT_DynamicVector_c< T >::curLen_ > 0 )
   {
      TCT_QSort( TCT_DynamicVector_c< T >::padata_, 
                 TCT_DynamicVector_c< T >::curLen_,
                 TC_CompareStrings );

      if ( uniquify )
      {
         // Dynamic vector may have duplicate data elements (per 'name' field),
         // scan vector and tag duplicate data elements for subsequent deletion
         size_t tagCount = this->SortTagDuplicates_( pduplicateNameList );

         // Re-sort dynamic vector again, if necessary 
         if ( tagCount > 0 )
         {
            // Sort again to move any tagged duplicate data to end of vector
            TCT_QSort( TCT_DynamicVector_c< T >::padata_, 
                       TCT_DynamicVector_c< T >::curLen_,
                       TC_CompareStrings );

            // Shrink dynamic vector length based on duplicate data elements
            TCT_DynamicVector_c< T >::curLen_ -= tagCount;
            ok = TCT_DynamicVector_c< T >::Shrink_( );
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : SortTagDuplicates_
// Purpose        : Tags duplicate name entries in this dynamic array, 
//                  replacing the names of any redundant data elements with
//                  a special '~' character string.  Tagging duplicate
//                  names with a '~' character string lets us later sort
//                  and remove the redundant name entries since the
//                  redundant names are sorted into positions following all
//                  valid alpha-numeric names.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > size_t TCT_SortedNameDynamicVector_c< T >::SortTagDuplicates_( 
      TCT_NameList_c< TC_Name_c >* pduplicateNameList )
{
   if ( pduplicateNameList )
   {
      pduplicateNameList->Clear( );
      pduplicateNameList->SetCapacity( this->GetLength( ));
   }

   size_t tagCount = 0;

   size_t i, j;
   for ( i = 0; i < TCT_DynamicVector_c< T >::curLen_; ++i )
   {
      const char* pszName_i = this->operator[]( i )->GetName( );

      bool foundDuplicate = false;
      for ( j = i + 1; j < TCT_DynamicVector_c< T >::curLen_; ++j )
      {
         const char* pszName_j = this->operator[]( j )->GetName( );
         if ( pszName_i && pszName_j &&
             strcmp( pszName_i, pszName_j ) == 0 )
         {
	    foundDuplicate = true;

            // Tag this duplicate name w/special prefix, then update count
            this->operator[]( j )->SetName( "~" );
            ++tagCount;
         }
         else
         {
            break;
         }
      }
      if ( foundDuplicate && pduplicateNameList )
      {
         pduplicateNameList->Add( pszName_i );
      } 
      i = j - 1;
   }
   return( tagCount );
}

//===========================================================================//
// Method         : ShowMessageDuplicateNameList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_SortedNameDynamicVector_c< T >::ShowMessageDuplicateNameList_(
      const TCT_NameList_c< TC_Name_c >& duplicateNameList,
            bool  isShowWarningEnabled,
            bool  isShowErrorEnabled,
      const char* pszShowElemType,
      const char* pszShowElemName,
      const char* pszShowDataType ) const
{
   TIO_PrintHandler_c& messageHandler = TIO_PrintHandler_c::GetInstance( );

   string srShowDataMessage = "";
   if ( pszShowDataType )
   {
      srShowDataMessage += pszShowDataType;
      srShowDataMessage += " ";
   }

   string srShowElemMessage = "";
   if ( pszShowElemType )
   {
      srShowElemMessage += " from ";
      srShowElemMessage += pszShowElemType;
   }
   if ( pszShowElemName )
   {
      srShowElemMessage += " ";
      srShowElemMessage += "\"";
      srShowElemMessage += pszShowElemName;
      srShowElemMessage += "\"";
   }

   if ( isShowWarningEnabled )
   {
      messageHandler.Warning( "Found and discarded duplicate %sname%s%s\n",
                              TIO_SR_STR( srShowDataMessage ),
                              duplicateNameList.GetLength( ) > 1 ? "s" : "",
                              TIO_SR_STR( srShowElemMessage ));

      for ( size_t i = 0 ; i < duplicateNameList.GetLength( ); ++i )
      {
         messageHandler.Write( "%sDuplicate name: %s\n",
                               TIO_PREFIX_WARNING_SPACE,
                               TIO_PSZ_STR( duplicateNameList[ i ]->GetName( )));
      }
   }
   else if ( isShowErrorEnabled )
   {
      messageHandler.Error( "Found and discarded duplicate %sname%s%s\n",
                              TIO_SR_STR( srShowDataMessage ),
                              duplicateNameList.GetLength( ) > 1 ? "s" : "",
                              TIO_SR_STR( srShowElemMessage ));

      for ( size_t i = 0 ; i < duplicateNameList.GetLength( ); ++i )
      {
         messageHandler.Write( "%sDuplicate name: %s\n",
                               TIO_PREFIX_ERROR_SPACE,
                               TIO_PSZ_STR( duplicateNameList[ i ]->GetName( )));
      }
   }
   return( messageHandler.IsWithinMaxCount( ));
}

//===========================================================================//
// Method         : ShowMessageInvalidRegExpName_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_SortedNameDynamicVector_c< T >::ShowMessageInvalidRegExpName_(
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
template< class T > bool TCT_SortedNameDynamicVector_c< T >::ShowMessageMissingRegExpName_(
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
