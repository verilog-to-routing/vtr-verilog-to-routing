//===========================================================================//
// Purpose : Template version for a TCT_Stack stack (FILO) class.
//
//           Inline methods include:
//           - GetLength
//           - Clear
//           - IsValid
//
//           Public methods include:
//           - TCT_Stack_c, ~TCT_Stack_c
//           - operator=
//           - operator==, operator!=
//           - operator[]
//           - Print
//           - Push, Pop, Peek
//
//===========================================================================//

#ifndef TCT_STACK_H
#define TCT_STACK_H

#include <stdio.h>

#include <deque>

//===========================================================================//
// Class declaration
//---------------------------------------------------------------------------//
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > class TCT_Stack_c 
{
public:

   TCT_Stack_c( void );
   TCT_Stack_c( const TCT_Stack_c< T >& stack );
   ~TCT_Stack_c( void );

   TCT_Stack_c& operator=( const TCT_Stack_c& stack );
   bool operator==( const TCT_Stack_c< T >& stack ) const;
   bool operator!=( const TCT_Stack_c< T >& stack ) const;

   T* operator[]( size_t index );
   T* operator[]( size_t index ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   size_t GetLength( void ) const;

   void Push( const T& data );
   bool Pop( T* pdata );
   T* Peek( void );
   T* Peek( size_t count );

   void Clear( void );

   bool IsValid( void ) const;

private:
   
   std::deque< T > stack_;
};

//===========================================================================//
// Class inline definition(s)
//---------------------------------------------------------------------------//
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > inline size_t TCT_Stack_c< T >::GetLength( 
      void ) const
{
   return( this->stack_.size( ));
}

//===========================================================================//
template< class T > inline void TCT_Stack_c< T >::Clear( 
      void )
{
   this->stack_.clear( );
}

//===========================================================================//
template< class T > inline bool TCT_Stack_c< T >::IsValid(
      void ) const
{
   return( this->GetLength( ) > 0 ? true : false );
}

//===========================================================================//
// Method         : TCT_Stack_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TCT_Stack_c< T >::TCT_Stack_c( 
      void )
{
}

//===========================================================================//
template< class T > TCT_Stack_c< T >::TCT_Stack_c(
      const TCT_Stack_c< T >& stack )
      :
      stack_( stack.stack_ )
{
}

//===========================================================================//
// Method         : ~TCT_Stack_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TCT_Stack_c< T >::~TCT_Stack_c( 
      void )
{
   this->stack_.clear( );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > inline TCT_Stack_c< T >& TCT_Stack_c< T >::operator=( 
      const TCT_Stack_c& stack )
{
   this->stack_.operator=( stack.stack_ );

   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_Stack_c< T >::operator==(
      const TCT_Stack_c< T >& stack ) const
{
   bool isEqual = false;

   if (( this->GetLength( ) > 0 ) &&
      ( stack.GetLength( ) > 0 ) &&
      ( this->GetLength( ) == stack.GetLength( )))
   {
      for ( size_t i = 0; i < this->GetLength( ); ++i )
      {
         const T& thisData = *this->operator[]( i );
         const T& stackData = *stack.operator[]( i );
         if ( thisData == stackData )
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
   return( isEqual );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_Stack_c< T >::operator!=(
      const TCT_Stack_c< T >& stack ) const
{
   return( !this->operator==( stack ) ? true : false );
}

//===========================================================================//
// Method         : operator[]
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_Stack_c< T >::operator[]( 
      size_t index )
{
   T* pdata = 0;
   if ( index < this->GetLength( ))
   {
      pdata = &this->stack_.operator[]( index );
   }
   return( pdata );
}

//===========================================================================//
template< class T > T* TCT_Stack_c< T >::operator[]( 
      size_t index ) const
{
   T* pdata = 0;
   if ( index < this->GetLength( ))
   {
      pdata = const_cast< TCT_Stack_c< T >* >( this )->operator[]( index );
   }
   return( pdata );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_Stack_c< T >::Print( 
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
// Method         : Push
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_Stack_c< T >::Push( 
      const T& data )
{
   this->stack_.push_back( data );
}

//===========================================================================//
// Method         : Pop
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_Stack_c< T >::Pop( 
      T* pdata )
{
   size_t len = this->GetLength( );

   if ( pdata && len )
   {
      *pdata = this->stack_.back( );
      this->stack_.pop_back( );
   }
   return( len ? true : false );
}

//===========================================================================//
// Method         : Peek
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_Stack_c< T >::Peek( 
      void )
{
   T* pdata = 0;

   size_t len = this->GetLength( );
   if ( len )
   {
      *pdata = this->stack_.back( );
   }
   return( pdata );
}

//===========================================================================//
template< class T > T* TCT_Stack_c< T >::Peek( 
      size_t count )
{
   T* pdata = 0;

   size_t len = this->GetLength( );
   if ( len && count < len )
   {
      pdata = this->operator[]( len - count - 1 );
   }
   return( pdata );
}

#endif
