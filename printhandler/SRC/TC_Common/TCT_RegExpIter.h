//===========================================================================//
// Purpose : Template version for a regular expression list iterator class.  
//           This class handles applying a name string, possibly defined 
//           using regular expression constructs, to a match list template 
//           object, returning 0 or more indices to list elements with 
//           matching name strings.
//
//           Inline methods include:
//           - TCT_RegExpIter, ~TCT_RegExpIter
//           - HasRegExp
//           - IsValid
//
//           Public methods include:
//           - Init
//           - Next
//
//===========================================================================//

#ifndef TCT_REGEXP_ITER_H
#define TCT_REGEXP_ITER_H

#include <stdio.h>
#include <limits.h>

#include <string>
using namespace std;

#include "RegExp.h"

#include "TIO_PrintHandler.h"

#include "TC_Typedefs.h"

// Define a default invalid index value
#define TCT_REGEXP_INDEX_INVALID SIZE_MAX

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > class TCT_RegExpIter_c
{
public:

   TCT_RegExpIter_c( void );
   TCT_RegExpIter_c( const char* pszRegExp, 
                     const T& matchList );
   TCT_RegExpIter_c( const string& srRegExp, 
                     const T& matchList );
   ~TCT_RegExpIter_c( void );

   bool Init( const char* pszRegExp, 
              const T& matchList );
   bool Init( const string& srRegExp, 
              const T& matchList );

   size_t Next( void );

   bool HasRegExp( void ) const;
   bool IsValid( void ) const;

private:

   RegExp* pregExp_;             // Ptr to object for RE pattern matching
   string* psrRegExp_;           // Ptr to simple RE pattern string

   T* pmatchList_;               // Ptr to a match list for pattern matching

   size_t matchIndex_;           // Current index to match list for matching
   size_t nextIndex_;            // Next index to match list for matching

   bool isValid_;                // true => object has been initialized;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > inline TCT_RegExpIter_c< T >::TCT_RegExpIter_c( 
      void ) 
      :
      pregExp_( 0 ),
      psrRegExp_( 0 ),
      pmatchList_( 0 ),
      matchIndex_( TCT_REGEXP_INDEX_INVALID ),
      nextIndex_( 0 ),
      isValid_( false )
{
}

//===========================================================================//
template< class T > inline TCT_RegExpIter_c< T >::TCT_RegExpIter_c( 
      const char* pszRegExp, 
      const T&    matchList )
      :
      pregExp_( 0 ),
      psrRegExp_( 0 ),
      pmatchList_( 0 ),
      matchIndex_( TCT_REGEXP_INDEX_INVALID ),
      nextIndex_( 0 ),
      isValid_( false )
{
   this->Init( pszRegExp, matchList );
}

//===========================================================================//
template< class T > inline TCT_RegExpIter_c< T >::TCT_RegExpIter_c( 
      const string& srRegExp, 
      const T&      matchList )
      :
      pregExp_( 0 ),
      psrRegExp_( 0 ),
      pmatchList_( 0 ),
      matchIndex_( TCT_REGEXP_INDEX_INVALID ),
      nextIndex_( 0 ),
      isValid_( false )
{
   this->Init( srRegExp, matchList );
}

//===========================================================================//
template< class T > inline TCT_RegExpIter_c< T >::~TCT_RegExpIter_c( 
      void )
{
   delete this->pregExp_;
   delete this->psrRegExp_;
}

//===========================================================================//
template< class T > inline bool TCT_RegExpIter_c< T >::HasRegExp( 
      void ) const
{
   return( this->pregExp_ ? true : false );
}

//===========================================================================//
template< class T > inline bool TCT_RegExpIter_c< T >::IsValid( 
      void ) const
{
   return( this->isValid_ ); 
}

//===========================================================================//
// Method         : Init
// Purpose        : Initialize this object for regular expression pattern 
//                  matching based on the given regular expression string
//                  and given match list used for pattern matching.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_RegExpIter_c< T >::Init( 
      const char* pszRegExp, 
      const T&    matchList )
{
   string srRegExp( pszRegExp ? pszRegExp : "" );

   return( this->Init( srRegExp, matchList ));
}

//===========================================================================//
template< class T > bool TCT_RegExpIter_c< T >::Init( 
      const string& srRegExp, 
      const T&      matchList )
{
   bool ok = true;

   // Reset object state (object may have been used by a previous pattern)
   delete this->pregExp_;
   this->pregExp_ = 0; 

   delete this->psrRegExp_;
   this->psrRegExp_ = 0; 

   this->pmatchList_ = const_cast< T* >( &matchList );
   this->matchIndex_ = TCT_REGEXP_INDEX_INVALID;
   this->nextIndex_ = 0;

   // Make regular expression object or simple string, whichever is needed   
   string srRegExp_( srRegExp );
   const char* pszSpecialChars = "^.?[]+*$";
   if ( srRegExp_.find_first_of( pszSpecialChars ) != string::npos )
   {
      size_t escape = srRegExp_.find( '\\' );
      while (( escape != string::npos ) &&
            ( escape < srRegExp_.length( ) - 1 ))
      {
         srRegExp_.replace( escape, 2, "" );   
         escape = srRegExp_.find( '\\' );
      }
   }

   if ( srRegExp_.find_first_of( pszSpecialChars ) != string::npos )
   {
      // Regular expression string may require 'pattern-matching'
      this->pregExp_ = new TC_NOTHROW RegExp( srRegExp.data( ));

      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.IsValidNew( this->pregExp_,
                                    sizeof( RegExp ),
                                   "TCT_RegExpIter_c< T >::Init" );
      if ( ok )
      {
         if ( !this->pregExp_->IsValidRE( ) )
         {
            printHandler.Error( "Invalid regular expression '%s', pattern is illegal!\n",
                                TIO_SR_STR( srRegExp ));
            ok = printHandler.IsWithinMaxErrorCount( );

            delete this->pregExp_;
            this->pregExp_ = 0;
         }
      }
   }
   else
   {
      // Simple string does not have to be 'pattern-matched'
      this->psrRegExp_ = new TC_NOTHROW string( srRegExp );

      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.IsValidNew( this->psrRegExp_,
                                    sizeof( string ),
                                    "TCT_RegExpIter_c< T >::Init" );
   }
   this->isValid_ = ok;

   return( ok );
}

//===========================================================================//
// Method         : Next
// Purpose        : Apply the current regular expression to the current 
//                  match list, returning an index to the next element in
//                  the current match list with a matching string
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
template< class T > size_t TCT_RegExpIter_c< T >::Next( 
      void )
{
   if ( this->pregExp_ )
   {
      // Using 'complex' pattern matching (ie. with special characters)
      this->matchIndex_ = TCT_REGEXP_INDEX_INVALID;
      while (( this->matchIndex_ == TCT_REGEXP_INDEX_INVALID ) &&
            ( this->nextIndex_ <= this->pmatchList_->GetLength( ) - 1 ))
      {
         // Get next string, then apply the current regular expression 
         size_t nextIndex = this->nextIndex_;
         const char* pszNextString = this->pmatchList_->FindName( nextIndex );
         string srNextString( pszNextString ? pszNextString : "" );

         size_t matchStart  = 0;
         size_t matchLength = 0;

         bool match = this->pregExp_->Index( srNextString.data( ),
                                             &matchStart,
                                             &matchLength );

         if ( match && ( srNextString.length( ) == matchLength ))
         {
            this->matchIndex_ = this->nextIndex_;
         }
         ++this->nextIndex_;
      }

      if ( this->matchIndex_ == TCT_REGEXP_INDEX_INVALID )
      {
         delete this->psrRegExp_;
         this->psrRegExp_ = 0;
      }
   }
   else if ( this->psrRegExp_ )
   {
      // Using 'simple' pattern matching (ie. no special characters)
      const string& srRegExp = *this->psrRegExp_;
      this->matchIndex_ = this->pmatchList_->FindIndex( srRegExp );

      delete this->psrRegExp_;
      this->psrRegExp_ = 0;
   }
   else
   {
      this->matchIndex_ = TCT_REGEXP_INDEX_INVALID;
   }
   return( this->matchIndex_ );
}

#endif 
