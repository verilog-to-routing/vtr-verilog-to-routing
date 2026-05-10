#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    May 2026
 * @brief   Defines the SAPack full legalizer class.
 */

#include "full_legalizer.h"

struct PartialPlacement;

/**
 * @brief SAPack: An experimental full legalizer that greedily places molecules
 *        into tiles based on the partial placement, then iteratively legalizes
 *        the resulting clusters using full intra-LB routing checks.
 */
class SAPack : public FullLegalizer {
  public:
    using FullLegalizer::FullLegalizer;

    void legalize(const PartialPlacement& p_placement) final;
};
