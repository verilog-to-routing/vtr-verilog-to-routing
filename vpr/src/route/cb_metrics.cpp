/*
 * Author: Oleg Petelin
 * Date: July 2014
 *
 * The connection block metrics quantify certain qualities of the connection block.
 * There are basically two orthogonal classes of metrics. One which is related to
 * what groups of wires a pin connects to (groups being defined by wire start points).
 * And a second class which is related to the quality of the switch pattern that the pins
 * make into the channel in general (specifically, how much the switches of the pins overlap
 * with eachother). Again, these two classes of metrics are fairly orthogonal, so it is
 * possible to adjust one while keeping the other relatively constant.
 *
 * The code below currently works for the input/output connection blocks of bidirectional
 * architectures and input connection blocks of unidirectional architectures.
 * This code provides functions for calculating the connection block metrics, as well
 * as for adjusting the actual connection block so that a certain value of a specified
 * metric is achieved (this is done using an annealer).
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctime>
#include <utility>
#include <cmath>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <map>
#include <iterator>

#include "vtr_random.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_math.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"

#include "cb_metrics.h"

using namespace std;

/* TODO: move to libarchfpga/include/util.h after ODIN II has been converted to use g++ */
/* a generic function for determining if a given map key exists */
template<typename F, typename T>
bool map_has_key(const F key, const std::map<F, T>* my_map) {
    bool exists;
    typename std::map<F, T>::const_iterator it = my_map->find(key);
    if (my_map->end() != it) {
        exists = true;
    } else {
        exists = false;
    }
    return exists;
}
/* a generic function for determining if a given set element exists */
template<typename T>
bool set_has_element(const T elem, const std::set<T>* my_set) {
    bool exists;
    typename std::set<T>::const_iterator it = my_set->find(elem);
    if (my_set->end() != it) {
        exists = true;
    } else {
        exists = false;
    }
    return exists;
}

/**** Function Declarations ****/
/* goes through each pin of pin_type and determines which side of the block it comes out on. results are stored in
 * the 'pin_locations' 2d-vector */
static void get_pin_locations(const t_type_ptr block_type, const e_pin_type pin_type, const int num_pin_type_pins, int***** tracks_connected_to_pin, t_2d_int_vec* pin_locations);

/* Gets the maximum Fc value from the Fc_array of this pin type. Errors out if the pins of this pin_type don't all have the same Fc */
static int get_max_Fc(const int* Fc_array, const t_type_ptr block_type, const e_pin_type pin_type);

/* initializes the fields of the cb_metrics class */
static void init_cb_structs(const t_type_ptr block_type, int***** tracks_connected_to_pin, const int num_segments, const t_segment_inf* segment_inf, const e_pin_type pin_type, const int num_pin_type_pins, const int nodes_per_chan, const int Fc, Conn_Block_Metrics* cb_metrics);

/* given a set of tracks connected to a pin, we'd like to find which of these tracks are connected to a number of switches
 * greater than 'criteria'. The resulting set of tracks is passed back in the 'result' vector */
static void find_tracks_with_more_switches_than(const set<int>* pin_tracks, const t_vec_vec_set* track_to_pins, const int side, const bool both_sides, const int criteria, vector<int>* result);
/* given a pin on some side of a block, we'd like to find the set of tracks that is NOT connected to that pin on that side. This set of tracks
 * is passed back in the 'result' vector */
static void find_tracks_unconnected_to_pin(const set<int>* pin_tracks, const vector<set<int> >* track_to_pins, vector<int>* result);

/* iterates through the elements of set 1 and returns the number of elements in set1 that are
 * also in set2 (in terms of bit vectors, this looks for the number of positions where both bit vectors
 * have a value of 1; values of 0 not counted... so, not quite true hamming proximity). Analogously, if we
 * wanted the hamming distance of these two sets, (in terms of bit vectors, the number of bit positions that are
 * different... i.e. the actual definition of hamming disatnce) that would be 2(set_size - returned_value) */
static int hamming_proximity_of_two_sets(const set<int>* set1, const set<int>* set2);
/* returns the pin diversity metric of a block */
static float get_pin_diversity(const int Fc, const int num_pin_type_pins, const Conn_Block_Metrics* cb_metrics);
/* Returns the wire homogeneity of a block's connection to tracks */
static float get_wire_homogeneity(const int Fc, const int nodes_per_chan, const int num_pin_type_pins, const int exponent, const bool both_sides, const Conn_Block_Metrics* cb_metrics);
/* Returns the hamming proximity of a block's connection to tracks */
static float get_hamming_proximity(const int Fc, const int num_pin_type_pins, const int exponent, const bool both_sides, const Conn_Block_Metrics* cb_metrics);
/* Returns Lemieux's cost function for sparse crossbars (see his 2001 book) applied here to the connection block */
static float get_lemieux_cost_func(const int exponent, const bool both_sides, const Conn_Block_Metrics* cb_metrics);

/* this annealer is used to adjust a desired wire or pin metric while keeping the other type of metric
 * relatively constant */
static bool annealer(const e_metric metric, const int nodes_per_chan, const t_type_ptr block_type, const e_pin_type pin_type, const int Fc, const int num_pin_type_pins, const float target_metric, const float target_metric_tolerance, int***** pin_to_track_connections, Conn_Block_Metrics* cb_metrics);
/* updates temperature based on current temperature and the annealer's outer loop iteration */
static double update_temp(const double temp);
/* determines whether to accept or reject a proposed move based on the resulting delta of the cost and current temperature */
static bool accept_move(const double del_cost, const double temp);
/* this function simply moves a switch from one track to another track (with an empty slot). The switch stays on the
 * same pin as before. */
static double try_move(const e_metric metric, const int nodes_per_chan, const float initial_orthogonal_metric, const float orthogonal_metric_tolerance, const t_type_ptr block_type, const e_pin_type pin_type, const int Fc, const int num_pin_type_pins, const double cost, const double temp, const float target_metric, int***** pin_to_track_connections, Conn_Block_Metrics* cb_metrics);

static void print_switch_histogram(const int nodes_per_chan, const Conn_Block_Metrics* cb_metrics);

/**** Function Definitions ****/

/* adjusts the connection block until the appropriate metric has hit its target value. the other orthogonal metric is kept constant
 * within some tolerance */
void adjust_cb_metric(const e_metric metric, const float target, const float target_tolerance, const t_type_ptr block_type, int***** pin_to_track_connections, const e_pin_type pin_type, const int* Fc_array, const t_chan_width* chan_width_inf, const int num_segments, const t_segment_inf* segment_inf) {
    Conn_Block_Metrics cb_metrics;

    /* various error checks */
    if (metric >= NUM_METRICS) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Invalid CB metric type specified: %d\n", (int)metric);
    }
    if (block_type->num_pins == 0) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Trying to adjust CB metrics for a block with no pins!\n");
    }
    if (block_type->height > 1 || block_type->width > 1) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Adjusting connection block metrics is currently intended for CLBs, which have height and width = 1\n");
    }
    if (chan_width_inf->x_min != chan_width_inf->x_max || chan_width_inf->y_min != chan_width_inf->y_max
        || chan_width_inf->x_max != chan_width_inf->y_max) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "This code currently assumes that channel width is uniform throughout the fpga");
    }

    int nodes_per_chan = chan_width_inf->x_min;

    /* get Fc */
    int Fc = get_max_Fc(Fc_array, block_type, pin_type);

    /* get the number of block pins that are of pin_type */
    int num_pin_type_pins = 0;
    if (DRIVER == pin_type) {
        num_pin_type_pins = block_type->num_drivers;
    } else if (RECEIVER == pin_type) {
        num_pin_type_pins = block_type->num_receivers;
    } else {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Found unexpected pin type when adjusting CB wire metric: %d\n", pin_type);
    }

    /* get initial values for metrics */
    get_conn_block_metrics(block_type, pin_to_track_connections, num_segments, segment_inf, pin_type,
                           Fc_array, chan_width_inf, &cb_metrics);

    /* now run the annealer to adjust the desired metric towards the target value */
    bool success = annealer(metric, nodes_per_chan, block_type, pin_type, Fc, num_pin_type_pins, target,
                            target_tolerance, pin_to_track_connections, &cb_metrics);
    if (!success) {
        VTR_LOG("Failed to adjust specified connection block metric\n");
    }

    print_switch_histogram(nodes_per_chan, &cb_metrics);
}

/* calculates all the connection block metrics and returns them through the cb_metrics variable */
void get_conn_block_metrics(const t_type_ptr block_type, int***** tracks_connected_to_pin, const int num_segments, const t_segment_inf* segment_inf, const e_pin_type pin_type, const int* Fc_array, const t_chan_width* chan_width_inf, Conn_Block_Metrics* cb_metrics) {
    if (chan_width_inf->x_min != chan_width_inf->x_max || chan_width_inf->y_min != chan_width_inf->y_max
        || chan_width_inf->x_max != chan_width_inf->y_max) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Currently this code assumes that channel width is uniform throughout the fpga");
    }
    int nodes_per_chan = chan_width_inf->x_min;

    /* get Fc */
    int Fc = get_max_Fc(Fc_array, block_type, pin_type);

    /* a bit of error checking */
    if (0 == block_type->index) {
        /* an empty block */
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Cannot calculate CB metrics for empty blocks\n");
    } else if (0 == Fc) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Can not compute CB metrics for pins not connected to any tracks\n");
    }

    /* get the number of block pins that are of pin_type */
    int num_pin_type_pins = UNDEFINED;
    if (DRIVER == pin_type) {
        num_pin_type_pins = block_type->num_drivers;
    } else if (RECEIVER == pin_type) {
        num_pin_type_pins = block_type->num_receivers;
    } else {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Found unexpected pin type when adjusting CB wire metric: %d\n", pin_type);
    }

    /* initialize CB metrics structures */
    init_cb_structs(block_type, tracks_connected_to_pin, num_segments, segment_inf, pin_type, num_pin_type_pins, nodes_per_chan,
                    Fc, cb_metrics);

    /* check based on block type whether we should account for pins on both sides of a channel when computing the relevant CB metrics
     * (i.e. from a block on the left and from a block on the right for a vertical channel, for instance) */
    bool both_sides = false;
    if (0 == strcmp("clb", block_type->name) && DRIVER == pin_type) {
        /* many CLBs are adjacent to eachother, so connections from one CLB
         *  will share the channel segment with its neighbor. We'd like to take this into
         *  account for the applicable metrics. */
        both_sides = true;
    } else {
        /* other blocks (i.e. IO, RAM, etc) are not as frequent as CLBs */
        both_sides = false;
    }

    /* get the metrics */
    cb_metrics->wire_homogeneity = get_wire_homogeneity(Fc, nodes_per_chan, num_pin_type_pins, 2, both_sides, cb_metrics);

    cb_metrics->hamming_proximity = get_hamming_proximity(Fc, num_pin_type_pins, 2, both_sides, cb_metrics);

    cb_metrics->lemieux_cost_func = get_lemieux_cost_func(2, both_sides, cb_metrics);

    cb_metrics->pin_diversity = get_pin_diversity(Fc, num_pin_type_pins, cb_metrics);
}

/* initializes the fields of the cb_metrics class */
static void init_cb_structs(const t_type_ptr block_type, int***** tracks_connected_to_pin, const int num_segments, const t_segment_inf* segment_inf, const e_pin_type pin_type, const int num_pin_type_pins, const int nodes_per_chan, const int Fc, Conn_Block_Metrics* cb_metrics) {
    /* can not calculate CB metrics for open pins */
    if (OPEN == pin_type) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Can not initialize CB metric structures for pins of OPEN type\n");
    }

    int num_wire_types = get_num_wire_types(num_segments, segment_inf);
    cb_metrics->num_wire_types = num_wire_types;

    get_pin_locations(block_type, pin_type, num_pin_type_pins, tracks_connected_to_pin, &cb_metrics->pin_locations);

    /* allocate the multi-dimensional vectors used for conveniently calculating CB metrics */
    for (int iside = 0; iside < 4; iside++) {
        cb_metrics->track_to_pins.push_back(vector<set<int> >());
        cb_metrics->pin_to_tracks.push_back(vector<set<int> >());
        cb_metrics->wire_types_used_count.push_back(vector<vector<int> >());
        for (int i = 0; i < nodes_per_chan; i++) {
            cb_metrics->track_to_pins.at(iside).push_back(set<int>());
        }
        for (int ipin = 0; ipin < (int)cb_metrics->pin_locations.at(iside).size(); ipin++) {
            cb_metrics->pin_to_tracks.at(iside).push_back(set<int>());
            cb_metrics->wire_types_used_count.at(iside).push_back(vector<int>());
            for (int itype = 0; itype < num_wire_types; itype++) {
                cb_metrics->wire_types_used_count.at(iside).at(ipin).push_back(0);
            }
        }
    }

    /*  set the values of the multi-dimensional vectors */
    set<int> counted_pins;
    int track = 0;
    /* over each side of the block */
    for (int iside = 0; iside < 4; iside++) {
        /* over each height unit */
        for (int iheight = 0; iheight < block_type->height; iheight++) {
            /*  over each width unit */
            for (int iwidth = 0; iwidth < block_type->width; iwidth++) {
                /* over each pin */
                for (int ipin = 0; ipin < (int)cb_metrics->pin_locations.at(iside).size(); ipin++) {
                    int pin = cb_metrics->pin_locations.at(iside).at(ipin);

                    bool pin_counted = false;
                    if ((int)counted_pins.size() == num_pin_type_pins) {
                        /* Some blocks like io appear to have four sides, but only one	*
                         *  of those sides is actually used in practice. So here we try	*
                         *  not to count the unused pins.				*/
                        break;
                    }

                    /* now iterate over each track connected to this pin */
                    for (int i = 0; i < Fc; i++) {
                        /* insert track into pin_to_tracks */
                        track = tracks_connected_to_pin[pin][iwidth][iheight][iside][i];

                        if (-1 == track) {
                            /* this pin is not connected to any tracks */
                            break;
                        }

                        pair<set<int>::iterator, bool> result1 = cb_metrics->pin_to_tracks.at(iside).at(ipin).insert(track);
                        if (!result1.second) {
                            /* this track should not already be a part of the set */
                            vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Attempted to insert element into pin_to_tracks set which already exists there\n");
                        }

                        /* insert the current pin into the corresponding tracks_to_pin entry */
                        pair<set<int>::iterator, bool> result2 = cb_metrics->track_to_pins.at(iside).at(track).insert(pin);
                        if (!result2.second) {
                            /* this pin should not already be a part of the set */
                            vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Attempted to insert element into track_to_pins set which already exists there\n");
                        }

                        /* keep track of how many of each wire type is used by the current pin */
                        cb_metrics->wire_types_used_count.at(iside).at(ipin).at(track % num_wire_types)++;
                        pin_counted = true;
                    }
                    if (pin_counted) {
                        counted_pins.insert(pin);
                    }
                }
            }
        }
    }
}

/* wires may be grouped in a channel according to their start points. i.e. at a given channel segment with L=4, there are up to
 * four 'types' of L=4 wires: those that start in this channel segment, those than end in this channel segment, and two types
 * that are in between. here we return the number of wire types.
 * the current connection block metrics code can only deal with channel segments that carry wires of only one length (i.e. L=4).
 * this may be enhanced in the future. */
int get_num_wire_types(const int num_segments, const t_segment_inf* segment_inf) {
    int num_wire_types = 0;

    if (num_segments == 1) {
        /* There are as many wire start points as the value of L */
        num_wire_types = segment_inf[0].length;
    } else {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Currently, the connection block metrics code can only deal with channel segments that carry wires of only one lenght.\n");
    }

    return num_wire_types;
}

/* Gets the maximum Fc value from the Fc_array of this pin type. Errors out if the pins of this pin_type don't all have the same Fc */
int get_max_Fc(const int* Fc_array, const t_type_ptr block_type, const e_pin_type pin_type) {
    int Fc = 0;
    for (int ipin = 0; ipin < block_type->num_pins; ++ipin) {
        int iclass = block_type->pin_class[ipin];
        if (Fc_array[ipin] > Fc && block_type->class_inf[iclass].type == pin_type) {
            /* currently I'm assuming that the Fc for all pins are the same. Check this here. */
            if (Fc != 0 && Fc_array[ipin] != Fc) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Two pins of the same type have different Fc values. This is currently not allowed for CB metrics\n");
            }
            Fc = Fc_array[ipin];
        }
    }
    return Fc;
}

/* returns the pin diversity metric of a block */
static float get_pin_diversity(const int Fc, const int num_pin_type_pins, const Conn_Block_Metrics* cb_metrics) {
    float total_pin_diversity = 0;
    float exp_factor = 3.3;
    int num_wire_types = cb_metrics->num_wire_types;

    /* Determine the diversity of each pin. The concept of this function is that	*
     *  a pin connecting to a wire class more than once returns diminishing gains.	*
     *  This is modelled as an exponential function s.t. at large ratios of  	*
     *  connections/expected_connections we will always get (almost) the same 	*
     *  contribution to pin diversity.						*/
    float mean = (float)Fc / (float)(num_wire_types);
    for (int iside = 0; iside < 4; iside++) {
        for (int ipin = 0; ipin < (int)cb_metrics->pin_locations.at(iside).size(); ipin++) {
            float pin_diversity = 0;
            for (int i = 0; i < num_wire_types; i++) {
                pin_diversity += (1 / (float)num_wire_types) * (1 - exp(-exp_factor * (float)cb_metrics->wire_types_used_count.at(iside).at(ipin).at(i) / mean));
            }
            total_pin_diversity += pin_diversity;
        }
    }
    total_pin_diversity /= num_pin_type_pins;
    return total_pin_diversity;
}

/* Returns Lemieux's cost function for sparse crossbars (see his 2001 book) applied here to the connection block */
static float get_lemieux_cost_func(const int exponent, const bool both_sides, const Conn_Block_Metrics* cb_metrics) {
    float lcf = 0;

    const t_2d_int_vec* pin_locations = &cb_metrics->pin_locations;
    const t_vec_vec_set* pin_to_tracks = &cb_metrics->pin_to_tracks;

    /* may want to calculate LCF for two sides at once to simulate presence of neighboring blocks */
    int mult = (both_sides) ? 2 : 1;

    /* iterate over the sides */
    for (int iside = 0; iside < (4 / mult); iside++) {
        /* how many pins do we need to iterate over? this depends on whether or not we take into
         * account pins on adjacent sides of a channel */
        int num_pins = 0;
        if (both_sides) {
            num_pins = (int)pin_locations->at(iside).size() + (int)pin_locations->at(iside + 2).size();
        } else {
            num_pins = (int)pin_locations->at(iside).size();
        }

        if (0 == num_pins) {
            continue;
        }

        float lcf_pins = 0;
        /* for each pin... */
        int pin_comparisons = 0;
        for (int ipin = 0; ipin < num_pins; ipin++) {
            int pin_ind = ipin;
            int pin_side = iside;
            if (both_sides) {
                if (ipin >= (int)pin_locations->at(iside).size()) {
                    pin_side += 2;
                    pin_ind = pin_ind % pin_locations->at(iside).size();
                }
            }
            float pin_lcf = 0;
            /* ...compare it's track connections to every other pin that we haven't already compared it to */
            for (int icomp = ipin + 1; icomp < num_pins; icomp++) {
                int comp_pin_ind = icomp;
                int comp_side = iside;
                if (both_sides) {
                    if (icomp >= (int)pin_locations->at(iside).size()) {
                        comp_side += 2;
                        comp_pin_ind = comp_pin_ind % pin_locations->at(iside).size();
                    }
                }
                pin_comparisons++;
                /* get the hamming proximity between the tracks of the two pins being compared */
                float pin_to_pin_lcf = (float)hamming_proximity_of_two_sets(&pin_to_tracks->at(pin_side).at(pin_ind), &pin_to_tracks->at(comp_side).at(comp_pin_ind));
                pin_to_pin_lcf = 2 * ((int)pin_to_tracks->at(pin_side).at(pin_ind).size() - pin_to_pin_lcf);
                if (0 == pin_to_pin_lcf) {
                    pin_to_pin_lcf = 1;
                }
                pin_to_pin_lcf = pow(1.0 / pin_to_pin_lcf, exponent);
                pin_lcf += pin_to_pin_lcf;
            }
            lcf_pins += pin_lcf;
        }
        lcf += lcf_pins / (0.5 * num_pins * (num_pins - 1));
    }
    lcf /= (4.0 / (both_sides ? 2.0 : 1.0));

    return lcf;
}

/* Returns the hamming proximity of a block's connection to tracks */
static float get_hamming_proximity(const int Fc, const int num_pin_type_pins, const int exponent, const bool both_sides, const Conn_Block_Metrics* cb_metrics) {
    float hamming_proximity = 0;

    const t_2d_int_vec* pin_locations = &cb_metrics->pin_locations;
    const t_vec_vec_set* pin_to_tracks = &cb_metrics->pin_to_tracks;

    /* may want to calculate HP for two sides at once to simulate presence of neighboring blocks */
    int mult = (both_sides) ? 2 : 1;

    /* iterate over the sides */
    for (int iside = 0; iside < (4 / mult); iside++) {
        /* how many pins do we need to iterate over? this depends on whether or not we take into
         * account pins on adjacent sides of a channel */
        int num_pins = 0;
        if (both_sides) {
            num_pins = (int)pin_locations->at(iside).size() + (int)pin_locations->at(iside + 2).size();
        } else {
            num_pins = (int)pin_locations->at(iside).size();
        }

        if (0 == num_pins) {
            continue;
        }

        float hp_pins = 0;
        /* for each pin... */
        int pin_comparisons = 0;
        for (int ipin = 0; ipin < num_pins; ipin++) {
            int pin_ind = ipin;
            int pin_side = iside;
            if (both_sides) {
                if (ipin >= (int)pin_locations->at(iside).size()) {
                    pin_side += 2;
                    pin_ind = pin_ind % pin_locations->at(iside).size();
                }
            }
            float pin_hp = 0;
            /* ...compare it's track connections to every other pin that we haven't already compared it to */
            for (int icomp = ipin + 1; icomp < num_pins; icomp++) {
                int comp_pin_ind = icomp;
                int comp_side = iside;
                if (both_sides) {
                    if (icomp >= (int)pin_locations->at(iside).size()) {
                        comp_side += 2;
                        comp_pin_ind = comp_pin_ind % pin_locations->at(iside).size();
                    }
                }
                pin_comparisons++;
                /* get the hamming proximity between the tracks of the two pins being compared */
                float pin_to_pin_hp = (float)hamming_proximity_of_two_sets(&pin_to_tracks->at(pin_side).at(pin_ind), &pin_to_tracks->at(comp_side).at(comp_pin_ind));
                pin_to_pin_hp = pow(pin_to_pin_hp, exponent);
                pin_hp += pin_to_pin_hp;
            }
            hp_pins += pin_hp;
        }
        hamming_proximity += hp_pins * 2.0 / (float)((num_pins - 1) * pow(Fc, exponent));
    }
    hamming_proximity /= num_pin_type_pins;

    return hamming_proximity;
}

/* iterates through the elements of set 1 and returns the number of elements in set1 that are
 * also in set2 (in terms of bit vectors, this looks for the number of positions where both bit vectors
 * have a value of 1; values of 0 not counted... so, not quite true hamming proximity). Analogously, if we
 * wanted the hamming distance of these two sets, (in terms of bit vectors, the number of bit positions that are
 * different... i.e. the actual definition of hamming disatnce) that would be 2(set_size - returned_value) */
static int hamming_proximity_of_two_sets(const set<int>* set1, const set<int>* set2) {
    int result = 0;

    set<int>::const_iterator it;
    for (it = set1->begin(); it != set1->end(); it++) {
        int element = *it;
        if (set_has_element(element, set2)) {
            result++;
        }
    }
    return result;
}

/* Returns the wire homogeneity of a block's connection to tracks */
static float get_wire_homogeneity(const int Fc, const int nodes_per_chan, const int num_pin_type_pins, const int exponent, const bool both_sides, const Conn_Block_Metrics* cb_metrics) {
    float total_wire_homogeneity = 0;
    float wire_homogeneity[4];
    int counted_pins_per_side[4];

    /* get the number of pins on each side */
    const t_2d_int_vec* pin_locations = &cb_metrics->pin_locations;
    for (int iside = 0; iside < 4; iside++) {
        counted_pins_per_side[iside] = (int)pin_locations->at(iside).size();
    }

    int unconnected_wires = 0;
    int total_conns = 0;
    int total_pins_on_side = 0;
    float mean = 0;
    float wire_homogeneity_temp = 0;
    /* If 'both_sides' is true, then the metric is calculated as if there is a block on both sides of the
     * channel. This is useful for frequently-occuring blocks like the CLB, which are packed together side by side */
    int mult = (both_sides) ? 2 : 1;
    /* and now compute the wire homogeneity metric */
    /* sides must be ordered as TOP, RIGHT, BOTTOM, LEFT. see the e_side enum */
    for (int side = 0; side < (4 / mult); side++) {
        mean = 0;
        unconnected_wires = 0;
        total_conns = total_pins_on_side = 0;
        for (int i = 0; i < mult; i++) {
            total_pins_on_side += counted_pins_per_side[side + mult * i];
        }

        if (total_pins_on_side == 0) {
            continue;
        }

        total_conns = total_pins_on_side * Fc;
        unconnected_wires = (total_conns) ? max(0, nodes_per_chan - total_conns) : 0;
        mean = (float)total_conns / (float)(nodes_per_chan - unconnected_wires);
        wire_homogeneity[side] = 0;
        for (int track = 0; track < nodes_per_chan; track++) {
            wire_homogeneity_temp = 0;
            for (int i = 0; i < mult; i++) {
                if (counted_pins_per_side[side + i * mult] > 0) {
                    /* only include sides with connected pins */
                    wire_homogeneity_temp += (float)cb_metrics->track_to_pins.at(side + i * mult).at(track).size();
                }
            }
            wire_homogeneity[side] += pow(fabs(wire_homogeneity_temp - mean), exponent);
        }
        float normalization = ((float)Fc * pow(((float)total_pins_on_side - mean), exponent) + (float)(nodes_per_chan - Fc) * pow(mean, exponent)) / (float)total_pins_on_side;
        wire_homogeneity[side] -= unconnected_wires * mean;
        wire_homogeneity[side] /= normalization;
        total_wire_homogeneity += wire_homogeneity[side];
    }
    total_wire_homogeneity /= num_pin_type_pins;

    return total_wire_homogeneity;
}

/* goes through each pin of pin_type and determines which side of the block it comes out on. results are stored in
 * the 'pin_locations' 2d-vector */
static void get_pin_locations(const t_type_ptr block_type, const e_pin_type pin_type, const int num_pin_type_pins, int***** tracks_connected_to_pin, t_2d_int_vec* pin_locations) {
    set<int> counted_pins;

    pin_locations->clear();
    /* over each side */
    for (int iside = 0; iside < 4; iside++) {
        /* go through each pin of the block */
        for (int ipin = 0; ipin < block_type->num_pins; ipin++) {
            /* if this pin is not of the correct type, skip it */
            e_pin_type this_pin_type = block_type->class_inf[block_type->pin_class[ipin]].type;
            if (this_pin_type != pin_type) {
                continue;
            }

            //TODO: block_type->pin_loc indicates that there are pins on all sides of an I/O block, but this is not actually the case...
            // In the future we should change pin_loc to indicate the correct pin locations
            /* push back an empty vector for this side */
            pin_locations->push_back(vector<int>());
            for (int iwidth = 0; iwidth < block_type->width; iwidth++) {
                for (int iheight = 0; iheight < block_type->height; iheight++) {
                    /* check if ipin is present at this side/width/height */
                    if (-1 != tracks_connected_to_pin[ipin][iwidth][iheight][iside][0]) {
                        if ((int)counted_pins.size() == num_pin_type_pins) {
                            /* Some blocks like io appear to have four sides, but only one	*
                             *  of those sides is actually used in practice. So here we try	*
                             *  not to count the unused pins.				*/
                            break;
                        }
                        /* if so, push it onto our pin_locations vector at the appropriate side */
                        pin_locations->at(iside).push_back(ipin);
                        counted_pins.insert(ipin);
                    }
                }
            }
            /* sort the vector at the current side in increasing order, for good measure */
            sort(pin_locations->at(iside).begin(), pin_locations->at(iside).end());
        }
    }
    /* now we have a vector of vectors [0..3][0..num_pins_on_this_side] specifying which pins are on which side */
}

/* given a set of tracks connected to a pin, we'd like to find which of these tracks are connected to a number of switches
 * greater than 'criteria'. The resulting set of tracks is passed back in the 'result' vector */
static void find_tracks_with_more_switches_than(const set<int>* pin_tracks, const t_vec_vec_set* track_to_pins, const int side, const bool both_sides, const int criteria, vector<int>* result) {
    result->clear();

    if (both_sides && side >= 2) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "when accounting for pins on both sides of a channel segment, the passed-in side should have index < 2. got %d\n", side);
    }

    /* for each track connected to the pin */
    set<int>::const_iterator it;
    for (it = pin_tracks->begin(); it != pin_tracks->end(); it++) {
        int track = *it;

        int num_switches = 0;
        if (both_sides) {
            num_switches = (int)track_to_pins->at(side).at(track).size() + (int)track_to_pins->at(side + 2).at(track).size();
        } else {
            num_switches = (int)track_to_pins->at(side).at(track).size();
        }
        if (num_switches > criteria) {
            result->push_back(track);
        }
    }
}

/* given a pin on some side of a block, we'd like to find the set of tracks that is NOT connected to that pin on that side. This set of tracks
 * is passed back in the 'result' vector */
static void find_tracks_unconnected_to_pin(const set<int>* pin_tracks, const vector<set<int> >* track_to_pins, vector<int>* result) {
    result->clear();
    /* for each track in the channel segment */
    for (int itrack = 0; itrack < (int)track_to_pins->size(); itrack++) {
        /* check if this track is not connected to the pin */
        if (!set_has_element(itrack, pin_tracks)) {
            result->push_back(itrack);
        }
    }
}

/* this function simply moves a switch from one track to another track (with an empty slot). The switch stays on the
 * same pin as before. */
static double try_move(const e_metric metric, const int nodes_per_chan, const float initial_orthogonal_metric, const float orthogonal_metric_tolerance, const t_type_ptr block_type, const e_pin_type pin_type, const int Fc, const int num_pin_type_pins, const double cost, const double temp, const float target_metric, int***** pin_to_track_connections, Conn_Block_Metrics* cb_metrics) {
    double new_cost = 0;
    float new_orthogonal_metric = 0;
    float new_metric = 0;

    /* will determine whether we should revert the attempted move at the end of this function */
    bool revert = false;
    /* indicates whether or not we allow a track to be fully disconnected from all the pins of the connection block
     * in the processs of trying a move (to allow this, preserve_tracks is set to false) */
    const bool preserve_tracks = true;

    t_vec_vec_set* pin_to_tracks = &cb_metrics->pin_to_tracks;
    t_vec_vec_set* track_to_pins = &cb_metrics->track_to_pins;
    t_3d_int_vec* wire_types_used_count = &cb_metrics->wire_types_used_count;
    int num_wire_types = cb_metrics->num_wire_types;

    /* for the CLB block types it is appropriate to account for pins on both sides of a channel segment when
     * calculating a CB metric (because CLBs are often found side by side) */
    bool both_sides = false;
    if (0 == strcmp("clb", block_type->name) && DRIVER == pin_type) {
        /* many CLBs are adjacent to eachother, so connections from one CLB
         *  will share the channel segment with its neighbor. We'd like to take this into
         *  account for the applicable metrics. */
        both_sides = true;
    } else {
        /* other blocks (i.e. IO, RAM, etc) are not as frequent as CLBs */
        both_sides = false;
    }

    static vector<int> set_of_tracks;
    /* the set_of_tracks vector is used to find sets of tracks satisfying some criteria that we want. we reserve memory for it, which
     * should be preserved between calls of this function so that we don't have to allocate memory every time */
    set_of_tracks.reserve(nodes_per_chan);
    set_of_tracks.clear();

    /* choose a random side, random pin, and a random switch */
    int rand_side = vtr::irand(3);
    int rand_pin_index = vtr::irand(cb_metrics->pin_locations.at(rand_side).size() - 1);
    int rand_pin = cb_metrics->pin_locations.at(rand_side).at(rand_pin_index);
    set<int>* tracks_connected_to_pin = &pin_to_tracks->at(rand_side).at(rand_pin_index);

    /* If the pin is unconnected, return. */
    if (0 == tracks_connected_to_pin->size()) {
        new_cost = cost;
    } else {
        /* get an old track connection i.e. one that is connected to our pin. this track has to have a certain number of switches.
         * for instance, if we want to avoid completely disconnecting a track, it should have > 1 switch so that when we move one
         * switch from this track to another, this track will still be connected. */

        /* find the set of tracks satisfying the 'number of switches' criteria mentioned above */
        int check_side = rand_side;
        if (both_sides && check_side >= 2) {
            check_side -= 2; /* will be checking this, along with (check_side + 2) */
        }
        if (preserve_tracks) {
            /* looking for tracks with 2 or more switches */
            find_tracks_with_more_switches_than(tracks_connected_to_pin, track_to_pins, check_side, both_sides, 1, &set_of_tracks);
        } else {
            /* looking for tracks with 1 or more switches */
            find_tracks_with_more_switches_than(tracks_connected_to_pin, track_to_pins, check_side, both_sides, 0, &set_of_tracks);
        }

        if (set_of_tracks.size() == 0) {
            new_cost = cost;
        } else {
            /* now choose a random track from the returned set of qualifying tracks */
            int old_track = vtr::irand(set_of_tracks.size() - 1);
            old_track = set_of_tracks.at(old_track);

            /* next, get a new track connection i.e. one that is not already connected to our randomly chosen pin */
            find_tracks_unconnected_to_pin(tracks_connected_to_pin, &track_to_pins->at(rand_side), &set_of_tracks);
            int new_track = vtr::irand(set_of_tracks.size() - 1);
            new_track = set_of_tracks.at(new_track);

            /* move the rand_pin's connection from the old track to the new track and see what the new cost is */
            /* update CB metrics structures */
            pin_to_tracks->at(rand_side).at(rand_pin_index).erase(old_track);
            pin_to_tracks->at(rand_side).at(rand_pin_index).insert(new_track);

            track_to_pins->at(rand_side).at(old_track).erase(rand_pin);
            track_to_pins->at(rand_side).at(new_track).insert(rand_pin);

            wire_types_used_count->at(rand_side).at(rand_pin_index).at(old_track % num_wire_types)--;
            wire_types_used_count->at(rand_side).at(rand_pin_index).at(new_track % num_wire_types)++;

            /* the orthogonal metric needs to stay within some tolerance of its initial value. here we get the
             * orthogonal metric after the above move */
            if (metric < NUM_WIRE_METRICS) {
                /* get the new pin diversity cost */
                new_orthogonal_metric = get_pin_diversity(Fc, num_pin_type_pins, cb_metrics);
            } else {
                /* get the new wire homogeneity cost */
                new_orthogonal_metric = get_wire_homogeneity(Fc, nodes_per_chan, num_pin_type_pins, 2, both_sides, cb_metrics);
            }

            /* check if the orthogonal metric has remained within tolerance */
            if (new_orthogonal_metric >= initial_orthogonal_metric - orthogonal_metric_tolerance
                && new_orthogonal_metric <= initial_orthogonal_metric + orthogonal_metric_tolerance) {
                /* The orthogonal metric is within tolerance. Can proceed */

                /* get the new metric */
                new_metric = 0;

                double delta_cost;
                switch (metric) {
                    case WIRE_HOMOGENEITY:
                        new_metric = get_wire_homogeneity(Fc, nodes_per_chan, num_pin_type_pins, 2, both_sides, cb_metrics);
                        break;
                    case HAMMING_PROXIMITY:
                        new_metric = get_hamming_proximity(Fc, num_pin_type_pins, 2, both_sides, cb_metrics);
                        break;
                    case LEMIEUX_COST_FUNC:
                        new_metric = get_lemieux_cost_func(2, both_sides, cb_metrics);
                        break;
                    case PIN_DIVERSITY:
                        new_metric = get_pin_diversity(Fc, num_pin_type_pins, cb_metrics);
                        break;
                    default:
                        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "try_move: illegal CB metric being adjusted: %d\n", (int)metric);
                        break;
                }
                new_cost = fabs(target_metric - new_metric);
                delta_cost = new_cost - cost;
                if (!accept_move(delta_cost, temp)) {
                    revert = true;
                }
            } else {
                /* the new orthogoanl metric changed too much. will undo the move made before */
                revert = true;
            }

            if (revert) {
                /* revert the attempted move */
                pin_to_tracks->at(rand_side).at(rand_pin_index).insert(old_track);
                pin_to_tracks->at(rand_side).at(rand_pin_index).erase(new_track);

                track_to_pins->at(rand_side).at(old_track).insert(rand_pin);
                track_to_pins->at(rand_side).at(new_track).erase(rand_pin);

                wire_types_used_count->at(rand_side).at(rand_pin_index).at(old_track % num_wire_types)++;
                wire_types_used_count->at(rand_side).at(rand_pin_index).at(new_track % num_wire_types)--;

                new_cost = cost;
            } else {
                /* accept the attempted move */

                /* need to update the actual pin-to-track mapping used by build_rr_graph */
                int track_index = 0;
                for (track_index = 0; track_index < Fc; track_index++) {
                    if (pin_to_track_connections[rand_pin][0][0][rand_side][track_index] == old_track) {
                        break;
                    }
                }
                pin_to_track_connections[rand_pin][0][0][rand_side][track_index] = new_track;

                /* update metrics */
                switch (metric) {
                    case WIRE_HOMOGENEITY:
                        cb_metrics->wire_homogeneity = new_metric;
                        cb_metrics->pin_diversity = new_orthogonal_metric;
                        break;
                    case HAMMING_PROXIMITY:
                        cb_metrics->hamming_proximity = new_metric;
                        cb_metrics->pin_diversity = new_orthogonal_metric;
                        break;
                    case LEMIEUX_COST_FUNC:
                        cb_metrics->lemieux_cost_func = new_metric;
                        cb_metrics->pin_diversity = new_orthogonal_metric;
                        break;
                    case PIN_DIVERSITY:
                        cb_metrics->pin_diversity = new_metric;
                        cb_metrics->wire_homogeneity = new_orthogonal_metric;
                        break;
                    default:
                        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "try_move: illegal CB metric: %d\n", (int)metric);
                        break;
                }
            }
        }
    }
    return new_cost;
}

/* this annealer is used to adjust a desired wire or pin metric while keeping the other type of metric
 * relatively constant */
static bool annealer(const e_metric metric, const int nodes_per_chan, const t_type_ptr block_type, const e_pin_type pin_type, const int Fc, const int num_pin_type_pins, const float target_metric, const float target_metric_tolerance, int***** pin_to_track_connections, Conn_Block_Metrics* cb_metrics) {
    bool success = false;
    double temp = INITIAL_TEMP;

    /* the annealer adjusts a wire metric or a pin metric while keeping the other one relatively constant. what I'm
     * calling the orthogonal metric is the metric we'd like to keep relatively constant within some tolerance */
    float initial_orthogonal_metric;
    float orthogonal_metric_tolerance;

    /* get initial metrics and cost */
    double cost = 0;
    orthogonal_metric_tolerance = 0.05;
    if (metric < NUM_WIRE_METRICS) {
        initial_orthogonal_metric = cb_metrics->pin_diversity;

        switch (metric) {
            case WIRE_HOMOGENEITY:
                cost = fabs(cb_metrics->wire_homogeneity - target_metric);
                break;
            case HAMMING_PROXIMITY:
                cost = fabs(cb_metrics->hamming_proximity - target_metric);
                break;
            case LEMIEUX_COST_FUNC:
                cost = fabs(cb_metrics->lemieux_cost_func - target_metric);
                break;
            default:
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "CB metrics annealer: illegal wire metric: %d\n", (int)metric);
                break;
        }
    } else {
        initial_orthogonal_metric = cb_metrics->wire_homogeneity;

        switch (metric) {
            case PIN_DIVERSITY:
                cost = fabs(cb_metrics->pin_diversity - target_metric);
                break;
            default:
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "CB metrics annealer: illegal pin metric: %d\n", (int)metric);
                break;
        }
    }

    /* the main annealer loop */
    for (int i_outer = 0; i_outer < MAX_OUTER_ITERATIONS; i_outer++) {
        for (int i_inner = 0; i_inner < MAX_INNER_ITERATIONS; i_inner++) {
            double new_cost = 0;
            new_cost = try_move(metric, nodes_per_chan, initial_orthogonal_metric, orthogonal_metric_tolerance,
                                block_type, pin_type, Fc, num_pin_type_pins, cost, temp, target_metric, pin_to_track_connections, cb_metrics);

            /* update the cost after trying the move */
            if (new_cost != cost) {
                cost = new_cost;
            }
        }

        temp = update_temp(temp);

        /* stop if temperature has decreased to 0 */
        if (0 == temp) {
            break;
        }

        /* also break if the target metric is within its specified tolerance */
        if (cost <= target_metric_tolerance) {
            break;
        }
    }

    if (cost <= target_metric_tolerance) {
        success = true;
    } else {
        success = false;
    }

    return success;
}

/* updates temperature based on current temperature and the annealer's outer loop iteration */
static double update_temp(const double temp) {
    double new_temp;
    double fac = TEMP_DECREASE_FAC;
    double temp_threshold = LOWEST_TEMP;

    /* just decrease temp by a constant factor */
    new_temp = fac * temp;

    if (temp < temp_threshold) {
        new_temp = 0;
    }

    return new_temp;
}

/* determines whether to accept or reject a proposed move based on the resulting delta of the cost and current temperature */
static bool accept_move(const double del_cost, const double temp) {
    bool accept = false;

    if (del_cost < 0) {
        /* cost has decreased -- always accept */
        accept = true;
    } else {
        /* determine probabilistically whether or not to accept */
        double probability = pow(2.718, -(del_cost / temp));
        double rand_value = (double)vtr::frand();
        if (rand_value < probability) {
            accept = true;
        } else {
            accept = false;
        }
    }

    return accept;
}

static void print_switch_histogram(const int nodes_per_chan, const Conn_Block_Metrics* cb_metrics) {
    /* key: number of switches; element: number of tracks with that switch count */
    map<int, int> switch_histogram;

    const t_vec_vec_set* track_to_pins = &cb_metrics->track_to_pins;

    for (int iside = 0; iside < 2; iside++) {
        for (int itrack = 0; itrack < nodes_per_chan; itrack++) {
            int num_switches = (int)track_to_pins->at(iside).at(itrack).size() + (int)track_to_pins->at(iside + 2).at(itrack).size();
            if (map_has_key(num_switches, &switch_histogram)) {
                switch_histogram.at(num_switches)++;
            } else {
                switch_histogram[num_switches] = 1;
            }
        }
    }

    VTR_LOG("\t===CB Metrics Switch Histogram===\n\t#switches ==> #num tracks carrying that number of switches\n");
    map<int, int>::const_iterator it;
    for (it = switch_histogram.begin(); it != switch_histogram.end(); it++) {
        VTR_LOG("\t%d ==> %d\n", it->first, it->second);
    }
}

/**** EXPERIMENTAL ****/

/* constructs a crossbar matrix from the connection block 'conn_block'. rows correspond to the pins,
 * columns correspond to the wires. entry (i,j) is set to 1 if that pin and track are connected. the
 * only pins accounted for here are the pins that actually have switches on the conn block side in question */
static void get_xbar_matrix(const int***** conn_block, const t_type_ptr block_type, e_pin_type pin_type, const int side, const bool both_sides, const int nodes_per_chan, const int Fc, t_xbar_matrix* xbar_matrix) {
    xbar_matrix->clear();
    if (both_sides && side >= 2) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "If analyzing both sides of a conn block, the initial side should have index < 2");
    }

    int num_pins = block_type->num_pins;

    /* if we're checking both sides of a channel, we're dealing with two CLBs instead of one */
    int pins_to_check = both_sides ? num_pins * 2 : num_pins;

    int checked_pins = 0;
    for (int ipin = 0; ipin < pins_to_check; ipin++) {
        int width = 0; /* CLBs have width/height of 0 */
        int height = 0;

        int pin = ipin;
        int check_side = side;
        if (pin >= num_pins) {
            /* checking on the other side of the channel */
            pin %= num_pins;
            check_side += 2;
        }

        /* skip if specified pin isn't of 'pin_type' */
        int pin_class_index = block_type->pin_class[pin];
        if (pin_type != block_type->class_inf[pin_class_index].type) {
            continue;
        }
        /* skip if specified pin doesn't connect to tracks on the specified side */
        if (conn_block[pin][width][height][check_side][0] == -1) {
            continue;
        }

        /* each pin corresponds to a row of the xbar matrix */
        xbar_matrix->push_back(vector<float>());

        /* each wire in the channel corresponds to a column of the xbar matrix */
        for (int icol = 0; icol < nodes_per_chan; icol++) {
            xbar_matrix->at(checked_pins).push_back(0.0);
        }
        /* set appropriate elements of the current xbar row to 1 (according to which tracks the pin connects to) */
        for (int iswitch = 0; iswitch < Fc; iswitch++) {
            int set_col = conn_block[pin][width][height][check_side][iswitch];
            xbar_matrix->at(checked_pins).at(set_col) = 1.0;
        }

        /* increment the number of pins for which we have created an xbar row */
        checked_pins++;
    }
}

/* returns a transposed version of the specified crossbar matrix */
static t_xbar_matrix transpose_xbar(const t_xbar_matrix* xbar) {
    t_xbar_matrix result;

    int new_cols = (int)xbar->size();
    int new_rows = (int)xbar->at(0).size();

    for (int i = 0; i < new_rows; i++) {
        result.push_back(vector<float>());
        for (int j = 0; j < new_cols; j++) {
            result.at(i).push_back(xbar->at(j).at(i));
        }
    }
    return result;
}

/* calculates the factorial of 'num'. result is a long double. some precision may be lost, but that's
 * OK for my purposes */
static long double factorial(const int num) {
    long double result = 1;
    if (num <= 1) {
        result = 1;
    } else {
        for (int i = 2; i <= num; i++) {
            result *= (long double)i;
        }
    }
    return result;
}

/* calculates n choose k. some precision may be lost, but for my purposes
 * that doesn't really matter */
static long double binomial_coefficient(const int n, const int k) {
    if (n < k) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "calculating the binomial coefficient requires that n >= k");
    }
    long double result;
    result = factorial(n) / (factorial(k) * factorial(n - k));
    return result;
}

static long double count_switch_configurations(const int level, const int signals_left, const int capacity_left, vector<int>* config, map<int, Wire_Counting>* count_map) {
    long double result = 0;

    if (capacity_left < signals_left) {
        printf("capacity left %d   signals left %d   level %d\n", capacity_left, signals_left, level);
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "the number of signals remaining should not be greater than the remaining capacity");
    }

    /* get the capacity of the current wire group (here, group is defined by the number of switches a wire carries) */
    map<int, Wire_Counting>::const_iterator it = count_map->begin();
    advance(it, level);
    int my_capacity = it->second.num_wires;

    /* get the total capacity of the remaining wire groups */
    int downstream_capacity = capacity_left - my_capacity;

    /* get the maximum number of signals this wire group can carry */
    int can_take = min(my_capacity, signals_left);

    /* we cannot push more signals onto the other wire groups than they can take. to avoid this the minimum number of signals allowed
     * to be carried by the current wire group may be above 0 */
    int start_fill = signals_left - downstream_capacity;
    if (start_fill < 0) {
        start_fill = 0;
    }

    /* now, for each number of signals this wire group can carry... */
    for (int isigs = start_fill; isigs <= can_take; isigs++) {
        config->at(level) = isigs;
        if (level == (int)count_map->size() - 1) {
            /* if this is the final wire group, we want to count the number of possible ways that we can
             * configure the switches in the channel to achieve the wire usage specified by the current value of the config vector */
            long double num_configs = 1;
            for (int i = 0; i < (int)config->size(); i++) {
                it = count_map->begin();
                advance(it, i);
                int num_switches_per_wire = it->first;
                int num_wires_in_group = it->second.num_wires;
                int num_wires_used_in_group = config->at(i);
                num_configs *= binomial_coefficient(num_wires_in_group, num_wires_used_in_group)
                               * (long double)pow((long double)num_switches_per_wire, (long double)num_wires_used_in_group);
            }

            /* add this info for the final wire group */
            it = count_map->begin();
            advance(it, level);
            map<int, long double>* configs_used = &count_map->at(it->first).configs_used;
            if (map_has_key(config->at(level), configs_used)) {
                configs_used->at(config->at(level)) += num_configs;
            } else {
                (*configs_used)[config->at(level)] = num_configs;
            }
            result = num_configs;
        } else {
            /* recurse to the next wire group */
            long double num_configs;
            num_configs = count_switch_configurations(level + 1, signals_left - isigs, downstream_capacity, config, count_map);
            /* add this info for the current wire group */
            it = count_map->begin();
            advance(it, level);
            map<int, long double>* configs_used = &count_map->at(it->first).configs_used;
            if (map_has_key(config->at(level), configs_used)) {
                configs_used->at(config->at(level)) += num_configs;
            } else {
                (*configs_used)[config->at(level)] = num_configs;
            }

            result += num_configs;
        }
    }

    return result;
}

/* 'normalizes' the given crossbar. normalization is done in a way such that an entry which was formerly equal to '1'
 * will now equal to the probability of that switch being available */
static void normalize_xbar(const float fraction_wires_used, t_xbar_matrix* xbar) {
    int rows = (int)xbar->size();       /* rows correspond to pins */
    int cols = (int)xbar->at(0).size(); /* columns correspond to wires */

    map<int, Wire_Counting> count_map;

    /* the number of signals that this channel can carry (each wire with switches contributes 1 to capacity) */
    int capacity = 0;

    /* create a map where the key is the number of switches, and the element is a class with a field
     * that specifies how many wires carry this number of switches */
    for (int iwire = 0; iwire < cols; iwire++) {
        int num_switches = 0;
        for (int ipin = 0; ipin < rows; ipin++) {
            if (1 == xbar->at(ipin).at(iwire)) {
                num_switches++;
            }
        }

        if (0 == num_switches) {
            continue;
        }
        capacity++;
        if (map_has_key(num_switches, &count_map)) {
            /* a map entry for this number of switches exists. increment the number of wires that use this number of switches */
            count_map.at(num_switches).num_wires++;
        } else {
            /* create a map entry. so far only 1 wire uses this number of switches */
            Wire_Counting dummy;
            dummy.num_wires = 1;
            count_map[num_switches] = dummy;
        }
    }

    /* now we have to count two things.
     * 1) the total number of possible switch configurations in which 'wires_used' wires are occupied by a signal
     * 2) in general, we have some number of 'wire groups' as defined above. each wire group is used a certain number of times in some
     * specific configuration of switches. we want to determine, for each wire group, the number of switch configurations
     * that leads to y wires of this group being used, for every feasable y.
     *
     * These two things should allow us to calculate the expectation of how many wires of each group are used on average.
     * And this in turn allows us to set the probabilities of each wire in the xbar matrix being available/unavailable
     */

    /* the config vector represents some distribution of signals over available wires. i.e. x wires of type 0 get used, y wires of type 1, etc
     * this vector is created here, but is updated inside the count_switch_configurations function */
    vector<int> config;
    for (int i = 0; i < (int)count_map.size(); i++) {
        config.push_back(0);
    }

    /* the nuber of wires that are used */
    int wires_used = vtr::nint(fraction_wires_used * (float)capacity);

    long double total_configurations = count_switch_configurations(0, wires_used, capacity, &config, &count_map);

    /* next we need to calculate the expectation of the number of wires available for each wire group */
    map<int, Wire_Counting>::const_iterator it;
    for (it = count_map.begin(); it != count_map.end(); it++) {
        map<int, long double>* configs_used = &count_map.at(it->first).configs_used;

        float* expectation_available = &count_map.at(it->first).expectation_available;
        (*expectation_available) = 0;
        map<int, long double>::const_iterator it2;
        for (it2 = configs_used->begin(); it2 != configs_used->end(); it2++) {
            int used = it2->first;
            long double num_configurations = it2->second;
            long double probability = num_configurations / total_configurations;
            (*expectation_available) += (float)probability * (float)used;
        }
        (*expectation_available) = (float)it->second.num_wires - (*expectation_available);
    }

    /* and now we adjust the column values of the xbar matrix according to the expected availability of the corresponding wires
     * (recall a column corresponds to a wire) */
    for (int iwire = 0; iwire < cols; iwire++) {
        int num_switches = 0;
        for (int ipin = 0; ipin < rows; ipin++) {
            if (1 == xbar->at(ipin).at(iwire)) {
                num_switches++;
            }
        }

        if (num_switches == 0) {
            continue;
        }

        float fraction_available = count_map.at(num_switches).expectation_available / count_map.at(num_switches).num_wires;

        for (int ipin = 0; ipin < rows; ipin++) {
            if (1 == xbar->at(ipin).at(iwire)) {
                xbar->at(ipin).at(iwire) = fraction_available;
                //xbar->at(ipin).at(iwire) = 0.15;
            }
        }
    }
}

/* prints a crossbar matrix */
static void print_xbar(t_xbar_matrix* xbar) {
    int rows = (int)xbar->size();
    int cols = (int)xbar->at(0).size();

    for (int irow = 0; irow < rows; irow++) {
        VTR_LOG("\t\t|\t");
        for (int icol = 0; icol < cols; icol++) {
            VTR_LOG("%.2f\t", xbar->at(irow).at(icol));
        }
        VTR_LOG(" |\n");
    }
}

static float probabilistic_or(const float a, const float b) {
    float result;
    result = a + b - a * b; /* assuming a & b are not mutually exclusive */
    return result;
}

/* allocates an xbar with specified number of rows and columns where each element is set to 'num' */
static void allocate_xbar(const int rows, const int cols, const float num, t_xbar_matrix* xbar) {
    xbar->clear();
    for (int irow = 0; irow < rows; irow++) {
        xbar->push_back(vector<float>());
        for (int icol = 0; icol < cols; icol++) {
            xbar->at(irow).push_back(num);
        }
    }
}

/* combines xbar1 followed by xbar2 into a compound xbar returned via the return value */
static t_xbar_matrix combine_two_xbars(const t_xbar_matrix* xbar1, const t_xbar_matrix* xbar2) {
    t_xbar_matrix xbar_out;

    if (xbar1->at(0).size() != xbar2->size()) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "xbar1 should have the same number of columns as xbar2 has rows");
    }

    /* get the dimensions of the combined xbar */
    int rows = (int)xbar1->size();
    int cols = (int)xbar2->at(0).size();

    /* allocate xbar_out */
    allocate_xbar(rows, cols, 0.0, &xbar_out);

    /* the xbars are combined similar to matrix multiplication, except instead of adding the multiplied elements together,
     * we treat the elements as probabilities and 'or' them together */

    /* create xbar_out */
    int vec_length = (int)xbar2->size();
    for (int irow = 0; irow < rows; irow++) {
        for (int icol = 0; icol < cols; icol++) {
            /* combine irow'th row of xbar1 with icol'th column of xbar2 */
            float result = 0;
            for (int ielem = 0; ielem < vec_length; ielem++) {
                float product = xbar1->at(irow).at(ielem) * xbar2->at(ielem).at(icol);
                result = probabilistic_or(product, result);
            }
            xbar_out.at(irow).at(icol) = result;
        }
    }

    return xbar_out;
}

void analyze_conn_blocks(const int***** opin_cb, const int***** ipin_cb, const t_type_ptr block_type, const int* Fc_array_out, const int* Fc_array_in, const t_chan_width* chan_width_inf) {
    if (0 != strcmp(block_type->name, "clb")) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "This code currently works for CLB blocks only");
    }
    if (chan_width_inf->x_min != chan_width_inf->x_max || chan_width_inf->y_min != chan_width_inf->y_max
        || chan_width_inf->x_max != chan_width_inf->y_max) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "This code currently assumes that channel width is uniform throughout the fpga");
    }
    int nodes_per_chan = chan_width_inf->x_min;

    /* get Fc */
    int Fc_out = get_max_Fc(Fc_array_out, block_type, DRIVER);
    int Fc_in = get_max_Fc(Fc_array_in, block_type, RECEIVER);

    /* get the basic input and output conn block crossbars */
    t_xbar_matrix output_xbar, input_xbar;
    get_xbar_matrix(opin_cb, block_type, DRIVER, 0, true, nodes_per_chan, Fc_out, &output_xbar);
    get_xbar_matrix(ipin_cb, block_type, RECEIVER, 0, false, nodes_per_chan, Fc_in, &input_xbar);
    input_xbar = transpose_xbar(&input_xbar);

    /* 'normalize' the output_xbar such that each entry which was formerly equal to '1' will now
     * equal to the probability of that corresponding switch being available for use */
    normalize_xbar(0.7, &output_xbar);

    /* combine the output and input CB crossbars */
    t_xbar_matrix compound_xbar;
    compound_xbar = combine_two_xbars(&output_xbar, &input_xbar);

    /* and then combine that with a full crossbar */
    //t_xbar_matrix full_xbar;
    //allocate_xbar((int)compound_xbar.size(), (int)compound_xbar.size(), 1.0, &full_xbar);
    //compound_xbar = combine_two_xbars(&compound_xbar, &full_xbar);

    printf("output crossbar\n");
    print_xbar(&output_xbar);
    printf("\n\ninput crossbar\n");
    print_xbar(&input_xbar);
    printf("\n\ncompound crossbar\n");
    print_xbar(&compound_xbar);

    for (int irow = 0; irow < (int)compound_xbar.size(); irow++) {
        float row_total = 0;
        for (int icol = 0; icol < (int)compound_xbar.at(0).size(); icol++) {
            row_total += compound_xbar.at(irow).at(icol);
        }
        printf("pin %d    total %f\n", irow, row_total);
    }
}

/* make a poor cb pattern. */
void make_poor_cb_pattern(const e_pin_type pin_type, const t_type_ptr block_type, const int* Fc_array, const t_chan_width* chan_width_inf, int***** cb) {
    if (block_type->num_pins == 0) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Trying to adjust CB metrics for a block with no pins!\n");
    }
    if (block_type->height > 1 || block_type->width > 1) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Adjusting connection block metrics is currently intended for CLBs, which have height and width = 1\n");
    }
    if (chan_width_inf->x_min != chan_width_inf->x_max || chan_width_inf->y_min != chan_width_inf->y_max
        || chan_width_inf->x_max != chan_width_inf->y_max) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "This code currently assumes that channel width is uniform throughout the fpga");
    }

    int nodes_per_chan = chan_width_inf->x_min;

    /* get Fc */
    int Fc = get_max_Fc(Fc_array, block_type, pin_type);

    /* clear the existing CB */
    for (int ipin = 0; ipin < block_type->num_pins; ipin++) {
        for (int iwidth = 0; iwidth < block_type->width; iwidth++) {
            for (int iheight = 0; iheight < block_type->height; iheight++) {
                for (int iside = 0; iside < 4; iside++) {
                    for (int ifc = 0; ifc < Fc; ifc++) {
                        cb[ipin][iwidth][iheight][iside][ifc] = -1;
                    }
                }
            }
        }
    }

    /* now set the CB to what would seem to be a bad switch pattern */
    for (int iside = 0; iside < 2; iside++) {
        int track_counter = 0;
        for (int ipin = 0; ipin < 2 * block_type->num_pins; ipin++) {
            for (int iwidth = 0; iwidth < block_type->width; iwidth++) {
                for (int iheight = 0; iheight < block_type->height; iheight++) {
                    int side = iside;
                    int pin = ipin;
                    if (ipin >= block_type->num_pins) {
                        side += 2;
                        pin %= block_type->num_pins;
                    }

                    /* if this pin is not of the correct type, skip it */
                    e_pin_type this_pin_type = block_type->class_inf[block_type->pin_class[pin]].type;
                    if (this_pin_type != pin_type || block_type->is_ignored_pin[pin]) {
                        continue;
                    }

                    /* make sure this pin exists at this location */
                    if (1 != block_type->pinloc[iwidth][iheight][side][pin]) {
                        continue;
                    }

                    /* consecutive assignment */
                    for (int iconn = 0; iconn < Fc; iconn++) {
                        cb[pin][iwidth][iheight][side][iconn] = track_counter % nodes_per_chan;
                        track_counter++;
                    }
                }
            }
        }
    }
}

/**** END EXPERIMENTAL ****/
