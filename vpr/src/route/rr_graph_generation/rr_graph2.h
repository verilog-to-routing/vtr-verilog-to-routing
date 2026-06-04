#pragma once

#include <vector>

#include "rr_graph_type.h"
#include "rr_types.h"
#include "device_grid.h"

/// @brief Check if a track drives a connection block at a given location.
///
/// Maps (@p chan, @p seg) to an offset along the wire containing that
/// location and checks the wire's connection-block pattern via @c seg_details[track].cb().
///
/// @param chan               Routing channel index.
/// @param seg                Segment coordinate along the channel.
/// @param track              Routing track index within the channel.
/// @param seg_details        Per-track segment details for the channel.
/// @param seg_dimension_cuts Positions where interposer cuts split the routing channel.
bool is_cblock(const int chan,
               const int seg,
               const int track,
               const t_chan_seg_details* seg_details,
               const std::vector<int>& seg_dimension_cuts);

/// @brief Check if a track connects to a switch block at a given location.
///
/// Maps (@p wire_seg, @p sb_seg) to an offset along the wire and checks the
/// wire's switch-block pattern via @c seg_details[track].sb().
///
/// @param chan            Routing channel index.
/// @param wire_seg        Segment coordinate along the routing channel.
/// @param sb_seg          Switch-block grid coordinate along the routing
///                        channel (same axis as wire_seg).
/// @param track           Routing track index within the channel.
/// @param seg_details        Per-track segment details for the channel.
/// @param directionality     Routing wires' directionality (bidirectional or unidirectional).
/// @param seg_dimension_cuts Positions where interposer cuts split the routing channel.
bool is_sblock(const int chan,
               int wire_seg,
               const int sb_seg,
               const int track,
               const t_chan_seg_details* seg_details,
               const e_directionality directionality,
               const std::vector<int>& seg_dimension_cuts);

/**
 * @brief Identifies and labels all mux endpoints at a given channel segment coordinate.
 *
 * This routine scans all routing tracks within a channel segment and collects the
 * track indices corresponding to valid mux endpoints that can be driven by OPINs at
 * that coordinate. The resulting list of eligible tracks is returned in natural
 * (increasing) track order.
 *
 * @param chan_num            Routing channel index.
 * @param seg_num             Segment coordinate along the channel.
 * @param seg_details         Per-track segment details for the channel.
 * @param seg_type_index      Segment type index (seg_details[track].index()) to
 *                            consider; UNDEFINED includes all segment types.
 * @param seg_dimension_cuts  Positions where interposer cuts split the routing channel.
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
                      int* num_wire_muxes_cb_restricted,
                      const std::vector<int>& seg_dimension_cuts);

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
