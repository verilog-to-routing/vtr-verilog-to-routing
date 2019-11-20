//===========================================================================//
// Purpose : Template version for a TCT_BSearch binary-search algorithm.
//
//           Functions include:
//           - TCT_BSearch
//           - TCT_BSearchIterate
//           - TCT_BSearchCompare
//
//===========================================================================//

#ifndef TCT_BSEARCH_H
#define TCT_BSEARCH_H

typedef int ( *TC_PFX_BSearchCompare_t )( const void*, const void* );

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_BSearch( const T& data, 
                                    const T* padata, size_t len,
                                    TC_PFX_BSearchCompare_t pfxCompare );
template< class T > T* TCT_BSearch( const T& data, 
                                    const T* padata, long len,
                                    TC_PFX_BSearchCompare_t pfxCompare );

template< class T > T* TCT_BSearchIterate( const T& data,
                                           const T* padata, 
                                           long i, long j,
                                           TC_PFX_BSearchCompare_t pfxCompare );

template< class T > int TCT_BSearchCompare( const T& dataA, 
                                            const T& dataB,
                                            TC_PFX_BSearchCompare_t pfxCompare );

//===========================================================================//
// Function       : TCT_BSearch
// Purpose        : Template version of a binary-search algorithm for an
//                  array of data elements.  Sort order is assumed to be
//                  based on the given 'BSearchCompare' function.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_BSearch(
      const T&     data,
      const T*     padata,
            size_t len,
            TC_PFX_BSearchCompare_t pfxCompare )
{
   return( TCT_BSearch( data, padata, static_cast< long >( len ), pfxCompare ));
}

//===========================================================================//
template< class T > T* TCT_BSearch(
      const T&   data,
      const T*   padata,
            long len,
            TC_PFX_BSearchCompare_t pfxCompare )
{
   return( TCT_BSearchIterate( data, padata, 0, len - 1, pfxCompare ));
}

//===========================================================================//
// Function       : TCT_BSearchIterate
// Purpose        : Template used to implement a binary-search algorithm 
//                  iteration.  This function was designed specifically to
//                  support the 'BSearch' template.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > T* TCT_BSearchIterate(
      const T&   data,
      const T*   padata,
            long i,
            long j,
            TC_PFX_BSearchCompare_t pfxCompare )
{
   T* pdata = 0;

   // Iterate to perform simple binary-search until match found or exhausted
   while ( i <= j )
   {
      long mid = (( j - i ) / 2 ) + i;

      int c = TCT_BSearchCompare( &data, padata + mid, pfxCompare );
      if ( c == 0 )
      {
         pdata = const_cast< T* >( padata + mid );
         break;
      }
      else if ( c < 0 )
      {
         j = mid - 1;
      }
      else if ( c > 0 )
      {
         i = mid + 1;
      }
   }
   return( pdata );
}

//===========================================================================//
// Function       : TCT_BSearchCompare
// Purpose        : Template used to compare two elements, returning an 
//                  'int' less than, greater than, or equal to zero.  Data
//                  elements are compared based on their 'key' field.  This
//                  function was designed specifically to support the
//                  'BSearch' template.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > int TCT_BSearchCompare( 
      const T& dataA, 
      const T& dataB,
            TC_PFX_BSearchCompare_t pfxCompare )
{
   const void* pvoidA = static_cast< const void* >( dataA->GetCompare( ));
   const void* pvoidB = static_cast< const void* >( dataB->GetCompare( ));

   return(( *pfxCompare )( pvoidA, pvoidB ));
}

#endif 
