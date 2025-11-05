#pragma once

#include <vector>

#include "build_switchblocks.h"
#include "rr_graph_type.h"
#include "rr_graph_fwd.h"
#include "rr_graph_builder.h"
#include "rr_types.h"
#include "device_grid.h"

/******************* Subroutines exported by rr_graph2.c *********************/

std::vector<t_seg_details> alloc_and_load_seg_details(int* max_chan_width,
                                                      const int max_len,
                                                      const std::vector<t_segment_inf>& segment_inf,
                                                      const bool use_full_seg_groups,
                                                      const e_directionality directionality);

void alloc_and_load_chan_details(const DeviceGrid& grid,
                                 const t_chan_width& nodes_per_chan,
                                 const std::vector<t_seg_details>& seg_details_x,
                                 const std::vector<t_seg_details>& seg_details_y,
                                 t_chan_details& chan_details_x,
                                 t_chan_details& chan_details_y);

t_chan_details init_chan_details(const DeviceGrid& grid,
                                 const t_chan_width& nodes_per_chan,
                                 const std::vector<t_seg_details>& seg_details,
                                 const e_parallel_axis seg_details_type);

void adjust_chan_details(const DeviceGrid& grid,
                         const t_chan_width& nodes_per_chan,
                         t_chan_details& chan_details_x,
                         t_chan_details& chan_details_y);

void adjust_seg_details(const int x,
                        const int y,
                        const DeviceGrid& grid,
                        const t_chan_width& nodes_per_chan,
                        t_chan_details& chan_details,
                        const e_parallel_axis seg_details_type);

int get_seg_start(const t_chan_seg_details* seg_details,
                  const int itrack,
                  const int chan_num,
                  const int seg_num);
int get_seg_end(const t_chan_seg_details* seg_details,
                const int itrack,
                const int istart,
                const int chan_num,
                const int seg_max);

bool is_cblock(const int chan,
               const int seg,
               const int track,
               const t_chan_seg_details* seg_details);

bool is_sblock(const int chan,
               int wire_seg,
               const int sb_seg,
               const int track,
               const t_chan_seg_details* seg_details,
               const enum e_directionality directionality);

/// Adds the fan-out edges from wire segment at (chan, seg, track) to adjacent blocks along the wire's length
int get_track_to_pins(RRGraphBuilder& rr_graph_builder,
                      int layer,
                      int seg,
                      int chan,
                      int track,
                      int tracks_per_chan,
                      RRNodeId from_rr_node,
                      t_rr_edge_info_set& rr_edges_to_create,
                      const t_track_to_pin_lookup& track_to_pin_lookup,
                      const t_chan_seg_details* seg_details,
                      e_rr_type chan_type,
                      int chan_length,
                      int wire_to_ipin_switch,
                      e_directionality directionality);

int get_track_to_tracks(RRGraphBuilder& rr_graph_builder,
                        const int layer,
                        const int from_chan,
                        const int from_seg,
                        const int from_track,
                        const e_rr_type from_type,
                        const int to_seg,
                        const e_rr_type to_type,
                        const int chan_len,
                        const int max_chan_width,
                        const DeviceGrid& grid,
                        const int Fs_per_side,
                        t_sblock_pattern& sblock_pattern,
                        RRNodeId from_rr_node,
                        t_rr_edge_info_set& rr_edges_to_create,
                        const t_chan_seg_details* from_seg_details,
                        const t_chan_seg_details* to_seg_details,
                        const t_chan_details& to_chan_details,
                        const e_directionality directionality,
                        const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                        const t_sb_connection_map* sb_conn_map);

/**
 * @brief Identifies and labels all mux endpoints at a given channel segment coordinate.
 *
 * This routine scans all routing tracks within a channel segment (specified by
 * 'chan_num' and 'seg_num') and collects the track indices corresponding to
 * valid mux endpoints that can be driven by OPINs in that channel segment.
 * The resulting list of eligible tracks is returned in natural (increasing) track order.
 *
 * @details If @p seg_type_index is UNDEFINED, all segment types are considered.
 */
void label_wire_muxes(const int chan_num,
                      const int seg_num,
                      const t_chan_seg_details* seg_details,
                      const int seg_type_index,
                      const int max_len,
                      const enum Direction dir,
                      const int max_chan_width,
                      const bool check_cb,
                      std::vector<int>& labels,
                      int* num_wire_muxes,
                      int* num_wire_muxes_cb_restricted);

t_sblock_pattern alloc_sblock_pattern_lookup(const DeviceGrid& grid,
                                             const t_chan_width& nodes_per_chan);

void load_sblock_pattern_lookup(const int i,
                                const int j,
                                const DeviceGrid& grid,
                                const t_chan_width& nodes_per_chan,
                                const t_chan_details& chan_details_x,
                                const t_chan_details& chan_details_y,
                                const int Fs,
                                const enum e_switch_block_type switch_block_type,
                                t_sblock_pattern& sblock_pattern);

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

void dump_seg_details(const t_chan_seg_details* seg_details,
                      int max_chan_width,
                      const char* fname);
void dump_seg_details(const t_chan_seg_details* seg_details,
                      int max_chan_width,
                      FILE* fp);
void dump_chan_details(const t_chan_details& chan_details_x,
                       const t_chan_details& chan_details_y,
                       const t_chan_width* nodes_per_chan,
                       const DeviceGrid& grid,
                       const char* fname);
void dump_sblock_pattern(const t_sblock_pattern& sblock_pattern,
                         int max_chan_width,
                         const DeviceGrid& grid,
                         const char* fname);

void dump_track_to_pin_map(t_track_to_pin_lookup& track_to_pin_map,
                           const std::vector<t_physical_tile_type>& types,
                           int max_chan_width,
                           FILE* fp);

inline int get_chan_width(enum e_side side, const t_chan_width& nodes_per_channel);
