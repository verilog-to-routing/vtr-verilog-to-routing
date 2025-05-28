/**C++File**************************************************************

  FileName    [acd66.hpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Ashenhurst-Curtis decomposition.]

  Synopsis    [Interface with the FPGA mapping package.]

  Author      [Alessandro Tempia Calvino]

  Affiliation [EPFL]

  Date        [Ver. 1.0. Started - Feb 8, 2024.]

***********************************************************************/
/*!
  \file acd66.hpp
  \brief Ashenhurst-Curtis decomposition for "66" cascade

  \author Alessandro Tempia Calvino
*/

#ifndef _ACD66_H_
#define _ACD66_H_
#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <type_traits>

#include "kitty_constants.hpp"
#include "kitty_constructors.hpp"
#include "kitty_dynamic_tt.hpp"
#include "kitty_operations.hpp"
#include "kitty_operators.hpp"
#include "kitty_static_tt.hpp"

#ifdef _MSC_VER
#  include <intrin.h>
#  define __builtin_popcount __popcnt
#endif

ABC_NAMESPACE_CXX_HEADER_START

namespace acd
{

class acd66_impl
{
private:
  static constexpr uint32_t max_num_vars = 11;
  using STT = kitty::static_truth_table<max_num_vars>;
  using LTT = kitty::static_truth_table<6>;

public:
  explicit acd66_impl( uint32_t const num_vars, bool multiple_shared_set = false, bool const verify = false )
      : num_vars( num_vars ), multiple_ss( multiple_shared_set ), verify( verify )
  {
    std::iota( permutations.begin(), permutations.end(), 0 );
  }

  /*! \brief Runs ACD 66 */
  bool run( word* ptt )
  {
    assert( num_vars > 6 );

    /* truth table is too large for the settings */
    if ( num_vars > max_num_vars || num_vars > 11 )
    {
      return false;
    }

    /* convert to static TT */
    init_truth_table( ptt );

    /* run ACD trying different bound sets and free sets */
    return find_decomposition();
  }

  /*! \brief Runs ACD 66 */
  int run( word* ptt, unsigned delay_profile )
  {
    assert( num_vars > 6 );

    /* truth table is too large for the settings */
    if ( num_vars > max_num_vars || num_vars > 11 )
    {
      return false;
    }

    uint32_t late_arriving = __builtin_popcount( delay_profile );

    /* too many late arriving variables */
    if ( late_arriving > 5 )
      return 0;

    /* convert to static TT */
    init_truth_table( ptt );
    best_tt = start_tt;

    /* permute late arriving variables to be the least significant */
    reposition_late_arriving_variables( delay_profile, late_arriving );

    /* run ACD trying different bound sets and free sets */
    return find_decomposition_offset( late_arriving ) ? ( delay_profile == 0 ? 2 : 1 ) : 0;
  }

  int compute_decomposition()
  {
    if ( best_multiplicity == UINT32_MAX )
      return -1;

    compute_decomposition_impl();

    if ( verify && !verify_impl() )
    {
      return 1;
    }

    return 0;
  }

  uint32_t get_num_edges()
  {
    if ( bs_support_size == UINT32_MAX )
    {
      return num_vars + 1 + num_shared_vars;
    }

    /* real value after support minimization */
    return bs_support_size + best_free_set + 1 + num_shared_vars;
  }

  /* contains a 1 for FS variables */
  unsigned get_profile()
  {
    unsigned profile = 0;

    if ( best_multiplicity == UINT32_MAX )
      return -1;

    if ( bs_support_size == UINT32_MAX )
    {
      for ( uint32_t i = 0; i < best_free_set; ++i )
      {
        profile |= 1 << permutations[i];
      }
    }
    else
    {
      for ( uint32_t i = 0; i < bs_support_size; ++i )
      {
        profile |= 1 << permutations[bs_support[i] + best_free_set];
      }
      profile = ~profile & ( ( 1u << num_vars ) - 1 );
    }

    return profile;
  }

  void get_decomposition( unsigned char* decompArray )
  {
    if ( bs_support_size == UINT32_MAX )
      return;

    get_decomposition_abc( decompArray );
  }

private:
  bool find_decomposition()
  {
    best_multiplicity = UINT32_MAX;
    best_free_set = UINT32_MAX;

    /* use multiple shared set variables */
    if ( multiple_ss )
    {
      return find_decomposition_bs_multi_ss( num_vars - 6 );
    }

    uint32_t max_free_set = num_vars == 11 ? 5 : 4;

    /* find ACD "66" for different number of variables in the free set */
    for ( uint32_t i = num_vars - 6; i <= max_free_set; ++i )
    {
      if ( find_decomposition_bs( i ) )
        return true;
    }

    best_multiplicity = UINT32_MAX;
    return false;
  }

  bool find_decomposition_offset( uint32_t offset )
  {
    best_multiplicity = UINT32_MAX;
    best_free_set = UINT32_MAX;

    /* use multiple shared set variables */
    if ( multiple_ss )
    {
      return find_decomposition_bs_offset_multi_ss( std::max( num_vars - 6, offset ), offset );
    }

    uint32_t max_free_set = ( num_vars == 11 || offset == 5 ) ? 5 : 4;

    /* find ACD "66" for different number of variables in the free set */
    for ( uint32_t i = std::max( num_vars - 6, offset ); i <= max_free_set; ++i )
    {
      if ( find_decomposition_bs_offset( i, offset ) )
        return true;
    }

    best_multiplicity = UINT32_MAX;
    return false;
  }

  void init_truth_table( word* ptt )
  {
    uint32_t const num_blocks = ( num_vars <= 6 ) ? 1 : ( 1 << ( num_vars - 6 ) );

    for ( uint32_t i = 0; i < num_blocks; ++i )
    {
      start_tt._bits[i] = ptt[i];
    }

    local_extend_to( start_tt, num_vars );
  }

  uint32_t column_multiplicity( STT const& tt, uint32_t const free_set_size )
  {
    assert( free_set_size <= 5 );

    uint32_t const num_blocks = ( num_vars > 6 ) ? ( 1u << ( num_vars - 6 ) ) : 1;
    uint64_t const shift = UINT64_C( 1 ) << free_set_size;
    uint64_t const mask = ( UINT64_C( 1 ) << shift ) - 1;
    uint32_t const limit = free_set_size < 5 ? 4 : 2;
    uint32_t cofactors[4];
    uint32_t size = 0;

    /* extract iset functions */
    for ( auto i = 0u; i < num_blocks; ++i )
    {
      uint64_t sub = tt._bits[i];
      for ( auto j = 0; j < ( 64 >> free_set_size ); ++j )
      {
        uint32_t fs_fn = static_cast<uint32_t>( sub & mask );
        uint32_t k;
        for ( k = 0; k < size; ++k )
        {
          if ( fs_fn == cofactors[k] )
            break;
        }
        if ( k == limit )
          return 5;
        if ( k == size )
          cofactors[size++] = fs_fn;
        sub >>= shift;
      }
    }

    return size;
  }

  uint32_t column_multiplicity2( STT const& tt, uint32_t const free_set_size, uint32_t const limit )
  {
    assert( free_set_size <= 5 );

    uint32_t const num_blocks = ( num_vars > 6 ) ? ( 1u << ( num_vars - 6 ) ) : 1;
    uint64_t const shift = UINT64_C( 1 ) << free_set_size;
    uint64_t const mask = ( UINT64_C( 1 ) << shift ) - 1;
    uint32_t cofactors[32];
    uint32_t size = 0;

    /* extract iset functions */
    for ( auto i = 0u; i < num_blocks; ++i )
    {
      uint64_t sub = tt._bits[i];
      for ( auto j = 0; j < ( 64 >> free_set_size ); ++j )
      {
        uint32_t fs_fn = static_cast<uint32_t>( sub & mask );
        uint32_t k;
        for ( k = 0; k < size; ++k )
        {
          if ( fs_fn == cofactors[k] )
            break;
        }
        if ( k == limit )
          return limit + 1;
        if ( k == size )
          cofactors[size++] = fs_fn;
        sub >>= shift;
      }
    }

    return size;
  }

  inline bool combinations_next( uint32_t k, uint32_t offset, uint32_t* pComb, uint32_t* pInvPerm, STT& tt )
  {
    uint32_t i;

    for ( i = k - 1; pComb[i] == num_vars - k + i; --i )
    {
      if ( i == offset )
        return false;
    }

    /* move vars */
    uint32_t var_old = pComb[i];
    uint32_t pos_new = pInvPerm[var_old + 1];
    std::swap( pInvPerm[var_old + 1], pInvPerm[var_old] );
    std::swap( pComb[i], pComb[pos_new] );
    swap_inplace_local( tt, i, pos_new );

    for ( uint32_t j = i + 1; j < k; j++ )
    {
      var_old = pComb[j];
      pos_new = pInvPerm[pComb[j - 1] + 1];
      std::swap( pInvPerm[pComb[j - 1] + 1], pInvPerm[var_old] );
      std::swap( pComb[j], pComb[pos_new] );
      swap_inplace_local( tt, j, pos_new );
    }

    return true;
  }

  inline bool combinations_next_simple( uint32_t k, uint32_t* pComb, uint32_t* pInvPerm, uint32_t size )
  {
    uint32_t i;

    for ( i = k - 1; pComb[i] == size - k + i; --i )
    {
      if ( i == 0 )
        return false;
    }

    /* move vars */
    uint32_t var_old = pComb[i];
    uint32_t pos_new = pInvPerm[var_old + 1];
    std::swap( pInvPerm[var_old + 1], pInvPerm[var_old] );
    std::swap( pComb[i], pComb[pos_new] );

    for ( uint32_t j = i + 1; j < k; j++ )
    {
      var_old = pComb[j];
      pos_new = pInvPerm[pComb[j - 1] + 1];
      std::swap( pInvPerm[pComb[j - 1] + 1], pInvPerm[var_old] );
      std::swap( pComb[j], pComb[pos_new] );
    }

    return true;
  }

  bool find_decomposition_bs( uint32_t free_set_size )
  {
    STT tt = start_tt;

    /* works up to 16 input truth tables */
    assert( num_vars <= 16 );

    /* init combinations */
    uint32_t pComb[16], pInvPerm[16];
    for ( uint32_t i = 0; i < num_vars; ++i )
    {
      pComb[i] = pInvPerm[i] = i;
    }

    /* enumerate combinations */
    best_free_set = free_set_size;
    do
    {
      uint32_t cost = column_multiplicity( tt, free_set_size );
      if ( cost == 2 )
      {
        best_tt = tt;
        best_multiplicity = cost;
        for ( uint32_t i = 0; i < num_vars; ++i )
        {
          permutations[i] = pComb[i];
        }
        return true;
      }
      else if ( cost <= 4 && free_set_size < 5 )
      {
        /* look for a shared variable */
        best_multiplicity = cost;
        int res = check_shared_set( tt );

        if ( res >= 0 )
        {
          best_tt = tt;
          for ( uint32_t i = 0; i < num_vars; ++i )
          {
            permutations[i] = pComb[i];
          }
          /* move shared variable as the most significative one */
          swap_inplace_local( best_tt, res, num_vars - 1 );
          std::swap( permutations[res], permutations[num_vars - 1] );
          num_shared_vars = 1;
          return true;
        }
      }
    } while ( combinations_next( free_set_size, 0, pComb, pInvPerm, tt ) );

    return false;
  }

  bool find_decomposition_bs_offset( uint32_t free_set_size, uint32_t offset )
  {
    STT tt = best_tt;

    /* works up to 16 input truth tables */
    assert( num_vars <= 16 );
    best_free_set = free_set_size;

    /* special case */
    if ( free_set_size == offset )
    {
      uint32_t cost = column_multiplicity( tt, free_set_size );
      if ( cost == 2 )
      {
        best_tt = tt;
        best_multiplicity = cost;
        return true;
      }
      else if ( cost <= 4 && free_set_size < 5 )
      {
        /* look for a shared variable */
        best_multiplicity = cost;
        int res = check_shared_set( tt );

        if ( res >= 0 )
        {
          best_tt = tt;
          /* move shared variable as the most significative one */
          swap_inplace_local( best_tt, res, num_vars - 1 );
          std::swap( permutations[res], permutations[num_vars - 1] );
          num_shared_vars = 1;
          return true;
        }
      }
      return false;
    }

    /* init combinations */
    uint32_t pComb[16], pInvPerm[16];
    for ( uint32_t i = 0; i < num_vars; ++i )
    {
      pComb[i] = pInvPerm[i] = i;
    }

    /* enumerate combinations */
    do
    {
      uint32_t cost = column_multiplicity( tt, free_set_size );
      if ( cost == 2 )
      {
        best_tt = tt;
        best_multiplicity = cost;
        for ( uint32_t i = 0; i < num_vars; ++i )
        {
          pInvPerm[i] = permutations[pComb[i]];
        }
        for ( uint32_t i = 0; i < num_vars; ++i )
        {
          permutations[i] = pInvPerm[i];
        }
        return true;
      }
      else if ( cost <= 4 && free_set_size < 5 )
      {
        /* look for a shared variable */
        best_multiplicity = cost;
        int res = check_shared_set( tt );

        if ( res >= 0 )
        {
          best_tt = tt;
          for ( uint32_t i = 0; i < num_vars; ++i )
          {
            pInvPerm[i] = permutations[pComb[i]];
          }
          for ( uint32_t i = 0; i < num_vars; ++i )
          {
            permutations[i] = pInvPerm[i];
          }
          /* move shared variable as the most significative one */
          swap_inplace_local( best_tt, res, num_vars - 1 );
          std::swap( permutations[res], permutations[num_vars - 1] );
          num_shared_vars = 1;
          return true;
        }
      }
    } while ( combinations_next( free_set_size, offset, pComb, pInvPerm, tt ) );

    return false;
  }

  bool find_decomposition_bs_multi_ss( uint32_t free_set_size )
  {
    STT tt = start_tt;

    /* works up to 16 input truth tables */
    assert( num_vars <= 16 );

    /* init combinations */
    uint32_t pComb[16], pInvPerm[16], shared_set[4];
    for ( uint32_t i = 0; i < num_vars; ++i )
    {
      pComb[i] = pInvPerm[i] = i;
    }

    /* enumerate combinations */
    best_free_set = free_set_size;
    do
    {
      uint32_t cost = column_multiplicity2( tt, free_set_size, 1 << ( 6 - free_set_size ) );
      if ( cost <= 2 )
      {
        best_tt = tt;
        best_multiplicity = cost;
        for ( uint32_t i = 0; i < num_vars; ++i )
        {
          permutations[i] = pComb[i];
        }
        return true;
      }

      uint32_t ss_vars_needed = cost <= 4 ? 1 : cost <= 8 ? 2
                                            : cost <= 16  ? 3
                                            : cost <= 32  ? 4
                                                          : 5;
      if ( ss_vars_needed + free_set_size < 6 )
      {
        /* look for a shared variable */
        best_multiplicity = cost;
        int res = check_shared_set_multi( tt, ss_vars_needed, shared_set );

        if ( res >= 0 )
        {
          best_tt = tt;
          for ( uint32_t i = 0; i < num_vars; ++i )
          {
            permutations[i] = pComb[i];
          }
          /* move shared variables as the most significative ones */
          for ( int32_t i = res - 1; i >= 0; --i )
          {
            swap_inplace_local( best_tt, shared_set[i] + best_free_set, num_vars - res + i );
            std::swap( permutations[shared_set[i] + best_free_set], permutations[num_vars - res + i] );
          }
          num_shared_vars = res;
          return true;
        }
      }
    } while ( combinations_next( free_set_size, 0, pComb, pInvPerm, tt ) );

    best_multiplicity = UINT32_MAX;
    return false;
  }

  bool find_decomposition_bs_offset_multi_ss( uint32_t free_set_size, uint32_t offset )
  {
    STT tt = best_tt;

    /* works up to 16 input truth tables */
    assert( num_vars <= 16 );
    best_free_set = free_set_size;
    uint32_t shared_set[4];

    /* special case */
    if ( free_set_size == offset )
    {
      uint32_t cost = column_multiplicity2( tt, free_set_size, 1 << ( 6 - free_set_size ) );
      if ( cost == 2 )
      {
        best_tt = tt;
        best_multiplicity = cost;
        return true;
      }

      uint32_t ss_vars_needed = cost <= 4 ? 1 : cost <= 8 ? 2
                                            : cost <= 16  ? 3
                                            : cost <= 32  ? 4
                                                          : 5;

      if ( ss_vars_needed + free_set_size < 6 )
      {
        /* look for a shared variable */
        best_multiplicity = cost;
        int res = check_shared_set_multi( tt, ss_vars_needed, shared_set );

        if ( res >= 0 )
        {
          best_tt = tt;
          /* move shared variables as the most significative ones */
          for ( int32_t i = res - 1; i >= 0; --i )
          {
            swap_inplace_local( best_tt, shared_set[i] + best_free_set, num_vars - res + i );
            std::swap( permutations[shared_set[i] + best_free_set], permutations[num_vars - res + i] );
          }
          num_shared_vars = res;
          return true;
        }
      }

      best_multiplicity = UINT32_MAX;
      return false;
    }

    /* init combinations */
    uint32_t pComb[16], pInvPerm[16];
    for ( uint32_t i = 0; i < num_vars; ++i )
    {
      pComb[i] = pInvPerm[i] = i;
    }

    /* enumerate combinations */
    do
    {
      uint32_t cost = column_multiplicity2( tt, free_set_size, 1 << ( 6 - free_set_size ) );
      if ( cost == 2 )
      {
        best_tt = tt;
        best_multiplicity = cost;
        for ( uint32_t i = 0; i < num_vars; ++i )
        {
          pInvPerm[i] = permutations[pComb[i]];
        }
        for ( uint32_t i = 0; i < num_vars; ++i )
        {
          permutations[i] = pInvPerm[i];
        }
        return true;
      }

      uint32_t ss_vars_needed = cost <= 4 ? 1 : cost <= 8 ? 2
                                            : cost <= 16  ? 3
                                            : cost <= 32  ? 4
                                                          : 5;

      if ( ss_vars_needed + free_set_size < 6 )
      {
        /* look for a shared variable */
        best_multiplicity = cost;
        int res = check_shared_set_multi( tt, ss_vars_needed, shared_set );

        if ( res >= 0 )
        {
          best_tt = tt;
          for ( uint32_t i = 0; i < num_vars; ++i )
          {
            pInvPerm[i] = permutations[pComb[i]];
          }
          for ( uint32_t i = 0; i < num_vars; ++i )
          {
            permutations[i] = pInvPerm[i];
          }
          /* move shared variables as the most significative ones */
          for ( int32_t i = res - 1; i >= 0; --i )
          {
            swap_inplace_local( best_tt, shared_set[i] + best_free_set, num_vars - res + i );
            std::swap( permutations[shared_set[i] + best_free_set], permutations[num_vars - res + i] );
          }
          num_shared_vars = res;
          return true;
        }
      }
    } while ( combinations_next( free_set_size, offset, pComb, pInvPerm, tt ) );

    best_multiplicity = UINT32_MAX;
    return false;
  }

  bool check_shared_var( STT const& tt, uint32_t free_set_size, uint32_t shared_var )
  {
    assert( free_set_size <= 5 );

    uint32_t const num_blocks = ( num_vars > 6 ) ? ( 1u << ( num_vars - 6 ) ) : 1;
    uint64_t const shift = UINT64_C( 1 ) << free_set_size;
    uint64_t const mask = ( UINT64_C( 1 ) << shift ) - 1;
    uint32_t cofactors[2][4];
    uint32_t size[2] = { 0, 0 };
    uint32_t shared_var_shift = shared_var - free_set_size;

    /* extract iset functions */
    uint32_t iteration_counter = 0;
    for ( auto i = 0u; i < num_blocks; ++i )
    {
      uint64_t sub = tt._bits[i];
      for ( auto j = 0; j < ( 64 >> free_set_size ); ++j )
      {
        uint32_t fs_fn = static_cast<uint32_t>( sub & mask );
        uint32_t p = ( iteration_counter >> shared_var_shift ) & 1;
        uint32_t k;
        for ( k = 0; k < size[p]; ++k )
        {
          if ( fs_fn == cofactors[p][k] )
            break;
        }
        if ( k == 2 )
          return false;
        if ( k == size[p] )
          cofactors[p][size[p]++] = fs_fn;
        sub >>= shift;
        ++iteration_counter;
      }
    }

    return true;
  }

  inline int check_shared_set( STT const& tt )
  {
    /* find one shared set variable */
    for ( uint32_t i = best_free_set; i < num_vars; ++i )
    {
      /* check the multiplicity of cofactors */
      if ( check_shared_var( tt, best_free_set, i ) )
      {
        return i;
      }
    }

    return -1;
  }

  bool check_shared_var_combined( STT const& tt, uint32_t free_set_size, uint32_t shared_vars[6], uint32_t num_shared_vars )
  {
    assert( free_set_size <= 5 );
    assert( num_shared_vars <= 4 );

    uint32_t const num_blocks = ( num_vars > 6 ) ? ( 1u << ( num_vars - 6 ) ) : 1;
    uint64_t const shift = UINT64_C( 1 ) << free_set_size;
    uint64_t const mask = ( UINT64_C( 1 ) << shift ) - 1;
    uint32_t cofactors[16][2];
    uint32_t size[16] = { 0 };

    /* extract iset functions */
    uint32_t iteration_counter = 0;
    for ( auto i = 0u; i < num_blocks; ++i )
    {
      uint64_t sub = tt._bits[i];
      for ( auto j = 0; j < ( 64 >> free_set_size ); ++j )
      {
        uint32_t fs_fn = static_cast<uint32_t>( sub & mask );
        uint32_t p = 0;
        for ( uint32_t k = 0; k < num_shared_vars; ++k )
        {
          p |= ( ( iteration_counter >> shared_vars[k] ) & 1 ) << k;
        }

        uint32_t k;
        for ( k = 0; k < size[p]; ++k )
        {
          if ( fs_fn == cofactors[p][k] )
            break;
        }
        if ( k == 2 )
          return false;
        if ( k == size[p] )
          cofactors[p][size[p]++] = fs_fn;
        sub >>= shift;
        ++iteration_counter;
      }
    }

    return true;
  }

  inline int check_shared_set_multi( STT const& tt, uint32_t target_num_ss, uint32_t* res_shared )
  {
    /* init combinations */
    uint32_t pComb[6], pInvPerm[6];

    /* search for a feasible shared set */
    for ( uint32_t i = target_num_ss; i < 6 - best_free_set; ++i )
    {
      for ( uint32_t i = 0; i < 6; ++i )
      {
        pComb[i] = pInvPerm[i] = i;
      }

      do
      {
        /* check for combined shared set */
        if ( check_shared_var_combined( tt, best_free_set, pComb, i ) )
        {
          for ( uint32_t j = 0; j < i; ++j )
          {
            res_shared[j] = pComb[j];
          }
          /* sort vars */
          std::sort( res_shared, res_shared + i );
          return i;
        }
      } while ( combinations_next_simple( i, pComb, pInvPerm, num_vars - best_free_set ) );
    }

    return -1;
  }

  void compute_decomposition_impl( bool verbose = false )
  {
    /* construct isets involved in multiplicity */
    LTT composition;
    LTT bs;
    LTT bs_dc;

    /* construct isets */
    uint32_t offset = 0;
    uint32_t num_blocks = ( num_vars > 6 ) ? ( 1u << ( num_vars - 6 ) ) : 1;
    uint64_t const shift = UINT64_C( 1 ) << best_free_set;
    uint64_t const mask = ( UINT64_C( 1 ) << shift ) - 1;
    uint32_t const num_groups = 1 << num_shared_vars;
    uint32_t const next_group = 1 << ( num_vars - best_free_set - num_shared_vars );

    uint64_t fs_fun[32] = { 0 };
    uint64_t dc_mask = ( ( UINT64_C( 1 ) << next_group ) - 1 );

    uint32_t group_index = 0;
    uint32_t set_index = 0;
    fs_fun[0] = best_tt._bits[0] & mask;
    bool set_dc = true;
    for ( auto i = 0u; i < num_blocks; ++i )
    {
      uint64_t cof = best_tt._bits[i];
      for ( auto j = 0; j < ( 64 >> best_free_set ); ++j )
      {
        uint64_t val = cof & mask;
        /* move to next block */
        if ( set_index == next_group )
        {
          if ( set_dc )
          {
            /* only one cofactor can be found in the group --> encoding can be 0 or 1 */
            fs_fun[group_index + 1] = fs_fun[group_index];
            bs_dc._bits |= dc_mask;
          }
          /* set don't care */
          set_dc = true;
          group_index += 2;
          set_index = 0;
          fs_fun[group_index] = val;
          dc_mask <<= next_group;
        }
        /* gather encoding */
        if ( val != fs_fun[group_index] )
        {
          bs._bits |= UINT64_C( 1 ) << ( j + offset );
          fs_fun[group_index + 1] = val;
          set_dc = false; // two cofactors are present
        }
        cof >>= shift;
        ++set_index;
      }
      offset = ( offset + ( 64 >> best_free_set ) ) & 0x3F;
    }

    if ( set_dc )
    {
      /* only one cofactor can be found in the group --> encoding can be 0 or 1 */
      fs_fun[group_index + 1] = fs_fun[group_index];
      bs_dc._bits |= dc_mask;
    }

    /* create composition function */
    for ( uint32_t i = 0; i < 2 * num_groups; ++i )
    {
      composition._bits |= fs_fun[i] << ( i * shift );
    }

    /* minimize support BS */
    LTT care;
    bs_support_size = 0;
    uint64_t constexpr masks[] = { 0x0, 0x3, 0xF, 0xFF, 0xFFFF, 0xFFFFFFFF, UINT64_MAX };
    care._bits = masks[num_vars - best_free_set] & ~bs_dc._bits;
    for ( uint32_t i = 0; i < num_vars - best_free_set; ++i )
    {
      if ( !has_var6( bs, care, i ) )
      {
        adjust_truth_table_on_dc( bs, care, i );
        continue;
      }

      if ( bs_support_size < i )
      {
        kitty::swap_inplace( bs, bs_support_size, i );
      }

      bs_support[bs_support_size] = i;
      ++bs_support_size;
    }

    /* assign functions */
    dec_funcs[0] = bs._bits;
    dec_funcs[1] = composition._bits;

    /* print functions */
    if ( verbose )
    {
      LTT f;
      f._bits = dec_funcs[0];
      std::cout << "BS function         : ";
      kitty::print_hex( f );
      std::cout << "\n";
      f._bits = dec_funcs[1];
      std::cout << "Composition function: ";
      kitty::print_hex( f );
      std::cout << "\n";
    }
  }

  template<typename TT_type>
  void local_extend_to( TT_type& tt, uint32_t real_num_vars )
  {
    if ( real_num_vars < 6 )
    {
      auto mask = *tt.begin();

      for ( auto i = real_num_vars; i < num_vars; ++i )
      {
        mask |= ( mask << ( 1 << i ) );
      }

      std::fill( tt.begin(), tt.end(), mask );
    }
    else
    {
      uint32_t num_blocks = ( 1u << ( real_num_vars - 6 ) );
      auto it = tt.begin();
      while ( it != tt.end() )
      {
        it = std::copy( tt.cbegin(), tt.cbegin() + num_blocks, it );
      }
    }
  }

  inline void reposition_late_arriving_variables( unsigned delay_profile, uint32_t late_arriving )
  {
    uint32_t k = 0;
    for ( uint32_t i = 0; i < late_arriving; ++i )
    {
      while ( ( ( delay_profile >> k ) & 1 ) == 0 )
        ++k;

      if ( permutations[i] == k )
      {
        ++k;
        continue;
      }

      std::swap( permutations[i], permutations[k] );
      swap_inplace_local( best_tt, i, k );
      ++k;
    }
  }

  void swap_inplace_local( STT& tt, uint8_t var_index1, uint8_t var_index2 )
  {
    if ( var_index1 == var_index2 )
    {
      return;
    }

    if ( var_index1 > var_index2 )
    {
      std::swap( var_index1, var_index2 );
    }

    assert( num_vars > 6 );
    const uint32_t num_blocks = 1 << ( num_vars - 6 );

    if ( var_index2 <= 5 )
    {
      const auto& pmask = kitty::detail::ppermutation_masks[var_index1][var_index2];
      const auto shift = ( 1 << var_index2 ) - ( 1 << var_index1 );
      std::transform( std::begin( tt._bits ), std::begin( tt._bits ) + num_blocks, std::begin( tt._bits ),
                      [shift, &pmask]( uint64_t word ) {
                        return ( word & pmask[0] ) | ( ( word & pmask[1] ) << shift ) | ( ( word & pmask[2] ) >> shift );
                      } );
    }
    else if ( var_index1 <= 5 ) /* in this case, var_index2 > 5 */
    {
      const auto step = 1 << ( var_index2 - 6 );
      const auto shift = 1 << var_index1;
      auto it = std::begin( tt._bits );
      while ( it != std::begin( tt._bits ) + num_blocks )
      {
        for ( auto i = decltype( step ){ 0 }; i < step; ++i )
        {
          const auto low_to_high = ( *( it + i ) & kitty::detail::projections[var_index1] ) >> shift;
          const auto high_to_low = ( *( it + i + step ) << shift ) & kitty::detail::projections[var_index1];
          *( it + i ) = ( *( it + i ) & ~kitty::detail::projections[var_index1] ) | high_to_low;
          *( it + i + step ) = ( *( it + i + step ) & kitty::detail::projections[var_index1] ) | low_to_high;
        }
        it += 2 * step;
      }
    }
    else
    {
      const auto step1 = 1 << ( var_index1 - 6 );
      const auto step2 = 1 << ( var_index2 - 6 );
      auto it = std::begin( tt._bits );
      while ( it != std::begin( tt._bits ) + num_blocks )
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

  inline bool has_var6( const LTT& tt, const LTT& care, uint8_t var_index )
  {
    if ( ( ( ( tt._bits >> ( uint64_t( 1 ) << var_index ) ) ^ tt._bits ) & kitty::detail::projections_neg[var_index] & ( care._bits >> ( uint64_t( 1 ) << var_index ) ) & care._bits ) != 0 )
    {
      return true;
    }

    return false;
  }

  void adjust_truth_table_on_dc( LTT& tt, LTT& care, uint32_t var_index )
  {
    uint64_t new_bits = tt._bits & care._bits;
    tt._bits = ( ( new_bits | ( new_bits >> ( uint64_t( 1 ) << var_index ) ) ) & kitty::detail::projections_neg[var_index] ) |
               ( ( new_bits | ( new_bits << ( uint64_t( 1 ) << var_index ) ) ) & kitty::detail::projections[var_index] );
    care._bits = ( care._bits | ( care._bits >> ( uint64_t( 1 ) << var_index ) ) ) & kitty::detail::projections_neg[var_index];
    care._bits = care._bits | ( care._bits << ( uint64_t( 1 ) << var_index ) );
  }

  /* Decomposition format for ABC
   *
   * The record is an array of unsigned chars where:
   *   - the first unsigned char entry stores the number of unsigned chars in the record
   *   - the second entry stores the number of LUTs
   * After this, several sub-records follow, each representing one LUT as follows:
   *   - an unsigned char entry listing the number of fanins
   *   - a list of fanins, from the LSB to the MSB of the truth table. The N inputs of the original function
   *     have indexes from 0 to N-1, followed by the internal signals in a topological order
   *   - the LUT truth table occupying 2^(M-3) bytes, where M is the fanin count of the LUT, from the LSB to the MSB.
   *     A 2-input LUT, which takes 4 bits, should be stretched to occupy 8 bits (one unsigned char)
   *     A 0- or 1-input LUT can be represented similarly but it is not expected that such LUTs will be represented
   */
  void get_decomposition_abc( unsigned char* decompArray )
  {
    unsigned char* pArray = decompArray;
    unsigned char bytes = 2;

    /* write number of LUTs */
    pArray++;
    *pArray = 2;
    pArray++;

    /* write BS LUT */
    /* write fanin size */
    *pArray = bs_support_size;
    pArray++;
    ++bytes;

    /* write support */
    for ( uint32_t i = 0; i < bs_support_size; ++i )
    {
      *pArray = (unsigned char)permutations[bs_support[i] + best_free_set];
      pArray++;
      ++bytes;
    }

    /* write truth table */
    uint32_t tt_num_bytes = ( bs_support_size <= 3 ) ? 1 : ( 1 << ( bs_support_size - 3 ) );
    for ( uint32_t i = 0; i < tt_num_bytes; ++i )
    {
      *pArray = (unsigned char)( ( dec_funcs[0] >> ( 8 * i ) ) & 0xFF );
      pArray++;
      ++bytes;
    }

    /* write top LUT */
    /* write fanin size */
    uint32_t support_size = best_free_set + 1 + num_shared_vars;
    *pArray = support_size;
    pArray++;
    ++bytes;

    /* write support */
    for ( uint32_t i = 0; i < best_free_set; ++i )
    {
      *pArray = (unsigned char)permutations[i];
      pArray++;
      ++bytes;
    }

    *pArray = (unsigned char)num_vars;
    pArray++;
    ++bytes;

    for ( uint32_t i = 0; i < num_shared_vars; ++i )
    {
      *pArray = (unsigned char)permutations[num_vars - num_shared_vars + i];
      pArray++;
      ++bytes;
    }

    /* write truth table */
    tt_num_bytes = ( support_size <= 3 ) ? 1 : ( 1 << ( support_size - 3 ) );
    for ( uint32_t i = 0; i < tt_num_bytes; ++i )
    {
      *pArray = (unsigned char)( ( dec_funcs[1] >> ( 8 * i ) ) & 0xFF );
      pArray++;
      ++bytes;
    }

    /* write numBytes */
    *decompArray = bytes;
  }

  bool verify_impl()
  {
    /* create PIs */
    STT pis[max_num_vars];
    for ( uint32_t i = 0; i < num_vars; ++i )
    {
      kitty::create_nth_var( pis[i], permutations[i] );
    }

    /* BS function patterns */
    STT bsi[6];
    for ( uint32_t i = 0; i < bs_support_size; ++i )
    {
      bsi[i] = pis[best_free_set + bs_support[i]];
    }

    /* compute first function */
    STT bsf_sim;
    for ( uint32_t i = 0u; i < ( 1 << num_vars ); ++i )
    {
      uint32_t pattern = 0u;
      for ( auto j = 0; j < bs_support_size; ++j )
      {
        pattern |= get_bit( bsi[j], i ) << j;
      }
      if ( ( dec_funcs[0] >> pattern ) & 1 )
      {
        set_bit( bsf_sim, i );
      }
    }

    /* compute first function */
    STT top_sim;
    for ( uint32_t i = 0u; i < ( 1 << num_vars ); ++i )
    {
      uint32_t pattern = 0u;
      for ( auto j = 0; j < best_free_set; ++j )
      {
        pattern |= get_bit( pis[j], i ) << j;
      }
      pattern |= get_bit( bsf_sim, i ) << best_free_set;

      /* shared variables */
      for ( auto j = 0u; j < num_shared_vars; ++j )
      {
        pattern |= get_bit( pis[num_vars - num_shared_vars + j], i ) << ( best_free_set + 1 + j );
      }

      if ( ( dec_funcs[1] >> pattern ) & 1 )
      {
        set_bit( top_sim, i );
      }
    }

    for ( uint32_t i = 0; i < ( 1 << ( num_vars - 6 ) ); ++i )
    {
      if ( top_sim._bits[i] != start_tt._bits[i] )
      {
        /* convert to dynamic_truth_table */
        // report_tt( bsf_sim );
        std::cout << "Found incorrect decomposition\n";
        report_tt( top_sim );
        std::cout << " instead_of\n";
        report_tt( start_tt );
        return false;
      }
    }

    return true;
  }

  uint32_t get_bit( const STT& tt, uint64_t index )
  {
    return ( tt._bits[index >> 6] >> ( index & 0x3f ) ) & 0x1;
  }

  void set_bit( STT& tt, uint64_t index )
  {
    tt._bits[index >> 6] |= uint64_t( 1 ) << ( index & 0x3f );
  }

  void report_tt( STT const& stt )
  {
    kitty::dynamic_truth_table tt( num_vars );

    std::copy( std::begin( stt._bits ), std::begin( stt._bits ) + ( 1 << ( num_vars - 6 ) ), std::begin( tt ) );
    kitty::print_hex( tt );
    std::cout << "\n";
  }

private:
  uint32_t best_multiplicity{ UINT32_MAX };
  uint32_t best_free_set{ UINT32_MAX };
  uint32_t bs_support_size{ UINT32_MAX };
  uint32_t num_shared_vars{ 0 };
  STT best_tt;
  STT start_tt;
  uint64_t dec_funcs[2];
  uint32_t bs_support[6];

  uint32_t const num_vars;
  bool const multiple_ss;
  bool const verify;
  std::array<uint32_t, max_num_vars> permutations;
};

} // namespace acd

ABC_NAMESPACE_CXX_HEADER_END

#endif // _ACD66_H_