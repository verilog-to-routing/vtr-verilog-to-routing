#ifndef _KITTY_OPERATORS_TT_H_
#define _KITTY_OPERATORS_TT_H_
#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>

#include "kitty_constants.hpp"
#include "kitty_dynamic_tt.hpp"
#include "kitty_static_tt.hpp"
#include "kitty_operations.hpp"

ABC_NAMESPACE_CXX_HEADER_START

namespace kitty
{

/*! \brief Operator for unary_not */
inline dynamic_truth_table operator~( const dynamic_truth_table& tt )
{
  return unary_not( tt );
}

/*! \brief Operator for unary_not */
template<uint32_t NumVars>
inline static_truth_table<NumVars> operator~( const static_truth_table<NumVars>& tt )
{
  return unary_not( tt );
}

/*! \brief Operator for binary_and */
inline dynamic_truth_table operator&( const dynamic_truth_table& first, const dynamic_truth_table& second )
{
  return binary_and( first, second );
}

/*! \brief Operator for binary_and */
template<uint32_t NumVars>
inline static_truth_table<NumVars> operator&( const static_truth_table<NumVars>& first, const static_truth_table<NumVars>& second )
{
  return binary_and( first, second );
}

/*! \brief Operator for binary_and and assign */
inline void operator&=( dynamic_truth_table& first, const dynamic_truth_table& second )
{
  first = binary_and( first, second );
}

/*! \brief Operator for binary_and and assign */
template<uint32_t NumVars>
inline void operator&=( static_truth_table<NumVars>& first, const static_truth_table<NumVars>& second )
{
  first = binary_and( first, second );
}

/*! \brief Operator for binary_or */
inline dynamic_truth_table operator|( const dynamic_truth_table& first, const dynamic_truth_table& second )
{
  return binary_or( first, second );
}

/*! \brief Operator for binary_or */
template<uint32_t NumVars>
inline static_truth_table<NumVars> operator|( const static_truth_table<NumVars>& first, const static_truth_table<NumVars>& second )
{
  return binary_or( first, second );
}

/*! \brief Operator for binary_or and assign */
inline void operator|=( dynamic_truth_table& first, const dynamic_truth_table& second )
{
  first = binary_or( first, second );
}

/*! \brief Operator for binary_or and assign */
template<uint32_t NumVars>
inline void operator|=( static_truth_table<NumVars, true>& first, const static_truth_table<NumVars, true>& second )
{
  // first = binary_or( first, second );
  /* runtime improved version */
  first._bits |= second._bits;
  first.mask_bits();
}

/*! \brief Operator for binary_or and assign */
template<uint32_t NumVars>
inline void operator|=( static_truth_table<NumVars, false>& first, const static_truth_table<NumVars, false>& second )
{
  // first = binary_or( first, second );
  /* runtime improved version */
  if ( NumVars == 7 )
  {
    first._bits[0] |= second._bits[0];
    first._bits[1] |= second._bits[1];
  }
  else if ( NumVars == 8 )
  {
    first._bits[0] |= second._bits[0];
    first._bits[1] |= second._bits[1];
    first._bits[2] |= second._bits[2];
    first._bits[3] |= second._bits[3];
  }
  else if ( NumVars == 9 )
  {
    first._bits[0] |= second._bits[0];
    first._bits[1] |= second._bits[1];
    first._bits[2] |= second._bits[2];
    first._bits[3] |= second._bits[3];
    first._bits[4] |= second._bits[4];
    first._bits[5] |= second._bits[5];
    first._bits[6] |= second._bits[6];
    first._bits[7] |= second._bits[7];
  }
  else
  {
    for ( uint32_t i = 0; i < first.num_blocks(); ++i )
    {
      first._bits[i] |= second._bits[i];
    }
  }
}

} // namespace kitty

ABC_NAMESPACE_CXX_HEADER_END

#endif // _KITTY_OPERATORS_TT_H_