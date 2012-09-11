//===========================================================================//
// Purpose : Template version for a TCT_QSort quick-sort algorithm.
//
//           Functions include:
//           - TCT_QSort
//           - TCT_QSortRecurse
//           - TCT_QSortPartitition
//           - TCT_QSortCompare
//
//===========================================================================//

#ifndef TCT_QSORT_H
#define TCT_QSORT_H

typedef int ( *TC_PFX_QSortCompare_t )( const void*, const void* );

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_QSort( T* padata, size_t len, 
                                    TC_PFX_QSortCompare_t pfxCompare );
template< class T > void TCT_QSort( T* padata, long len, 
                                    TC_PFX_QSortCompare_t pfxCompare );

template< class T > void TCT_QSortRecurse( T* padata, 
                                           long i, long j,
                                           TC_PFX_QSortCompare_t pfxCompare );
template< class T > long TCT_QSortPartition( T* padata, 
                                             long i, long j,
                                             TC_PFX_QSortCompare_t pfxCompare );

template< class T > int  TCT_QSortCompare( const T& dataA, 
                                           const T& dataB,
                                           TC_PFX_QSortCompare_t pfxCompare );

//===========================================================================//
// Function       : TCT_QSort
// Purpose        : Template version of a quick-sort algorithm for an array
//                  of data elements.  Sort order is based on the given 
//                  'QSortCompare' function.  
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_QSort( 
      T*     padata, 
      size_t len,
      TC_PFX_QSortCompare_t pfxCompare )
{
   TCT_QSort( padata, static_cast< long >( len ), pfxCompare );
}

//===========================================================================//
template< class T > void TCT_QSort( 
      T*   padata, 
      long len,
      TC_PFX_QSortCompare_t pfxCompare )
{
   TCT_QSortRecurse( padata, 0, len - 1, pfxCompare );
}

//===========================================================================//
// Function       : TCT_QSortRecurse
// Purpose        : Template used to implement a quick-sort algorithm 
//                  recursion.  This function was designed specifically
//                  to support the 'QSort' template.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > void TCT_QSortRecurse( 
      T*   padata, 
      long i, 
      long j,
      TC_PFX_QSortCompare_t pfxCompare )
{
   // Call to partition the array, returns pivot data element index
   long pivot = TCT_QSortPartition( padata, i, j, pfxCompare );

   // Test if sub-array must be partitioned again, if so, apply recursion
   if ( i < pivot )
   {
      TCT_QSortRecurse( padata, i, pivot - 1, pfxCompare );
   }

   // Test if sub-array must be partitioned again, if so, apply recursion
   if ( j > pivot )
   {
      TCT_QSortRecurse( padata, pivot + 1, j, pfxCompare );
   }
}

//===========================================================================//
// Function       : TCT_QSortPartition
// Purpose        : Template used to implement quick-sort algorithm 
//                  partitioning.  This function was designed specifically
//                  to support the 'QSort' template.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > long TCT_QSortPartition( 
      T*   padata, 
      long i, 
      long j,
      TC_PFX_QSortCompare_t pfxCompare )
{
   // Make copy of array[i] for pivot data element (which frees 1 location)
   T data = *( padata + i );

   // Randomize to improve quick-sort algorithm's worst-case runtime
   if ( j - i > 2 )
   {
      long r = TCT_Rand( i, j );

      *( padata + i ) = *( padata + r );
      *( padata + r ) = data;
      data = *( padata + i );
   }

   while ( i < j )
   {
      // Scan from right to left, skip elements greater than pivot element
      while (( i < j ) && 
            ( TCT_QSortCompare( padata + j, &data, pfxCompare ) > 0 ))
      {
         --j;
      }

      if ( i != j )
      {
         // Swap values and adjust index, if needed
         *( padata + i ) = *( padata + j );
         ++i;
      }

      // Scan from left to right, skip elements less than pivot element
      while (( i < j ) && 
            ( TCT_QSortCompare( padata + i, &data, pfxCompare ) < 0 ))
      {
         ++i;
      }

      if ( i != j )
      {
         // Swap values and adjust index, if needed
         *( padata + j ) = *( padata + i );
         --j;
      }
   }

   // Reinsert pivot element into array and return pivot element index
   *( padata + i ) = data;

   return( i );
}

//===========================================================================//
// Function       : TCT_QSortCompare
// Purpose        : Template used to compare two elements, returning an 
//                  'int' less than, greater than, or equal to zero.
//                  Elements are compared based on their 'key' fields.
//                  This function was designed specifically to support the
//                  'QSort' template.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
template< class T > int TCT_QSortCompare( 
      const T& dataA, 
      const T& dataB,
      TC_PFX_QSortCompare_t pfxCompare )
{
   const void* pvoidA = static_cast< const void* >( dataA->GetCompare( ));
   const void* pvoidB = static_cast< const void* >( dataB->GetCompare( ));

   return(( *pfxCompare )( pvoidA, pvoidB ));
}

#endif
