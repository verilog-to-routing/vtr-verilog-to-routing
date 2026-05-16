#pragma once
/**
 * @file router_lookahead_constants.h
 * @brief Constants shared across the router lookahead subsystem.
 */

#include <limits>

/**
 * @brief Sentinel value written by the router lookahead into cost-map entries
 *        for which no routing path was found.
 *
 * Any code that consumes delay or cost values produced by the lookahead should
 * compare against this sentinel before doing arithmetic on the value, since it
 * is far outside the range of any real routing delay or congestion cost and
 * will produce meaningless results if used directly.
 *
 * NOTE: This sentinel value was originally selected for cost terms within the
 *       placer and router. If this was set to infinity, this could cause issues
 *       with optimization. A very large value was chosen to allow the algorithms
 *       to optimize, but still heavily penalize these paths.
 */
static constexpr float ROUTER_LOOKAHEAD_NO_PATH_SENTINEL = std::numeric_limits<float>::max() / 1e12;
