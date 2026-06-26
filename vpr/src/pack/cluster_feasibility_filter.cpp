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
 * TODO (for Haydar myself): You can try to add reference to the Jason Luu PhD Thesis if doi available.
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
static void bfs_and_assign_class(t_pb_graph_pin* seed_pin,
                                 const int node_depth,
                                 const int class_id,
                                 std::unordered_set<t_pb_graph_pin*>& visited);
static void load_all_pin_classes(t_pb_graph_node* pb_graph_node);

static void discover_all_forced_connections(t_pb_graph_node* pb_graph_node);
static bool is_forced_connection(const t_pb_graph_pin* pb_graph_pin);

/**
 * Identify all pin class information for complex block
 */
void load_pin_classes_in_pb_graph_head(t_pb_graph_node* pb_graph_node) {
    /* Allocate memory for primitives */
    alloc_pin_classes_in_pb_graph_node(pb_graph_node);

    load_all_pin_classes(pb_graph_node);

    /* Load internal output-to-input connections within each cluster */
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
 * @brief BFS from a primitive seed_pin in all directions.
 *
 * Assigns class_id to:
 *  - Every reachable primitive pin of the same type (input/clock or output)
 *    that is still UNDEFINED at node_depth  →  parent_pin_class[node_depth]
 *  - Every reachable boundary pin of the cluster node at node_depth that is
 *    of the same type  →  pin_class
 *
 * Intermediate (non-primitive, non-boundary) pins are traversed but not assigned.
 * visited prevents re-entering the same pin within one BFS call.
 *
 * @param seed_pin   A primitive pin that seeds the BFS. Must be a primitive pin.
 * @param node_depth Depth of the cluster node whose pin classes are being computed.
 * @param class_id   The class ID to assign to all pins in this equivalence group.
 * @param visited    Set of already-visited pins for this BFS call.
 */
static void bfs_and_assign_class(t_pb_graph_pin* seed_pin,
                                 const int node_depth,
                                 const int class_id,
                                 std::unordered_set<t_pb_graph_pin*>& visited) {
    std::queue<t_pb_graph_pin*> queue;
    queue.push(seed_pin);
    visited.insert(seed_pin);

    while (!queue.empty()) {
        t_pb_graph_pin* pin = queue.front();
        queue.pop();

        const int pin_node_depth = pin->parent_node->pb_type->depth;

        if (pin->is_primitive_pin()) {
            /* Assign class to same-type primitive pins. */
            if (pin->port->type == seed_pin->port->type
                && pin->port->is_clock == seed_pin->port->is_clock
                && pin->parent_pin_class[node_depth] == UNDEFINED) {
                pin->parent_pin_class[node_depth] = class_id;
            }
        } else if (pin_node_depth == node_depth) {
            /* Assign class to same-type cluster boundary pins. */
            if (pin->port->type == seed_pin->port->type
                && pin->port->is_clock == seed_pin->port->is_clock) {
                pin->pin_class = class_id;
            }
        }

        /* Decide which edges to follow based on depth relative to the cluster boundary.
         *
         * Inside the cluster (depth > node_depth): follow both directions freely.
         * At the cluster boundary (depth == node_depth): only follow edges going
         *   inward — output_edges for input/clock pins, input_edges for output pins.
         *   This prevents the BFS from escaping into sibling or ancestor subtrees.
         * Above the cluster (depth < node_depth): do not follow any edges.
         */
        if (pin_node_depth > node_depth) {
            for (int i = 0; i < pin->num_input_edges; i++) {
                t_pb_graph_pin* n = pin->input_edges[i]->input_pins[0];
                if (!visited.count(n)) { visited.insert(n); queue.push(n); }
            }
            for (int i = 0; i < pin->num_output_edges; i++) {
                t_pb_graph_pin* n = pin->output_edges[i]->output_pins[0];
                if (!visited.count(n)) { visited.insert(n); queue.push(n); }
            }
        } else if (pin_node_depth == node_depth) {
            if (pin->port->type == IN_PORT || pin->port->is_clock) {
                for (int i = 0; i < pin->num_output_edges; i++) {
                    t_pb_graph_pin* n = pin->output_edges[i]->output_pins[0];
                    if (!visited.count(n)) { visited.insert(n); queue.push(n); }
                }
            } else {
                for (int i = 0; i < pin->num_input_edges; i++) {
                    t_pb_graph_pin* n = pin->input_edges[i]->input_pins[0];
                    if (!visited.count(n)) { visited.insert(n); queue.push(n); }
                }
            }
        }
        /* depth < node_depth: do not follow edges — stay within the cluster. */
    }
}

/**
 * @brief Compute pin classes for every non-primitive node in the pb_graph tree
 *        rooted at pb_graph_node, in a single recursive pass.
 *
 * For each non-primitive node N at depth D:
 *  1. Clear pin_class on N's own boundary pins.
 *  2. For each primitive input/clock pin under N that is still UNDEFINED at
 *     depth D: seed a BFS via bfs_and_assign_class, which tags all connected
 *     same-type primitive pins and boundary pins with the same class ID.
 *     Increment input_class after each BFS.
 *  3. Same for primitive output pins, incrementing output_class.
 *  4. Allocate num_input_pin_class / num_output_pin_class (+1 catch-all for
 *     primitive pins not reachable from any boundary pin) and call sum_pin_class.
 *  5. Recurse into children so every level of the hierarchy is processed.
 *
 * Starting BFS from unassigned primitive pins (rather than boundary pins) ensures
 * that equivalent boundary pins — multiple boundary pins reaching the same set of
 * primitives — are grouped into the same class, matching the original semantics.
 */
static void load_all_pin_classes(t_pb_graph_node* pb_graph_node) {
    if (pb_graph_node->is_primitive())
        return;

    const int node_depth = pb_graph_node->pb_type->depth;
    int input_class = 0;
    int output_class = 0;

    /* Clear pin_class on all boundary pins of this node. */
    for (int i = 0; i < pb_graph_node->num_input_ports; i++)
        for (int j = 0; j < pb_graph_node->num_input_pins[i]; j++)
            pb_graph_node->input_pins[i][j].pin_class = UNDEFINED;
    for (int i = 0; i < pb_graph_node->num_output_ports; i++)
        for (int j = 0; j < pb_graph_node->num_output_pins[i]; j++)
            pb_graph_node->output_pins[i][j].pin_class = UNDEFINED;
    for (int i = 0; i < pb_graph_node->num_clock_ports; i++)
        for (int j = 0; j < pb_graph_node->num_clock_pins[i]; j++)
            pb_graph_node->clock_pins[i][j].pin_class = UNDEFINED;

    /* Collect all primitive nodes in the subtree. */
    std::vector<t_pb_graph_node*> primitives;
    std::queue<t_pb_graph_node*> node_queue;
    node_queue.push(pb_graph_node);
    while (!node_queue.empty()) {
        t_pb_graph_node* node = node_queue.front();
        node_queue.pop();
        if (node->is_primitive()) {
            primitives.push_back(node);
        } else {
            for (int i = 0; i < node->pb_type->num_modes; i++)
                for (int j = 0; j < node->pb_type->modes[i].num_pb_type_children; j++)
                    for (int k = 0; k < node->pb_type->modes[i].pb_type_children[j].num_pb; k++)
                        node_queue.push(&node->child_pb_graph_nodes[i][j][k]);
        }
    }

    /* For each unassigned primitive input/clock pin, BFS to discover its class. */
    for (t_pb_graph_node* prim : primitives) {
        for (int i = 0; i < prim->num_input_ports; i++) {
            for (int j = 0; j < prim->num_input_pins[i]; j++) {
                if (prim->input_pins[i][j].parent_pin_class[node_depth] == UNDEFINED) {
                    std::unordered_set<t_pb_graph_pin*> visited;
                    bfs_and_assign_class(&prim->input_pins[i][j], node_depth, input_class, visited);
                    input_class++;
                }
            }
        }
        for (int i = 0; i < prim->num_clock_ports; i++) {
            for (int j = 0; j < prim->num_clock_pins[i]; j++) {
                if (prim->clock_pins[i][j].parent_pin_class[node_depth] == UNDEFINED) {
                    std::unordered_set<t_pb_graph_pin*> visited;
                    bfs_and_assign_class(&prim->clock_pins[i][j], node_depth, input_class, visited);
                    input_class++;
                }
            }
        }
    }

    /* For each unassigned primitive output pin, BFS to discover its class. */
    for (t_pb_graph_node* prim : primitives) {
        for (int i = 0; i < prim->num_output_ports; i++) {
            for (int j = 0; j < prim->num_output_pins[i]; j++) {
                if (prim->output_pins[i][j].parent_pin_class[node_depth] == UNDEFINED) {
                    std::unordered_set<t_pb_graph_pin*> visited;
                    bfs_and_assign_class(&prim->output_pins[i][j], node_depth, output_class, visited);
                    output_class++;
                }
            }
        }
    }

    /* Allocate class size arrays. +1 for the catch-all class that holds
     * primitive pins not reachable from any cluster boundary pin. */
    pb_graph_node->num_input_pin_class = input_class + 1;
    pb_graph_node->input_pin_class_size = new int[input_class + 1]();
    pb_graph_node->num_output_pin_class = output_class + 1;
    pb_graph_node->output_pin_class_size = new int[output_class + 1]();
    sum_pin_class(pb_graph_node);

    /* Recurse into children so every level of the hierarchy is processed. */
    for (int i = 0; i < pb_graph_node->pb_type->num_modes; i++)
        for (int j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++)
            for (int k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++)
                load_all_pin_classes(&pb_graph_node->child_pb_graph_nodes[i][j][k]);
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
