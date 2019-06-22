/*	Jason Luu
 * April 15, 2011
 * Loads statistical information (min/max delays, power) onto the pb_graph.  */

#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std;

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "arch_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "pb_type_graph.h"
#include "token.h"
#include "pb_type_graph_annotations.h"
#include "read_xml_arch_file.h"

static void load_pack_pattern_annotations(const int line_num, t_pb_graph_node* pb_graph_node, const int mode, const char* annot_in_pins, const char* annot_out_pins, const char* value);

static void load_delay_annotations(const int line_num,
                                   t_pb_graph_node* pb_graph_node,
                                   const int mode,
                                   const enum e_pin_to_pin_annotation_format input_format,
                                   const enum e_pin_to_pin_delay_annotations delay_type,
                                   const char* annot_in_pins,
                                   const char* annot_out_pins,
                                   const char* clock,
                                   const char* value);

static void inferr_unspecified_pb_graph_node_delays(t_pb_graph_node* pb_graph_node);
static void inferr_unspecified_pb_graph_pin_delays(t_pb_graph_pin* pb_graph_pin);
static void inferr_unspecified_pb_graph_edge_delays(t_pb_graph_edge* pb_graph_pin);

static t_pb_graph_pin* find_clock_pin(t_pb_graph_node* gnode, const char* clock, int line_num);

void load_pb_graph_pin_to_pin_annotations(t_pb_graph_node* pb_graph_node) {
    int i, j, k, m;
    const t_pb_type* pb_type;
    t_pin_to_pin_annotation* annotations;

    pb_type = pb_graph_node->pb_type;

    /* Load primitive critical path delays */
    if (pb_type->num_modes == 0) {
        annotations = pb_type->annotations;
        for (i = 0; i < pb_type->num_annotations; i++) {
            if (annotations[i].type == E_ANNOT_PIN_TO_PIN_DELAY) {
                for (j = 0; j < annotations[i].num_value_prop_pairs; j++) {
                    if (annotations[i].prop[j] == E_ANNOT_PIN_TO_PIN_DELAY_MAX
                        || annotations[i].prop[j] == E_ANNOT_PIN_TO_PIN_DELAY_MIN
                        || annotations[i].prop[j] == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX
                        || annotations[i].prop[j] == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN
                        || annotations[i].prop[j] == E_ANNOT_PIN_TO_PIN_DELAY_TSETUP
                        || annotations[i].prop[j] == E_ANNOT_PIN_TO_PIN_DELAY_THOLD) {
                        load_delay_annotations(annotations[i].line_num, pb_graph_node, OPEN,
                                               annotations[i].format, (enum e_pin_to_pin_delay_annotations)annotations[i].prop[j],
                                               annotations[i].input_pins,
                                               annotations[i].output_pins,
                                               annotations[i].clock,
                                               annotations[i].value[j]);
                    } else {
                        VTR_ASSERT(false);
                    }
                }
            }
        }
    } else {
        /* Load interconnect delays */
        for (i = 0; i < pb_type->num_modes; i++) {
            for (j = 0; j < pb_type->modes[i].num_interconnect; j++) {
                annotations = pb_type->modes[i].interconnect[j].annotations;
                for (k = 0; k < pb_type->modes[i].interconnect[j].num_annotations; k++) {
                    if (annotations[k].type == E_ANNOT_PIN_TO_PIN_DELAY) {
                        for (m = 0; m < annotations[k].num_value_prop_pairs; m++) {
                            if (annotations[k].prop[m] == E_ANNOT_PIN_TO_PIN_DELAY_MAX
                                || annotations[k].prop[m] == E_ANNOT_PIN_TO_PIN_DELAY_MIN
                                || annotations[k].prop[m] == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX
                                || annotations[k].prop[m] == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN
                                || annotations[k].prop[m] == E_ANNOT_PIN_TO_PIN_DELAY_TSETUP
                                || annotations[k].prop[m] == E_ANNOT_PIN_TO_PIN_DELAY_THOLD) {
                                load_delay_annotations(annotations[k].line_num, pb_graph_node, i,
                                                       annotations[k].format,
                                                       (enum e_pin_to_pin_delay_annotations)annotations[k].prop[m],
                                                       annotations[k].input_pins,
                                                       annotations[k].output_pins,
                                                       annotations[k].clock,
                                                       annotations[k].value[m]);
                            } else {
                                VTR_ASSERT(false);
                            }
                        }
                    } else if (annotations[k].type == E_ANNOT_PIN_TO_PIN_PACK_PATTERN) {
                        VTR_ASSERT(annotations[k].num_value_prop_pairs == 1);
                        load_pack_pattern_annotations(annotations[k].line_num, pb_graph_node, i,
                                                      annotations[k].input_pins,
                                                      annotations[k].output_pins,
                                                      annotations[k].value[0]);
                    } else {
                        /* Todo:
                         * load_power_annotations(pb_graph_node);
                         */
                    }
                }
            }
        }
    }

    inferr_unspecified_pb_graph_node_delays(pb_graph_node);

    //Recursively annotate child pb's
    for (i = 0; i < pb_type->num_modes; i++) {
        for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
            for (k = 0; k < pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                load_pb_graph_pin_to_pin_annotations(&pb_graph_node->child_pb_graph_nodes[i][j][k]);
            }
        }
    }
}

/*
 * Add the pattern name to the pack_pattern field for each pb_graph_edge that is used in a pack pattern
 */
static void load_pack_pattern_annotations(const int line_num, t_pb_graph_node* pb_graph_node, const int mode, const char* annot_in_pins, const char* annot_out_pins, const char* value) {
    int i, j, k, m, n, p, iedge;
    t_pb_graph_pin ***in_port, ***out_port;
    int *num_in_ptrs, *num_out_ptrs, num_in_sets, num_out_sets;
    t_pb_graph_node** children = nullptr;

    children = pb_graph_node->child_pb_graph_nodes[mode];
    in_port = alloc_and_load_port_pin_ptrs_from_string(line_num, pb_graph_node, children,
                                                       annot_in_pins, &num_in_ptrs, &num_in_sets, false, false);
    out_port = alloc_and_load_port_pin_ptrs_from_string(line_num, pb_graph_node, children,
                                                        annot_out_pins, &num_out_ptrs, &num_out_sets, false, false);

    /* Discover edge then annotate edge with name of pack pattern */
    k = 0;
    for (i = 0; i < num_in_sets; i++) {
        for (j = 0; j < num_in_ptrs[i]; j++) {
            p = 0;
            for (m = 0; m < num_out_sets; m++) {
                for (n = 0; n < num_out_ptrs[m]; n++) {
                    for (iedge = 0; iedge < in_port[i][j]->num_output_edges; iedge++) {
                        if (in_port[i][j]->output_edges[iedge]->output_pins[0] == out_port[m][n]) {
                            break;
                        }
                    }
                    /* jluu Todo: This is inefficient, I know the interconnect so I know what edges exist
                     * can use this info to only annotate existing edges */
                    if (iedge != in_port[i][j]->num_output_edges) {
                        in_port[i][j]->output_edges[iedge]->num_pack_patterns++;
                        in_port[i][j]->output_edges[iedge]->pack_pattern_names = (const char**)vtr::realloc(
                            in_port[i][j]->output_edges[iedge]->pack_pattern_names,
                            sizeof(char*) * in_port[i][j]->output_edges[iedge]->num_pack_patterns);
                        in_port[i][j]->output_edges[iedge]->pack_pattern_names[in_port[i][j]->output_edges[iedge]->num_pack_patterns - 1] = value;
                    }
                    p++;
                }
            }
            k++;
        }
    }

    if (in_port != nullptr) {
        for (i = 0; i < num_in_sets; i++) {
            free(in_port[i]);
        }
        free(in_port);
        free(num_in_ptrs);
    }
    if (out_port != nullptr) {
        for (i = 0; i < num_out_sets; i++) {
            free(out_port[i]);
        }
        free(out_port);
        free(num_out_ptrs);
    }
}

static void load_delay_annotations(const int line_num,
                                   t_pb_graph_node* pb_graph_node,
                                   const int mode,
                                   const enum e_pin_to_pin_annotation_format input_format,
                                   const enum e_pin_to_pin_delay_annotations delay_type,
                                   const char* annot_in_pins,
                                   const char* annot_out_pins,
                                   const char* clock,
                                   const char* value) {
    int i, j, k, m, n, p, iedge;
    t_pb_graph_pin ***in_port, ***out_port;
    int *num_in_ptrs, *num_out_ptrs, num_in_sets, num_out_sets;
    float** delay_matrix;
    t_pb_graph_node** children = nullptr;

    int num_inputs, num_outputs;
    int num_entries_in_matrix;

    in_port = out_port = nullptr;
    num_out_sets = num_in_sets = 0;
    num_out_ptrs = num_in_ptrs = nullptr;

    /* Primarily 3 kinds of delays that affect critical path:
     * 1.  Intrablock interconnect delays
     * 2.  Combinational primitives (pin-to-pin delays of primitive)
     * 3.  Sequential primitives (setup and clock-to-q times)
     *
     * Note:	Proper I/O modelling requires knowledge of the extra-chip world (eg. the load that pin is driving, drive strength, etc)
     * For now, I/O delays are modelled as a constant in the architecture file by setting the pad-I/O block interconnect delay to be a constant I/O delay
     *
     * Algorithm: Intrablock and combinational primitive delays apply to edges
     * Sequential delays apply to pins
     * 1.  Determine if delay applies to pin or edge
     * 2.  Format the delay information
     * 3.  Load delay information
     */
    //FIXME: This code is horrible!

    /* Determine what pins to read based on delay type */
    num_inputs = num_outputs = 0;
    if (mode == OPEN) {
        children = nullptr;
    } else {
        children = pb_graph_node->child_pb_graph_nodes[mode];
    }
    if (delay_type == E_ANNOT_PIN_TO_PIN_DELAY_TSETUP
        || delay_type == E_ANNOT_PIN_TO_PIN_DELAY_THOLD
        || delay_type == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN
        || delay_type == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX) {
        VTR_ASSERT(pb_graph_node->pb_type->blif_model != nullptr);
        in_port = alloc_and_load_port_pin_ptrs_from_string(line_num, pb_graph_node,
                                                           children, annot_in_pins, &num_in_ptrs, &num_in_sets, false,
                                                           false);
    } else {
        VTR_ASSERT(delay_type == E_ANNOT_PIN_TO_PIN_DELAY_MAX
                   || delay_type == E_ANNOT_PIN_TO_PIN_DELAY_MIN);
        in_port = alloc_and_load_port_pin_ptrs_from_string(line_num, pb_graph_node,
                                                           children, annot_in_pins, &num_in_ptrs, &num_in_sets, false,
                                                           false);
        out_port = alloc_and_load_port_pin_ptrs_from_string(line_num, pb_graph_node,
                                                            children, annot_out_pins, &num_out_ptrs, &num_out_sets, false,
                                                            false);
    }

    num_inputs = 0;
    for (i = 0; i < num_in_sets; i++) {
        num_inputs += num_in_ptrs[i];
    }

    if (out_port != nullptr) {
        num_outputs = 0;
        for (i = 0; i < num_out_sets; i++) {
            num_outputs += num_out_ptrs[i];
        }
    } else {
        num_outputs = 1;
    }

    //Allocate and load the delay matrix
    delay_matrix = (float**)vtr::malloc(sizeof(float*) * num_inputs);
    for (i = 0; i < num_inputs; i++) {
        delay_matrix[i] = (float*)vtr::malloc(sizeof(float) * num_outputs);
    }

    if (input_format == E_ANNOT_PIN_TO_PIN_MATRIX) {
        if (!check_my_atof_2D(num_inputs, num_outputs, value, &num_entries_in_matrix)) {
            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                      "Number of entries in matrix (%d) does not match number of pin-to-pin"
                      "connections (%d).",
                      num_entries_in_matrix, num_inputs * num_outputs);
        }
        my_atof_2D(delay_matrix, num_inputs, num_outputs, value);
    } else {
        VTR_ASSERT(input_format == E_ANNOT_PIN_TO_PIN_CONSTANT);
        float flt_val = vtr::atof(value);
        for (i = 0; i < num_inputs; i++) {
            for (j = 0; j < num_outputs; j++) {
                delay_matrix[i][j] = flt_val;
            }
        }
    }

    if (delay_type == E_ANNOT_PIN_TO_PIN_DELAY_TSETUP
        || delay_type == E_ANNOT_PIN_TO_PIN_DELAY_THOLD
        || delay_type == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN
        || delay_type == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX) {
        //Annotate primitive sequential timing information
        k = 0;
        for (i = 0; i < num_in_sets; i++) {
            for (j = 0; j < num_in_ptrs[i]; j++) {
                if (delay_type == E_ANNOT_PIN_TO_PIN_DELAY_TSETUP) {
                    in_port[i][j]->tsu = delay_matrix[k][0];
                } else if (delay_type == E_ANNOT_PIN_TO_PIN_DELAY_THOLD) {
                    in_port[i][j]->thld = delay_matrix[k][0];
                } else if (delay_type == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX) {
                    in_port[i][j]->tco_max = delay_matrix[k][0];
                } else {
                    VTR_ASSERT(delay_type == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN);
                    in_port[i][j]->tco_min = delay_matrix[k][0];
                }
                in_port[i][j]->associated_clock_pin = find_clock_pin(pb_graph_node, clock, line_num);
                k++;
            }
        }
    } else {
        VTR_ASSERT(delay_type == E_ANNOT_PIN_TO_PIN_DELAY_MAX
                   || delay_type == E_ANNOT_PIN_TO_PIN_DELAY_MIN);
        if (!pb_graph_node->is_primitive()) {
            /* Not a primitive, annotate pb interconnect delay */

            //Fast look-up for out pin membership
            std::set<t_pb_graph_pin*> out_pins;
            for (m = 0; m < num_out_sets; m++) {
                for (n = 0; n < num_out_ptrs[m]; n++) {
                    out_pins.insert(out_port[m][n]);
                }
            }

            //Mark the edge delays
            //
            //We walk all the out-going edges from the input pins, if the target
            //is in the output set we mark the edge with the appropriate delay
            k = 0;
            for (i = 0; i < num_in_sets; i++) {
                for (j = 0; j < num_in_ptrs[i]; j++) {
                    p = 0;
                    for (iedge = 0; iedge < in_port[i][j]->num_output_edges; iedge++) {
                        t_pb_graph_edge* pb_edge = in_port[i][j]->output_edges[iedge];
                        if (out_pins.count(pb_edge->output_pins[0])) {
                            if (delay_type == E_ANNOT_PIN_TO_PIN_DELAY_MAX) {
                                if (pb_edge->delay_max != 0) {
                                    vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                                              "Multiple max delay values specified");
                                }
                                pb_edge->delay_max = delay_matrix[k][p];
                            } else {
                                VTR_ASSERT(delay_type == E_ANNOT_PIN_TO_PIN_DELAY_MIN);
                                if (pb_edge->delay_min != 0) {
                                    vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                                              "Multiple min delay values specified");
                                }
                                pb_edge->delay_min = delay_matrix[k][p];
                            }
                            ++p;
                        }
                    }
                    ++k;
                }
            }
        } else {
            /* Primitive, annotate combinational delay */
            //Record the existing src/sink pairs
            std::set<std::pair<t_pb_graph_pin*, t_pb_graph_pin*>> existing_edges;
            for (i = 0; i < num_in_sets; i++) {
                for (j = 0; j < num_in_ptrs[i]; j++) {
                    t_pb_graph_pin* src_pin = in_port[i][j];
                    for (k = 0; k < src_pin->num_pin_timing; ++k) {
                        t_pb_graph_pin* sink_pin = src_pin->pin_timing[k];
                        auto edge_pair = std::make_pair(src_pin, sink_pin);
                        VTR_ASSERT_MSG(!existing_edges.count(edge_pair), "No duplicates");
                        existing_edges.emplace(src_pin, sink_pin);
                    }
                }
            }

            //Identify any new src/sink pairs in the current annotation
            std::multimap<t_pb_graph_pin*, t_pb_graph_pin*> new_edges;
            for (i = 0; i < num_in_sets; i++) {
                for (j = 0; j < num_in_ptrs[i]; j++) {
                    t_pb_graph_pin* src_pin = in_port[i][j];
                    for (m = 0; m < num_out_sets; m++) {
                        for (n = 0; n < num_out_ptrs[m]; n++) {
                            t_pb_graph_pin* sink_pin = out_port[m][n];
                            auto edge = std::make_pair(src_pin, sink_pin);
                            if (!existing_edges.count(edge)) {
                                new_edges.insert(edge);
                            }
                        }
                    }
                }
            }

            //Allocate space for any new src/sink pairs
            k = 0;
            for (i = 0; i < num_in_sets; i++) {
                for (j = 0; j < num_in_ptrs[i]; j++) {
                    t_pb_graph_pin* src_pin = in_port[i][j];

                    auto new_sink_range = new_edges.equal_range(src_pin);
                    size_t num_new_sinks = std::distance(new_sink_range.first, new_sink_range.second);
                    if (num_new_sinks > 0) {
                        //Reallocate
                        src_pin->num_pin_timing += num_new_sinks;
                        src_pin->pin_timing = (t_pb_graph_pin**)vtr::realloc(src_pin->pin_timing, sizeof(t_pb_graph_pin*) * src_pin->num_pin_timing);
                        src_pin->pin_timing_del_max = (float*)vtr::realloc(src_pin->pin_timing_del_max, sizeof(float) * src_pin->num_pin_timing);
                        src_pin->pin_timing_del_min = (float*)vtr::realloc(src_pin->pin_timing_del_min, sizeof(float) * src_pin->num_pin_timing);

                        //Store the sink pins and initial delays to invalid
                        size_t ipin_timing = src_pin->num_pin_timing - num_new_sinks;
                        for (auto iter = new_sink_range.first; iter != new_sink_range.second; ++iter) {
                            src_pin->pin_timing[ipin_timing] = iter->second;
                            src_pin->pin_timing_del_max[ipin_timing] = std::numeric_limits<float>::quiet_NaN();
                            src_pin->pin_timing_del_min[ipin_timing] = std::numeric_limits<float>::quiet_NaN();
                            ++ipin_timing;
                        }
                    }

                    //Apply the timing annotation (now that all pin_timing for it exist)
                    p = 0;
                    for (m = 0; m < num_out_sets; m++) {
                        for (n = 0; n < num_out_ptrs[m]; n++) {
                            if (delay_type == E_ANNOT_PIN_TO_PIN_DELAY_MAX) {
                                if (src_pin->num_pin_timing_del_max_annotated >= src_pin->num_pin_timing) {
                                    vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                                              "Max delay already appears annotated on '%s' port '%s' (duplicate delay annotations?)",
                                              src_pin->parent_node->pb_type->name,
                                              src_pin->port->name);
                                }

                                if (!std::isnan(src_pin->pin_timing_del_max[src_pin->num_pin_timing_del_max_annotated])) {
                                    vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                                              "Multiple max delay values specified");
                                }

                                src_pin->pin_timing_del_max[src_pin->num_pin_timing_del_max_annotated] = delay_matrix[k][p];
                                src_pin->num_pin_timing_del_max_annotated++;

                            } else {
                                VTR_ASSERT(delay_type == E_ANNOT_PIN_TO_PIN_DELAY_MIN);
                                if (src_pin->num_pin_timing_del_min_annotated >= src_pin->num_pin_timing) {
                                    vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                                              "Min delay already appears annotated on '%s' port '%s' (duplicate delay annotations?)",
                                              src_pin->parent_node->pb_type->name,
                                              src_pin->port->name);
                                }

                                if (!std::isnan(src_pin->pin_timing_del_min[src_pin->num_pin_timing_del_min_annotated])) {
                                    vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                                              "Multiple min delay values specified");
                                }

                                src_pin->pin_timing_del_min[src_pin->num_pin_timing_del_min_annotated] = delay_matrix[k][p];
                                src_pin->num_pin_timing_del_min_annotated++;
                            }
                            p++;
                        }
                    }
                    k++;
                }
            }
        }
    }

    //Clean-up
    if (in_port != nullptr) {
        for (i = 0; i < num_in_sets; i++) {
            free(in_port[i]);
        }
        free(in_port);
        free(num_in_ptrs);
    }
    if (out_port != nullptr) {
        for (i = 0; i < num_out_sets; i++) {
            free(out_port[i]);
        }
        free(out_port);
        free(num_out_ptrs);
    }
    for (i = 0; i < num_inputs; i++) {
        free(delay_matrix[i]);
    }
    free(delay_matrix);
}

static void inferr_unspecified_pb_graph_node_delays(t_pb_graph_node* pb_graph_node) {
    //Walk through all the pin timing, and sequential values and edges to inferr
    //any missing min/max setup/hold values if one of the pair is unspecified.
    //
    //For example if only max and setup delays were specified, then the min and
    //hold delays would be set to the same values.

    for (int iport = 0; iport < pb_graph_node->num_input_ports; ++iport) {
        for (int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ++ipin) {
            t_pb_graph_pin* pin = &pb_graph_node->input_pins[iport][ipin];
            inferr_unspecified_pb_graph_pin_delays(pin);
        }
    }

    for (int iport = 0; iport < pb_graph_node->num_output_ports; ++iport) {
        for (int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ++ipin) {
            t_pb_graph_pin* pin = &pb_graph_node->output_pins[iport][ipin];
            inferr_unspecified_pb_graph_pin_delays(pin);
        }
    }

    for (int iport = 0; iport < pb_graph_node->num_clock_ports; ++iport) {
        for (int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ++ipin) {
            t_pb_graph_pin* pin = &pb_graph_node->clock_pins[iport][ipin];
            inferr_unspecified_pb_graph_pin_delays(pin);
        }
    }
}

static void inferr_unspecified_pb_graph_pin_delays(t_pb_graph_pin* pb_graph_pin) {
    /*
     * Sequential values
     */
    if (std::isnan(pb_graph_pin->thld) && !std::isnan(pb_graph_pin->tsu)) {
        //Hold missing, inferr from setup
        pb_graph_pin->thld = pb_graph_pin->tsu;
    } else if (!std::isnan(pb_graph_pin->thld) && std::isnan(pb_graph_pin->thld)) {
        //Setup missing, inferr from hold
        pb_graph_pin->tsu = pb_graph_pin->thld;
    }

    if (std::isnan(pb_graph_pin->tco_min) && !std::isnan(pb_graph_pin->tco_max)) {
        //tco_min missing, inferr from tco_max
        pb_graph_pin->tco_min = pb_graph_pin->tco_max;
    } else if (!std::isnan(pb_graph_pin->tco_min) && std::isnan(pb_graph_pin->tco_min)) {
        //tco_max missing, inferr from tco_min
        pb_graph_pin->tco_max = pb_graph_pin->tco_min;
    }

    /*
     * Combinational delays (i.e. pin_timing)
     */
    for (int ipin_timing = 0; ipin_timing < pb_graph_pin->num_pin_timing; ++ipin_timing) {
        if (std::isnan(pb_graph_pin->pin_timing_del_min[ipin_timing]) && !std::isnan(pb_graph_pin->pin_timing_del_max[ipin_timing])) {
            //min missing, inferr from max
            pb_graph_pin->pin_timing_del_min[ipin_timing] = pb_graph_pin->pin_timing_del_max[ipin_timing];
        } else if (!std::isnan(pb_graph_pin->pin_timing_del_min[ipin_timing]) && std::isnan(pb_graph_pin->pin_timing_del_max[ipin_timing])) {
            //max missing, inferr from min
            pb_graph_pin->pin_timing_del_max[ipin_timing] = pb_graph_pin->pin_timing_del_min[ipin_timing];
        }
    }

    /*
     * Edges (i.e. intra-pb interconnect)
     */
    for (int iedge = 0; iedge < pb_graph_pin->num_input_edges; ++iedge) {
        inferr_unspecified_pb_graph_edge_delays(pb_graph_pin->input_edges[iedge]);
    }

    for (int iedge = 0; iedge < pb_graph_pin->num_output_edges; ++iedge) {
        inferr_unspecified_pb_graph_edge_delays(pb_graph_pin->output_edges[iedge]);
    }
}

static void inferr_unspecified_pb_graph_edge_delays(t_pb_graph_edge* pb_graph_edge) {
    if (std::isnan(pb_graph_edge->delay_min) && !std::isnan(pb_graph_edge->delay_max)) {
        //min missing, inferr from max
        pb_graph_edge->delay_min = pb_graph_edge->delay_max;
    } else if (!std::isnan(pb_graph_edge->delay_min) && std::isnan(pb_graph_edge->delay_min)) {
        //max missing, inferr from min
        pb_graph_edge->delay_max = pb_graph_edge->delay_min;
    }
}

static t_pb_graph_pin* find_clock_pin(t_pb_graph_node* gnode, const char* clock, int line_num) {
    t_pb_graph_pin* clock_pin = nullptr;
    for (int iport = 0; iport < gnode->num_clock_ports; ++iport) {
        VTR_ASSERT(gnode->num_clock_pins[iport] == 1);

        if (strcmp(clock, gnode->clock_pins[iport][0].port->name) == 0) {
            VTR_ASSERT(!clock_pin);
            if (clock_pin) {
                vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                          "Found duplicate clock-pin match");
            }
            clock_pin = &gnode->clock_pins[iport][0];
        }
    }

    if (!clock_pin) {
        vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
                  "Failed to find associated clock pin");
    }
    return clock_pin;
}
