//===========================================================================//
// Purpose : Template version for a TCT_DynamicVector dynamic vector class.
//           This vector class dynamically expands and shrinks the total 
//           memory usage requirements based on current vector length.
//
//           Inline methods include:
//           - SetCapacity
//           - ResetCapacity
//           - GetCapacity
//           - GetLength
//           - Set
//           - Add
//           - Delete
//           - Clear
//           - IsValid
//
//           Public methods include:
//           - TCT_DynamicVector_c, ~TCT_DynamicVector_c
//           - operator=
//           - operator==, operator!=
//           - operator[]
//           - Print
//
//           Protected methods include:
//           - Init_
//           - Set_
//           - Add_
//           - Delete_
//           - Clear_
//           - Expand_
//           - Shrink_
//
//===========================================================================//

#ifndef TCT_DYNAMIC_VECTOR_H
#define TCT_DYNAMIC_VECTOR_H

#include <stdio.h>

#include "TIO_PrintHandler.h"

#include "TCT_Generic.h"

#include "TC_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > class TCT_DynamicVector_c
{
public:

   TCT_DynamicVector_c( size_t allocLen = 1,
                        size_t reallocLen = 1 );
   TCT_DynamicVector_c( const TCT_DynamicVector_c& dynamicVector );
   ~TCT_DynamicVector_c( void );

   TCT_DynamicVector_c< T >& operator=( const TCT_DynamicVector_c& dynamicVector );

   bool operator==( const TCT_DynamicVector_c& dynamicVector ) const;
   bool operator!=( const TCT_DynamicVector_c& dynamicVector ) const;

   T* operator[]( size_t i );
   T* operator[]( size_t i ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetCapacity( size_t capacity );
   void SetCapacity( size_t allocLen,
                     size_t reallocLen );
   void ResetCapacity( size_t capacity );
   void ResetCapacity( size_t allocLen,
                       size_t reallocLen );
   size_t GetCapacity( void ) const;
   size_t GetLength( void ) const;

   bool Set( size_t i, const T& data );
   T* Add( const T& data );
   void Delete( size_t i );
   void Clear( void );

   bool IsValid( void ) const;

protected:

   bool Init_( size_t allocLen, size_t reallocLen );
   bool Init_( const TCT_DynamicVector_c& dynamicVector );

   bool Set_( size_t i, const T& data );
   T* Add_( const T& data );
   void Delete_( size_t i );
   void Clear_( void );

   bool Expand_( void );
   bool Shrink_( void );

protected:

   T*     padata_;      // Ptr to a dynamic vector of <T> data elements

   size_t curLen_;      // Current dynamic vector size 
                        // (ie. defines count of elements currently in use)
   size_t maxLen_;      // Max dynamic vector size (w/o memory reallocate)
                        // (ie. assume curLen_ <= maxLen_)
   size_t allocLen_;    // Initial allocate/deallocate size 
                        // (ie. used for dynamic memory expand/shrink)
   size_t reallocLen_;  // Reallocate allocate/deallocate size 
                        // (ie. used for dynamic memory expand/shrink)

   size_t addCount_;    // Track number of adds made to this dynamic vector
   size_t deleteCount_; // Track number of deletes made to this dynamic vector
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > inline void TCT_DynamicVector_c< T >::SetCapacity(
      size_t capacity )
{
   this->Clear_( );
   this->Init_( capacity, capacity );
}

//===========================================================================//
template< class T > inline void TCT_DynamicVector_c< T >::SetCapacity(
      size_t allocLen,
      size_t reallocLen )
{
   this->Clear_( );
   this->Init_( allocLen, reallocLen );
}

//===========================================================================//
template< class T > inline void TCT_DynamicVector_c< T >::ResetCapacity(
      size_t capacity )
{
   this->maxLen_ = capacity;
   this->Init_( capacity, capacity );
}

//===========================================================================//
template< class T > inline void TCT_DynamicVector_c< T >::ResetCapacity(
      size_t allocLen,
      size_t reallocLen )
{
   this->maxLen_ = TCT_Max( allocLen, reallocLen );
   this->Init_( allocLen, reallocLen );
}

//===========================================================================//
template< class T > inline size_t TCT_DynamicVector_c< T >::GetCapacity( 
      void ) const
{
   return( TCT_Max( this->allocLen_, this->reallocLen_ ));
}

//===========================================================================//
template< class T > inline size_t TCT_DynamicVector_c< T >::GetLength( 
      void ) const
{
   return( this->curLen_ );
}

//===========================================================================//
template< class T > inline bool TCT_DynamicVector_c< T >::Set( 
            size_t i,
      const T&     data )
{
   return( this->Set_( i, data ));
}

//===========================================================================//
template< class T > inline T* TCT_DynamicVector_c< T >::Add( 
      const T& data )
{
   return( this->Add_( data ));
}

//===========================================================================//
template< class T > inline void TCT_DynamicVector_c< T >::Delete( 
      size_t i )
{
   this->Delete_( i );
}

//===========================================================================//
template< class T > inline void TCT_DynamicVector_c< T >::Clear( 
      void )
{
   this->Clear_( );
}

//===========================================================================//
template< class T > inline bool TCT_DynamicVector_c< T >::IsValid( 
      void ) const
{
   return( this->GetLength( ) > 0 ? true : false );
}

//===========================================================================//
// Method         : TCT_DynamicVector_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > TCT_DynamicVector_c< T >::TCT_DynamicVector_c( 
      size_t allocLen,
      size_t reallocLen )
      :
      padata_( 0 ),
      curLen_( 0 ),
      maxLen_( 0 ),
      allocLen_( 0 ),
      reallocLen_( 0 ),
      addCount_( 0 ),
      deleteCount_( 0 )
{
   this->Init_( allocLen, reallocLen );
}

//===========================================================================//
template< class T > TCT_DynamicVector_c< T >::TCT_DynamicVector_c( 
      const TCT_DynamicVector_c& dynamicVector )
      :
      padata_( 0 ),
      curLen_( 0 ),
      maxLen_( 0 ),
      allocLen_( 0 ),
      reallocLen_( 0 ),
      addCount_( 0 ),
      deleteCount_( 0 )
{
   this->Init_( dynamicVector );
}

//===========================================================================//
// Method         : ~TCT_DynamicVector_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > TCT_DynamicVector_c< T >::~TCT_DynamicVector_c( 
      void )
{
   this->curLen_ = 0;
   this->Shrink_( );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > TCT_DynamicVector_c< T >& TCT_DynamicVector_c< T >::operator=( 
      const TCT_DynamicVector_c& dynamicVector )
{
   if ( &dynamicVector != this )
   {
      this->Init_( dynamicVector );
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
template< class T > bool TCT_DynamicVector_c< T >::operator==( 
      const TCT_DynamicVector_c& dynamicVector ) const
{
   bool isEqual = this->GetLength( ) == dynamicVector.GetLength( ) ? 
                  true : false;
   if ( isEqual )
   {
      for ( size_t i = 0; i < this->GetLength( ); ++i )
      {
         isEqual = *this->operator[]( i ) == *dynamicVector.operator[]( i ) ?
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
template< class T > bool TCT_DynamicVector_c< T >::operator!=( 
      const TCT_DynamicVector_c& dynamicVector ) const
{
   return( !this->operator==( dynamicVector ) ? true : false );
}

//===========================================================================//
// Method         : operator[]
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_DynamicVector_c< T >::operator[]( 
      size_t i )
{
   return( i < this->maxLen_ ? this->padata_ + i : 0 );
}

//===========================================================================//
template< class T > T* TCT_DynamicVector_c< T >::operator[]( 
      size_t i ) const
{
   return( i < this->maxLen_ ? this->padata_ + i : 0 );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_DynamicVector_c< T >::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "// %u/%u [%u/%u] (+%u/-%u)",  
                       this->curLen_, this->maxLen_, 
                       this->allocLen_, this->reallocLen_,
                       this->addCount_, this->deleteCount_ );

   for ( size_t i = 0; i < this->GetLength( ); ++i )
   {
      const T& data = *this->operator[]( i );
      data.Print( pfile, spaceLen );
   }
}

//===========================================================================//
// Method         : Init_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_DynamicVector_c< T >::Init_( 
      size_t allocLen,
      size_t reallocLen )
{
   bool ok = true;

   // Init alloc and realloc lengths per given values
   this->allocLen_ = allocLen;
   this->reallocLen_ = reallocLen;

   // Force alloc and realloc lengths to minimum acceptable values
   this->allocLen_ = TCT_Max( this->allocLen_, static_cast< size_t >( 1 ));
   this->reallocLen_ = TCT_Max( this->allocLen_, this->reallocLen_ );

   // And force alloc length to a multiple of realloc length
   // (for easier subsequent dynamic vector memory expand/shrink)
   this->allocLen_ = (( this->allocLen_ - 1 ) / this->reallocLen_ ) + 1;
   this->allocLen_ *= this->reallocLen_;

   return( ok );
}

//===========================================================================//
template< class T > bool TCT_DynamicVector_c< T >::Init_( 
      const TCT_DynamicVector_c& dynamicVector )
{
   bool ok = true;

   this->Clear_( );

   this->allocLen_ = dynamicVector.allocLen_;
   this->reallocLen_ = dynamicVector.reallocLen_;
   this->curLen_ = dynamicVector.curLen_;
   this->maxLen_ = dynamicVector.maxLen_;

   if ( this->maxLen_ > 0 )
   {
      this->padata_ = new TC_NOTHROW T[ this->maxLen_ ];

      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.IsValidNew( this->padata_,
                                    sizeof( T ) * this->maxLen_,
                                    "TCT_DynamicVector_c< T >::Init_" );
      if ( ok )
      {
         for ( size_t i = 0; i < this->maxLen_; ++i )
         {
 	    // Need to copy each element into the newly allocated list
	    // (using 'deep' copy, instead of a faster 'shallow' memcpy)
            *( this->padata_ + i ) = *( dynamicVector.padata_ + i );
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : Set_
// Purpose        : Sets a new data element in this dynamic vector.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_DynamicVector_c< T >::Set_( 
            size_t i,
      const T&     data )
{
   bool ok = true;

   while ( i >= this->curLen_ )
   {
      ok = this->Expand_( );
      if ( !ok )
         break;
   }

   if ( ok )
   {
      *( this->padata_ + i ) = data;
   }
   return( ok );
}

//===========================================================================//
// Method         : Add_
// Purpose        : Adds a new data element to this dynamic vector.  The 
//                  vector length is automatically expanded, if needed.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_DynamicVector_c< T >::Add_( 
      const T& data )
{
   T* pdata = 0;

   // Expand dynamic vector to accommodate new data element, if needed
   if ( this->Expand_( ))
   {
      // Add new data element to end of vector
      pdata = this->operator[]( this->curLen_ );
      *pdata = data;

      // Update current length to reflect new vector size
      ++this->curLen_;

      // Track total number of add operations made to this vector too
      ++this->addCount_; 
   }
   return( pdata );
}

//===========================================================================//
// Method         : Delete_
// Purpose        : Deletes an existing data element from this dynamic 
//                  vector.  The vector length is automatically shrunk, if 
//                  needed.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_DynamicVector_c< T >::Delete_( 
      size_t i )
{
   if ( i < this->curLen_ )
   {
      // Delete existing data element by shifting subsequent data elements
      while ( i < this->curLen_ - 1 )
      {
 	 // Need to copy each element into the newly allocated list
         // (using 'deep' copy, instead of a faster 'shallow' memcpy)
         *( this->padata_ + i ) = *( this->padata_ + i + 1 );

         ++i;
      }

      // Update current length to reflect new vector size
      --this->curLen_;

      // Track total number of delete operations made to this vector too
      ++this->deleteCount_; 

      // Shrink dynamic vector to account for deleted data element, if needed
      this->Shrink_( );
   }
}

//===========================================================================//
// Method         : Clear_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_DynamicVector_c< T >::Clear_( 
      void )
{
   this->curLen_ = 0;
   this->Shrink_( );

   this->padata_ = 0;

   this->addCount_ = 0;
   this->deleteCount_ = 0;
}

//===========================================================================//
// Method         : Expand_
// Purpose        : Expands this dynamic vector whenever the current length
//                  exceeds the maximum length.  The new vector length is
//                  adjusted based on the current alloc/realloc length.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_DynamicVector_c< T >::Expand_( 
      void )
{
   bool ok = true;

   // Test if current length requires expanding dynamic vector max length
   if ( this->curLen_ >= this->maxLen_ )
   {
      // Increase max length based on either alloc or realloc length
      size_t incLen = ( this->maxLen_ == 0 ? 
                        this->allocLen_ : this->reallocLen_ );
      this->maxLen_ += incLen;

      // Make local copy of current vector, then allocate and copy new vector
      T* padata = new TC_NOTHROW T[ this->maxLen_ ];

      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.IsValidNew( padata,
                                    sizeof( T ) * this->maxLen_,
                                    "TCT_DynamicVector_c< T >::Expand_" );
      if ( ok )
      {
         for ( size_t i = 0; i < this->curLen_; ++i )
         {
 	    // Need to copy each element into the newly allocated list
	    // (using 'deep' copy, instead of a faster 'shallow' memcpy)
            *( padata + i ) = *( this->padata_ + i );
         }
      }
      else
      { 
         this->curLen_ = this->maxLen_ = 0;
      }
      delete[] this->padata_;
      this->padata_ = padata;
   }
   return( ok );
}

//===========================================================================//
// Method         : Shrink_
// Purpose        : Shrink this dynamic vector length when the current length
//                  has been reduced to the maximum length less the realloc
//                  length.  The new vector length is adjusted based on the
//                  given realloc length.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_DynamicVector_c< T >::Shrink_( 
      void )
{
   bool ok = true;

   // Test if dynamic vector length can be constracted based on current length
   if (( this->maxLen_ > 0 ) &&
      ( this->curLen_ <= ( this->maxLen_ - this->reallocLen_ )))
   {
      // Ready to deallocate a portion of dynamic vector
      this->maxLen_ = ( this->curLen_ / this->reallocLen_ ) + 1;
      this->maxLen_ = ( this->curLen_ ? this->maxLen_ : 0 );
      this->maxLen_ *= this->reallocLen_;
      if ( this->maxLen_ != 0 )
      {
         T* padata = new TC_NOTHROW T[ this->maxLen_ ];
 
         TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
         ok = printHandler.IsValidNew( padata,
                                       sizeof( T ) * this->maxLen_,
                                       "TCT_DynamicVector_c< T >::Shrink_" );
         if ( ok )
         {
 	    // Need to copy each element into the newly allocated list
	    // (using 'deep' copy, instead of a faster 'shallow' memcpy)
            for ( size_t i = 0; i < this->maxLen_; ++i )
            {
               *( padata + i ) = *( this->padata_ + i );
            }
         }
         else
         {
            this->curLen_ = this->maxLen_ = 0;
         }
         delete[] this->padata_;
         this->padata_ = padata;
      }
      else
      {
         delete[] this->padata_;
         this->padata_ = 0;
      }
   }
   return( ok );
}

#endif
