#include <algorithm>

/* for hb */
#include "memories.h"
#include "adders.h"
#include "multipliers.h"

#include "odin_globals.h"
#include "odin_types.h"
#include "netlist_utils.h"
#include "netlist_statistic.h"
#include "odin_util.h"
#include "vtr_memory.h"

/**
 * The cutoff for the number of netlist nodes. 
 * Technically, Odin-II prints statistics for 
 * netlist nodes that the total number of them
 * is greater than this value. 
 */
#define UNUSED_NODE_TYPE 0

static void init(metric_t* m);
static void print_stats(metric_t* m);
static void copy(metric_t* dest, metric_t* src);

static void add_to_stat(metric_t* dest, long long branching_factor);
static void count_node_type(nnode_t* node, netlist_t* netlist);

static metric_t* get_upward_stat(nnet_t* net, netlist_t* netlist, uintptr_t traverse_mark_number);
static metric_t* get_downward_stat(nnet_t* net, netlist_t* netlist, uintptr_t traverse_mark_number);
static metric_t* get_upward_stat(nnode_t* node, netlist_t* netlist, uintptr_t traverse_mark_number);
static metric_t* get_downward_stat(nnode_t* node, netlist_t* netlist, uintptr_t traverse_mark_number);

static metric_t* get_downward_stat(metric_t* destination, signal_list_t* signals, netlist_t* netlist, uintptr_t traverse_mark_number);
static metric_t* get_upward_stat(metric_t* destination, signal_list_t* signals, netlist_t* netlist, uintptr_t traverse_mark_number);
static metric_t* get_downward_stat(metric_t* destination, nnode_t** node_list, long long node_count, netlist_t* netlist, uintptr_t traverse_mark_number);
static metric_t* get_upward_stat(metric_t* destination, nnode_t** node_list, long long node_count, netlist_t* netlist, uintptr_t traverse_mark_number);
static metric_t* get_downward_stat(metric_t* destination, nnet_t** net_list, long long net_count, netlist_t* netlist, uintptr_t traverse_mark_number);
static metric_t* get_upward_stat(metric_t* destination, nnet_t** net_list, long long net_count, netlist_t* netlist, uintptr_t traverse_mark_number);

static void init(metric_t* m) {
    m->min_depth = 0;
    m->max_depth = 0;
    m->avg_depth = 0;
    m->avg_width = 0;
}

void init_stat(netlist_t* netlist) {
    for (int i = 0; i < operation_list_END; i++) {
        /* we init to -2 to skip unused elements */
        netlist->num_of_type[i] = UNUSED_NODE_TYPE;
    }

    init(&netlist->output_node_stat);
    netlist->num_of_node = 0;
    netlist->num_logic_element = 0;
}

void mixing_optimization_stats(nnode_t* node, netlist_t* netlist) {
    // Reinitialize statistics (to avoid interference)
    init_stat(netlist);
    // assuming the optimization is started from the node of the type that
    // matches the node type
    switch (node->type) {
        case MULTIPLY: {
            stat_t* multiply_stats = get_stats(node, netlist, mult_optimization_traverse_value);
            node->weight = multiply_stats->downward.max_depth;
            vtr::free(multiply_stats);
            break;
        }
        default:
            error_message(NETLIST, unknown_location, "Counting weights for mixing optimization for %i: Hard block type is unimplemented", node->type);
            break;
    }
}

static void print_stats(metric_t* m) {
    printf("\n\t%s:%0.4lf\n\t%s: %0.4lf\n\t%s: %0.4lf\n\t%s: %0.4lf\n",
           "shortest path", m->min_depth,
           "critical path", m->max_depth,
           "average path", m->avg_depth,
           "overall fan-out", m->avg_width);
}
_static_unused(print_stats) //quiet warning

    static void copy(metric_t* dest, metric_t* src) {
    if (dest) {
        init(dest);
        if (src) {
            dest->min_depth = src->min_depth;
            dest->max_depth = src->max_depth;
            dest->avg_depth = src->avg_depth;
            dest->avg_width = src->avg_width;
        }
    }
}

static void aggregate(metric_t* dest, metric_t** sources, long long source_count) {
    long long actual_count = 0;
    init(dest);

    // compute stats from parent
    for (long long i = 0; sources && i < source_count; i += 1) {
        metric_t* src = sources[i];
        if (src) {
            actual_count += 1;
            if (dest->min_depth == 0) {
                dest->min_depth = src->min_depth;
            } else {
                dest->min_depth = std::min(src->min_depth, dest->min_depth);
            }
            dest->max_depth = std::max(src->max_depth, dest->max_depth);
            dest->avg_depth += src->avg_depth;
            dest->avg_width += src->avg_width;
        }
    }

    if (actual_count) {
        dest->avg_depth /= actual_count;
        dest->avg_width /= actual_count;
    }
}

static void add_to_stat(metric_t* dest, long long branching_factor) {
    dest->min_depth += 1;
    dest->max_depth += 1;
    dest->avg_depth += 1;
    dest->avg_width += branching_factor;
}

static bool traverse(nnode_t* node, uintptr_t traverse_mark_number) {
    bool traverse = (node->traverse_visited != traverse_mark_number);
    node->traverse_visited = traverse_mark_number;
    return traverse;
}

static bool traverse(nnet_t* net, uintptr_t traverse_mark_number) {
    bool traverse = (net->traverse_visited != traverse_mark_number);
    net->traverse_visited = traverse_mark_number;
    return traverse;
}

static void increment_type_count(operation_list op, netlist_t* netlist) {
    if (netlist->num_of_type[op] < 0) {
        netlist->num_of_type[op] = 0;
    }
    netlist->num_of_type[op] += 1;
}
static void count_node_type(operation_list op, nnode_t* node, netlist_t* netlist) {
    switch (op) {
        case LOGICAL_OR:
        case LOGICAL_AND:
        case LOGICAL_NOR:
        case LOGICAL_NAND:
        case LOGICAL_XOR:
        case LOGICAL_XNOR:
        case LOGICAL_NOT: {
            increment_type_count(op, netlist);
            count_node_type(GENERIC, node, netlist);
            break;
        }
        case MUX_2: //fallthrough
        case SMUX_2: {
            increment_type_count(MUX_2, netlist);
            count_node_type(GENERIC, node, netlist);
            break;
        }
        case GENERIC:
            /**
             * generic a packed into luts
             * so we **roughly **estimate placements, this allows us
             * to give predictive placement
             */
            if (physical_lut_size > 1 && node->num_input_pins > physical_lut_size) {
                /* the estimate is based on the width of the node split to fit into the lut*/
                long long input_width = node->num_input_pins;
                /* we have to account for the glue logic to join the result down to one pin */
                while (input_width > 1) {
                    long long logic_element = input_width / physical_lut_size;
                    logic_element += ((input_width % physical_lut_size) != 0);
                    input_width = logic_element;
                    netlist->num_logic_element += logic_element;
                }
            } else {
                netlist->num_logic_element += 1;
            }
            increment_type_count(op, netlist);
            netlist->num_of_node += 1;
            break;

        case MINUS:
            /* Minus nodes are built of Add nodes */
            count_node_type(ADD, node, netlist);
            break;

        case PAD_NODE:
        case GND_NODE:
        case VCC_NODE:
            /* These are irrelevent so we dont output */
            break;

        case INPUT_NODE:
        case OUTPUT_NODE:
            /* these stay untouched but are not added to the total*/
            increment_type_count(op, netlist);
            break;

        case CLOCK_NODE:
        case FF_NODE:
        case MULTIPLY:
        case ADD:
        case MEMORY:
        case HARD_IP:
            /* these stay untouched */
            increment_type_count(op, netlist);
            netlist->num_of_node += 1;
            break;

        default:
            /* everything else is generic */
            count_node_type(GENERIC, node, netlist);
            break;
    }
}

static void count_node_type(nnode_t* node, netlist_t* netlist) {
    count_node_type(node->type, node, netlist);
}

static metric_t* get_upward_stat(nnet_t* net, netlist_t* netlist, uintptr_t traverse_mark_number) {
    metric_t* destination = NULL;
    if (net) {
        destination = &(net->stat.upward);

        if (traverse(net, traverse_mark_number)) {
            init(destination);

            if (net->num_driver_pins) {
                metric_t** parent_stat = (metric_t**)vtr::calloc(net->num_driver_pins, sizeof(metric_t*));
                for (int i = 0; i < net->num_driver_pins; i++)
                    parent_stat[i] = get_upward_stat(net->driver_pins[i]->node, netlist, traverse_mark_number);
                aggregate(destination, parent_stat, net->num_driver_pins);
                vtr::free(parent_stat);
            }
        }
    }
    return destination;
}

static metric_t* get_upward_stat(nnode_t* node, netlist_t* netlist, uintptr_t traverse_mark_number) {
    metric_t* destination = NULL;
    if (node) {
        destination = &(node->stat.upward);

        if (traverse(node, traverse_mark_number)) {
            count_node_type(node, netlist);

            init(destination);
            if (node->num_input_pins) {
                metric_t** parent_stat = (metric_t**)vtr::calloc(node->num_input_pins, sizeof(metric_t*));
                for (long long i = 0; i < node->num_input_pins; i++) {
                    if (node->input_pins[i]) {
                        parent_stat[i] = get_upward_stat(node->input_pins[i]->net, netlist, traverse_mark_number);
                    }
                }
                aggregate(destination, parent_stat, node->num_input_pins);
                vtr::free(parent_stat);
            }
            add_to_stat(destination, node->num_input_pins);
        }
    }
    return destination;
}

static metric_t* get_downward_stat(nnet_t* net, netlist_t* netlist, uintptr_t traverse_mark_number) {
    metric_t* destination = NULL;
    if (net) {
        destination = &(net->stat.downward);

        if (traverse(net, traverse_mark_number)) {
            init(destination);
            if (net->num_fanout_pins) {
                metric_t** child_stat = (metric_t**)vtr::calloc(net->num_fanout_pins, sizeof(metric_t*));
                for (long long i = 0; i < net->num_fanout_pins; i++) {
                    if (net->fanout_pins[i]) {
                        child_stat[i] = get_downward_stat(net->fanout_pins[i]->node, netlist, traverse_mark_number);
                    }
                }
                aggregate(destination, child_stat, net->num_fanout_pins);
                vtr::free(child_stat);
            }
        }
    }
    return destination;
}

static metric_t* get_downward_stat(nnode_t* node, netlist_t* netlist, uintptr_t traverse_mark_number) {
    metric_t* destination = NULL;

    if (node) {
        destination = &(node->stat.downward);
        if (traverse(node, traverse_mark_number)) {
            count_node_type(node, netlist);

            init(destination);
            if (node->num_output_pins) {
                metric_t** child_stat = (metric_t**)vtr::calloc(node->num_output_pins, sizeof(metric_t*));
                for (long long i = 0; i < node->num_output_pins; i++) {
                    if (node->output_pins[i]) {
                        child_stat[i] = get_downward_stat(node->output_pins[i]->net, netlist, traverse_mark_number);
                    }
                }
                aggregate(destination, child_stat, node->num_output_pins);
                vtr::free(child_stat);
            }
            add_to_stat(destination, node->num_output_pins);
        }
    }
    return destination;
}

static metric_t* get_downward_stat(metric_t* destination, signal_list_t* signals, netlist_t* netlist, uintptr_t traverse_mark_number) {
    if (signals) {
        if (signals->count) {
            metric_t** child_stat = (metric_t**)vtr::calloc(signals->count, sizeof(metric_t*));
            for (long long i = 0; i < signals->count; i++) {
                if (signals->pins[i]) {
                    // node -> net
                    if (signals->pins[i]->type == OUTPUT) {
                        child_stat[i] = get_downward_stat(signals->pins[i]->net, netlist, traverse_mark_number);
                    }
                    // net -> node
                    else {
                        child_stat[i] = get_downward_stat(signals->pins[i]->node, netlist, traverse_mark_number);
                    }
                }
            }
            aggregate(destination, child_stat, signals->count);
            vtr::free(child_stat);
        }
    }
    return destination;
}

static metric_t* get_upward_stat(metric_t* destination, signal_list_t* signals, netlist_t* netlist, uintptr_t traverse_mark_number) {
    if (signals) {
        if (signals->count) {
            metric_t** child_stat = (metric_t**)vtr::calloc(signals->count, sizeof(metric_t*));
            for (long long i = 0; i < signals->count; i++) {
                if (signals->pins[i]) {
                    // net -> node
                    if (signals->pins[i]->type == INPUT) {
                        child_stat[i] = get_upward_stat(signals->pins[i]->net, netlist, traverse_mark_number);
                    }
                    // node -> net
                    else {
                        child_stat[i] = get_upward_stat(signals->pins[i]->node, netlist, traverse_mark_number);
                    }
                }
            }
            aggregate(destination, child_stat, signals->count);

            vtr::free(child_stat);
        }
    }
    return destination;
}

static metric_t* get_upward_stat(metric_t* destination, nnode_t** node_list, long long node_count, netlist_t* netlist, uintptr_t traverse_mark_number) {
    if (node_list) {
        if (node_count) {
            metric_t** child_stat = (metric_t**)vtr::calloc(node_count, sizeof(metric_t*));
            for (long long i = 0; i < node_count; i++) {
                child_stat[i] = get_upward_stat(node_list[i], netlist, traverse_mark_number);
            }
            aggregate(destination, child_stat, node_count);

            vtr::free(child_stat);
        }
    }
    return destination;
}

static metric_t* get_downward_stat(metric_t* destination, nnode_t** node_list, long long node_count, netlist_t* netlist, uintptr_t traverse_mark_number) {
    if (node_list) {
        if (node_count) {
            metric_t** child_stat = (metric_t**)vtr::calloc(node_count, sizeof(metric_t*));
            for (long long i = 0; i < node_count; i++) {
                child_stat[i] = get_downward_stat(node_list[i], netlist, traverse_mark_number);
            }
            aggregate(destination, child_stat, node_count);

            vtr::free(child_stat);
        }
    }
    return destination;
}

static metric_t* get_upward_stat(metric_t* destination, nnet_t** net_list, long long net_count, netlist_t* netlist, uintptr_t traverse_mark_number) {
    if (net_list) {
        if (net_count) {
            metric_t** child_stat = (metric_t**)vtr::calloc(net_count, sizeof(metric_t*));
            for (long long i = 0; i < net_count; i++) {
                child_stat[i] = get_upward_stat(net_list[i], netlist, traverse_mark_number);
            }
            aggregate(destination, child_stat, net_count);

            vtr::free(child_stat);
        }
    }
    return destination;
}

static metric_t* get_downward_stat(metric_t* destination, nnet_t** net_list, long long net_count, netlist_t* netlist, uintptr_t traverse_mark_number) {
    if (net_list) {
        if (net_count) {
            metric_t** child_stat = (metric_t**)vtr::calloc(net_count, sizeof(metric_t*));
            for (long long i = 0; i < net_count; i++) {
                child_stat[i] = get_downward_stat(net_list[i], netlist, traverse_mark_number);
            }
            aggregate(destination, child_stat, net_count);

            vtr::free(child_stat);
        }
    }
    return destination;
}

stat_t* get_stats(nnode_t* node, netlist_t* netlist, uintptr_t traverse_mark_number) {
    stat_t* stat = (stat_t*)vtr::malloc(sizeof(stat_t));
    copy(&stat->downward, get_downward_stat(node, netlist, traverse_mark_number));
    copy(&stat->upward, get_upward_stat(node, netlist, traverse_mark_number));
    return stat;
}

stat_t* get_stats(nnet_t* net, netlist_t* netlist, uintptr_t traverse_mark_number) {
    stat_t* stat = (stat_t*)vtr::malloc(sizeof(stat_t));
    copy(&stat->downward, get_downward_stat(net, netlist, traverse_mark_number));
    copy(&stat->upward, get_upward_stat(net, netlist, traverse_mark_number));
    return stat;
}

stat_t* get_stats(signal_list_t* input_signals, signal_list_t* output_signals, netlist_t* netlist, uintptr_t traverse_mark_number) {
    stat_t* stat = (stat_t*)vtr::malloc(sizeof(stat_t));
    // we recompute bellow the input since this is were connection can change
    get_downward_stat(&stat->downward, input_signals, netlist, traverse_mark_number);
    // we recompute above the output since this is were connection can change
    get_upward_stat(&stat->upward, output_signals, netlist, traverse_mark_number);
    return stat;
}

stat_t* get_stats(nnode_t** node_list, long long node_count, netlist_t* netlist, uintptr_t traverse_mark_number) {
    stat_t* stat = (stat_t*)vtr::malloc(sizeof(stat_t));
    get_downward_stat(&stat->downward, node_list, node_count, netlist, traverse_mark_number);
    get_upward_stat(&stat->upward, node_list, node_count, netlist, traverse_mark_number);
    return stat;
}

stat_t* get_stats(nnet_t** net_list, long long net_count, netlist_t* netlist, uintptr_t traverse_mark_number) {
    stat_t* stat = (stat_t*)vtr::malloc(sizeof(stat_t));
    get_downward_stat(&stat->downward, net_list, net_count, netlist, traverse_mark_number);
    get_upward_stat(&stat->upward, net_list, net_count, netlist, traverse_mark_number);
    return stat;
}

stat_t* delete_stat(stat_t* stat) {
    if (stat) {
        vtr::free(stat);
        stat = NULL;
    }
    return stat;
}

static const char _travelsal_id = 0;
static const uintptr_t travelsal_id = (uintptr_t)&_travelsal_id;

/*---------------------------------------------------------------------------------------------
 * function: dfs_to_cp() it starts from output towards input of the netlist to calculate critical path
 *-------------------------------------------------------------------------------------------*/
void compute_statistics(netlist_t* netlist, bool display) {
    if (netlist) {
        // reinit the node count
        init_stat(netlist);

        get_upward_stat(&netlist->output_node_stat, netlist->top_output_nodes, netlist->num_top_output_nodes, netlist, travelsal_id + 1);

        if (display) {
            printf("\n\t==== Stats ====\n");
            for (long long op = 0; op < operation_list_END; op += 1) {
                if (netlist->num_of_type[op] > UNUSED_NODE_TYPE) {
                    std::string hdr = std::string("Number of <")
                                      + operation_list_STR[op][ODIN_LONG_STRING]
                                      + "> node: ";

                    printf("%-42s%lld\n", hdr.c_str(), netlist->num_of_type[op]);
                }
            }
            printf("%-42s%lld\n", "Total estimated number of lut: ", netlist->num_logic_element);
            printf("%-42s%lld\n", "Total number of node: ", netlist->num_of_node);
            printf("%-42s%0.0f\n", "Longest path: ", netlist->output_node_stat.max_depth);
            printf("%-42s%0.0f\n", "Average path: ", netlist->output_node_stat.avg_depth);
            printf("\n");
        }
    }
}