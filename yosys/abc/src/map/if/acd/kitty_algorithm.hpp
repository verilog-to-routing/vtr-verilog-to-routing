#ifndef _KITTY_ALGORITHM_H_
#define _KITTY_ALGORITHM_H_
#pragma once

#include <algorithm>
#include <cassert>

#include "kitty_constants.hpp"
#include "kitty_dynamic_tt.hpp"
#include "kitty_static_tt.hpp"

ABC_NAMESPACE_CXX_HEADER_START

namespace kitty
{

/*! \brief Perform bitwise unary operation on truth table

  \param tt Truth table
  \param op Unary operation that takes as input a word (`uint64_t`) and returns a word

  \return new constructed truth table of same type and dimensions
 */
template<typename TT, typename Fn>
TT unary_operation( const TT& tt, Fn&& op )
{
  TT result = tt.construct();
  std::transform( tt.cbegin(), tt.cend(), result.begin(), op );
  result.mask_bits();
  return result;
}

/*! \brief Perform bitwise binary operation on two truth tables

  The dimensions of `first` and `second` must match.  This is ensured
  at compile-time for static truth tables, but at run-time for dynamic
  truth tables.

  \param first First truth table
  \param second Second truth table
  \param op Binary operation that takes as input two words (`uint64_t`) and returns a word

  \return new constructed truth table of same type and dimensions
 */
template<typename TT, typename Fn>
TT binary_operation( const TT& first, const TT& second, Fn&& op )
{
  assert( first.num_vars() == second.num_vars() );

  TT result = first.construct();
  std::transform( first.cbegin(), first.cend(), second.cbegin(), result.begin(), op );
  result.mask_bits();
  return result;
}

/*! \brief Computes a predicate based on two truth tables

  The dimensions of `first` and `second` must match.  This is ensured
  at compile-time for static truth tables, but at run-time for dynamic
  truth tables.

  \param first First truth table
  \param second Second truth table
  \param op Binary operation that takes as input two words (`uint64_t`) and returns a Boolean

  \return true or false based on the predicate
 */
template<typename TT, typename Fn>
bool binary_predicate( const TT& first, const TT& second, Fn&& op )
{
  assert( first.num_vars() == second.num_vars() );

  return std::equal( first.begin(), first.end(), second.begin(), op );
}

/*! \brief Assign computed values to bits

  The functor `op` computes bits which are assigned to the bits of the
  truth table.

  \param tt Truth table
  \param op Unary operation that takes no input and returns a word (`uint64_t`)
*/
template<typename TT, typename Fn>
void assign_operation( TT& tt, Fn&& op )
{
  std::generate( tt.begin(), tt.end(), op );
  tt.mask_bits();
}

/*! \brief Iterates through each block of a truth table

 The functor `op` is called for every block of the truth table.

 \param tt Truth table
 \param op Unary operation that takes as input a word (`uint64_t`) and returns void
*/
template<typename TT, typename Fn>
void for_each_block( const TT& tt, Fn&& op )
{
  std::for_each( tt.cbegin(), tt.cend(), op );
}

/*! \brief Iterates through each block of a truth table in reverse
    order

 The functor `op` is called for every block of the truth table in
 reverse order.

 \param tt Truth table
 \param op Unary operation that takes as input a word (`uint64_t`) and returns void
*/
template<typename TT, typename Fn>
void for_each_block_reversed( const TT& tt, Fn&& op )
{
  std::for_each( tt.crbegin(), tt.crend(), op );
}

} // namespace kitty

ABC_NAMESPACE_CXX_HEADER_END

#endif // _KITTY_ALGORITHM_H_