#pragma once
/**
 * @file
 * @brief Grouping of clock domains by the timing transfers analyzed between them.
 */

#include <cstddef>

#include "tatum/TimingGraphFwd.hpp"
#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/analyzers/SetupTimingAnalyzer.hpp"
#include "tatum/analyzers/HoldTimingAnalyzer.hpp"
#include "tatum/util/tatum_linear_map.hpp"
#include "vtr_strong_id.h"

/// @brief A unique ID of a group of clock domains with synchronous (analyzed) transfers between them.
typedef vtr::StrongId<struct clock_domain_group_id_tag, size_t> ClockDomainGroupId;

/**
 * @brief Partitions the clock domains into groups, such that clock domains with analyzed setup or
 *        hold timing transfers between them are in the same group.
 *
 *  @param timing_graph
 *      The timing graph which was analyzed.
 *  @param timing_constraints
 *      The timing constraints which were analyzed.
 *  @param setup_analyzer
 *      A setup timing analyzer whose timing has been updated.
 *  @param hold_analyzer
 *      A hold timing analyzer whose timing has been updated.
 *
 *  @return The group of every clock domain. Group IDs are contiguous, starting from 0.
 */
tatum::util::linear_map<tatum::DomainId, ClockDomainGroupId> group_synchronous_clock_domains(const tatum::TimingGraph& timing_graph,
                                                                                             const tatum::TimingConstraints& timing_constraints,
                                                                                             const tatum::SetupTimingAnalyzer& setup_analyzer,
                                                                                             const tatum::HoldTimingAnalyzer& hold_analyzer);
