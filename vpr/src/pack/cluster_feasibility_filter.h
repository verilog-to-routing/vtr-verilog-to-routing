/*
 * Feasibility filter used during packing that determines if various necessary conditions for legality are met
 *
 * Important for 2 reasons:
 * 1) Quickly reject cases that are bad so that we don't waste time exploring useless cases in packing
 * 2) Robustness issue.  During packing, we have a limited size queue to store candidates to try to pack.  A good filter helps keep that queue filled with candidates likely to pass.
 *
 * 1st major filter: Pin counting based on pin classes
 * Rationale: If the number of a particular gruop of pins supplied by the pb_graph_node in the architecture is insufficient to meet a candidate packing solution's demand for that group of pins, then that
 * candidate solution is for sure invalid without any further legalization checks.  For example, if a candidate solution requires 2 clock pins but the architecture only has one clock, then that solution
 * can't be legal.
 *
 * Implementation details:
 * a) Definition of a pin class - If there exists a path (ignoring directionality of connections) from pin A to pin B and pin A and pin B are of the same type (input, output, or clock), then pin A and pin B are in the same pin class.  Otherwise, pin A and pin B are in different pin classes.
 * b) Code Identifies pin classes.  Given a candidate solution
 *
 * Author: Jason Luu
 * Date: May 16, 2012
 *
 */

#ifndef CLUSTER_FEASIBILITY_CHECK_H
#define CLUSTER_FEASIBILITY_CHECK_H
#include "arch_types.h"

void load_pin_classes_in_pb_graph_head(t_pb_graph_node* pb_graph_node);

#endif
