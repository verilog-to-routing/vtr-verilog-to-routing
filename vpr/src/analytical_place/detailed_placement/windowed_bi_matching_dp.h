#pragma once
/**
 * @file
 * @author  Athavan Balakumar
 * @date    May 2026
 * @brief   Defines a windowed bipartite matching based detailed placer approach
 */

#include "detailed_placer.h"

/**
 * @brief Detailed placer that optimizes legalized placements using local bipartite matching.
 *
 * This placer aims to partition the placement into local windows, then use bipartite
 * matching in each window to improve placement quality (Currently under development).
 */

class WindowedBiMatchingDetailedPlacer : public DetailedPlacer {
  public:
    using DetailedPlacer::DetailedPlacer;

    void optimize_placement() final;
};
