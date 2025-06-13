#ifndef _KITTY_CONSTANTS_H_
#define _KITTY_CONSTANTS_H_
#pragma once

#include <cstdint>
#include <vector>
#include <numeric>      // std::iota

ABC_NAMESPACE_CXX_HEADER_START

namespace kitty
{

namespace detail
{

static constexpr uint64_t projections[] = {
    UINT64_C( 0xaaaaaaaaaaaaaaaa ),
    UINT64_C( 0xcccccccccccccccc ),
    UINT64_C( 0xf0f0f0f0f0f0f0f0 ),
    UINT64_C( 0xff00ff00ff00ff00 ),
    UINT64_C( 0xffff0000ffff0000 ),
    UINT64_C( 0xffffffff00000000 ) };

static constexpr uint64_t projections_neg[] = {
    UINT64_C( 0x5555555555555555 ),
    UINT64_C( 0x3333333333333333 ),
    UINT64_C( 0x0f0f0f0f0f0f0f0f ),
    UINT64_C( 0x00ff00ff00ff00ff ),
    UINT64_C( 0x0000ffff0000ffff ),
    UINT64_C( 0x00000000ffffffff ) };

static constexpr uint64_t masks[] = {
    UINT64_C( 0x0000000000000001 ),
    UINT64_C( 0x0000000000000003 ),
    UINT64_C( 0x000000000000000f ),
    UINT64_C( 0x00000000000000ff ),
    UINT64_C( 0x000000000000ffff ),
    UINT64_C( 0x00000000ffffffff ),
    UINT64_C( 0xffffffffffffffff ) };

static constexpr uint64_t permutation_masks[][3] = {
    { UINT64_C( 0x9999999999999999 ), UINT64_C( 0x2222222222222222 ), UINT64_C( 0x4444444444444444 ) },
    { UINT64_C( 0xc3c3c3c3c3c3c3c3 ), UINT64_C( 0x0c0c0c0c0c0c0c0c ), UINT64_C( 0x3030303030303030 ) },
    { UINT64_C( 0xf00ff00ff00ff00f ), UINT64_C( 0x00f000f000f000f0 ), UINT64_C( 0x0f000f000f000f00 ) },
    { UINT64_C( 0xff0000ffff0000ff ), UINT64_C( 0x0000ff000000ff00 ), UINT64_C( 0x00ff000000ff0000 ) },
    { UINT64_C( 0xffff00000000ffff ), UINT64_C( 0x00000000ffff0000 ), UINT64_C( 0x0000ffff00000000 ) } };

static constexpr uint64_t ppermutation_masks[][6][3] = {
    { { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0x9999999999999999 ), UINT64_C( 0x2222222222222222 ), UINT64_C( 0x4444444444444444 ) },
      { UINT64_C( 0xa5a5a5a5a5a5a5a5 ), UINT64_C( 0x0a0a0a0a0a0a0a0a ), UINT64_C( 0x5050505050505050 ) },
      { UINT64_C( 0xaa55aa55aa55aa55 ), UINT64_C( 0x00aa00aa00aa00aa ), UINT64_C( 0x5500550055005500 ) },
      { UINT64_C( 0xaaaa5555aaaa5555 ), UINT64_C( 0x0000aaaa0000aaaa ), UINT64_C( 0x5555000055550000 ) },
      { UINT64_C( 0xaaaaaaaa55555555 ), UINT64_C( 0x00000000aaaaaaaa ), UINT64_C( 0x5555555500000000 ) } },
    { { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0xc3c3c3c3c3c3c3c3 ), UINT64_C( 0x0c0c0c0c0c0c0c0c ), UINT64_C( 0x3030303030303030 ) },
      { UINT64_C( 0xcc33cc33cc33cc33 ), UINT64_C( 0x00cc00cc00cc00cc ), UINT64_C( 0x3300330033003300 ) },
      { UINT64_C( 0xcccc3333cccc3333 ), UINT64_C( 0x0000cccc0000cccc ), UINT64_C( 0x3333000033330000 ) },
      { UINT64_C( 0xcccccccc33333333 ), UINT64_C( 0x00000000cccccccc ), UINT64_C( 0x3333333300000000 ) } },
    { { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0xf00ff00ff00ff00f ), UINT64_C( 0x00f000f000f000f0 ), UINT64_C( 0x0f000f000f000f00 ) },
      { UINT64_C( 0xf0f00f0ff0f00f0f ), UINT64_C( 0x0000f0f00000f0f0 ), UINT64_C( 0x0f0f00000f0f0000 ) },
      { UINT64_C( 0xf0f0f0f00f0f0f0f ), UINT64_C( 0x00000000f0f0f0f0 ), UINT64_C( 0x0f0f0f0f00000000 ) } },
    { { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0xff0000ffff0000ff ), UINT64_C( 0x0000ff000000ff00 ), UINT64_C( 0x00ff000000ff0000 ) },
      { UINT64_C( 0xff00ff0000ff00ff ), UINT64_C( 0x00000000ff00ff00 ), UINT64_C( 0x00ff00ff00000000 ) } },
    { { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ), UINT64_C( 0x0000000000000000 ) },
      { UINT64_C( 0xffff00000000ffff ), UINT64_C( 0x00000000ffff0000 ), UINT64_C( 0x0000ffff00000000 ) } } };

static constexpr int32_t hex_to_int[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                          0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
                                          -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                          -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

} // namespace detail

} // namespace kitty

ABC_NAMESPACE_CXX_HEADER_END

#endif //_KITTY_CONSTANTS_H_