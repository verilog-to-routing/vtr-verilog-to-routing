/**
 * @file place_delay_model.h
 * @brief This file contains all the class and function declarations related to
 *        the placer delay model. For implementations, see place_delay_model.cpp.
 */

#pragma once

#include "vtr_ndmatrix.h"
#include "vtr_flat_map.h"
#include "vpr_types.h"
#include "router_delay_profiling.h"

#ifndef __has_attribute
#    define __has_attribute(x) 0 // Compatibility with non-clang compilers.
#endif

#if defined(COMPILER_GCC) && defined(NDEBUG)
#    define ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(COMPILER_MSVC) && defined(NDEBUG)
#    define ALWAYS_INLINE __forceinline
#elif __has_attribute(always_inline)
#    define ALWAYS_INLINE __attribute__((always_inline)) // clang
#else
#    define ALWAYS_INLINE inline
#endif

///@brief Forward declarations.
class PlaceDelayModel;
class PlacerState;

///@brief Returns the delay of one point to point connection.
float comp_td_single_connection_delay(const PlaceDelayModel* delay_model,
                                      const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs,
                                      ClusterNetId net_id,
                                      int ipin);

///@brief Recompute all point to point delays, updating `connection_delay` matrix.
void comp_td_connection_delays(const PlaceDelayModel* delay_model,
                               PlacerState& placer_state);

///@brief Abstract interface to a placement delay model.
class PlaceDelayModel {
  public:
    virtual ~PlaceDelayModel() = default;

    ///@brief Computes place delay model.
    virtual void compute(RouterDelayProfiler& route_profiler,
                         const t_placer_opts& placer_opts,
                         const t_router_opts& router_opts,
                         int longest_length)
        = 0;

    /**
     * @brief Returns the delay estimate between the specified block pins.
     *
     * Either compute or read methods must be invoked before invoking delay.
     */
    virtual float delay(const t_physical_tile_loc& from_loc, int from_pin, const t_physical_tile_loc& to_loc, int to_pin) const = 0;

    ///@brief Dumps the delay model to an echo file.
    virtual void dump_echo(std::string filename) const = 0;

    /**
     * @brief Write place delay model to specified file.
     *
     * May be unimplemented, in which case method should throw an exception.
     */
    virtual void write(const std::string& file) const = 0;

    /**
     * @brief Read place delay model from specified file.
     *
     * May be unimplemented, in which case method should throw an exception.
     */
    virtual void read(const std::string& file) = 0;
};



