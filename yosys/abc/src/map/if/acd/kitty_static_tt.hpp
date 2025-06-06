#ifndef _KITTY_STATIC_TT_H_
#define _KITTY_STATIC_TT_H_
#pragma once

#include <array>
#include <cstdint>

#include "kitty_constants.hpp"

ABC_NAMESPACE_CXX_HEADER_START

namespace kitty
{

template<uint32_t NumVars, bool = ( NumVars <= 6 )>
struct static_truth_table;

/*! Truth table (for up to 6 variables) in which number of variables is known at compile time.
 */
template<uint32_t NumVars>
struct static_truth_table<NumVars, true>
{
  /*! \cond PRIVATE */
  enum
  {
    NumBits = uint64_t( 1 ) << NumVars
  };
  /*! \endcond */

  /*! Constructs a new static truth table instance with the same number of variables. */
  inline static_truth_table<NumVars> construct() const
  {
    return static_truth_table<NumVars>();
  }

  /*! Returns number of variables.
   */
  inline uint32_t num_vars() const noexcept { return NumVars; }

  /*! Returns number of blocks.
   */
  inline uint32_t num_blocks() const noexcept { return 1u; }

  /*! Returns number of bits.
   */
  inline uint32_t num_bits() const noexcept { return NumBits; }

  /*! \brief Begin iterator to bits.
   */
  inline uint64_t * begin() noexcept { return &_bits; }

  /*! \brief End iterator to bits.
   */
  inline uint64_t * end() noexcept { return ( &_bits ) + 1; }

  /*! \brief Begin iterator to bits.
   */
  inline const uint64_t * begin() const noexcept { return &_bits; }

  /*! \brief End iterator to bits.
   */
  inline const uint64_t * end() const noexcept { return ( &_bits ) + 1; }

  /*! \brief Reverse begin iterator to bits.
   */
  inline uint64_t * rbegin() noexcept { return &_bits; }

  /*! \brief Reverse end iterator to bits.
   */
  inline uint64_t * rend() noexcept { return ( &_bits ) + 1; }

  /*! \brief Constant begin iterator to bits.
   */
  inline const uint64_t * cbegin() const noexcept { return &_bits; }

  /*! \brief Constant end iterator to bits.
   */
  inline const uint64_t * cend() const noexcept { return ( &_bits ) + 1; }

  /*! \brief Constant reverse begin iterator to bits.
   */
  inline const uint64_t * crbegin() const noexcept { return &_bits; }

  /*! \brief Constant everse end iterator to bits.
   */
  inline const uint64_t * crend() const noexcept { return ( &_bits ) + 1; }

  /*! \brief Assign other truth table if number of variables match.

    This replaces the current truth table with another truth table, if `other`
    has the same number of variables.  Otherwise, the truth table is not
    changed.

    \param other Other truth table
  */
  template<class TT>
  static_truth_table<NumVars>& operator=( const TT& other )
  {
    if ( other.num_vars() == num_vars() )
    {
      std::copy( other.begin(), other.end(), begin() );
    }

    return *this;
  }

  /*! Masks the number of valid truth table bits.

    If the truth table has less than 6 variables, it may not use all
    the bits.  This operation makes sure to zero out all non-valid
    bits.
  */
  inline void mask_bits() noexcept { _bits &= detail::masks[NumVars]; }

  /*! \cond PRIVATE */
public: /* fields */
  uint64_t _bits = 0;
  /*! \endcond */
};

/*! Truth table (more than 6 variables) in which number of variables is known at compile time.
 */
template<uint32_t NumVars>
struct static_truth_table<NumVars, false>
{
  /*! \cond PRIVATE */
  enum
  {
    NumBlocks = ( NumVars <= 6 ) ? 1u : ( 1u << ( NumVars - 6 ) )
  };

  enum
  {
    NumBits = uint64_t( 1 ) << NumVars
  };
  /*! \endcond */

  /*! Standard constructor.

    The number of variables provided to the truth table must be known
    at runtime.  The number of blocks will be computed as a compile
    time constant.
  */
  static_truth_table()
  {
    _bits.fill( 0 );
  }

  /*! Constructs a new static truth table instance with the same number of variables. */
  inline static_truth_table<NumVars> construct() const
  {
    return static_truth_table<NumVars>();
  }

  /*! Returns number of variables.
   */
  inline uint32_t num_vars() const noexcept { return NumVars; }

  /*! Returns number of blocks.
   */
  inline uint32_t num_blocks() const noexcept { return NumBlocks; }

  /*! Returns number of bits.
   */
  inline uint32_t num_bits() const noexcept { return NumBits; }

  /*! \brief Begin iterator to bits.
   */
  inline typename std::array<uint64_t, NumBlocks>::iterator begin() noexcept { return _bits.begin(); }

  /*! \brief End iterator to bits.
   */
  inline typename std::array<uint64_t, NumBlocks>::iterator end() noexcept { return _bits.end(); }

  /*! \brief Begin iterator to bits.
   */
  inline typename std::array<uint64_t, NumBlocks>::const_iterator begin() const noexcept { return _bits.begin(); }

  /*! \brief End iterator to bits.
   */
  inline typename std::array<uint64_t, NumBlocks>::const_iterator end() const noexcept { return _bits.end(); }

  /*! \brief Reverse begin iterator to bits.
   */
  inline typename std::array<uint64_t, NumBlocks>::reverse_iterator rbegin() noexcept { return _bits.rbegin(); }

  /*! \brief Reverse end iterator to bits.
   */
  inline typename std::array<uint64_t, NumBlocks>::reverse_iterator rend() noexcept { return _bits.rend(); }

  /*! \brief Constant begin iterator to bits.
   */
  inline typename std::array<uint64_t, NumBlocks>::const_iterator cbegin() const noexcept { return _bits.cbegin(); }

  /*! \brief Constant end iterator to bits.
   */
  inline typename std::array<uint64_t, NumBlocks>::const_iterator cend() const noexcept { return _bits.cend(); }

  /*! \brief Constant reverse begin iterator to bits.
   */
  inline typename std::array<uint64_t, NumBlocks>::const_reverse_iterator crbegin() const noexcept { return _bits.crbegin(); }

  /*! \brief Constant teverse end iterator to bits.
   */
  inline typename std::array<uint64_t, NumBlocks>::const_reverse_iterator crend() const noexcept { return _bits.crend(); }

  /*! \brief Assign other truth table if number of variables match.

    This replaces the current truth table with another truth table, if `other`
    has the same number of variables.  Otherwise, the truth table is not
    changed.

    \param other Other truth table
  */
  template<class TT>
  static_truth_table<NumVars>& operator=( const TT& other )
  {
    if ( other.num_bits() == num_bits() )
    {
      std::copy( other.begin(), other.end(), begin() );
    }

    return *this;
  }

  /*! Masks the number of valid truth table bits.

    We know that we will have at least 7 variables in this data
    structure.
  */
  inline void mask_bits() noexcept {}

  /*! \cond PRIVATE */
public: /* fields */
  std::array<uint64_t, NumBlocks> _bits;
  /*! \endcond */
};

} //namespace kitty

ABC_NAMESPACE_CXX_HEADER_END

#endif // _KITTY_STATIC_TT_H_