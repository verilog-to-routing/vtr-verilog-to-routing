#pragma once

#include <vector>
#include "rr_types.h"
#include "rr_graph_type.h"
#include "physical_types.h"

void alloc_and_load_chan_details(const t_chan_width& nodes_per_chan,
                                 const std::vector<t_seg_details>& seg_details_x,
                                 const std::vector<t_seg_details>& seg_details_y,
                                 t_chan_details& chan_details_x,
                                 t_chan_details& chan_details_y);

std::vector<t_seg_details> alloc_and_load_seg_details(int* max_chan_width,
                                                      const int max_len,
                                                      const std::vector<t_segment_inf>& segment_inf,
                                                      const bool use_full_seg_groups,
                                                      const e_directionality directionality);

                                                      /**
 * @brief Assigns routing tracks to each segment type based on their frequencies and lengths.
 *
 * This function determines how many routing tracks (or sets of tracks) to assign to each
 * segment type in order to match the desired frequency distribution specified in
 * the segment information.
 *
 * When @p use_full_seg_groups is true, the function assigns tracks in multiples of the
 * segment length, which may result in a total track count that slightly overshoots or
 * undershoots the target @p num_sets. The algorithm proceeds by:
 * - Calculating the demand for each segment type.
 * - Iteratively assigning tracks to the segment type with the highest remaining demand.
 * - Optionally undoing the last assignment if it overshoots the target by more than half a group.
 *
 * @param num_sets Total number of track sets to assign.
 * @param segment_inf Vector containing segment type information (frequency, length, etc.).
 * @param use_full_seg_groups If true, assign tracks in full segment-length groups.
 * @return A vector where each element indicates the number of tracks assigned to the corresponding segment type.
 */
std::vector<int> get_seg_track_counts(int num_sets,
                                      const std::vector<t_segment_inf>& segment_inf,
                                      bool use_full_seg_groups);

/**
 * @brief Returns interposer cut positions along the segment dimension for the given channel type.
 *
 * For CHANX (horizontal wires), returns vertical interposer cut x-positions.
 * For CHANY (vertical wires), returns horizontal interposer cut y-positions.
 * Returns an empty vector when there are no interposer cuts.
 */
std::vector<int> get_chan_seg_interposer_cuts(e_rr_type chan_type);

int get_seg_start(const t_chan_seg_details* seg_details,
                  const int itrack,
                  const int chan_num,
                  const int seg_num,
                  const std::vector<int>& seg_dimension_cuts);

int get_seg_end(const t_chan_seg_details* seg_details,
                const int itrack,
                const int istart,
                const int chan_num,
                const int seg_max,
                const std::vector<int>& seg_dimension_cuts);                                      