//===========================================================================//
// Purpose : Template version for a TCT_OrderedVector2D ordered vector class.
//
//           Inline methods include:
//           - GetWidth
//           - GetHeight
//           - IsValid
//
//           Public methods include:
//           - TCT_OrderedVector2D_c, ~TCT_OrderedVector2D_c
//           - operator=
//           - operator==, operator!=
//           - operator[], operator()
//           - Print
//           - ExtractString
//           - SetCapacity
//           - Clear
//
//===========================================================================//

#ifndef TCT_ORDERED_VECTOR_2D_H
#define TCT_ORDERED_VECTOR_2D_H

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
template< class T > class TCT_OrderedVector2D_c
{
public:

   TCT_OrderedVector2D_c( size_t width = TC_DEFAULT_CAPACITY,
                          size_t height = TC_DEFAULT_CAPACITY,
                          const T* pdata = 0 );
   TCT_OrderedVector2D_c( size_t width,
                          size_t height,
                          const T& data );
   TCT_OrderedVector2D_c( const TCT_OrderedVector2D_c& orderedVector2D );
   ~TCT_OrderedVector2D_c( void );

   TCT_OrderedVector2D_c& operator=( const TCT_OrderedVector2D_c& orderedVector2D );

   bool operator==( const TCT_OrderedVector2D_c& orderedVector2D ) const;
   bool operator!=( const TCT_OrderedVector2D_c& orderedVector2D ) const;

   T* operator[]( size_t index );
   T* operator[]( size_t index ) const;

   T* operator()( size_t i, size_t j );
   T* operator()( size_t i, size_t j ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( TC_DataMode_t mode,
                       string* psrData,
                       size_t precision = SIZE_MAX,
                       size_t maxLen = INT_MAX,
                       size_t spaceLen = 0,
                       size_t firstLen = SIZE_MAX ) const;

   void SetCapacity( size_t width,
                     size_t height,
                     const T* pdata = 0 );
   void SetCapacity( size_t width,
                     size_t height,
                     const T& data );
   size_t GetWidth( void ) const;
   size_t GetHeight( void ) const;

   T* At( size_t i, size_t j );
   T* At( size_t i, size_t j ) const;

   void Clear( void );
  
   bool IsValid( void ) const;

private:

   size_t width_;
   size_t height_;   
   std::vector< std::vector< T > > vector2D_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > inline size_t TCT_OrderedVector2D_c< T >::GetWidth(
      void ) const
{
   return( this->width_ );
}

//===========================================================================//
template< class T > inline size_t TCT_OrderedVector2D_c< T >::GetHeight( 
      void ) const
{
   return( this->height_ );
}

//===========================================================================//
template< class T > inline bool TCT_OrderedVector2D_c< T >::IsValid( 
      void ) const
{
   return(( this->GetWidth( ) > 0 ) && ( this->GetHeight( ) > 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : TCT_OrderedVector2D_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_OrderedVector2D_c< T >::TCT_OrderedVector2D_c( 
      size_t   width,
      size_t   height,
      const T* pdata )
      :
      width_( width ),
      height_( height )
{
   this->SetCapacity( width, height, pdata );
}

//===========================================================================//
template< class T > TCT_OrderedVector2D_c< T >::TCT_OrderedVector2D_c( 
      size_t   width,
      size_t   height,
      const T& data )
      :
      width_( width ),
      height_( height )
{
   this->SetCapacity( width, height, data );
}

//===========================================================================//
template< class T > TCT_OrderedVector2D_c< T >::TCT_OrderedVector2D_c( 
      const TCT_OrderedVector2D_c& orderedVector2D )
      :
      width_( orderedVector2D.width_ ),
      height_( orderedVector2D.height_ ),
      vector2D_( orderedVector2D.vector2D_ )
{
}

//===========================================================================//
// Method         : ~TCT_OrderedVector2D_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_OrderedVector2D_c< T >::~TCT_OrderedVector2D_c( 
      void )
{
   this->Clear( );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > TCT_OrderedVector2D_c< T >& TCT_OrderedVector2D_c< T >::operator=( 
      const TCT_OrderedVector2D_c& orderedVector2D )
{
   this->width_ = orderedVector2D.width_;
   this->height_ = orderedVector2D.height_;

   for ( size_t i = 0; i < this->width_; ++i )
   {
      this->vector2D_[i].operator=( orderedVector2D.vector2D_[i] );
   }
   this->vector2D_.operator=( orderedVector2D.vector2D_ );

   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > bool TCT_OrderedVector2D_c< T >::operator==( 
      const TCT_OrderedVector2D_c& orderedVector2D ) const
{
   bool isEqual = ( this->width_ == orderedVector2D.width_ ) &&
                  ( this->height_ == orderedVector2D.height_ ) ?
                  true : false;
   if ( isEqual )
   {
      for ( size_t j = 0; j < this->height_; ++j )
      {
         for ( size_t i = 0; i < this->width_; ++i )
         {
	    isEqual = ( this->vector2D_[i][j] == orderedVector2D[i][j] ) ?
                      true : false;
            if ( !isEqual )
               break;
         }
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
template< class T > bool TCT_OrderedVector2D_c< T >::operator!=( 
      const TCT_OrderedVector2D_c& orderedVector2D ) const
{
   return( !this->operator==( orderedVector2D ) ? true : false );
}

//===========================================================================//
// Method         : operator[]
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_OrderedVector2D_c< T >::operator[]( 
      size_t index )
{
   T* pdata = 0;

   if ( index < TCT_Max( this->width_, this->height_ ))
   {
      pdata = &this->vector2D_.operator[]( index ).operator[]( 0 );
   }
   return( pdata );
}

//===========================================================================//
template< class T > T* TCT_OrderedVector2D_c< T >::operator[]( 
      size_t index ) const
{
   T* pdata = const_cast< TCT_OrderedVector2D_c< T >* >( this )->operator[]( index );

   return( pdata );
}

//===========================================================================//
// Method         : operator()
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_OrderedVector2D_c< T >::operator()( 
      size_t i,
      size_t j )
{
   return( this->At( i, j ));
}

//===========================================================================//
template< class T > T* TCT_OrderedVector2D_c< T >::operator()( 
      size_t i,
      size_t j ) const
{
   return( this->At( i, j ));
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedVector2D_c< T >::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   for ( size_t j = 0; j < this->height_; ++j )
   {
      for ( size_t i = 0; i < this->width_; ++i )
      {
         const T& data = *this->At( i, j );
         if ( data.IsValid( ))  
         {
            data.Print( pfile, spaceLen );
         }
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
template< class T > void TCT_OrderedVector2D_c< T >::ExtractString(
      TC_DataMode_t mode,
      string*       psrData,
      size_t        precision,
      size_t        maxLen,
      size_t        spaceLen,
      size_t        firstLen ) const
{
   if ( psrData )
   {
      *psrData = "";

      if ( precision == SIZE_MAX )
      {
         TC_MinGrid_c& minGrid = TC_MinGrid_c::GetInstance( );
         precision = minGrid.GetPrecision( );
      }

      if ( firstLen == SIZE_MAX )
      {
	 firstLen = spaceLen;
      }

      for ( size_t j = 0; j < this->height_; ++j )
      {
	 if (( j == 0 ) && ( firstLen > 0 ))
	 {
            char szFirstText[ TIO_FORMAT_STRING_LEN_MAX ];
            sprintf( szFirstText, "%*s", firstLen, firstLen ? " " : "" );
            *psrData += szFirstText;
         }
	 else if (( j > 0 ) && ( spaceLen > 0 ))
         {
            char szSpaceText[ TIO_FORMAT_STRING_LEN_MAX ];
            sprintf( szSpaceText, "%*s", spaceLen, spaceLen ? " " : "" );
            *psrData += szSpaceText;
         }

         for ( size_t i = 0; i < this->width_; ++i )
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
               iDataValue = *reinterpret_cast< int* >( this->At( i, j ));
               sprintf( szDataString, "%d", iDataValue );
               break;

            case TC_DATA_UINT:
               uiDataValue = *reinterpret_cast< unsigned int* >( this->At( i, j ));
               sprintf( szDataString, "%u", uiDataValue );
               break;

            case TC_DATA_LONG:
               lDataValue = *reinterpret_cast< long* >( this->At( i, j ));
               sprintf( szDataString, "%ld", lDataValue );
               break;

            case TC_DATA_ULONG:
               ulDataValue = *reinterpret_cast< unsigned long* >( this->At( i, j ));
               sprintf( szDataString, "%lu", ulDataValue );
               break;

            case TC_DATA_SIZE:
               sDataValue = *reinterpret_cast< size_t* >( this->At( i, j ));
               sprintf( szDataString, "%lu", sDataValue );
               break;

            case TC_DATA_FLOAT:
               fDataValue = *reinterpret_cast< double* >( this->At( i, j ));
               sprintf( szDataString, "%0.*f", precision, fDataValue );
               break;

            case TC_DATA_EXP:
               eDataValue = *reinterpret_cast< double* >( this->At( i, j ));
               sprintf( szDataString, "%0.*e", precision + 1, eDataValue );
               break;

            case TC_DATA_STRING:
               srDataValue = *reinterpret_cast< string* >( this->At( i, j ));
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
            *psrData += ( i < this->width_ - 1 ? " " : "" );
         }
	 *psrData += "\n";
      }
   }
}

//===========================================================================//
// Method         : SetCapacity
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedVector2D_c< T >::SetCapacity(
      size_t   width,
      size_t   height,
      const T* pdata )
{
   this->width_ = width;
   this->height_ = height;

   T data;
   if ( pdata )
      data = *pdata;

   this->vector2D_.resize( this->width_ );
   for ( size_t i = 0; i < this->width_; ++i )
   {
      this->vector2D_[i].resize( this->height_ );
      for ( size_t j = 0; j < this->height_; ++j )
      {
         this->vector2D_[i][j] = data;
      }
   }
}

//===========================================================================//
template< class T > void TCT_OrderedVector2D_c< T >::SetCapacity(
      size_t   width,
      size_t   height,
      const T& data )
{
   this->SetCapacity( width, height, &data );
}

//===========================================================================//
// Method         : At
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_OrderedVector2D_c< T >::At( 
      size_t i,
      size_t j )
{
   T* pdata = 0;

   if (( i < this->width_ ) && ( j < this->height_ ))
   {
      std::vector< T >& vector1D_ = this->vector2D_[i];
      pdata = &vector1D_[j];
   }
   return( pdata );
}

//===========================================================================//
template< class T > T* TCT_OrderedVector2D_c< T >::At( 
      size_t i,
      size_t j ) const
{
   T* pdata = const_cast< TCT_OrderedVector2D_c< T >* >( this )->At( i, j );

   return( pdata );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_OrderedVector2D_c< T >::Clear( 
      void )
{
   for ( size_t i = 0; i < this->width_; ++i )
   {
      this->vector2D_[i].clear( );
   }
   this->vector2D_.clear( );
}

#endif
