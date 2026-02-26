#pragma once

#include <vector>

#include "rr_graph_type.h"
#include "rr_types.h"
#include "device_grid.h"

bool is_cblock(const int chan,
               const int seg,
               const int track,
               const t_chan_seg_details* seg_details,
               const std::vector<int>& seg_dimension_cuts);

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
