#ifndef _KITTY_CONSTRUCT_TT_H_
#define _KITTY_CONSTRUCT_TT_H_
#pragma once

#include <cctype>
#include <chrono>
#include <istream>
#include <random>
#include <stack>

#include "kitty_constants.hpp"
#include "kitty_dynamic_tt.hpp"
#include "kitty_static_tt.hpp"

ABC_NAMESPACE_CXX_HEADER_START

namespace kitty
{

/*! \brief Creates truth table with number of variables

  If some truth table instance is given, one can create a truth table with the
  same type by calling the `construct()` method on it.  This function helps if
  only the number of variables is known and the base type and uniforms the
  creation of static and dynamic truth tables.  Note, however, that for static
  truth tables `num_vars` must be consistent to the number of variables in the
  truth table type.

  \param num_vars Number of variables
*/
template<typename TT>
inline TT create( unsigned num_vars )
{
  (void)num_vars;
  TT tt;
  assert( tt.num_vars() == num_vars );
  return tt;
}

/*! \cond PRIVATE */
template<>
inline dynamic_truth_table create<dynamic_truth_table>( unsigned num_vars )
{
  return dynamic_truth_table( num_vars );
}
/*! \endcond */

/*! \brief Constructs projections (single-variable functions)

  \param tt Truth table
  \param var_index Index of the variable, must be smaller than the truth table's number of variables
  \param complement If true, realize inverse projection
*/
template<typename TT>
void create_nth_var( TT& tt, uint8_t var_index, bool complement = false )
{
  if ( tt.num_vars() <= 6 )
  {
    /* assign from precomputed table */
    tt._bits[0] = complement ? ~detail::projections[var_index] : detail::projections[var_index];

    /* mask if truth table does not require all bits */
    tt.mask_bits();
    return;
  }

  if ( var_index < 6 )
  {
    std::fill( std::begin( tt._bits ), std::end( tt._bits ), complement ? ~detail::projections[var_index] : detail::projections[var_index] );
  }
  else
  {
    const auto c = 1 << ( var_index - 6 );
    const auto zero = uint64_t( 0 );
    const auto one = ~zero;
    auto block = uint64_t( 0u );

    while ( block < tt.num_blocks() )
    {
      for ( auto i = 0; i < c; ++i )
      {
        tt._bits[block++] = complement ? one : zero;
      }
      for ( auto i = 0; i < c; ++i )
      {
        tt._bits[block++] = complement ? zero : one;
      }
    }
  }
}

} // namespace kitty

ABC_NAMESPACE_CXX_HEADER_END

#endif // _KITTY_CONSTRUCT_TT_H_