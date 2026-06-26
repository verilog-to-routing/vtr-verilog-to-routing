/*
 * Feasibility filter used during packing that determines if various necessary conditions for legality are met
 *
 * Important for 2 reasons:
 * 1) Quickly reject cases that are bad so that we don't waste time exploring useless cases in packing
 * 2) Robustness issue. During packing, we have a limited size queue to store candidates to try to pack.
 * A good filter helps keep that queue filled with candidates likely to pass.
 *
 * 1st major filter: Pin counting based on pin classes
 * Rationale: If the number of a particular group of pins supplied by the pb_graph_node in the architecture
 * is insufficient to meet a candidate packing solution's demand for that group of pins, then that candidate
 * solution is for sure invalid without any further legalization checks. For example, if a candidate solution
 * requires 2 clock pins but the architecture only has one clock, then that solution can't be legal.
 *
 * Implementation details:
 * a) Definition of a pin class - If there exists a path (ignoring directionality of connections) from pin A
 * to pin B and pin A and pin B are of the same type (input, output, or clock), then pin A and pin B are
 * in the same pin class. Otherwise, pin A and pin B are in different pin classes.
 * b) Code Identifies pin classes. Given a candidate solution
 *
 * TODO: May 30, 2012 Jason Luu - Must take into consideration modes when doing pin counting. For fracturable
 * LUTs FI = 5, the soft logic block sees 6 pins instead of 5 pins for the dual LUT mode messing up the pin counter.
 * The packer still produces correct results but runs slower than its best (experiment on a modified architecture
 * file that forces correct pin counting shows 40x speedup vs VPR 6.0 as opposed to 3x speedup at the time)
 *
 * Author: Jason Luu
 * Date: May 16, 2012
 */

#include <queue>
#include <unordered_set>
#include <vector>
#include "physical_types.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vpr_types.h"

#include "cluster_feasibility_filter.h"

/* header functions that identify pin classes */
static void alloc_pin_classes_in_pb_graph_node(t_pb_graph_node* pb_graph_node);
static void load_list_of_connectable_input_pin_ptrs(t_pb_graph_node* pb_graph_node);
static void expand_pb_graph_node_and_load_output_to_input_connections(t_pb_graph_pin* current_pb_graph_pin,
                                                                      t_pb_graph_pin* reference_pin,
                                                                      const int depth,
                                                                      std::unordered_set<t_pb_graph_pin*>& seen_pins);
static void unmark_fanout_intermediate_nodes(t_pb_graph_pin* current_pb_graph_pin,
                                             std::unordered_set<t_pb_graph_pin*>& seen_pins);
static void sum_pin_class(t_pb_graph_node* pb_graph_node);
static void assign_pin_class_in_subtree(t_pb_graph_pin* seed_pin,
                                 t_pb_graph_node* pb_graph_node,
                                 const int class_id);
static void load_all_pin_classes(t_pb_graph_node* pb_graph_node);

static void discover_all_forced_connections(t_pb_graph_node* pb_graph_node);
static bool is_forced_connection(const t_pb_graph_pin* pb_graph_pin);

/**
 * @brief Identify all pin class information for complex block
 */
void load_pin_classes_in_pb_graph_head(t_pb_graph_node* pb_graph_node) {
    // Allocate memory for primitives
    alloc_pin_classes_in_pb_graph_node(pb_graph_node);

    // Assign pin classes at every level of the pb_graph hierarchy.
    load_all_pin_classes(pb_graph_node);

    // Load internal output-to-input connections within each cluster
    load_list_of_connectable_input_pin_ptrs(pb_graph_node);
    discover_all_forced_connections(pb_graph_node);
}

/**
 * Recursive function to allocate memory space for pin classes in primitives
 */
static void alloc_pin_classes_in_pb_graph_node(t_pb_graph_node* pb_graph_node) {
    int i, j, k;

    /* If primitive, allocate space, else go to primitive */
    if (pb_graph_node->is_primitive()) {
        /* allocate space */
        for (i = 0; i < pb_graph_node->num_input_ports; i++) {
            for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
                pb_graph_node->input_pins[i][j].parent_pin_class = new int[pb_graph_node->pb_type->depth];
                for (k = 0; k < pb_graph_node->pb_type->depth; k++) {
                    pb_graph_node->input_pins[i][j].parent_pin_class[k] = UNDEFINED;
                }
            }
        }
        for (i = 0; i < pb_graph_node->num_output_ports; i++) {
            for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
                pb_graph_node->output_pins[i][j].parent_pin_class = new int[pb_graph_node->pb_type->depth];
                pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs = new t_pb_graph_pin**[pb_graph_node->pb_type->depth];
                pb_graph_node->output_pins[i][j].num_connectable_primitive_input_pins = new int[pb_graph_node->pb_type->depth];
                for (k = 0; k < pb_graph_node->pb_type->depth; k++) {
                    pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs[k] = nullptr;
                    pb_graph_node->output_pins[i][j].num_connectable_primitive_input_pins[k] = 0;
                    pb_graph_node->output_pins[i][j].parent_pin_class[k] = UNDEFINED;
                }
            }
        }
        for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
            for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
                pb_graph_node->clock_pins[i][j].parent_pin_class = new int[pb_graph_node->pb_type->depth];
                for (k = 0; k < pb_graph_node->pb_type->depth; k++) {
                    pb_graph_node->clock_pins[i][j].parent_pin_class[k] = UNDEFINED;
                }
            }
        }
    } else {
        for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
            for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
                for (k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                    alloc_pin_classes_in_pb_graph_node(&pb_graph_node->child_pb_graph_nodes[i][j][k]);
                }
            }
        }
    }
}

/**
 * Load internal output-to-input connections within each cluster
 */
static void load_list_of_connectable_input_pin_ptrs(t_pb_graph_node* pb_graph_node) {
    int i, j, k;

    // We keep track of the pins we have already seen when expanding the pb
    // graph.
    std::unordered_set<t_pb_graph_pin*> seen_pins;

    if (pb_graph_node->is_primitive()) {
        /* If this is a primitive, discover what input pins the output pins can connect to */
        for (i = 0; i < pb_graph_node->num_output_ports; i++) {
            for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
                for (k = 0; k < pb_graph_node->pb_type->depth; k++) {
                    expand_pb_graph_node_and_load_output_to_input_connections(&pb_graph_node->output_pins[i][j],
                                                                              &pb_graph_node->output_pins[i][j], k,
                                                                              seen_pins);
                    unmark_fanout_intermediate_nodes(&pb_graph_node->output_pins[i][j],
                                                     seen_pins);
                }
            }
        }
    }
    /* Expand down to primitives */
    for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
        for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
            for (k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                load_list_of_connectable_input_pin_ptrs(&pb_graph_node->child_pb_graph_nodes[i][j][k]);
            }
        }
    }
}

/*	Traverse outputs of output pin or primitive to see what input pins it reaches
 * Record list of input pins based on depth
 */
static void expand_pb_graph_node_and_load_output_to_input_connections(t_pb_graph_pin* current_pb_graph_pin,
                                                                      t_pb_graph_pin* reference_pin,
                                                                      const int depth,
                                                                      std::unordered_set<t_pb_graph_pin*>& seen_pins) {
    int i;

    if (seen_pins.count(current_pb_graph_pin) == 0
        && current_pb_graph_pin->parent_node->pb_type->depth > depth) {
        seen_pins.insert(current_pb_graph_pin);
        for (i = 0; i < current_pb_graph_pin->num_output_edges; i++) {
            VTR_ASSERT(current_pb_graph_pin->output_edges[i]->num_output_pins == 1);
            expand_pb_graph_node_and_load_output_to_input_connections(current_pb_graph_pin->output_edges[i]->output_pins[0],
                                                                      reference_pin, depth,
                                                                      seen_pins);
        }
        if (current_pb_graph_pin->is_primitive_pin()
            && current_pb_graph_pin->port->type == IN_PORT) {
            reference_pin->num_connectable_primitive_input_pins[depth]++;

            if (reference_pin->num_connectable_primitive_input_pins[depth] - 1 > 0) {
                std::vector<t_pb_graph_pin*> temp(reference_pin->list_of_connectable_input_pin_ptrs[depth],
                                                  reference_pin->list_of_connectable_input_pin_ptrs[depth] + reference_pin->num_connectable_primitive_input_pins[depth] - 1);

                delete[] reference_pin->list_of_connectable_input_pin_ptrs[depth];
                reference_pin->list_of_connectable_input_pin_ptrs[depth] = new t_pb_graph_pin*[reference_pin->num_connectable_primitive_input_pins[depth]];
                for (i = 0; i < reference_pin->num_connectable_primitive_input_pins[depth] - 1; i++)
                    reference_pin->list_of_connectable_input_pin_ptrs[depth][i] = temp[i];

                reference_pin->list_of_connectable_input_pin_ptrs[depth][reference_pin->num_connectable_primitive_input_pins[depth]
                                                                         - 1] = current_pb_graph_pin;
            }

            else {
                reference_pin->list_of_connectable_input_pin_ptrs[depth] = new t_pb_graph_pin*[reference_pin->num_connectable_primitive_input_pins[depth]];
            }
            reference_pin->list_of_connectable_input_pin_ptrs[depth][reference_pin->num_connectable_primitive_input_pins[depth]
                                                                     - 1] = current_pb_graph_pin;
        }
    }
}

/**
 * Mark all fanout of pin as un-seen.
 */
static void unmark_fanout_intermediate_nodes(t_pb_graph_pin* current_pb_graph_pin,
                                             std::unordered_set<t_pb_graph_pin*>& seen_pins) {
    int i;
    if (seen_pins.count(current_pb_graph_pin) != 0) {
        seen_pins.erase(current_pb_graph_pin);
        for (i = 0; i < current_pb_graph_pin->num_output_edges; i++) {
            VTR_ASSERT(current_pb_graph_pin->output_edges[i]->num_output_pins == 1);
            unmark_fanout_intermediate_nodes(current_pb_graph_pin->output_edges[i]->output_pins[0],
                                             seen_pins);
        }
    }
}

/* count up pin classes of the same number for the given cluster */
static void sum_pin_class(t_pb_graph_node* pb_graph_node) {
    int i, j;

    /* This is a primitive, for each pin in primitive, sum appropriate pin class */
    for (i = 0; i < pb_graph_node->num_input_ports; i++) {
        for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
            VTR_ASSERT(pb_graph_node->input_pins[i][j].pin_class < pb_graph_node->num_input_pin_class);
            if (pb_graph_node->input_pins[i][j].pin_class == UNDEFINED) {
                VTR_LOG_WARN("%s[%d].%s[%d] unconnected pin in architecture.\n",
                             pb_graph_node->pb_type->name,
                             pb_graph_node->placement_index,
                             pb_graph_node->input_pins[i][j].port->name,
                             pb_graph_node->input_pins[i][j].pin_number);
                continue;
            }
            pb_graph_node->input_pin_class_size[pb_graph_node->input_pins[i][j].pin_class]++;
        }
    }
    for (i = 0; i < pb_graph_node->num_output_ports; i++) {
        for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
            VTR_ASSERT(pb_graph_node->output_pins[i][j].pin_class < pb_graph_node->num_output_pin_class);
            if (pb_graph_node->output_pins[i][j].pin_class == UNDEFINED) {
                VTR_LOG_WARN("%s[%d].%s[%d] unconnected pin in architecture.\n",
                             pb_graph_node->pb_type->name,
                             pb_graph_node->placement_index,
                             pb_graph_node->output_pins[i][j].port->name,
                             pb_graph_node->output_pins[i][j].pin_number);
                continue;
            }
            pb_graph_node->output_pin_class_size[pb_graph_node->output_pins[i][j].pin_class]++;
        }
    }
    for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
        for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
            VTR_ASSERT(pb_graph_node->clock_pins[i][j].pin_class < pb_graph_node->num_input_pin_class);
            if (pb_graph_node->clock_pins[i][j].pin_class == UNDEFINED) {
                VTR_LOG_WARN("%s[%d].%s[%d] unconnected pin in architecture.\n",
                             pb_graph_node->pb_type->name,
                             pb_graph_node->placement_index,
                             pb_graph_node->clock_pins[i][j].port->name,
                             pb_graph_node->clock_pins[i][j].pin_number);
                continue;
            }
            pb_graph_node->input_pin_class_size[pb_graph_node->clock_pins[i][j].pin_class]++;
        }
    }
}

/**
 * @brief Assigns class_id to all same-type pins reachable from seed_pin
 *        within pb_graph_node and its subtree.
 *
 * Starting from seed_pin, the BFS follows edges in both directions freely inside the
 * subtree. At the boundary of pb_graph_node it only follows edges going inward, preventing
 * the BFS from escaping into sibling or ancestor subtrees.
 *
 * All reachable primitive pins of the same type as seed_pin are assigned class_id via
 * parent_pin_class[depth]. All reachable boundary pins of pb_graph_node of the same type
 * are assigned class_id via pin_class. Intermediate pins are traversed but not assigned.
 *
 * @param seed_pin      A primitive pin that seeds the BFS.
 * @param pb_graph_node The pb_graph node whose subtree the BFS is confined to.
 * @param class_id      The class ID to assign to all pins in this equivalence group.
 */
static void assign_pin_class_in_subtree(t_pb_graph_pin* seed_pin,
                                 t_pb_graph_node* pb_graph_node,
                                 const int class_id) {
    const int node_depth = pb_graph_node->pb_type->depth;

    std::unordered_set<t_pb_graph_pin*> visited;
    std::queue<t_pb_graph_pin*> queue;
    queue.push(seed_pin);
    visited.insert(seed_pin);

    while (!queue.empty()) {
        t_pb_graph_pin* pin = queue.front();
        queue.pop();

        // Assign class_id to same-type primitive pins via parent_pin_class.
        if (pin->is_primitive_pin()) {
            if (pin->port->type == seed_pin->port->type
                && pin->port->is_clock == seed_pin->port->is_clock
                && pin->parent_pin_class[node_depth] == UNDEFINED) {
                pin->parent_pin_class[node_depth] = class_id;
            }
        // Assign class_id to same-type boundary pins of pb_graph_node via pin_class.
        } else if (pin->parent_node == pb_graph_node) {
            if (pin->port->type == seed_pin->port->type
                && pin->port->is_clock == seed_pin->port->is_clock) {
                pin->pin_class = class_id;
            }
        }

        // Inside the subtree: follow edges in both directions.
        // At the boundary: only follow edges going inward to stay within the subtree.
        // Above the boundary: do not follow any edges.
        const int pin_node_depth = pin->parent_node->pb_type->depth;
        if (pin_node_depth > node_depth) {
            for (int edge_idx = 0; edge_idx < pin->num_input_edges; edge_idx++) {
                VTR_ASSERT(pin->input_edges[edge_idx]->num_input_pins == 1);
                t_pb_graph_pin* neighbor_pin = pin->input_edges[edge_idx]->input_pins[0];
                if (!visited.count(neighbor_pin)) {
                    visited.insert(neighbor_pin);
                    queue.push(neighbor_pin);
                }
            }
            for (int edge_idx = 0; edge_idx < pin->num_output_edges; edge_idx++) {
                VTR_ASSERT(pin->output_edges[edge_idx]->num_output_pins == 1);
                t_pb_graph_pin* neighbor_pin = pin->output_edges[edge_idx]->output_pins[0];
                if (!visited.count(neighbor_pin)) {
                    visited.insert(neighbor_pin);
                    queue.push(neighbor_pin);
                }
            }
        } else if (pin->parent_node == pb_graph_node) {
            if (pin->port->type == IN_PORT || pin->port->is_clock) {
                for (int edge_idx = 0; edge_idx < pin->num_output_edges; edge_idx++) {
                    VTR_ASSERT(pin->output_edges[edge_idx]->num_output_pins == 1);
                    t_pb_graph_pin* neighbor_pin = pin->output_edges[edge_idx]->output_pins[0];
                    if (!visited.count(neighbor_pin)) {
                        visited.insert(neighbor_pin);
                        queue.push(neighbor_pin);
                    }
                }
            } else {
                for (int edge_idx = 0; edge_idx < pin->num_input_edges; edge_idx++) {
                    VTR_ASSERT(pin->input_edges[edge_idx]->num_input_pins == 1);
                    t_pb_graph_pin* neighbor_pin = pin->input_edges[edge_idx]->input_pins[0];
                    if (!visited.count(neighbor_pin)) {
                        visited.insert(neighbor_pin);
                        queue.push(neighbor_pin);
                    }
                }
            }
        }
    }
}

/**
 * @brief Assigns pin classes to every non-primitive node in the pb_graph tree
 *        rooted at pb_graph_node.
 *
 * A pin class is a group of pins on a pb_graph node that are mutually reachable
 * through the interconnect within that node's subtree (ignoring edge directionality)
 * and of the same type (input/clock or output). Each class gets a unique ID.
 *
 * The function recurses into children so that pin classes are computed independently
 * at every level of the hierarchy. Each level records:
 *  - pin_class on its own boundary pins (which class they belong to)
 *  - parent_pin_class[depth] on primitive pins (which class they belong to as seen
 *    from the ancestor pb_graph node at this depth)
 */
static void load_all_pin_classes(t_pb_graph_node* pb_graph_node) {
    if (pb_graph_node->is_primitive())
        return;

    const int node_depth = pb_graph_node->pb_type->depth;
    int input_class = 0;
    int output_class = 0;

    // Clear pin_class on all boundary pins of this node.
    for (int port_idx = 0; port_idx < pb_graph_node->num_input_ports; port_idx++)
        for (int pin_idx = 0; pin_idx < pb_graph_node->num_input_pins[port_idx]; pin_idx++)
            pb_graph_node->input_pins[port_idx][pin_idx].pin_class = UNDEFINED;
    for (int port_idx = 0; port_idx < pb_graph_node->num_output_ports; port_idx++)
        for (int pin_idx = 0; pin_idx < pb_graph_node->num_output_pins[port_idx]; pin_idx++)
            pb_graph_node->output_pins[port_idx][pin_idx].pin_class = UNDEFINED;
    for (int port_idx = 0; port_idx < pb_graph_node->num_clock_ports; port_idx++)
        for (int pin_idx = 0; pin_idx < pb_graph_node->num_clock_pins[port_idx]; pin_idx++)
            pb_graph_node->clock_pins[port_idx][pin_idx].pin_class = UNDEFINED;

    // Collect all primitive nodes in the subtree.
    std::vector<t_pb_graph_node*> primitives;
    std::queue<t_pb_graph_node*> node_queue;
    node_queue.push(pb_graph_node);
    while (!node_queue.empty()) {
        t_pb_graph_node* node = node_queue.front();
        node_queue.pop();
        if (node->is_primitive()) {
            primitives.push_back(node);
        } else {
            for (int mode_idx = 0; mode_idx < node->pb_type->num_modes; mode_idx++)
                for (int pb_type_idx = 0; pb_type_idx < node->pb_type->modes[mode_idx].num_pb_type_children; pb_type_idx++)
                    for (int pb_num_idx = 0; pb_num_idx < node->pb_type->modes[mode_idx].pb_type_children[pb_type_idx].num_pb; pb_num_idx++)
                        node_queue.push(&node->child_pb_graph_nodes[mode_idx][pb_type_idx][pb_num_idx]);
        }
    }

    // For each unassigned primitive input, clock, and output pin; BFS to discover its class.
    for (t_pb_graph_node* prim : primitives) {
        for (int port_idx = 0; port_idx < prim->num_input_ports; port_idx++) {
            for (int pin_idx = 0; pin_idx < prim->num_input_pins[port_idx]; pin_idx++) {
                if (prim->input_pins[port_idx][pin_idx].parent_pin_class[node_depth] == UNDEFINED) {
                    assign_pin_class_in_subtree(&prim->input_pins[port_idx][pin_idx], pb_graph_node, input_class);
                    input_class++;
                }
            }
        }
        for (int port_idx = 0; port_idx < prim->num_clock_ports; port_idx++) {
            for (int pin_idx = 0; pin_idx < prim->num_clock_pins[port_idx]; pin_idx++) {
                if (prim->clock_pins[port_idx][pin_idx].parent_pin_class[node_depth] == UNDEFINED) {
                    assign_pin_class_in_subtree(&prim->clock_pins[port_idx][pin_idx], pb_graph_node, input_class);
                    input_class++;
                }
            }
        }
        for (int port_idx = 0; port_idx < prim->num_output_ports; port_idx++) {
            for (int pin_idx = 0; pin_idx < prim->num_output_pins[port_idx]; pin_idx++) {
                if (prim->output_pins[port_idx][pin_idx].parent_pin_class[node_depth] == UNDEFINED) {
                    assign_pin_class_in_subtree(&prim->output_pins[port_idx][pin_idx], pb_graph_node, output_class);
                    output_class++;
                }
            }
        }
    }

    // Allocate class size arrays.
    pb_graph_node->num_input_pin_class = input_class;
    pb_graph_node->input_pin_class_size = new int[input_class]();
    pb_graph_node->num_output_pin_class = output_class;
    pb_graph_node->output_pin_class_size = new int[output_class]();
    sum_pin_class(pb_graph_node);

    // Recurse into children so every level of the hierarchy is processed.
    for (int mode_idx = 0; mode_idx < pb_graph_node->pb_type->num_modes; mode_idx++)
        for (int pb_type_idx = 0; pb_type_idx < pb_graph_node->pb_type->modes[mode_idx].num_pb_type_children; pb_type_idx++)
            for (int pb_num_idx = 0; pb_num_idx < pb_graph_node->pb_type->modes[mode_idx].pb_type_children[pb_type_idx].num_pb; pb_num_idx++)
                load_all_pin_classes(&pb_graph_node->child_pb_graph_nodes[mode_idx][pb_type_idx][pb_num_idx]);
}

/* Recursively visit all pb_graph_pins and determine primitive output pins that
 * connect to nothing else than one primitive input pin. If a net maps to this
 * output pin, then the primitive corresponding to that input must be used */
static void discover_all_forced_connections(t_pb_graph_node* pb_graph_node) {
    int i, j, k;

    /* If primitive, allocate space, else go to primitive */
    if (pb_graph_node->is_primitive()) {
        for (i = 0; i < pb_graph_node->num_output_ports; i++) {
            for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
                pb_graph_node->output_pins[i][j].is_forced_connection = is_forced_connection(&pb_graph_node->output_pins[i][j]);
            }
        }
    } else {
        for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
            for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
                for (k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                    discover_all_forced_connections(&pb_graph_node->child_pb_graph_nodes[i][j][k]);
                }
            }
        }
    }
}

/**
 * Given an output pin, determine if it connects to only one input pin and nothing else.
 */
static bool is_forced_connection(const t_pb_graph_pin* pb_graph_pin) {
    if (pb_graph_pin->num_output_edges > 1) {
        return false;
    }
    if (pb_graph_pin->num_output_edges == 0) {
        if (pb_graph_pin->is_primitive_pin()) {
            /* Check that this pin belongs to a primitive */
            return true;
        } else {
            return false;
        }
    }
    return is_forced_connection(pb_graph_pin->output_edges[0]->output_pins[0]);
}
