#ifndef _KITTY_OPERATIONS_TT_H_
#define _KITTY_OPERATIONS_TT_H_
#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <iostream>

#include "kitty_algorithm.hpp"
#include "kitty_constants.hpp"
#include "kitty_dynamic_tt.hpp"
#include "kitty_static_tt.hpp"

ABC_NAMESPACE_CXX_HEADER_START

namespace kitty
{

/*! Inverts all bits in a truth table, based on a condition */
template<typename TT>
inline TT unary_not_if( const TT& tt, bool cond )
{
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4146 )
#endif
  const auto mask = -static_cast<uint64_t>( cond );
#ifdef _MSC_VER
#pragma warning( pop )
#endif
  return unary_operation( tt, [mask]( uint64_t a )
                          { return a ^ mask; } );
}

/*! \brief Inverts all bits in a truth table */
template<typename TT>
inline TT unary_not( const TT& tt )
{
  return unary_operation( tt, []( uint64_t a )
                          { return ~a; } );
}

/*! \brief Bitwise AND of two truth tables */
template<typename TT>

inline TT binary_and( const TT& first, const TT& second )
{
  return binary_operation( first, second, std::bit_and<uint64_t>() );
}

/*! \brief Bitwise OR of two truth tables */
template<typename TT>
inline TT binary_or( const TT& first, const TT& second )
{
  return binary_operation( first, second, std::bit_or<uint64_t>() );
}

/*! \brief Swaps two variables in a truth table

  The function swaps variable `var_index1` with `var_index2`.  The
  function will change `tt` in-place.  If `tt` should not be changed,
  one can use `swap` instead.

  \param tt Truth table
  \param var_index1 First variable
  \param var_index2 Second variable
*/
template<typename TT>
void swap_inplace( TT& tt, uint8_t var_index1, uint8_t var_index2 )
{
  if ( var_index1 == var_index2 )
  {
    return;
  }

  if ( var_index1 > var_index2 )
  {
    std::swap( var_index1, var_index2 );
  }

  if ( tt.num_vars() <= 6 )
  {
    const auto& pmask = detail::ppermutation_masks[var_index1][var_index2];
    const auto shift = ( 1 << var_index2 ) - ( 1 << var_index1 );
    tt._bits[0] = ( tt._bits[0] & pmask[0] ) | ( ( tt._bits[0] & pmask[1] ) << shift ) | ( ( tt._bits[0] & pmask[2] ) >> shift );
  }
  else if ( var_index2 <= 5 )
  {
    const auto& pmask = detail::ppermutation_masks[var_index1][var_index2];
    const auto shift = ( 1 << var_index2 ) - ( 1 << var_index1 );
    std::transform( std::begin( tt._bits ), std::end( tt._bits ), std::begin( tt._bits ),
                    [shift, &pmask]( uint64_t word )
                    {
                      return ( word & pmask[0] ) | ( ( word & pmask[1] ) << shift ) | ( ( word & pmask[2] ) >> shift );
                    } );
  }
  else if ( var_index1 <= 5 ) /* in this case, var_index2 > 5 */
  {
    const auto step = 1 << ( var_index2 - 6 );
    const auto shift = 1 << var_index1;
    auto it = std::begin( tt._bits );
    while ( it != std::end( tt._bits ) )
    {
      for ( auto i = decltype( step ){ 0 }; i < step; ++i )
      {
        const auto low_to_high = ( *( it + i ) & detail::projections[var_index1] ) >> shift;
        const auto high_to_low = ( *( it + i + step ) << shift ) & detail::projections[var_index1];
        *( it + i ) = ( *( it + i ) & ~detail::projections[var_index1] ) | high_to_low;
        *( it + i + step ) = ( *( it + i + step ) & detail::projections[var_index1] ) | low_to_high;
      }
      it += 2 * step;
    }
  }
  else
  {
    const auto step1 = 1 << ( var_index1 - 6 );
    const auto step2 = 1 << ( var_index2 - 6 );
    auto it = std::begin( tt._bits );
    while ( it != std::end( tt._bits ) )
    {
      for ( auto i = 0; i < step2; i += 2 * step1 )
      {
        for ( auto j = 0; j < step1; ++j )
        {
          std::swap( *( it + i + j + step1 ), *( it + i + j + step2 ) );
        }
      }
      it += 2 * step2;
    }
  }
}

template<uint32_t NumVars>
inline void swap_inplace( static_truth_table<NumVars, true>& tt, uint8_t var_index1, uint8_t var_index2 )
{
  if ( var_index1 == var_index2 )
  {
    return;
  }

  if ( var_index1 > var_index2 )
  {
    std::swap( var_index1, var_index2 );
  }

  const auto& pmask = detail::ppermutation_masks[var_index1][var_index2];
  const auto shift = ( 1 << var_index2 ) - ( 1 << var_index1 );
  tt._bits = ( tt._bits & pmask[0] ) | ( ( tt._bits & pmask[1] ) << shift ) | ( ( tt._bits & pmask[2] ) >> shift );
}

/*! \brief Extends smaller truth table to larger one

  The most significant variables will not be in the functional support of the
  resulting truth table, but the method is helpful to align a truth table when
  being used with another one.

  \param tt Larger truth table to create
  \param from Smaller truth table to copy from
*/
template<typename TT, typename TTFrom>
void extend_to_inplace( TT& tt, const TTFrom& from )
{
  assert( tt.num_vars() >= from.num_vars() );

  if ( from.num_vars() < 6 )
  {
    auto mask = *from.begin();

    for ( auto i = from.num_vars(); i < std::min<uint8_t>( 6, tt.num_vars() ); ++i )
    {
      mask |= ( mask << ( 1 << i ) );
    }

    std::fill( tt.begin(), tt.end(), mask );
  }
  else
  {
    auto it = tt.begin();
    while ( it != tt.end() )
    {
      it = std::copy( from.cbegin(), from.cend(), it );
    }
  }
}

/*! \brief Extends smaller truth table to larger static one

  This is an out-of-place version of `extend_to_inplace` that has the truth
  table as a return value.  It only works for creating static truth tables.  The
  template parameter `NumVars` must be equal or larger to the number of
  variables in `from`.

  \param from Smaller truth table to copy from
*/
template<uint32_t NumVars, typename TTFrom>
inline static_truth_table<NumVars> extend_to( const TTFrom& from )
{
  static_truth_table<NumVars> tt;
  extend_to_inplace( tt, from );
  return tt;
}

/*! \brief Checks whether truth table depends on given variable index

  \param tt Truth table
  \param var_index Variable index
*/
template<typename TT>
bool has_var( const TT& tt, uint8_t var_index )
{
  assert( var_index < tt.num_vars() );

  if ( tt.num_vars() <= 6 || var_index < 6 )
  {
    return std::any_of( std::begin( tt._bits ), std::end( tt._bits ),
                        [var_index]( uint64_t word )
                        { return ( ( word >> ( uint64_t( 1 ) << var_index ) ) & detail::projections_neg[var_index] ) !=
                                 ( word & detail::projections_neg[var_index] ); } );
  }

  const auto step = 1 << ( var_index - 6 );
  for ( auto i = 0u; i < static_cast<uint32_t>( tt.num_blocks() ); i += 2 * step )
  {
    for ( auto j = 0; j < step; ++j )
    {
      if ( tt._bits[i + j] != tt._bits[i + j + step] )
      {
        return true;
      }
    }
  }
  return false;
}

/*! \brief Checks whether truth table depends on given variable index

  \param tt Truth table
  \param care Care set
  \param var_index Variable index
*/
template<typename TT>
bool has_var( const TT& tt, const TT& care, uint8_t var_index )
{
  assert( var_index < tt.num_vars() );
  assert( tt.num_vars() == care.num_vars() );

  if ( tt.num_vars() <= 6 || var_index < 6 )
  {
    auto it_tt = std::begin( tt._bits );
    auto it_care = std::begin( care._bits );
    while ( it_tt != std::end( tt._bits ) )
    {
      if ( ( ( ( *it_tt >> ( uint64_t( 1 ) << var_index ) ) ^ *it_tt ) & detail::projections_neg[var_index]
           & ( *it_care >> ( uint64_t( 1 ) << var_index ) ) & *it_care ) != 0 )
      {
        return true;
      }
      ++it_tt;
      ++it_care;
    }

    return false;
  }

  const auto step = 1 << ( var_index - 6 );
  for ( auto i = 0u; i < static_cast<uint32_t>( tt.num_blocks() ); i += 2 * step )
  {
    for ( auto j = 0; j < step; ++j )
    {
      if ( ( ( tt._bits[i + j] ^ tt._bits[i + j + step] ) & care._bits[i + j] & care._bits[i + j + step] ) != 0 )
      {
        return true;
      }
    }
  }
  return false;
}

/*! \brief Shrinks larger truth table to smaller one

  The function expects that the most significant bits, which are cut off, are
  not in the functional support of the original function.  Only then it is
  ensured that the resulting function is equivalent.

  \param tt Smaller truth table to create
  \param from Larger truth table to copy from
*/
template<typename TT, typename TTFrom>
void shrink_to_inplace( TT& tt, const TTFrom& from )
{
  assert( tt.num_vars() <= from.num_vars() );

  std::copy( from.begin(), from.begin() + tt.num_blocks(), tt.begin() );

  if ( tt.num_vars() < 6 )
  {
    tt.mask_bits();
  }
}

/*! \brief Shrinks larger truth table to smaller dynamic one

  This is an out-of-place version of `shrink_to` that has the truth table as a
  return value.  It only works for creating dynamic tables.  The parameter
  `num_vars` must be equal or smaller to the number of variables in `from`.

  \param from Smaller truth table to copy from
*/
template<typename TTFrom>
inline dynamic_truth_table shrink_to( const TTFrom& from, unsigned num_vars )
{
  auto tt = create<dynamic_truth_table>( num_vars );
  shrink_to_inplace( tt, from );
  return tt;
}

/*! \brief Prints truth table in hexadecimal representation

  The most-significant bit will be the first character of the string.

  \param tt Truth table
  \param os Output stream
*/
template<typename TT>
void print_hex( const TT& tt, std::ostream& os = std::cout )
{
  auto const chunk_size =
      std::min<uint64_t>( tt.num_vars() <= 1 ? 1 : ( tt.num_bits() >> 2 ), 16 );

  for_each_block_reversed( tt, [&os, chunk_size]( uint64_t word )
                           {
    std::string chunk( chunk_size, '0' );

    auto it = chunk.rbegin();
    while (word && it != chunk.rend()) {
      auto hex = word & 0xf;
      if (hex < 10) {
        *it = '0' + static_cast<char>(hex);
      } else {
        *it = 'a' + static_cast<char>(hex - 10);
      }
      ++it;
      word >>= 4;
    }
    os << chunk; } );
}

} //namespace kitty

ABC_NAMESPACE_CXX_HEADER_END

#endif // _KITTY_OPERATIONS_TT_H_