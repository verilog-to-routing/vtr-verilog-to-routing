#pragma once

#include <vector>
#include "rr_types.h"
#include "rr_graph_type.h"
#include "physical_types.h"

/// Allocates and loads the chan_details data structure, a 2D array of
/// seg_details structures. This array is used to handle unique seg_details
/// (ie. channel segments) for each horizontal and vertical channel segment.
void alloc_and_load_chan_details(const t_chan_width& nodes_per_chan,
                                 const std::vector<t_seg_details>& seg_details_x,
                                 const std::vector<t_seg_details>& seg_details_y,
                                 t_chan_details& chan_details_x,
                                 t_chan_details& chan_details_y);

/// @brief Allocates and populates per-track segment metadata for one routing channel axis.
///
/// Builds a vector of @c t_seg_details (one entry per routing track) from architecture
/// segment definitions. Track counts per segment type follow the frequency distribution in
/// @p segment_inf. For each track, the function sets length,longline flag, staggered start offset,
/// connection-box and switch-box patterns, R/C, architecture switch indices, and wire direction.
///
/// When @p use_full_seg_groups is true, tracks are assigned in multiples of segment length,
/// and @p max_chan_width may be adjusted to the actual number of tracks allocated.
///
/// @param max_chan_width In: desired channel width; out: actual width after track assignment.
/// @param max_len Maximum segment length (grid dimension along this axis, used for longlines).
/// @param segment_inf Architecture segment types (frequency, length, cb/sb patterns, etc.).
/// @param use_full_seg_groups If true, assign tracks in full segment-length groups.
/// @param directionality BI_DIRECTIONAL or UNI_DIRECTIONAL routing of segment wires.
/// @return Per-track @c t_seg_details for horizontal (CHANX) or vertical (CHANY) channels.
std::vector<t_seg_details> alloc_and_load_seg_details(int* max_chan_width,
                                                      const int max_len,
                                                      const std::vector<t_segment_inf>& segment_inf,
                                                      const bool use_full_seg_groups,
                                                      const e_directionality directionality);

/// @brief Assigns routing tracks to each segment type based on their frequencies and lengths.
///
/// This function determines how many routing tracks (or sets of tracks) to assign to each
/// segment type in order to match the desired frequency distribution specified in
/// the segment information.
///
/// When @p use_full_seg_groups is true, the function assigns tracks in multiples of the
/// segment length, which may result in a total track count that slightly overshoots or
/// undershoots the target @p num_sets. The algorithm proceeds by:
/// - Calculating the demand for each segment type.
/// - Iteratively assigning tracks to the segment type with the highest remaining demand.
/// - Optionally undoing the last assignment if it overshoots the target by more than half a group.
///
/// @param num_sets Total number of track sets to assign.
/// @param segment_inf Vector containing segment type information (frequency, length, etc.).
/// @param use_full_seg_groups If true, assign tracks in full segment-length groups.
/// @return A vector where each element indicates the number of tracks assigned to the corresponding segment type.
std::vector<int> get_seg_track_counts(int num_sets,
                                      const std::vector<t_segment_inf>& segment_inf,
                                      bool use_full_seg_groups);

/// @brief Returns interposer cut positions along the segment dimension for the given channel type.
///
/// For CHANX (horizontal wires), returns vertical interposer cut x-positions.
/// For CHANY (vertical wires), returns horizontal interposer cut y-positions.
/// Returns an empty vector when there are no interposer cuts.
const std::vector<int>& get_chan_interposer_cuts(e_rr_type chan_type);

/// @brief Returns the channel segment number at which a given track
///        at a given channel segment number started (e.g. starting x-coordinate
///        for CHANX wires).
///        When interposer cuts are present (seg_dimension_cuts is non-empty),
///        wires going through a cut are split at the cut position.
int get_seg_start(const t_chan_seg_details* seg_details,
                  const int itrack,
                  const int chan_num,
                  const int seg_num,
                  const std::vector<int>& seg_dimension_cuts);

/// @brief Returns the channel segment number at which a given track ends.
/// Computes the natural endpoint from @p istart and segment length, then clips to @p seg_max.
/// When interposer cuts are present (seg_dimension_cuts is non-empty),
/// wires spanning a cut are truncated at the cut position.
/// @p istart must be the natural start (from get_seg_start with empty cuts), not cut-adjusted.
int get_seg_end(const t_chan_seg_details* seg_details,
                const int itrack,
                const int istart,
                const int chan_num,
                const int seg_max,
                const std::vector<int>& seg_dimension_cuts);
