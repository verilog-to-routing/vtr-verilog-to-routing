#ifndef VPR_UTILS_H
#define VPR_UTILS_H

#include <vector>
#include <regex>
#include "vpr_types.h"
#include "atom_netlist.h"
#include "clustered_netlist.h"
#include "netlist.h"
#include "vtr_vector.h"
#include "arch_util.h"
class DeviceGrid;

const t_model* find_model(const t_model* models, const std::string& name, bool required = true);
const t_model_ports* find_model_port(const t_model* model, const std::string& name, bool required = true);

void print_tabs(FILE* fpout, int num_tab);

bool is_clb_external_pin(ClusterBlockId blk_id, int pb_pin_id);

bool is_opin(int ipin, t_type_ptr type);

bool is_input_type(t_type_ptr type);
bool is_output_type(t_type_ptr type);
bool is_io_type(t_type_ptr type);
bool is_empty_type(t_type_ptr type);

int get_unique_pb_graph_node_id(const t_pb_graph_node* pb_graph_node);

void get_class_range_for_block(const ClusterBlockId blk_id, int* class_low, int* class_high);

void get_pin_range_for_block(const ClusterBlockId blk_id,
                             int* pin_low,
                             int* pin_high);

void sync_grid_to_blocks();

//Returns the name of the pin_index'th pin on the specified block type
std::string block_type_pin_index_to_name(t_type_ptr type, int pin_index);

//Returns the name of the class_index'th pin class on the specified block type
std::vector<std::string> block_type_class_index_to_pin_names(t_type_ptr type, int class_index);

//Returns a user-friendly architectural identifier for the specified RR node
std::string rr_node_arch_name(int inode);

/**************************************************************
 * Intra-Logic Block Utility Functions
 **************************************************************/

//Class for looking up pb graph pins from block pin indicies
class IntraLbPbPinLookup {
  public:
    IntraLbPbPinLookup(t_type_descriptor* block_types, int num_block_types);
    IntraLbPbPinLookup(const IntraLbPbPinLookup& rhs);
    IntraLbPbPinLookup& operator=(IntraLbPbPinLookup rhs);
    ~IntraLbPbPinLookup();

    //Returns the pb graph pin associated with the specified type (index into block types array) and
    // pb pin index (index into block_pb().pb_route)
    const t_pb_graph_pin* pb_gpin(int itype, int ipin) const;

    friend void swap(IntraLbPbPinLookup& lhs, IntraLbPbPinLookup& rhs);

  private:
    t_type_descriptor* block_types_;
    int num_types_;

    t_pb_graph_pin*** intra_lb_pb_pin_lookup_;
};

//Find the atom pins (driver or sinks) connected to the specified top-level CLB pin
std::vector<AtomPinId> find_clb_pin_connected_atom_pins(ClusterBlockId clb, int clb_pin, const IntraLbPbPinLookup& pb_gpin_lookup);

//Find the atom pin driving to the specified top-level CLB pin
AtomPinId find_clb_pin_driver_atom_pin(ClusterBlockId clb, int clb_pin, const IntraLbPbPinLookup& pb_gpin_lookup);

//Find the atom pins driven by the specified top-level CLB pin
std::vector<AtomPinId> find_clb_pin_sink_atom_pins(ClusterBlockId clb, int clb_pin, const IntraLbPbPinLookup& pb_gpin_lookup);

std::tuple<ClusterNetId, int, int> find_pb_route_clb_input_net_pin(ClusterBlockId clb, int sink_pb_route_id);

//Return the pb pin index corresponding to the pin clb_pin on block clb,
//acounting for the effect of 'z' position > 0.
//
//  Note that a CLB pin index does not (neccessarily) map directly to the pb_route index representing the first stage
//  of internal routing in the block, since a block may have capacity > 1 (e.g. IOs)
//
//  In the clustered netlist blocks with capacity > 1 may have their 'z' position > 0, and their clb pin indicies offset
//  by the number of pins on the type (c.f. post_place_sync()).
//
//  This offset is not mirrored in the t_pb or pb graph, so we need to recover the basic pin index before processing
//  further -- which is what this function does.
int find_clb_pb_pin(ClusterBlockId clb, int clb_pin);

//Return the clb_pin corresponding to the pb_pin on the specified block
int find_pb_pin_clb_pin(ClusterBlockId clb, int pb_pin);

//Returns the port matching name within pb_gnode
const t_port* find_pb_graph_port(const t_pb_graph_node* pb_gnode, std::string port_name);

//Returns the graph pin matching name at pin index
const t_pb_graph_pin* find_pb_graph_pin(const t_pb_graph_node* pb_gnode, std::string port_name, int index);

AtomPinId find_atom_pin(ClusterBlockId blk_id, const t_pb_graph_pin* pb_gpin);

//Returns the block type matching name, or nullptr (if not found)
t_type_descriptor* find_block_type_by_name(std::string name, t_type_descriptor* types, int num_types);

//Returns the block type which is most common in the device grid
t_type_ptr find_most_common_block_type(const DeviceGrid& grid);

//Parses a block_name.port[x:y] (e.g. LAB.data_in[3:10]) pin range specification, if no pin range is specified
//looks-up the block port and fills in the full range
InstPort parse_inst_port(std::string str);

int find_pin_class(t_type_ptr type, std::string port_name, int pin_index_in_port, e_pin_type pin_type);

int find_pin(t_type_ptr type, std::string port_name, int pin_index_in_port);

//Returns the block type which is most likely the logic block
t_type_ptr infer_logic_block_type(const DeviceGrid& grid);

//Returns true if the specified block type contains the specified blif model name
bool block_type_contains_blif_model(t_type_ptr type, std::string blif_model_name);

//Returns true of a pb_type (or it's children) contain the specified blif model name
bool pb_type_contains_blif_model(const t_pb_type* pb_type, const std::string& blif_model_name);

int get_max_primitives_in_pb_type(t_pb_type* pb_type);
int get_max_depth_of_pb_type(t_pb_type* pb_type);
int get_max_nets_in_pb_type(const t_pb_type* pb_type);
bool primitive_type_feasible(AtomBlockId blk_id, const t_pb_type* cur_pb_type);
t_pb_graph_pin* get_pb_graph_node_pin_from_model_port_pin(const t_model_ports* model_port, const int model_pin, const t_pb_graph_node* pb_graph_node);
const t_pb_graph_pin* find_pb_graph_pin(const AtomNetlist& netlist, const AtomLookup& netlist_lookup, const AtomPinId pin_id);
t_pb_graph_pin* get_pb_graph_node_pin_from_block_pin(ClusterBlockId iblock, int ipin);
t_pb_graph_pin** alloc_and_load_pb_graph_pin_lookup_from_index(t_type_ptr type);
void free_pb_graph_pin_lookup_from_index(t_pb_graph_pin** pb_graph_pin_lookup_from_type);
vtr::vector<ClusterBlockId, t_pb**> alloc_and_load_pin_id_to_pb_mapping();
void free_pin_id_to_pb_mapping(vtr::vector<ClusterBlockId, t_pb**>& pin_id_to_pb_mapping);

float compute_primitive_base_cost(const t_pb_graph_node* primitive);
int num_ext_inputs_atom_block(AtomBlockId blk_id);

void get_port_pin_from_blk_pin(int blk_type_index, int blk_pin, int* port, int* port_pin);
void free_port_pin_from_blk_pin();

void get_blk_pin_from_port_pin(int blk_type_index, int port, int port_pin, int* blk_pin);
void free_blk_pin_from_port_pin();

void alloc_and_load_idirect_from_blk_pin(t_direct_inf* directs, int num_directs, int*** idirect_from_blk_pin, int*** direct_type_from_blk_pin);

void parse_direct_pin_name(char* src_string, int line, int* start_pin_index, int* end_pin_index, char* pb_type_name, char* port_name);

void free_pb_stats(t_pb* pb);
void free_pb(t_pb* pb);
void revalid_molecules(const t_pb* pb, const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules);

void print_switch_usage();
void print_usage_by_wire_length();

AtomBlockId find_memory_sibling(const t_pb* pb);

void place_sync_external_block_connections(ClusterBlockId iblk);

int max_pins_per_grid_tile();

void pretty_print_uint(const char* prefix, size_t value, int num_digits, int scientific_precision);
void pretty_print_float(const char* prefix, double value, int num_digits, int scientific_precision);
#endif
