/**
 * Jason Luu
 * July 17, 2009
 * pb_graph creates the internal routing edges that join together the different
 * pb_types modes within a pb_type
 *
 * Daniel Chen
 * July, 2013
 * Pb_graph checks implemented:
 *		- Double edges between two pins
 *		- Input pin equivalence mismatch with architecture file (warning)
 */

#include <cstdio>
#include <cstring>
#include <queue>

#include "vtr_util.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"
#include "vtr_token.h"

#include "vpr_error.h"

#include "physical_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "pb_type_graph.h"
#include "pb_type_graph_annotations.h"
#include "cluster_feasibility_filter.h"

/* variable global to this section that indexes each pb graph pin within a cluster */
static vtr::t_linked_vptr* edges_head;
static vtr::t_linked_vptr* num_edges_head;

/* TODO: Software engineering decision needed: Move this file to libarch?
 *
 */

static int check_pb_graph();
static void alloc_and_load_pb_graph(t_pb_graph_node* pb_graph_node,
                                    t_pb_graph_node* parent_pb_graph_node,
                                    t_pb_type* pb_type,
                                    const int index,
                                    const int flat_index,
                                    bool load_power_structures,
                                    int& pin_count_in_cluster,
                                    int& primitive_num);

static void alloc_and_load_pb_graph_pin_sinks(t_pb_graph_node* pb_graph_node);

/* Assign a unique id to each IPIN/OPIN pin at logical block level (i.e. fill pin_logical_num_to_pb_pin_mapping in logical_block)*/
static void set_pins_logical_num(t_logical_block_type* logical_block);

/* Return all pb_graph_nodes, in all modes and sub-blocks, contained in the given logical_block */
static std::vector<const t_pb_graph_node*> get_primitive_pb_graph_nodes(t_logical_block_type_ptr logical_block);

/* Add SRC/SINK pins for all the ports of the blocks in the logical block */
static void add_primitive_logical_classes(t_logical_block_type* logical_block);

/* Add SRC/SINK pins for the given ports */
static int add_port_logical_classes(t_logical_block_type* logical_block,
                                    t_pb_graph_pin** pb_graph_pins,
                                    int num_ports,
                                    int* num_pins);

static void alloc_and_load_mode_interconnect(t_pb_graph_node* pb_graph_parent_node,
                                             t_pb_graph_node** pb_graph_children_nodes,
                                             const t_mode* mode,
                                             bool load_power_structures);

static void store_pin_sinks_edge_id(t_pb_graph_node* pb_graph_node);

static bool realloc_and_load_pb_graph_pin_ptrs_at_var(const int line_num,
                                                      const t_pb_graph_node* pb_graph_parent_node,
                                                      t_pb_graph_node** pb_graph_children_nodes,
                                                      const bool interconnect_error_check,
                                                      const bool is_input_to_interc,
                                                      const Tokens& tokens,
                                                      int* token_index,
                                                      int* num_pins,
                                                      t_pb_graph_pin*** pb_graph_pins);

static t_pb_graph_pin* get_pb_graph_pin_from_name(const char* port_name,
                                                  const t_pb_graph_node* pb,
                                                  const int pin);

static void alloc_and_load_complete_interc_edges(t_interconnect* interconnect,
                                                 t_pb_graph_pin*** input_pb_graph_node_pin_ptrs,
                                                 const int num_input_sets,
                                                 const int* num_input_ptrs,
                                                 t_pb_graph_pin*** output_pb_graph_node_pin_ptrs,
                                                 const int num_output_sets,
                                                 const int* num_output_ptrs);

static void alloc_and_load_direct_interc_edges(t_interconnect* interconnect,
                                               t_pb_graph_pin*** input_pb_graph_node_pin_ptrs,
                                               const int num_input_sets,
                                               const int* num_input_ptrs,
                                               t_pb_graph_pin*** output_pb_graph_node_pin_ptrs,
                                               const int num_output_sets,
                                               const int* num_output_ptrs);

static void alloc_and_load_mux_interc_edges(t_interconnect* interconnect,
                                            t_pb_graph_pin*** input_pb_graph_node_pin_ptrs,
                                            const int num_input_sets,
                                            const int* num_input_ptrs,
                                            t_pb_graph_pin*** output_pb_graph_node_pin_ptrs,
                                            const int num_output_sets,
                                            const int* num_output_ptrs);

static void echo_pb_rec(const t_pb_graph_node* pb, const int level, FILE* fp);
static void echo_pb_pins(t_pb_graph_pin** pb_graph_pins, const int num_ports, const int level, FILE* fp);

static void alloc_and_load_interconnect_pins(t_interconnect_pins* interc_pins,
                                             t_interconnect* interconnect,
                                             t_pb_graph_pin*** input_pins,
                                             int num_input_sets,
                                             int* num_input_pins,
                                             t_pb_graph_pin*** output_pins,
                                             int num_output_sets,
                                             int* num_output_pins);

/**
 * @brief Traverses all pb_nodes in the pb_graph to check for architectural errors.
 *
 * This subroutine recursively visits each pb_node within the given pb_graph and
 * checks for various misspecified architectures, such as repeated edges between
 * two pins and logically-equivalent pin mismatches.
 *
 * @param pb_graph_node Pointer to the pb_graph_node to start the traversal from.
 */
static void check_pb_node_rec(const t_pb_graph_node* pb_graph_node);

/**
 * @brief Checks for repeated edges connected to a single pb_graph_pin.
 *
 * This subroutine traverses all the incoming edges associated with a single pb_graph_pin
 * and checks for repeated edges connected to it. Only incoming edges are checked,
 * since all edges must land on a pin. By traversing all the incoming edges of all pins,
 * all edges in the graph are checked exactly once.
 *
 * @param cur_pin Pointer to the pb_graph_pin whose incoming edges will be checked.
 */
static void check_repeated_edges_at_pb_pin(t_pb_graph_pin* cur_pin);

/**
 * @brief Less-than operator for t_pb_graph_edge_comparator.
 *
 * Defines a comparison operator to allow t_pb_graph_edge_comparator objects
 * to be used as keys in associative containers (such as std::map).
 * This is used for detecting repeated edges in the pb_graph.
 *
 * @return true if edge1 is considered less than edge2, false otherwise.
 */
static bool operator<(const t_pb_graph_edge_comparator& edge1,
                      const t_pb_graph_edge_comparator& edge2);

/**
 * @brief Checks that all pins in a logically-equivalent input port connect to the exact same pins.
 *
 * This subroutine verifies that for an input port declared as logically-equivalent,
 * all of its pins connect to the exact same set of pins (not just a subset).
 * For the first pin in the port (i_pin == 0), its outgoing edges are used as the reference,
 * and subsequent pins are compared against this set.
 *
 * @return true if all pins in the logically-equivalent port connect to the exact same pins, false otherwise.
 */
static bool check_input_pins_equivalence(const t_pb_graph_pin* cur_pin,
                                         int i_pin,
                                         std::map<int, int>& edges_map,
                                         int* line_num);

/* computes the index of a pb graph node at its level of the pb hierarchy */
static int compute_flat_index_for_child_node(int num_children_of_type,
                                             int parent_flat_index,
                                             int child_index);

/**
 * Allocate memory into types and load the pb graph with interconnect edges
 */
void alloc_and_load_all_pb_graphs(bool load_power_structures, bool is_flat) {
    int errors;
    edges_head = nullptr;
    num_edges_head = nullptr;
    auto& device_ctx = g_vpr_ctx.mutable_device();

    for (auto& type : device_ctx.logical_block_types) {
        if (type.pb_type) {
            type.pb_graph_head = new t_pb_graph_node();
            int pin_count_in_cluster = 0;
            int primitive_num = 0;
            alloc_and_load_pb_graph(type.pb_graph_head,
                                    nullptr,
                                    type.pb_type,
                                    0,
                                    0,
                                    load_power_structures,
                                    pin_count_in_cluster,
                                    primitive_num);
            type.pb_graph_head->total_pb_pins = pin_count_in_cluster;
            load_pin_classes_in_pb_graph_head(type.pb_graph_head);
            if (is_flat) {
                alloc_and_load_pb_graph_pin_sinks(type.pb_graph_head);
                set_pins_logical_num(&type);
                add_primitive_logical_classes(&type);
            }
        } else {
            type.pb_graph_head = nullptr;
            VTR_ASSERT(&type == device_ctx.EMPTY_LOGICAL_BLOCK_TYPE);
        }
    }

    errors = check_pb_graph();
    if (errors > 0) {
        VTR_LOG_ERROR("in pb graph");
        exit(1);
    }
    for (auto& type : device_ctx.logical_block_types) {
        if (type.pb_type) {
            load_pb_graph_pin_to_pin_annotations(type.pb_graph_head);
        }
    }
}

/**
 * Print out the pb_type graph
 */
void echo_pb_graph(char* filename) {
    FILE* fp;

    fp = vtr::fopen(filename, "w");

    fprintf(fp, "Physical Blocks Graph\n");
    fprintf(fp, "--------------------------------------------\n\n");

    const auto& device_ctx = g_vpr_ctx.device();
    for (const t_logical_block_type& type : device_ctx.logical_block_types) {
        fprintf(fp, "type %s\n", type.name.c_str());
        if (type.pb_graph_head)
            echo_pb_rec(type.pb_graph_head, 1, fp);
    }

    fclose(fp);
}

/**
 * check pb_type graph and return the number of errors
 */
static int check_pb_graph() {
    int num_errors;
    /* TODO: Error checks to do
     * 1.  All pin and edge connections are bidirectional and match each other
     * 3.  All ports are unique in a pb_type
     * 4.  Number of pb of a pb_type in graph is the same as requested number
     * 5.  All pins are connected to edges (warning)
     */
    num_errors = 0;
    auto& device_ctx = g_vpr_ctx.device();
    for (auto& type : device_ctx.logical_block_types) {
        if (type.pb_type) {
            check_pb_node_rec(type.pb_graph_head);
        }
    }
    return num_errors;
}

static void alloc_and_load_pb_graph(t_pb_graph_node* pb_graph_node,
                                    t_pb_graph_node* parent_pb_graph_node,
                                    t_pb_type* pb_type,
                                    const int index,
                                    const int flat_index,
                                    bool load_power_structures,
                                    int& pin_count_in_cluster,
                                    int& primitive_num) {
    int i, j, k, i_input, i_output, i_clockport;

    pb_graph_node->placement_index = index;
    pb_graph_node->pb_type = pb_type;
    pb_graph_node->parent_pb_graph_node = parent_pb_graph_node;

    pb_graph_node->num_input_ports = 0;
    pb_graph_node->num_output_ports = 0;
    pb_graph_node->num_clock_ports = 0;

    pb_graph_node->total_primitive_count = 0;
    pb_graph_node->flat_site_index = 0;

    /* Generate ports for pb graph node */
    for (i = 0; i < pb_type->num_ports; i++) {
        if (pb_type->ports[i].type == IN_PORT && !pb_type->ports[i].is_clock) {
            pb_graph_node->num_input_ports++;
        } else if (pb_type->ports[i].type == OUT_PORT) {
            pb_graph_node->num_output_ports++;
        } else {
            VTR_ASSERT(pb_type->ports[i].is_clock && pb_type->ports[i].type == IN_PORT);
            pb_graph_node->num_clock_ports++;
        }
    }

    if (pb_graph_node->num_input_ports > 0) {
        pb_graph_node->num_input_pins = new int[pb_graph_node->num_input_ports]{0};
        pb_graph_node->input_pins = new t_pb_graph_pin* [pb_graph_node->num_input_ports] { nullptr };
    }

    if (pb_graph_node->num_output_ports > 0) {
        pb_graph_node->num_output_pins = new int[pb_graph_node->num_output_ports]{0};
        pb_graph_node->output_pins = new t_pb_graph_pin* [pb_graph_node->num_output_ports] { nullptr };
    }

    if (pb_graph_node->num_clock_ports > 0) {
        pb_graph_node->num_clock_pins = new int[pb_graph_node->num_clock_ports]{0};
        pb_graph_node->clock_pins = new t_pb_graph_pin* [pb_graph_node->num_clock_ports] { nullptr };
    }

    i_input = i_output = i_clockport = 0;
    pb_graph_node->pin_num_range.low = pin_count_in_cluster;
    for (i = 0; i < pb_type->num_ports; i++) {
        if (pb_type->ports[i].model_port) {
            VTR_ASSERT(pb_type->is_primitive());
        } else {
            VTR_ASSERT(pb_type->num_modes != 0 || pb_type->ports[i].is_clock);
        }
        if (pb_type->ports[i].type == IN_PORT && !pb_type->ports[i].is_clock) {
            pb_graph_node->input_pins[i_input] = new t_pb_graph_pin[pb_type->ports[i].num_pins];
            pb_graph_node->num_input_pins[i_input] = pb_type->ports[i].num_pins;
            for (j = 0; j < pb_type->ports[i].num_pins; j++) {
                pb_graph_node->input_pins[i_input][j].pin_number = j;
                pb_graph_node->input_pins[i_input][j].port = &pb_type->ports[i];
                pb_graph_node->input_pins[i_input][j].parent_node = pb_graph_node;
                pb_graph_node->input_pins[i_input][j].pin_count_in_cluster = pin_count_in_cluster;
                if (pb_graph_node->pb_type->blif_model != nullptr) {
                    if (strcmp(pb_graph_node->pb_type->blif_model, LogicalModels::MODEL_OUTPUT) == 0) {
                        pb_graph_node->input_pins[i_input][j].type = PB_PIN_OUTPAD;
                    } else if (pb_graph_node->num_clock_ports != 0) {
                        pb_graph_node->input_pins[i_input][j].type = PB_PIN_SEQUENTIAL;
                    } else {
                        pb_graph_node->input_pins[i_input][j].type = PB_PIN_TERMINAL;
                    }
                }
                pin_count_in_cluster++;
            }
            i_input++;
        } else if (pb_type->ports[i].type == OUT_PORT) {
            pb_graph_node->output_pins[i_output] = new t_pb_graph_pin[pb_type->ports[i].num_pins];
            pb_graph_node->num_output_pins[i_output] = pb_type->ports[i].num_pins;
            for (j = 0; j < pb_type->ports[i].num_pins; j++) {
                pb_graph_node->output_pins[i_output][j].pin_number = j;
                pb_graph_node->output_pins[i_output][j].port = &pb_type->ports[i];
                pb_graph_node->output_pins[i_output][j].parent_node = pb_graph_node;
                pb_graph_node->output_pins[i_output][j].pin_count_in_cluster = pin_count_in_cluster;
                if (pb_graph_node->pb_type->blif_model != nullptr) {
                    if (strcmp(pb_graph_node->pb_type->blif_model, LogicalModels::MODEL_INPUT) == 0) {
                        pb_graph_node->output_pins[i_output][j].type = PB_PIN_INPAD;
                    } else if (pb_graph_node->num_clock_ports != 0) {
                        pb_graph_node->output_pins[i_output][j].type = PB_PIN_SEQUENTIAL;
                    } else {
                        pb_graph_node->output_pins[i_output][j].type = PB_PIN_TERMINAL;
                    }
                }
                pin_count_in_cluster++;
            }
            i_output++;
        } else {
            VTR_ASSERT(pb_type->ports[i].is_clock && pb_type->ports[i].type == IN_PORT);
            pb_graph_node->clock_pins[i_clockport] = new t_pb_graph_pin[pb_type->ports[i].num_pins];
            pb_graph_node->num_clock_pins[i_clockport] = pb_type->ports[i].num_pins;
            for (j = 0; j < pb_type->ports[i].num_pins; j++) {
                pb_graph_node->clock_pins[i_clockport][j].pin_number = j;
                pb_graph_node->clock_pins[i_clockport][j].port = &pb_type->ports[i];
                pb_graph_node->clock_pins[i_clockport][j].parent_node = pb_graph_node;
                pb_graph_node->clock_pins[i_clockport][j].pin_count_in_cluster = pin_count_in_cluster;
                if (pb_graph_node->pb_type->blif_model != nullptr) {
                    pb_graph_node->clock_pins[i_clockport][j].type = PB_PIN_CLOCK;
                }
                pin_count_in_cluster++;
            }
            i_clockport++;
        }
    }

    pb_graph_node->pin_num_range.high = (pin_count_in_cluster - 1);

    /* Power */
    if (load_power_structures) {
        pb_graph_node->pb_node_power = new t_pb_graph_node_power();
        pb_graph_node->pb_node_power->transistor_cnt_buffers = 0.;
        pb_graph_node->pb_node_power->transistor_cnt_interc = 0.;
        pb_graph_node->pb_node_power->transistor_cnt_pb_children = 0.;
    }

    if (pb_graph_node->is_primitive()) {
        pb_graph_node->primitive_num = primitive_num;
        primitive_num++;
    }

    /* Allocate and load child nodes for each mode and create interconnect in each mode */

    pb_graph_node->child_pb_graph_nodes = (t_pb_graph_node***)vtr::calloc(pb_type->num_modes, sizeof(t_pb_graph_node**));
    for (i = 0; i < pb_type->num_modes; i++) {
        pb_graph_node->child_pb_graph_nodes[i] = (t_pb_graph_node**)vtr::calloc(pb_type->modes[i].num_pb_type_children,
                                                                                sizeof(t_pb_graph_node*));
        for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
            pb_graph_node->child_pb_graph_nodes[i][j] = (t_pb_graph_node*)vtr::calloc(pb_type->modes[i].pb_type_children[j].num_pb, sizeof(t_pb_graph_node));
            int num_children_of_type = pb_type->modes[i].pb_type_children[j].num_pb;

            for (k = 0; k < num_children_of_type; k++) {
                int child_flat_index = compute_flat_index_for_child_node(num_children_of_type, flat_index, k);
                alloc_and_load_pb_graph(&pb_graph_node->child_pb_graph_nodes[i][j][k],
                                        pb_graph_node,
                                        &pb_type->modes[i].pb_type_children[j],
                                        k,
                                        child_flat_index,
                                        load_power_structures,
                                        pin_count_in_cluster,
                                        primitive_num);
            }
        }
    }

    pb_graph_node->interconnect_pins = new t_interconnect_pins*[pb_type->num_modes];

    for (i = 0; i < pb_type->num_modes; i++) {
        pb_graph_node->interconnect_pins[i] = nullptr;
        /* Create interconnect for mode */
        alloc_and_load_mode_interconnect(pb_graph_node,
                                         pb_graph_node->child_pb_graph_nodes[i],
                                         &pb_type->modes[i],
                                         load_power_structures);
    }

    // update the total number of primitives of that type
    if (pb_graph_node->is_primitive()) {
        int total_count = 1;
        auto pb_node = pb_graph_node;
        while (!pb_node->is_root()) {
            total_count *= pb_node->pb_type->num_pb;
            pb_node = pb_node->parent_pb_graph_node;
        }
        pb_graph_node->total_primitive_count = total_count;

        // if this is a primitive, then flat_index corresponds
        // to its index within all primitives of this type
        pb_graph_node->flat_site_index = flat_index;
    }
}

static void alloc_and_load_pb_graph_pin_sinks(t_pb_graph_node* pb_graph_node) {
    std::list<t_pb_graph_node*> pb_graph_node_list;

    pb_graph_node_list.push_back(pb_graph_node);

    while (!pb_graph_node_list.empty()) {
        auto curr_pb_graph_node = pb_graph_node_list.front();
        pb_graph_node_list.pop_front();
        store_pin_sinks_edge_id(curr_pb_graph_node);

        for (int mode_num = 0; mode_num < curr_pb_graph_node->pb_type->num_modes; mode_num++) {
            for (int child_pb_type_num = 0; child_pb_type_num < curr_pb_graph_node->pb_type->modes[mode_num].num_pb_type_children; child_pb_type_num++) {
                for (int child_pb_num = 0; child_pb_num < curr_pb_graph_node->pb_type->modes[mode_num].pb_type_children[child_pb_type_num].num_pb; child_pb_num++) {
                    pb_graph_node_list.push_back(&curr_pb_graph_node->child_pb_graph_nodes[mode_num][child_pb_type_num][child_pb_num]);
                }
            }
        }
    }
}

static void set_pins_logical_num(t_logical_block_type* logical_block) {
    /* Index of the pins located on the root-level block starts from 0 -
     * For other pins it starts from the roo_level pb_type.num_pins */
    auto root_pb_graph_node = logical_block->pb_graph_head;
    std::queue<t_pb_graph_node*> pb_graph_node_to_set_pin_index_q;

    std::unordered_map<int, t_pb_graph_pin*>& pb_pin_idx_map = logical_block->pin_logical_num_to_pb_pin_mapping;
    int offset = 0;

    pb_graph_node_to_set_pin_index_q.push(root_pb_graph_node);
    /* Iterated over all sub-blocks from the root-level block down to primitives */
    while (!pb_graph_node_to_set_pin_index_q.empty()) {
        auto curr_pb_graph_node = pb_graph_node_to_set_pin_index_q.front();
        pb_graph_node_to_set_pin_index_q.pop();
        auto curr_pb_type = curr_pb_graph_node->pb_type;

        /* There are three types of ports which can be defined for each block : Input - Output - Clock
         * In this for loop, we first iterate over the ports of the same type
         */
        for (int port_type = 0; port_type < 3; port_type++) {
            int num_ports;
            int* num_pins;
            t_pb_graph_pin** pins;
            if (port_type == 0) {
                /* Port type = Input */
                num_ports = curr_pb_graph_node->num_input_ports;
                num_pins = curr_pb_graph_node->num_input_pins;
                pins = curr_pb_graph_node->input_pins;
            } else if (port_type == 1) {
                /* Port type = Output */
                num_ports = curr_pb_graph_node->num_output_ports;
                num_pins = curr_pb_graph_node->num_output_pins;
                pins = curr_pb_graph_node->output_pins;
            } else {
                /* Port type = Clock */
                num_ports = curr_pb_graph_node->num_clock_ports;
                num_pins = curr_pb_graph_node->num_clock_pins;
                pins = curr_pb_graph_node->clock_pins;
            }
            for (int port_idx = 0; port_idx < num_ports; port_idx++) {
                for (int pin_idx = 0; pin_idx < num_pins[port_idx]; pin_idx++) {
                    auto curr_pb_graph_pin = &pins[port_idx][pin_idx];
                    pb_pin_idx_map.insert(std::make_pair(curr_pb_graph_pin->pin_count_in_cluster, curr_pb_graph_pin));
                }
            }
        }
        /* logical indices for the next block should be shifted by the number of pins seen so far. As a result, the given logical index will be unique in the logical_block level*/
        offset += curr_pb_type->num_pins;

        if (curr_pb_graph_node->is_root()) {
            VTR_ASSERT(offset == curr_pb_graph_node->pb_type->num_pins);
        }

        /* Push sub-blocks to the queue */
        for (int mode_idx = 0; mode_idx < curr_pb_type->num_modes; mode_idx++) {
            auto curr_mode = &curr_pb_type->modes[mode_idx];
            for (int pb_type_idx = 0; pb_type_idx < curr_mode->num_pb_type_children; pb_type_idx++) {
                int num_pb = curr_mode->pb_type_children[pb_type_idx].num_pb;
                for (int pb_idx = 0; pb_idx < num_pb; pb_idx++) {
                    auto child_pb_graph_node = &(curr_pb_graph_node->child_pb_graph_nodes[mode_idx][pb_type_idx][pb_idx]);
                    pb_graph_node_to_set_pin_index_q.push(child_pb_graph_node);
                }
            }
        }
    }
}

static std::vector<const t_pb_graph_node*> get_primitive_pb_graph_nodes(t_logical_block_type_ptr logical_block) {
    std::vector<const t_pb_graph_node*> pb_graph_nodes;

    std::list<const t_pb_graph_node*> pb_graph_node_q;
    pb_graph_node_q.push_back(logical_block->pb_graph_head);

    while (!pb_graph_node_q.empty()) {
        auto pb_graph_node = pb_graph_node_q.front();
        pb_graph_node_q.pop_front();
        if (pb_graph_node->is_primitive()) {
            pb_graph_nodes.push_back(pb_graph_node);
        }

        auto pb_type = pb_graph_node->pb_type;
        /* Iterating over different modes of the block */
        for (int mode_idx = 0; mode_idx < pb_graph_node->pb_type->num_modes; mode_idx++) {
            /* Iterating over different block types in the given mode */
            for (int pb_type_idx = 0; pb_type_idx < (pb_type->modes[mode_idx]).num_pb_type_children; pb_type_idx++) {
                int num_pb = pb_type->modes[mode_idx].pb_type_children[pb_type_idx].num_pb;
                /* Iterating over blocks of the given type and mode */
                for (int pb_idx = 0; pb_idx < num_pb; pb_idx++) {
                    const t_pb_graph_node* child_pb_graph_node = &(pb_graph_node->child_pb_graph_nodes[mode_idx][pb_type_idx][pb_idx]);
                    /* Add the child pb_pb_graph_node to the queue, so that this block and its children get added pb_graph_nodes later */
                    pb_graph_node_q.push_back(child_pb_graph_node);
                }
            }
        }
    }

    return pb_graph_nodes;
}

static void add_primitive_logical_classes(t_logical_block_type* logical_block) {
    auto pb_graph_nodes = get_primitive_pb_graph_nodes(logical_block);
    for (auto pb_graph_node : pb_graph_nodes) {
        int first_class_num = (int)logical_block->primitive_logical_class_inf.size();
        int num_added_classes = 0;
        /* There are three types of ports which can be defined for each block : Input - Output - Clock
         * In this for loop, we first iterate over the ports of the same type
         */
        for (int port_type = 0; port_type < 3; port_type++) {
            int num_ports;
            int* num_pins;
            t_pb_graph_pin** pb_graph_pins;
            if (port_type == 0) {
                num_ports = pb_graph_node->num_input_ports;
                num_pins = pb_graph_node->num_input_pins;
                pb_graph_pins = pb_graph_node->input_pins;

            } else if (port_type == 1) {
                num_ports = pb_graph_node->num_output_ports;
                num_pins = pb_graph_node->num_output_pins;
                pb_graph_pins = pb_graph_node->output_pins;

            } else {
                VTR_ASSERT(port_type == 2);
                num_ports = pb_graph_node->num_clock_ports;
                num_pins = pb_graph_node->num_clock_pins;
                pb_graph_pins = pb_graph_node->clock_pins;
            }
            num_added_classes += add_port_logical_classes(logical_block, pb_graph_pins, num_ports, num_pins);
        }
        logical_block->primitive_pb_graph_node_class_range.insert(std::make_pair(pb_graph_node, t_class_range(first_class_num,
                                                                                                              first_class_num + num_added_classes - 1)));
    }
}

static int add_port_logical_classes(t_logical_block_type* logical_block,
                                    t_pb_graph_pin** pb_graph_pins,
                                    int num_ports,
                                    int* num_pins) {
    int num_added_classes = 0;
    std::vector<t_class>& primitive_logical_class_inf = logical_block->primitive_logical_class_inf;

    for (int port_idx = 0; port_idx < num_ports; port_idx++) {
        if (num_pins[port_idx] == 0)
            continue;
        auto port = pb_graph_pins[port_idx][0].port;
        if (port->equivalent != PortEquivalence::NONE) {
            t_class class_inf;
            int class_num = (int)primitive_logical_class_inf.size();
            class_inf.num_pins = port->num_pins;
            class_inf.equivalence = port->equivalent;

            if (port->type == IN_PORT) {
                class_inf.type = RECEIVER;
            } else {
                VTR_ASSERT(port->type == OUT_PORT);
                class_inf.type = DRIVER;
            }

            for (int pin_idx = 0; pin_idx < num_pins[port_idx]; pin_idx++) {
                auto pb_graph_pin = &(pb_graph_pins[port_idx][pin_idx]);
                class_inf.pinlist.push_back(pb_graph_pin->pin_count_in_cluster);
                logical_block->primitive_pb_pin_to_logical_class_num_mapping.insert(std::make_pair(pb_graph_pin, class_num));
            }
            primitive_logical_class_inf.push_back(class_inf);
            num_added_classes++;
        } else {
            for (int pin_idx = 0; pin_idx < num_pins[port_idx]; pin_idx++) {
                t_class class_inf;
                int class_num = (int)primitive_logical_class_inf.size();
                class_inf.num_pins = 1;
                class_inf.equivalence = port->equivalent;

                if (port->type == IN_PORT) {
                    class_inf.type = RECEIVER;
                } else {
                    VTR_ASSERT(port->type == OUT_PORT);
                    class_inf.type = DRIVER;
                }

                auto pb_graph_pin = &(pb_graph_pins[port_idx][pin_idx]);
                class_inf.pinlist.push_back(pb_graph_pin->pin_count_in_cluster);
                logical_block->primitive_pb_pin_to_logical_class_num_mapping.insert(std::make_pair(pb_graph_pin, class_num));
                primitive_logical_class_inf.push_back(class_inf);
                num_added_classes++;
            }
        }
    }

    return num_added_classes;
}

void free_pb_graph_edges() {
    t_pb_graph_edge* edges;
    vtr::t_linked_vptr *cur, *cur_num;

    while (edges_head != nullptr) {
        cur = edges_head;
        cur_num = num_edges_head;
        edges = (t_pb_graph_edge*)cur->data_vptr;
        for (int i = 0; i < (intptr_t)cur_num->data_vptr; i++) {
            delete[] edges[i].input_pins;
            delete[] edges[i].output_pins;
            if (edges[i].pack_pattern_indices) {
                delete[] edges[i].pack_pattern_indices;
            }
            // if (edges[i].pack_pattern_names) {
            // vtr::free(edges[i].pack_pattern_names);
            // }
        }
        edges_head = edges_head->next;
        num_edges_head = num_edges_head->next;
        delete[] edges;
        delete cur_num;
        delete cur;
    }
}

static void alloc_and_load_interconnect_pins(t_interconnect_pins* interc_pins,
                                             t_interconnect* interconnect,
                                             t_pb_graph_pin*** input_pins,
                                             int num_input_sets,
                                             int* num_input_pins,
                                             t_pb_graph_pin*** output_pins,
                                             int num_output_sets,
                                             int* num_output_pins) {
    int set_idx;
    int pin_idx;
    int port_idx;
    int num_ports;

    interc_pins->interconnect = interconnect;

    switch (interconnect->type) {
        case DIRECT_INTERC: /* intentionally fallthrough */
            VTR_ASSERT(num_output_sets == 1);
            /* intentionally fallthrough */
            [[fallthrough]];
        case MUX_INTERC:
            if (!interconnect->interconnect_power->port_info_initialized) {
                for (set_idx = 0; set_idx < num_input_sets; set_idx++) {
                    VTR_ASSERT(num_input_pins[set_idx] == num_output_pins[0]);
                }
                interconnect->interconnect_power->num_pins_per_port = num_input_pins[0];
                interconnect->interconnect_power->num_input_ports = num_input_sets;
                interconnect->interconnect_power->num_output_ports = 1;

                /* No longer used - mux architectures are not configurable, the
                 * default is always assumed
                 * if (interconnect->mux_arch) {
                 * interconnect->mux_arch->num_inputs =
                 * interconnect->num_input_ports;
                 *
                 * mux_arch_fix_levels(interconnect->mux_arch);
                 *
                 * interconnect->mux_arch->mux_graph_head =
                 * alloc_and_load_mux_graph(interconnect->num_input_ports,
                 * interconnect->mux_arch->levels);
                 * }
                 */

                interconnect->interconnect_power->port_info_initialized = true;
            }

            interc_pins->input_pins = new t_pb_graph_pin**[num_input_sets];
            for (set_idx = 0; set_idx < num_input_sets; set_idx++) {
                interc_pins->input_pins[set_idx] = new t_pb_graph_pin*[interconnect->interconnect_power->num_pins_per_port];
            }

            interc_pins->output_pins = new t_pb_graph_pin**[1];
            interc_pins->output_pins[0] = new t_pb_graph_pin*[interconnect->interconnect_power->num_pins_per_port];

            for (pin_idx = 0; pin_idx < interconnect->interconnect_power->num_pins_per_port; pin_idx++) {
                for (set_idx = 0; set_idx < num_input_sets; set_idx++) {
                    interc_pins->input_pins[set_idx][pin_idx] = input_pins[set_idx][pin_idx];
                }
                interc_pins->output_pins[0][pin_idx] = output_pins[0][pin_idx];
            }

            break;
        case COMPLETE_INTERC:

            if (!interconnect->interconnect_power->port_info_initialized) {
                /* The code does not support bus-based crossbars, so all pins from all input sets
                 * connect to all pins from all output sets */
                interconnect->interconnect_power->num_pins_per_port = 1;

                num_ports = 0;
                for (set_idx = 0; set_idx < num_input_sets; set_idx++) {
                    num_ports += num_input_pins[set_idx];
                }
                interconnect->interconnect_power->num_input_ports = num_ports;

                num_ports = 0;
                for (set_idx = 0; set_idx < num_output_sets; set_idx++) {
                    num_ports += num_output_pins[set_idx];
                }
                interconnect->interconnect_power->num_output_ports = num_ports;

                /*
                 * if (interconnect->mux_arch) {
                 * interconnect->mux_arch->num_inputs =
                 * interconnect->num_input_ports;
                 *
                 * mux_arch_fix_levels(interconnect->mux_arch);
                 *
                 * interconnect->mux_arch->mux_graph_head =
                 * alloc_and_load_mux_graph(interconnect->num_input_ports,
                 * interconnect->mux_arch->levels);
                 * }*/

                interconnect->interconnect_power->port_info_initialized = true;
            }

            /* Input Pins */
            interc_pins->input_pins = new t_pb_graph_pin**[interconnect->interconnect_power->num_input_ports];
            for (port_idx = 0; port_idx < interconnect->interconnect_power->num_input_ports; port_idx++) {
                interc_pins->input_pins[port_idx] = new t_pb_graph_pin*[interconnect->interconnect_power->num_pins_per_port];
            }
            num_ports = 0;
            for (set_idx = 0; set_idx < num_input_sets; set_idx++) {
                for (pin_idx = 0; pin_idx < num_input_pins[set_idx]; pin_idx++) {
                    interc_pins->input_pins[num_ports++][0] = input_pins[set_idx][pin_idx];
                }
            }

            /* Output Pins */
            interc_pins->output_pins = new t_pb_graph_pin**[interconnect->interconnect_power->num_output_ports];
            for (port_idx = 0; port_idx < interconnect->interconnect_power->num_output_ports; port_idx++) {
                interc_pins->output_pins[port_idx] = new t_pb_graph_pin*[interconnect->interconnect_power->num_pins_per_port];
            }
            num_ports = 0;
            for (set_idx = 0; set_idx < num_output_sets; set_idx++) {
                for (pin_idx = 0; pin_idx < num_output_pins[set_idx]; pin_idx++) {
                    interc_pins->output_pins[num_ports++][0] = output_pins[set_idx][pin_idx];
                }
            }

            break;
        default:
            VTR_ASSERT_MSG(false, "Unrecognzied interconnect type.");
    }
}

/**
 * Generate interconnect associated with a mode of operation
 * pb_graph_parent_node: parent node of pb in mode
 * pb_graph_children_nodes: [0..num_pb_type_in_mode-1][0..num_pb]
 * mode: mode of operation
 */
static void alloc_and_load_mode_interconnect(t_pb_graph_node* pb_graph_parent_node,
                                             t_pb_graph_node** pb_graph_children_nodes,
                                             const t_mode* mode,
                                             bool load_power_structures) {
    int i, j;
    int *num_input_pb_graph_node_pins, *num_output_pb_graph_node_pins; /* number of pins in a set [0..num_sets-1] */
    int num_input_pb_graph_node_sets, num_output_pb_graph_node_sets;
    /* Points to pins specified in the port string, later used to insert edges */
    t_pb_graph_pin ***input_pb_graph_node_pins, ***output_pb_graph_node_pins; /* [0..num_sets_in_port - 1][0..num_ptrs - 1] */

    if (load_power_structures) {
        VTR_ASSERT(pb_graph_parent_node->interconnect_pins[mode->index] == nullptr);
        if (mode->num_interconnect > 0) {
            pb_graph_parent_node->interconnect_pins[mode->index] = new t_interconnect_pins[mode->num_interconnect];
        }
        for (i = 0; i < mode->num_interconnect; i++)
            pb_graph_parent_node->interconnect_pins[mode->index][i] = t_interconnect_pins();
    }

    for (i = 0; i < mode->num_interconnect; i++) {
        /* determine the interconnect input and output pins */
        input_pb_graph_node_pins = alloc_and_load_port_pin_ptrs_from_string(mode->interconnect[i].line_num, pb_graph_parent_node,
                                                                            pb_graph_children_nodes, mode->interconnect[i].input_string,
                                                                            &num_input_pb_graph_node_pins, &num_input_pb_graph_node_sets,
                                                                            true, true);

        output_pb_graph_node_pins = alloc_and_load_port_pin_ptrs_from_string(mode->interconnect[i].line_num, pb_graph_parent_node,
                                                                             pb_graph_children_nodes, mode->interconnect[i].output_string,
                                                                             &num_output_pb_graph_node_pins, &num_output_pb_graph_node_sets,
                                                                             false, true);

        if (load_power_structures) {
            alloc_and_load_interconnect_pins(&pb_graph_parent_node->interconnect_pins[mode->index][i],
                                             &mode->interconnect[i], input_pb_graph_node_pins,
                                             num_input_pb_graph_node_sets, num_input_pb_graph_node_pins,
                                             output_pb_graph_node_pins, num_output_pb_graph_node_sets,
                                             num_output_pb_graph_node_pins);
        }

        /* process the interconnect based on its type */
        switch (mode->interconnect[i].type) {
            case COMPLETE_INTERC:
                alloc_and_load_complete_interc_edges(&mode->interconnect[i],
                                                     input_pb_graph_node_pins, num_input_pb_graph_node_sets,
                                                     num_input_pb_graph_node_pins, output_pb_graph_node_pins,
                                                     num_output_pb_graph_node_sets,
                                                     num_output_pb_graph_node_pins);

                break;

            case DIRECT_INTERC:
                alloc_and_load_direct_interc_edges(&mode->interconnect[i],
                                                   input_pb_graph_node_pins, num_input_pb_graph_node_sets,
                                                   num_input_pb_graph_node_pins, output_pb_graph_node_pins,
                                                   num_output_pb_graph_node_sets,
                                                   num_output_pb_graph_node_pins);
                break;

            case MUX_INTERC:
                alloc_and_load_mux_interc_edges(&mode->interconnect[i],
                                                input_pb_graph_node_pins, num_input_pb_graph_node_sets,
                                                num_input_pb_graph_node_pins, output_pb_graph_node_pins,
                                                num_output_pb_graph_node_sets,
                                                num_output_pb_graph_node_pins);

                break;

            default:
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), mode->interconnect[i].line_num,
                          "Unknown interconnect %d for mode %s in pb_type %s, input %s, output %s\n",
                          mode->interconnect[i].type, mode->name,
                          pb_graph_parent_node->pb_type->name, mode->interconnect[i].input_string,
                          mode->interconnect[i].output_string);
        }
        for (j = 0; j < num_input_pb_graph_node_sets; j++) {
            delete[] input_pb_graph_node_pins[j];
        }
        delete[] input_pb_graph_node_pins;
        for (j = 0; j < num_output_pb_graph_node_sets; j++) {
            delete[] output_pb_graph_node_pins[j];
        }
        delete[] output_pb_graph_node_pins;
        delete[] num_input_pb_graph_node_pins;
        delete[] num_output_pb_graph_node_pins;
    }
}

static void store_pin_sinks_edge_id(t_pb_graph_node* pb_graph_node) {
    static int NUM_PORT_TYPES = 3;

    for (int port_type = 0; port_type < NUM_PORT_TYPES; port_type++) {
        int num_ports = 0;
        int* num_pins = nullptr;
        t_pb_graph_pin** pins = nullptr;
        switch (port_type) {
            case 0:
                num_ports = pb_graph_node->num_input_ports;
                num_pins = pb_graph_node->num_input_pins;
                pins = pb_graph_node->input_pins;
                break;
            case 1:
                num_ports = pb_graph_node->num_output_ports;
                num_pins = pb_graph_node->num_output_pins;
                pins = pb_graph_node->output_pins;
                break;
            case 2:
                num_ports = pb_graph_node->num_clock_ports;
                num_pins = pb_graph_node->num_clock_pins;
                pins = pb_graph_node->clock_pins;
                break;
            default:
                VTR_ASSERT(false);
        }

        for (int port_idx = 0; port_idx < num_ports; ++port_idx) {
            for (int pin_idx = 0; pin_idx < num_pins[port_idx]; ++pin_idx) {
                t_pb_graph_pin& pb_pin = pins[port_idx][pin_idx];
                for (int edge_idx = 0; edge_idx < pb_pin.num_output_edges; ++edge_idx) {
                    auto& output_edge = pb_pin.output_edges[edge_idx];
                    int num_edge_output_pin = output_edge->num_output_pins;
                    for (int edge_output_pin_idx = 0; edge_output_pin_idx < num_edge_output_pin; ++edge_output_pin_idx) {
                        t_pb_graph_pin* edge_output_pin = output_edge->output_pins[edge_output_pin_idx];
                        pb_pin.sink_pin_edge_idx_map.insert(std::make_pair(edge_output_pin, edge_idx));
                    }
                }
            }
        }
    }
}

/**
 * creates an array of pointers to the pb graph node pins in order from the port string
 * returns t_pb_graph_pin ptr indexed by [0..num_sets_in_port - 1][0..num_ptrs - 1]
 */
t_pb_graph_pin*** alloc_and_load_port_pin_ptrs_from_string(const int line_num,
                                                           const t_pb_graph_node* pb_graph_parent_node,
                                                           t_pb_graph_node** pb_graph_children_nodes,
                                                           const char* port_string,
                                                           int** num_ptrs,
                                                           int* num_sets,
                                                           const bool is_input_to_interc,
                                                           const bool interconnect_error_check) {
    const Tokens tokens(port_string);
    *num_sets = 0;
    bool in_squig_bracket = false;

    // count the number of sets available
    for (size_t i = 0; i < tokens.size(); i++) {
        VTR_ASSERT(tokens[i].type != e_token_type::NULL_TOKEN);
        if (tokens[i].type == e_token_type::OPEN_SQUIG_BRACKET) {
            if (in_squig_bracket) {
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                          "{ inside { in port %s\n", port_string);
            }
            in_squig_bracket = true;
        } else if (tokens[i].type == e_token_type::CLOSE_SQUIG_BRACKET) {
            if (!in_squig_bracket) {
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                          "No matching '{' for '}' in port %s\n", port_string);
            }
            (*num_sets)++;
            in_squig_bracket = false;
        } else if (tokens[i].type == e_token_type::DOT) {
            if (!in_squig_bracket) {
                (*num_sets)++;
            }
        }
    }

    if (in_squig_bracket) {
        (*num_sets)++;
        vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                  "No matching '{' for '}' in port %s\n", port_string);
    }

    t_pb_graph_pin*** pb_graph_pins = new t_pb_graph_pin**[*num_sets];
    *num_ptrs = new int[*num_sets];
    for (int i = 0; i < *num_sets; i++) {
        pb_graph_pins[i] = nullptr;
        (*num_ptrs)[i] = 0;
    }

    int curr_set = 0;
    for (int i = 0; i < int(tokens.size()); i++) {
        VTR_ASSERT(tokens[i].type != e_token_type::NULL_TOKEN);
        if (tokens[i].type == e_token_type::OPEN_SQUIG_BRACKET) {
            if (in_squig_bracket) {
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                          "{ inside { in port %s\n", port_string);
            }
            in_squig_bracket = true;
        } else if (tokens[i].type == e_token_type::CLOSE_SQUIG_BRACKET) {
            if ((*num_ptrs)[curr_set] == 0) {
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                          "No data contained in {} in port %s\n", port_string);
            }
            if (!in_squig_bracket) {
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                          "No matching '{' for '}' in port %s\n", port_string);
            }
            curr_set++;
            in_squig_bracket = false;
        } else if (tokens[i].type == e_token_type::STRING) {
            bool success = false;
            try {
                success = realloc_and_load_pb_graph_pin_ptrs_at_var(line_num,
                                                                    pb_graph_parent_node, pb_graph_children_nodes,
                                                                    interconnect_error_check, is_input_to_interc, tokens, &i,
                                                                    &((*num_ptrs)[curr_set]), &pb_graph_pins[curr_set]);
            } catch (VprError& e) {
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                          "Syntax error processing port string '%s' (%s)\n", port_string, e.what());
            }
            VTR_ASSERT(success);

            if (!in_squig_bracket) {
                curr_set++;
            }
        }
    }
    VTR_ASSERT(curr_set == *num_sets);
    return pb_graph_pins;
}

/**
 * Creates edges to connect all input pins to output pins
 */
static void alloc_and_load_complete_interc_edges(t_interconnect* interconnect,
                                                 t_pb_graph_pin*** input_pb_graph_node_pin_ptrs,
                                                 const int num_input_sets,
                                                 const int* num_input_ptrs,
                                                 t_pb_graph_pin*** output_pb_graph_node_pin_ptrs,
                                                 const int num_output_sets,
                                                 const int* num_output_ptrs) {
    int i_inset, i_outset, i_inpin, i_outpin;
    int in_count, out_count;
    t_pb_graph_edge* edges;
    int i_edge;
    vtr::t_linked_vptr* cur;

    VTR_ASSERT(interconnect->infer_annotations == false);

    /* Allocate memory for edges, and reallocate more memory for pins connecting to those edges */
    in_count = out_count = 0;

    for (i_inset = 0; i_inset < num_input_sets; i_inset++) {
        in_count += num_input_ptrs[i_inset];
    }
    for (i_outset = 0; i_outset < num_output_sets; i_outset++) {
        out_count += num_output_ptrs[i_outset];
    }

    edges = new t_pb_graph_edge[in_count * out_count];
    for (int i = 0; i < (in_count * out_count); i++)
        edges[i] = t_pb_graph_edge();
    cur = new vtr::t_linked_vptr;
    cur->next = edges_head;
    edges_head = cur;
    cur->data_vptr = (void*)edges;
    cur = new vtr::t_linked_vptr;
    cur->next = num_edges_head;
    num_edges_head = cur;
    cur->data_vptr = (void*)((intptr_t)in_count * out_count);

    for (i_inset = 0; i_inset < num_input_sets; i_inset++) {
        for (i_inpin = 0; i_inpin < num_input_ptrs[i_inset]; i_inpin++) {
            input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->output_edges.resize(input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->num_output_edges + out_count);
        }
    }

    for (i_outset = 0; i_outset < num_output_sets; i_outset++) {
        for (i_outpin = 0; i_outpin < num_output_ptrs[i_outset]; i_outpin++) {
            output_pb_graph_node_pin_ptrs[i_outset][i_outpin]->input_edges.resize(output_pb_graph_node_pin_ptrs[i_outset][i_outpin]->num_input_edges + in_count);
        }
    }

    i_edge = 0;

    /* Load connections between pins and record these updates in the edges */
    for (i_inset = 0; i_inset < num_input_sets; i_inset++) {
        for (i_inpin = 0; i_inpin < num_input_ptrs[i_inset]; i_inpin++) {
            for (i_outset = 0; i_outset < num_output_sets; i_outset++) {
                for (i_outpin = 0; i_outpin < num_output_ptrs[i_outset]; i_outpin++) {
                    auto in_pin = input_pb_graph_node_pin_ptrs[i_inset][i_inpin];
                    auto out_pin = output_pb_graph_node_pin_ptrs[i_outset][i_outpin];
                    in_pin->output_edges[in_pin->num_output_edges] = &edges[i_edge];
                    in_pin->num_output_edges++;
                    out_pin->input_edges[out_pin->num_input_edges] = &edges[i_edge];
                    out_pin->num_input_edges++;

                    edges[i_edge].num_input_pins = 1;
                    edges[i_edge].input_pins = new t_pb_graph_pin*[1];
                    edges[i_edge].input_pins[0] = in_pin;
                    edges[i_edge].num_output_pins = 1;
                    edges[i_edge].output_pins = new t_pb_graph_pin*[1];
                    edges[i_edge].output_pins[0] = out_pin;

                    edges[i_edge].interconnect = interconnect;
                    edges[i_edge].driver_set = i_inset;
                    edges[i_edge].driver_pin = i_inpin;

                    i_edge++;
                }
            }
        }
    }
    VTR_ASSERT(i_edge == in_count * out_count);
}

static void alloc_and_load_direct_interc_edges(t_interconnect* interconnect,
                                               t_pb_graph_pin*** input_pb_graph_node_pin_ptrs,
                                               const int num_input_sets,
                                               const int* num_input_ptrs,
                                               t_pb_graph_pin*** output_pb_graph_node_pin_ptrs,
                                               const int num_output_sets,
                                               const int* num_output_ptrs) {
    if (num_input_sets != 1) {
        vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), interconnect->line_num,
                  "Direct interconnect only allows connections from one set of input pins to one-or-more output sets. "
                  "There are %d input sets.",
                  num_input_sets);
    }

    int pins_per_set = num_input_ptrs[0];
    for (int i = 0; i < num_output_sets; ++i) {
        if (pins_per_set != num_output_ptrs[i]) {
            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), interconnect->line_num,
                      "Direct interconnect sets must have an equal number of input pins and output pins. "
                      "Input set has %d input pins but output set %d has %d output pins\n",
                      pins_per_set, i, num_output_ptrs[i]);
        }
    }

    /* Allocate memory for edges */
    t_pb_graph_edge* edges = new t_pb_graph_edge[pins_per_set * num_output_sets];
    for (int i = 0; i < (pins_per_set * num_output_sets); i++)
        edges[i] = t_pb_graph_edge();
    vtr::t_linked_vptr* cur = new vtr::t_linked_vptr;
    cur->next = edges_head;
    edges_head = cur;
    cur->data_vptr = (void*)edges;
    cur = new vtr::t_linked_vptr;
    cur->next = num_edges_head;
    num_edges_head = cur;
    cur->data_vptr = (void*)((intptr_t)num_input_ptrs[0]);

    /* Reallocate memory for pins and load connections between pins and record these updates in the edges */
    for (int ipin = 0; ipin < pins_per_set; ++ipin) {
        t_pb_graph_pin* input_pin = input_pb_graph_node_pin_ptrs[0][ipin];

        //Allocate space for input pin set's out-going edges (one to each out-set)
        input_pin->output_edges.resize(input_pin->num_output_edges + num_output_sets);

        //Associate each input pin with it's new out-going edges
        for (int iset = 0; iset < num_output_sets; ++iset) {
            int iedge = iset * pins_per_set + ipin;
            int ipin_edge = input_pin->num_output_edges + iset;
            input_pin->output_edges[ipin_edge] = &edges[iedge];
        }
        input_pin->num_output_edges += num_output_sets;
    }

    for (int iset = 0; iset < num_output_sets; ++iset) {
        for (int ipin = 0; ipin < pins_per_set; ++ipin) {
            t_pb_graph_pin* output_pin = output_pb_graph_node_pin_ptrs[iset][ipin];

            //Allocate space for output pin set's in-coming edge (one edge per pin)
            output_pin->input_edges.resize(output_pin->num_input_edges + 1);

            int ipin_edge = output_pin->num_input_edges;
            int iedge = iset * pins_per_set + ipin;
            output_pin->input_edges[ipin_edge] = &edges[iedge];
            output_pin->num_input_edges += 1;
        }
    }

    //Set-up the edges
    for (int iset = 0; iset < num_output_sets; ++iset) {
        for (int ipin = 0; ipin < pins_per_set; ++ipin) {
            int iedge = iset * pins_per_set + ipin;

            auto in_pin = input_pb_graph_node_pin_ptrs[0][ipin];
            auto out_pin = output_pb_graph_node_pin_ptrs[iset][ipin];

            edges[iedge].num_input_pins = 1;
            edges[iedge].input_pins = new t_pb_graph_pin*[1];
            edges[iedge].input_pins[0] = in_pin;
            edges[iedge].num_output_pins = 1;
            edges[iedge].output_pins = new t_pb_graph_pin*[1];
            edges[iedge].output_pins[0] = out_pin;

            edges[iedge].interconnect = interconnect;
            edges[iedge].driver_set = 0;
            edges[iedge].driver_pin = ipin;
            edges[iedge].infer_pattern = interconnect->infer_annotations;
        }
    }
}

static void alloc_and_load_mux_interc_edges(t_interconnect* interconnect,
                                            t_pb_graph_pin*** input_pb_graph_node_pin_ptrs,
                                            const int num_input_sets,
                                            const int* num_input_ptrs,
                                            t_pb_graph_pin*** output_pb_graph_node_pin_ptrs,
                                            const int num_output_sets,
                                            const int* num_output_ptrs) {
    int i_inset, i_inpin, i_outpin;
    t_pb_graph_edge* edges;
    vtr::t_linked_vptr* cur;

    VTR_ASSERT(interconnect->infer_annotations == false);

    /* Allocate memory for edges, and reallocate more memory for pins connecting to those edges */
    if (num_output_sets != 1) {
        vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), interconnect->line_num,
                  "Mux must have one output\n");
    }

    edges = new t_pb_graph_edge[num_input_sets];
    for (int i = 0; i < (num_input_sets); i++)
        edges[i] = t_pb_graph_edge();
    cur = new vtr::t_linked_vptr;
    cur->next = edges_head;
    edges_head = cur;
    cur->data_vptr = (void*)edges;
    cur = new vtr::t_linked_vptr;
    cur->next = num_edges_head;
    num_edges_head = cur;
    cur->data_vptr = (void*)((intptr_t)num_input_sets);

    for (i_inset = 0; i_inset < num_input_sets; i_inset++) {
        for (i_inpin = 0; i_inpin < num_input_ptrs[i_inset]; i_inpin++) {
            input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->output_edges.resize(input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->num_output_edges + 1);
        }
    }

    for (i_outpin = 0; i_outpin < num_output_ptrs[0]; i_outpin++) {
        output_pb_graph_node_pin_ptrs[0][i_outpin]->input_edges.resize(output_pb_graph_node_pin_ptrs[0][i_outpin]->num_input_edges
                                                                       + num_input_sets);
    }

    /* Load connections between pins and record these updates in the edges */
    for (i_inset = 0; i_inset < num_input_sets; i_inset++) {
        if (num_output_ptrs[0] != num_input_ptrs[i_inset]) {
            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), interconnect->line_num,
                      "# of pins for a particular data line of a mux must equal number of pins at output of mux\n");
        }
        edges[i_inset].input_pins = new t_pb_graph_pin*[num_output_ptrs[0]];
        edges[i_inset].output_pins = new t_pb_graph_pin*[num_output_ptrs[0]];
        for (int i = 0; i < num_output_ptrs[0]; i++) {
            edges[i_inset].input_pins[i] = nullptr;
            edges[i_inset].output_pins[i] = nullptr;
        }

        edges[i_inset].num_input_pins = num_output_ptrs[0];
        edges[i_inset].num_output_pins = num_output_ptrs[0];
        for (i_inpin = 0; i_inpin < num_input_ptrs[i_inset]; i_inpin++) {
            auto in_pin = input_pb_graph_node_pin_ptrs[i_inset][i_inpin];
            auto out_pin = output_pb_graph_node_pin_ptrs[0][i_inpin];
            in_pin->output_edges[in_pin->num_output_edges] = &edges[i_inset];
            in_pin->num_output_edges++;
            out_pin->input_edges[out_pin->num_input_edges] = &edges[i_inset];
            out_pin->num_input_edges++;

            edges[i_inset].input_pins[i_inpin] = in_pin;
            edges[i_inset].output_pins[i_inpin] = out_pin;

            if (i_inpin != 0) {
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), interconnect->line_num,
                          "Bus-based mux not yet supported, will consider for future work\n");
            }
            edges[i_inset].interconnect = interconnect;
            edges[i_inset].driver_set = i_inset;
            edges[i_inset].driver_pin = i_inpin;
        }
    }
}

/**
 * populate array of pb graph pins for a single variable of type pb_type[int:int].port[int:int]
 * pb_graph_pins: pointer to array from [0..num_port_pins] of pb_graph_pin pointers
 * tokens: array of tokens to scan
 * num_pins: current number of pins in pb_graph_pin array
 */
static bool realloc_and_load_pb_graph_pin_ptrs_at_var(const int line_num,
                                                      const t_pb_graph_node* pb_graph_parent_node,
                                                      t_pb_graph_node** pb_graph_children_nodes,
                                                      const bool interconnect_error_check,
                                                      const bool is_input_to_interc,
                                                      const Tokens& tokens,
                                                      int* token_index,
                                                      int* num_pins,
                                                      t_pb_graph_pin*** pb_graph_pins) {
    int pb_msb, pb_lsb;
    int pin_msb, pin_lsb;
    const char* pb_name = tokens[*token_index].data.c_str();
    t_port* iport;
    int add_or_subtract_pb, add_or_subtract_pin;

    VTR_ASSERT(tokens[*token_index].type == e_token_type::STRING);
    const t_pb_graph_node* pb_node_array = nullptr;
    int max_pb_node_array = 0;

    t_mode* mode = nullptr;
    if (pb_graph_children_nodes) {
        mode = pb_graph_children_nodes[0][0].pb_type->parent_mode;
    }

    pb_msb = pb_lsb = OPEN;
    pin_msb = pin_lsb = OPEN;

    // parse pb
    bool found = false;
    if (pb_graph_parent_node->pb_type->name == tokens[*token_index].data) {
        // Parent pb_type
        pb_node_array = pb_graph_parent_node;
        max_pb_node_array = 1;
        pb_msb = pb_lsb = 0;
        found = true;
        (*token_index)++;
        if (tokens[*token_index].type == e_token_type::OPEN_SQUARE_BRACKET) {
            (*token_index)++;
            if (tokens[*token_index].type != e_token_type::INT) {
                return false; // clb[abc
            }
            pb_msb = vtr::atoi(tokens[*token_index].data);
            VTR_ASSERT_MSG(pb_msb >= 0, "Pin most-significant-bit must be non-negative");
            (*token_index)++;
            if (tokens[*token_index].type != e_token_type::COLON) {
                if (tokens[*token_index].type != e_token_type::CLOSE_SQUARE_BRACKET) {
                    return false; // clb[9abc
                }
                pb_lsb = pb_msb;
                (*token_index)++;
            } else {
                (*token_index)++;
                if (tokens[*token_index].type != e_token_type::INT) {
                    return false; // clb[9:abc
                }
                pb_lsb = vtr::atoi(tokens[*token_index].data);
                VTR_ASSERT_MSG(pb_lsb >= 0, "Pin most-significant-bit must be non-negative");
                (*token_index)++;
                if (tokens[*token_index].type != e_token_type::CLOSE_SQUARE_BRACKET) {
                    return false; // clb[9:0abc
                }
                (*token_index)++;
            }
            // Check to make sure indices from user match internal data structures for the indices of the parent
            if ((pb_lsb != pb_msb)
                && (pb_lsb != pb_graph_parent_node->placement_index)) {
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                          "Incorrect placement index for %s, expected index %d", tokens[0].data.c_str(),
                          pb_graph_parent_node->placement_index);
            }

            if ((pb_lsb != pb_msb)) {
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                          "Cannot specify range for a parent pb: '%s'", tokens[0].data.c_str());
            }

            pb_lsb = pb_msb = 0; // Internal representation of parent is always 0
        }
    } else {
        // Children pb_types
        if (mode) {
            for (int i = 0; i < mode->num_pb_type_children; i++) {
                VTR_ASSERT(&mode->pb_type_children[i] == pb_graph_children_nodes[i][0].pb_type);
                if (mode->pb_type_children[i].name == tokens[*token_index].data) {
                    pb_node_array = pb_graph_children_nodes[i];
                    max_pb_node_array = mode->pb_type_children[i].num_pb;
                    found = true;
                    (*token_index)++;

                    if (tokens[*token_index].type == e_token_type::OPEN_SQUARE_BRACKET) {
                        (*token_index)++;
                        if (tokens[*token_index].type != e_token_type::INT) {
                            return false;
                        }
                        pb_msb = vtr::atoi(tokens[*token_index].data);
                        VTR_ASSERT_MSG(pb_msb >= 0, "Pin most-significant-bit must be non-negative");
                        (*token_index)++;
                        if (tokens[*token_index].type != e_token_type::COLON) {
                            if (tokens[*token_index].type != e_token_type::CLOSE_SQUARE_BRACKET) {
                                return false;
                            }
                            pb_lsb = pb_msb;
                            (*token_index)++;
                        } else {
                            (*token_index)++;
                            if (tokens[*token_index].type != e_token_type::INT) {
                                return false;
                            }
                            pb_lsb = vtr::atoi(tokens[*token_index].data);
                            VTR_ASSERT_MSG(pb_lsb >= 0, "Pin most-significant-bit must be non-negative");
                            (*token_index)++;
                            if (tokens[*token_index].type != e_token_type::CLOSE_SQUARE_BRACKET) {
                                return false;
                            }
                            (*token_index)++;
                        }
                        // Check range of children pb
                        if (pb_lsb < 0 || pb_lsb >= max_pb_node_array || pb_msb < 0 || pb_msb >= max_pb_node_array) {
                            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                                      "Mode '%s' -> pb '%s' [%d,%d] out of range [%d,%d]", mode->name,
                                      mode->pb_type_children[i].name, pb_msb, pb_lsb, max_pb_node_array - 1, 0);
                        }
                    } else {
                        pb_msb = pb_node_array[0].pb_type->num_pb - 1;
                        pb_lsb = 0;
                    }
                    break; // found pb_type_children, no need to keep traversing
                }
            }
        } else {
            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                      "Parent pb node '%s' missing", pb_graph_parent_node->pb_type->name);
        }
    }

    if (!found) {
        vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                  "Unknown pb_type name %s, not defined in namespace of mode %s",
                  pb_name, mode->name);
    }

    found = false;

    if (tokens[*token_index].type != e_token_type::DOT) {
        return false; // clb[9:0]123
    }
    (*token_index)++;

    bool is_string = tokens[*token_index].type == e_token_type::STRING;
    bool is_int = tokens[*token_index].type == e_token_type::INT;

    if (!is_string && !is_int)
        return false;

    // parse ports and port pins of pb
    const char* port_name = tokens[*token_index].data.c_str();
    (*token_index)++;

    if (get_pb_graph_pin_from_name(port_name, &pb_node_array[pb_lsb], 0) == nullptr) {
        vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                  "Failed to find port name %s", port_name);
    }

    if (tokens[*token_index].type == e_token_type::OPEN_SQUARE_BRACKET) {
        (*token_index)++;
        if (tokens[*token_index].type != e_token_type::INT) {
            return false;
        }
        pin_msb = vtr::atoi(tokens[*token_index].data);
        VTR_ASSERT_MSG(pin_msb >= 0, "Pin most-significant-bit must be non-negative");
        (*token_index)++;
        if (tokens[*token_index].type != e_token_type::COLON) {
            if (tokens[*token_index].type != e_token_type::CLOSE_SQUARE_BRACKET) {
                return false;
            }
            pin_lsb = pin_msb;
            (*token_index)++;
        } else {
            (*token_index)++;
            if (tokens[*token_index].type != e_token_type::INT) {
                return false;
            }
            pin_lsb = vtr::atoi(tokens[*token_index].data);
            VTR_ASSERT_MSG(pin_lsb >= 0, "Pin most-significant-bit must be non-negative");
            (*token_index)++;
            if (tokens[*token_index].type != e_token_type::CLOSE_SQUARE_BRACKET) {
                return false;
            }
            (*token_index)++;
        }
    } else {
        iport = get_pb_graph_pin_from_name(port_name, &pb_node_array[pb_lsb], 0)->port;
        pin_msb = iport->num_pins - 1;
        pin_lsb = 0;
    }
    (*token_index)--;

    if (pb_msb < pb_lsb) {
        add_or_subtract_pb = -1;
    } else {
        add_or_subtract_pb = 1;
    }

    if (pin_msb < pin_lsb) {
        add_or_subtract_pin = -1;
    } else {
        add_or_subtract_pin = 1;
    }

    int prev_num_pins = *num_pins;
    *num_pins += (abs(pb_msb - pb_lsb) + 1) * (abs(pin_msb - pin_lsb) + 1);

    if (prev_num_pins > 0) {
        std::vector<t_pb_graph_pin*> temp(*pb_graph_pins, *pb_graph_pins + prev_num_pins);
        delete[] *pb_graph_pins;
        *pb_graph_pins = new t_pb_graph_pin*[*num_pins];
        for (int i = 0; i < prev_num_pins; i++)
            (*pb_graph_pins)[i] = temp[i];
    } else {
        *pb_graph_pins = new t_pb_graph_pin*[*num_pins];
    }

    int ipb = pb_lsb;
    int i = 0;
    int j = 0;

    while (ipb != pb_msb + add_or_subtract_pb) {
        int ipin = pin_lsb;
        j = 0;
        while (ipin != pin_msb + add_or_subtract_pin) {
            int idx = prev_num_pins + i * (abs(pin_msb - pin_lsb) + 1) + j;
            (*pb_graph_pins)[idx] = get_pb_graph_pin_from_name(port_name, &pb_node_array[ipb], ipin);
            if ((*pb_graph_pins)[idx] == nullptr) {
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                          "Pin %s.%s[%d] cannot be found",
                          pb_node_array[ipb].pb_type->name, port_name, ipin);
            }
            iport = (*pb_graph_pins)[idx]->port;
            if (!iport) {
                return false;
            }

            // Error checking before assignment
            if (interconnect_error_check) {
                if (pb_node_array == pb_graph_parent_node) {
                    if (is_input_to_interc) {
                        if (iport->type != IN_PORT) {
                            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                                      "Input to interconnect from parent is not an input or clock pin");
                        }
                    } else {
                        if (iport->type != OUT_PORT) {
                            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                                      "Output from interconnect from parent is not an input or clock pin");
                        }
                    }
                } else {
                    if (is_input_to_interc) {
                        if (iport->type != OUT_PORT) {
                            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                                      "output from interconnect from parent is not an input or clock pin");
                        }
                    } else {
                        if (iport->type != IN_PORT) {
                            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                                      "Input to interconnect from parent is not an input or clock pin");
                        }
                    }
                }
            }

            // load pb_graph_pin for pin

            ipin += add_or_subtract_pin;
            j++;
        }
        i++;
        ipb += add_or_subtract_pb;
    }

    VTR_ASSERT((abs(pb_msb - pb_lsb) + 1) * (abs(pin_msb - pin_lsb) + 1) == i * j);

    return true;
}

static t_pb_graph_pin* get_pb_graph_pin_from_name(const char* port_name,
                                                  const t_pb_graph_node* pb,
                                                  const int pin) {
    int i;

    for (i = 0; i < pb->num_input_ports; i++) {
        if (0 == strcmp(port_name, pb->input_pins[i][0].port->name)) {
            if (pin < pb->input_pins[i][0].port->num_pins) {
                return &pb->input_pins[i][pin];
            } else {
                return nullptr;
            }
        }
    }
    for (i = 0; i < pb->num_output_ports; i++) {
        if (0 == strcmp(port_name, pb->output_pins[i][0].port->name)) {
            if (pin < pb->output_pins[i][0].port->num_pins) {
                return &pb->output_pins[i][pin];
            } else {
                return nullptr;
            }
        }
    }
    for (i = 0; i < pb->num_clock_ports; i++) {
        if (0 == strcmp(port_name, pb->clock_pins[i][0].port->name)) {
            if (pin < pb->clock_pins[i][0].port->num_pins) {
                return &pb->clock_pins[i][pin];
            } else {
                return nullptr;
            }
        }
    }
    return nullptr;
}

static void echo_pb_rec(const t_pb_graph_node* pb_graph_node, const int level, FILE* fp) {
    int i, j, k;

    print_tabs(fp, level);
    fprintf(fp, "Physical Block: type \"%s\"  index %d  num_children %d\n",
            pb_graph_node->pb_type->name, pb_graph_node->placement_index,
            pb_graph_node->pb_type->num_pb);

    if (pb_graph_node->parent_pb_graph_node) {
        print_tabs(fp, level + 1);
        fprintf(fp, "Parent Block: type \"%s\" index %d \n",
                pb_graph_node->parent_pb_graph_node->pb_type->name,
                pb_graph_node->parent_pb_graph_node->placement_index);
    }

    print_tabs(fp, level);
    fprintf(fp, "Input Ports: total ports %d\n",
            pb_graph_node->num_input_ports);
    echo_pb_pins(pb_graph_node->input_pins, pb_graph_node->num_input_ports,
                 level, fp);
    print_tabs(fp, level);
    fprintf(fp, "Output Ports: total ports %d\n",
            pb_graph_node->num_output_ports);
    echo_pb_pins(pb_graph_node->output_pins, pb_graph_node->num_output_ports,
                 level, fp);
    print_tabs(fp, level);
    fprintf(fp, "Clock Ports: total ports %d\n",
            pb_graph_node->num_clock_ports);
    echo_pb_pins(pb_graph_node->clock_pins, pb_graph_node->num_clock_ports,
                 level, fp);
    print_tabs(fp, level);
    for (i = 0; i < pb_graph_node->num_input_pin_class; i++) {
        fprintf(fp, "Input class %d: %d pins, ", i,
                pb_graph_node->input_pin_class_size[i]);
    }
    fprintf(fp, "\n");
    print_tabs(fp, level);
    for (i = 0; i < pb_graph_node->num_output_pin_class; i++) {
        fprintf(fp, "Output class %d: %d pins, ", i,
                pb_graph_node->output_pin_class_size[i]);
    }
    fprintf(fp, "\n");

    if (!pb_graph_node->pb_type->is_primitive()) {
        print_tabs(fp, level);
        fprintf(fp, "Children:\n");
    }
    for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
        for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
            for (k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                echo_pb_rec(&pb_graph_node->child_pb_graph_nodes[i][j][k],
                            level + 1, fp);
            }
        }
    }
}

static void echo_pb_pins(t_pb_graph_pin** pb_graph_pins, const int num_ports, const int level, FILE* fp) {
    int i, j, k, m;
    t_port* port;

    print_tabs(fp, level);

    for (i = 0; i < num_ports; i++) {
        port = pb_graph_pins[i][0].port;
        print_tabs(fp, level);
        fprintf(fp, "Port \"%s\": num_pins %d type %d parent name \"%s\"\n",
                port->name, port->num_pins, port->type,
                port->parent_pb_type->name);

        for (j = 0; j < port->num_pins; j++) {
            print_tabs(fp, level + 1);
            VTR_ASSERT(j == pb_graph_pins[i][j].pin_number);
            VTR_ASSERT(pb_graph_pins[i][j].port == port);
            fprintf(fp,
                    "Pin %d port name \"%s\" num input edges %d num output edges %d parent type \"%s\" parent index %d\n",
                    pb_graph_pins[i][j].pin_number,
                    pb_graph_pins[i][j].port->name,
                    pb_graph_pins[i][j].num_input_edges,
                    pb_graph_pins[i][j].num_output_edges,
                    pb_graph_pins[i][j].parent_node->pb_type->name,
                    pb_graph_pins[i][j].parent_node->placement_index);
            print_tabs(fp, level + 2);
            if (pb_graph_pins[i][j].is_primitive_pin()) {
                fprintf(fp, "pin class (depth, pin class): ");
                for (k = 0; k < pb_graph_pins[i][j].parent_node->pb_type->depth; k++) {
                    fprintf(fp, "(%d %d), ", k,
                            pb_graph_pins[i][j].parent_pin_class[k]);
                }
                fprintf(fp, "\n");
                if (pb_graph_pins[i][j].port->type == OUT_PORT) {
                    for (k = 0; k < pb_graph_pins[i][j].parent_node->pb_type->depth; k++) {
                        print_tabs(fp, level + 2);
                        fprintf(fp, "connectable input pins within depth %d: %d\n",
                                k, pb_graph_pins[i][j].num_connectable_primitive_input_pins[k]);
                        for (m = 0; m < pb_graph_pins[i][j].num_connectable_primitive_input_pins[k]; m++) {
                            print_tabs(fp, level + 3);
                            fprintf(fp, "pb_graph_node %s[%d].%s[%d] \n",
                                    pb_graph_pins[i][j].list_of_connectable_input_pin_ptrs[k][m]->parent_node->pb_type->name,
                                    pb_graph_pins[i][j].list_of_connectable_input_pin_ptrs[k][m]->parent_node->placement_index,
                                    pb_graph_pins[i][j].list_of_connectable_input_pin_ptrs[k][m]->port->name,
                                    pb_graph_pins[i][j].list_of_connectable_input_pin_ptrs[k][m]->pin_number);
                        }
                    }
                }
            } else {
                fprintf(fp, "pin class %d \n", pb_graph_pins[i][j].pin_class);
            }
            for (k = 0; k < pb_graph_pins[i][j].num_input_edges; k++) {
                print_tabs(fp, level + 2);
                fprintf(fp, "Input edge %d num inputs %d num outputs %d\n", k,
                        pb_graph_pins[i][j].input_edges[k]->num_input_pins,
                        pb_graph_pins[i][j].input_edges[k]->num_output_pins);
                print_tabs(fp, level + 3);
                fprintf(fp, "Input edge inputs\n");
                for (m = 0; m < pb_graph_pins[i][j].input_edges[k]->num_input_pins; m++) {
                    print_tabs(fp, level + 3);
                    fprintf(fp, "pin number %d port_name \"%s\"\n",
                            pb_graph_pins[i][j].input_edges[k]->input_pins[m]->pin_number,
                            pb_graph_pins[i][j].input_edges[k]->input_pins[m]->port->name);
                }
                print_tabs(fp, level + 3);
                fprintf(fp, "Input edge outputs\n");
                for (m = 0; m < pb_graph_pins[i][j].input_edges[k]->num_output_pins; m++) {
                    print_tabs(fp, level + 3);
                    fprintf(fp,
                            "pin number %d port_name \"%s\" parent type \"%s\" parent index %d\n",
                            pb_graph_pins[i][j].input_edges[k]->output_pins[m]->pin_number,
                            pb_graph_pins[i][j].input_edges[k]->output_pins[m]->port->name,
                            pb_graph_pins[i][j].input_edges[k]->output_pins[m]->parent_node->pb_type->name,
                            pb_graph_pins[i][j].input_edges[k]->output_pins[m]->parent_node->placement_index);
                }
            }
            for (k = 0; k < pb_graph_pins[i][j].num_output_edges; k++) {
                print_tabs(fp, level + 2);
                fprintf(fp, "Output edge %d num inputs %d num outputs %d\n", k,
                        pb_graph_pins[i][j].output_edges[k]->num_input_pins,
                        pb_graph_pins[i][j].output_edges[k]->num_output_pins);
                print_tabs(fp, level + 3);
                fprintf(fp, "Output edge inputs\n");
                for (m = 0; m < pb_graph_pins[i][j].output_edges[k]->num_input_pins; m++) {
                    print_tabs(fp, level + 3);
                    fprintf(fp, "pin number %d port_name \"%s\"\n",
                            pb_graph_pins[i][j].output_edges[k]->input_pins[m]->pin_number,
                            pb_graph_pins[i][j].output_edges[k]->input_pins[m]->port->name);
                }
                print_tabs(fp, level + 3);
                fprintf(fp, "Output edge outputs\n");
                for (m = 0; m < pb_graph_pins[i][j].output_edges[k]->num_output_pins; m++) {
                    print_tabs(fp, level + 3);
                    fprintf(fp, "pin number %d port_name \"%s\" parent type \"%s\" parent index %d\n",
                            pb_graph_pins[i][j].output_edges[k]->output_pins[m]->pin_number,
                            pb_graph_pins[i][j].output_edges[k]->output_pins[m]->port->name,
                            pb_graph_pins[i][j].output_edges[k]->output_pins[m]->parent_node->pb_type->name,
                            pb_graph_pins[i][j].output_edges[k]->output_pins[m]->parent_node->placement_index);
                }
            }
        }
    }
}

static void check_pb_node_rec(const t_pb_graph_node* pb_graph_node) {
    int line_num = 0;
    std::map<int, int> logic_equivalent_pins_map;

    for (int i = 0; i < pb_graph_node->num_input_ports; i++) {
        for (int j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
            check_repeated_edges_at_pb_pin(&pb_graph_node->input_pins[i][j]);
            // Checks the equivalency of pins of an input port
            if (pb_graph_node->input_pins[i][j].port->equivalent != PortEquivalence::NONE) {
                if (!check_input_pins_equivalence(&pb_graph_node->input_pins[i][j],
                                                  j, logic_equivalent_pins_map, &line_num)) {
                    VTR_LOG_WARN("[LINE %d] false logically-equivalent pin %s[%d].%s[%d].\n",
                                 line_num, pb_graph_node->pb_type->name,
                                 pb_graph_node->placement_index,
                                 pb_graph_node->input_pins[i][j].port->name,
                                 pb_graph_node->input_pins[i][j].pin_number);
                }
            }
        }
        logic_equivalent_pins_map.clear();
    }

    for (int i = 0; i < pb_graph_node->num_output_ports; i++) {
        for (int j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
            check_repeated_edges_at_pb_pin(&pb_graph_node->output_pins[i][j]);
        }
    }

    for (int i = 0; i < pb_graph_node->num_clock_ports; i++) {
        for (int j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
            check_repeated_edges_at_pb_pin(&pb_graph_node->clock_pins[i][j]);
        }
    }

    for (int i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
        for (int j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
            for (int k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                check_pb_node_rec(&pb_graph_node->child_pb_graph_nodes[i][j][k]);
            }
        }
    }
}

static void check_repeated_edges_at_pb_pin(t_pb_graph_pin* cur_pin) {
    t_pb_graph_edge_comparator edges_info;
    std::map<t_pb_graph_edge_comparator, int> edges_map;

    // First check the incoming edges into cur_pin
    for (int i_edge = 0; i_edge < cur_pin->num_input_edges; i_edge++) {
        t_pb_graph_edge* cur_edge = cur_pin->input_edges[i_edge];
        for (int i_pin = 0; i_pin < cur_edge->num_input_pins; i_pin++) {
            // Populate the edge_comparator struct and attempt to insert it into STL map
            edges_info.parent_edge = cur_edge;
            edges_info.input_pin = cur_edge->input_pins[i_pin];
            edges_info.output_pin = cur_pin;
            edges_info.input_pin_id_in_cluster = cur_edge->input_pins[i_pin]->pin_count_in_cluster;
            edges_info.output_pin_id_in_cluster = cur_pin->pin_count_in_cluster;
            auto [it, success] = edges_map.insert(std::pair<t_pb_graph_edge_comparator, int>(edges_info, 0));
            if (!success) {
                // Print out the connection that already exists in the map and then the new one
                // we are trying to insert into the map.
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), cur_edge->interconnect->line_num,
                          "Duplicate edges detected between: \n"
                          "%s[%d].%s[%d]--->%s[%d].%s[%d] \n"
                          "Found edges on line %d and %d.\n",
                          it->first.input_pin->parent_node->pb_type->name,
                          it->first.input_pin->parent_node->placement_index,
                          it->first.input_pin->port->name,
                          it->first.input_pin->pin_number,
                          it->first.output_pin->parent_node->pb_type->name,
                          it->first.output_pin->parent_node->placement_index,
                          it->first.output_pin->port->name,
                          it->first.output_pin->pin_number,
                          it->first.parent_edge->interconnect->line_num,
                          cur_edge->interconnect->line_num);
            }
        }
    }

    edges_map.clear();
}

static bool operator<(const t_pb_graph_edge_comparator& edge1,
                      const t_pb_graph_edge_comparator& edge2) {
    return (edge1.input_pin_id_in_cluster < edge2.input_pin_id_in_cluster) || (edge1.output_pin_id_in_cluster < edge2.output_pin_id_in_cluster);
}

static bool check_input_pins_equivalence(const t_pb_graph_pin* cur_pin,
                                         int i_pin,
                                         std::map<int, int>& logic_equivalent_pins_map,
                                         int* line_num) {
    bool pins_equivalent = true;

    if (i_pin == 0) {
        VTR_ASSERT(logic_equivalent_pins_map.empty());
    }
    int edge_count = 0;
    for (int i = 0; i < cur_pin->num_output_edges; i++) {
        t_pb_graph_edge* cur_edge = cur_pin->output_edges[i];
        *line_num = cur_edge->interconnect->line_num;
        for (int j = 0; j < cur_edge->num_output_pins; j++) {
            if (i_pin == 0) {
                // First pin of an equivalent port, populate edges_map first
                logic_equivalent_pins_map.insert(std::pair<int, int>(cur_edge->output_pins[j]->pin_count_in_cluster, 0));
            } else {
                // Rest of the pins of an equivalent port, they should connect to the
                // same set of pins
                if (logic_equivalent_pins_map.find(cur_edge->output_pins[j]->pin_count_in_cluster) == logic_equivalent_pins_map.end()) {
                    // Could not find the outpin that cur_pin connects to
                    pins_equivalent = false;
                }
            }
            edge_count++;
        }
    }

    if (edge_count != (int)logic_equivalent_pins_map.size()) {
        // The number of outgoing edges for each pin of an logically-equivalent
        // port should be exactly the same
        pins_equivalent = false;
    }

    return pins_equivalent;
}

const t_pb_graph_edge* get_edge_between_pins(const t_pb_graph_pin* driver_pin, const t_pb_graph_pin* pin) {
    if (driver_pin == nullptr || pin == nullptr) {
        return nullptr;
    }

    int node_index = pin->pin_count_in_cluster;
    for (int iedge = 0; iedge < driver_pin->num_output_edges; iedge++) {
        t_pb_graph_edge* edge = driver_pin->output_edges[iedge];
        VTR_ASSERT(edge->num_output_pins == 1);

        if (edge->output_pins[0]->pin_count_in_cluster == node_index) {
            return edge;
        }
    }

    return nullptr;
}

/* Date:June 8th, 2024
 * Author: Kate Thurmer
 * Purpose: This subroutine computes the index of a pb graph node at its
 *          level of the pb hierarchy; it is computed by the parent and
 *          passed to each child of each child pb type. When the child is
 *          a primitive, the computed indes is its flat site index.
 *          For example, if there are 10 ALMs, each with 2 FFs and 2 LUTs,
 *          then the ALM at index N, when calling this function for
 *          its FF child at index M, would compute the child's index as:
 *          N*(FFs per ALM) + M
 *          e.g. for FF[1] in ALM[5], this returns
 *          5*(2 FFS per ALM) + 1 = 11
 */
static int compute_flat_index_for_child_node(int num_children_of_type,
                                             int parent_flat_index,
                                             int child_index) {
    return parent_flat_index * num_children_of_type + child_index;
}
