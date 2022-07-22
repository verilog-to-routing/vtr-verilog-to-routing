#ifndef RR_GRAPH2_H
#define RR_GRAPH2_H
#include <vector>

#include "build_switchblocks.h"
#include "rr_graph_fwd.h"
#include "rr_graph_util.h"
#include "rr_graph_view.h"
#include "rr_graph_builder.h"
#include "rr_types.h"
#include "device_grid.h"

/******************* Types shared by rr_graph2 functions *********************/

/* [0..grid.width()-1][0..grid.width()][0..3 (From side)] \
 * [0..3 (To side)][0...max_chan_width][0..3 (to_mux,to_trac,alt_mux,alt_track)] 
 * originally initialized to UN_SET until alloc_and_load_sb is called */
typedef vtr::NdMatrix<short, 6> t_sblock_pattern;

/* This map is used to get indices w.r.t segment_inf_x or segment_inf_y based on parallel_axis of a segment, 
 * from indices w.r.t the **unified** segment vector, segment_inf in devices context which stores all segments 
 * regardless of their axis. (see get_parallel_segs for more details)*/
typedef std::unordered_multimap<size_t, std::pair<size_t, e_parallel_axis>> t_unified_to_parallel_seg_index;

/******************* Subroutines exported by rr_graph2.c *********************/

void alloc_and_load_rr_node_indices(RRGraphBuilder& rr_graph_builder,
                                    const t_chan_width* nodes_per_chan,
                                    const DeviceGrid& grid,
                                    int* index,
                                    const t_chan_details& chan_details_x,
                                    const t_chan_details& chan_details_y);

bool verify_rr_node_indices(const DeviceGrid& grid,
                            const RRGraphView& rr_graph,
                            const t_rr_graph_storage& rr_nodes);

//Returns all x-channel or y-channel wires at the specified location
std::vector<int> get_rr_node_chan_wires_at_location(const t_rr_node_indices& L_rr_node_indices,
                                                    t_rr_type rr_type,
                                                    int x,
                                                    int y);

//Return the first rr node of the specified type and coordinates
// For non-IPIN/OPIN types 'side' is ignored
int get_rr_node_index(const t_rr_node_indices& L_rr_node_indices,
                      int x,
                      int y,
                      t_rr_type rr_type,
                      int ptc,
                      e_side side = NUM_SIDES);

int find_average_rr_node_index(int device_width,
                               int device_height,
                               t_rr_type rr_type,
                               int ptc,
                               const t_rr_node_indices& L_rr_node_indices);

t_seg_details* alloc_and_load_seg_details(int* max_chan_width,
                                          const int max_len,
                                          const std::vector<t_segment_inf>& segment_inf,
                                          const bool use_full_seg_groups,
                                          const enum e_directionality directionality,
                                          int* num_seg_details = nullptr);

void alloc_and_load_chan_details(const DeviceGrid& grid,
                                 const t_chan_width* nodes_per_chan,
                                 const int num_seg_details_x,
                                 const int num_seg_details_y,
                                 const t_seg_details* seg_details_x,
                                 const t_seg_details* seg_details_y,
                                 t_chan_details& chan_details_x,
                                 t_chan_details& chan_details_y);
t_chan_details init_chan_details(const DeviceGrid& grid,
                                 const t_chan_width* nodes_per_chan,
                                 const int num_seg_details,
                                 const t_seg_details* seg_details,
                                 const enum e_parallel_axis seg_details_type);
void adjust_chan_details(const DeviceGrid& grid,
                         const t_chan_width* nodes_per_chan,
                         t_chan_details& chan_details_x,
                         t_chan_details& chan_details_y);
void adjust_seg_details(const int x,
                        const int y,
                        const DeviceGrid& grid,
                        const t_chan_width* nodes_per_chan,
                        t_chan_details& chan_details,
                        const enum e_parallel_axis seg_details_type);

void free_chan_details(t_chan_details& chan_details_x,
                       t_chan_details& chan_details_y);

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

int get_bidir_opin_connections(RRGraphBuilder& rr_graph_builder,
                               const int i,
                               const int j,
                               const int ipin,
                               RRNodeId from_rr_node,
                               t_rr_edge_info_set& rr_edges_to_create,
                               const t_pin_to_track_lookup& opin_to_track_map,
                               const t_chan_details& chan_details_x,
                               const t_chan_details& chan_details_y);

int get_unidir_opin_connections(RRGraphBuilder& rr_graph_builder,
                                const int chan,
                                const int seg,
                                int Fc,
                                const int seg_type_index,
                                const t_rr_type chan_type,
                                const t_chan_seg_details* seg_details,
                                RRNodeId from_rr_node,
                                t_rr_edge_info_set& rr_edges_to_create,
                                vtr::NdMatrix<int, 3>& Fc_ofs,
                                const int max_len,
                                const t_chan_width& nodes_per_chan,
                                bool* Fc_clipped);

int get_track_to_pins(RRGraphBuilder& rr_graph_builder,
                      int seg,
                      int chan,
                      int track,
                      int tracks_per_chan,
                      RRNodeId from_rr_node,
                      t_rr_edge_info_set& rr_edges_to_create,
                      const t_track_to_pin_lookup& track_to_pin_lookup,
                      const t_chan_seg_details* seg_details,
                      enum e_rr_type chan_type,
                      int chan_length,
                      int wire_to_ipin_switch,
                      enum e_directionality directionality);

int get_track_to_tracks(RRGraphBuilder& rr_graph_builder,
                        const int from_chan,
                        const int from_seg,
                        const int from_track,
                        const t_rr_type from_type,
                        const int to_seg,
                        const t_rr_type to_type,
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
                        const enum e_directionality directionality,
                        const vtr::NdMatrix<std::vector<int>, 3>& switch_block_conn,
                        t_sb_connection_map* sb_conn_map);

t_sblock_pattern alloc_sblock_pattern_lookup(const DeviceGrid& grid,
                                             t_chan_width* nodes_per_chan);

void load_sblock_pattern_lookup(const int i,
                                const int j,
                                const DeviceGrid& grid,
                                const t_chan_width* nodes_per_chan,
                                const t_chan_details& chan_details_x,
                                const t_chan_details& chan_details_y,
                                const int Fs,
                                const enum e_switch_block_type switch_block_type,
                                t_sblock_pattern& sblock_pattern);

std::vector<t_segment_inf> get_parallel_segs(const std::vector<t_segment_inf>& segment_inf,
                                             t_unified_to_parallel_seg_index& seg_index_map,
                                             enum e_parallel_axis parallel_axis);

int get_parallel_seg_index(const int abs,
                           const t_unified_to_parallel_seg_index& index_map,
                           const e_parallel_axis parallel_axis);

std::unique_ptr<int[]> get_ordered_seg_track_counts(const std::vector<t_segment_inf>& segment_inf_x,
                                                    const std::vector<t_segment_inf>& segment_inf_y,
                                                    const std::vector<t_segment_inf>& segment_inf,
                                                    const std::unique_ptr<int[]>& segment_sets_x,
                                                    const std::unique_ptr<int[]>& segment_sets_y);

std::unique_ptr<int[]> get_seg_track_counts(const int num_sets,
                                            const std::vector<t_segment_inf>& segment_inf,
                                            const bool use_full_seg_groups);

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

void add_to_rr_node_indices(t_rr_node_indices& rr_node_indices, const t_rr_graph_storage& rr_nodes, int inode);

void insert_at_ptc_index(std::vector<int>& rr_indices, int ptc, int inode);

inline int get_chan_width(enum e_side side, const t_chan_width* nodes_per_channel);
#endif
