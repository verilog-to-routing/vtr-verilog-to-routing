#ifndef CB_METRICS_H
#define CB_METRICS_H

#include <vector>
#include <set>

#define MAX_OUTER_ITERATIONS 100000
#define MAX_INNER_ITERATIONS 10
#define INITIAL_TEMP 1
#define LOWEST_TEMP 0.00001
#define TEMP_DECREASE_FAC 0.999

/**** Enums ****/
/* Defines the different kinds of metrics that we can adjust */
enum e_metric {
    WIRE_HOMOGENEITY = 0,
    HAMMING_PROXIMITY,
    LEMIEUX_COST_FUNC, /* described by lemieux in his 2001 book; used there for creating routable sparse crossbars */
    NUM_WIRE_METRICS,
    PIN_DIVERSITY,
    NUM_METRICS
};

/**** Typedefs ****/
/* 2D vector of integers */
typedef std::vector<std::vector<int> > t_2d_int_vec;
/* 3D vector of integers */
typedef std::vector<std::vector<std::vector<int> > > t_3d_int_vec;
/* a vector of vectors of integer sets. used for pin-to-track and track-to-pin lookups */
typedef std::vector<std::vector<std::set<int> > > t_vec_vec_set;

/**** Classes ****/
/* Contains various useful structures to calculate connection block metrics, and is used to
 * hold the CB metrics themselves */
class Conn_Block_Metrics {
  public:
    /* the actual metrics */
    float pin_diversity;
    float wire_homogeneity;
    float hamming_proximity;
    float lemieux_cost_func;

    int num_wire_types;         /* the number of different wire types, used for computing pin diversity */
    t_2d_int_vec pin_locations; /* [0..3][0..num_on_this_side-1]. Keeps track of which pins come out on which side of the block */

    /* these vectors simplify the calculation of the various metrics */
    t_vec_vec_set track_to_pins;        /* [0..3][0..W-1][0..pins_connected-1]. A convenient lookup for which pins connect to a given track */
    t_vec_vec_set pin_to_tracks;        /* [0..3][0..num_pins_on_side-1][0..tracks_connected-1]. A lookup for which tracks connect to a given pin */
    t_3d_int_vec wire_types_used_count; /* [0..3][0..num_pins_on_side-1][0..num_wire_types-1]. Keeps track of how many times each pin connects to each of the wire types */

    void clear() {
        pin_diversity = wire_homogeneity = hamming_proximity = lemieux_cost_func = 0;
        num_wire_types = 0;
        pin_locations.clear();
        track_to_pins.clear();
        pin_to_tracks.clear();
        wire_types_used_count.clear();
    }
};

/**** Function Declarations ****/

/* wires may be grouped in a channel according to their start points. i.e. at a given channel segment with L=4, there are up to
 * four 'types' of L=4 wires: those that start in this channel segment, those than end in this channel segment, and two types
 * that are in between. here we return the number of wire types.
 * the current connection block metrics code can only deal with channel segments that carry wires of only one length (i.e. L=4).
 * this may be enhanced in the future. */
int get_num_wire_types(const int num_segments, const t_segment_inf* segment_inf);

/* calculates all the connection block metrics and returns them through the cb_metrics variable */
void get_conn_block_metrics(const t_type_ptr block_type, int***** tracks_connected_to_pin, const int num_segments, const t_segment_inf* segment_inf, const e_pin_type pin_type, const int* Fc_array, const t_chan_width* chan_width_inf, Conn_Block_Metrics* cb_metrics);
/* adjusts the connection block until the appropriate wire metric has hit it's target value. the pin metric is kept constant
 * within some tolerance */
void adjust_cb_metric(const e_metric metric, const float target, const float target_tolerance, const t_type_ptr block_type, int***** pin_to_track_connections, const e_pin_type pin_type, const int* Fc_array, const t_chan_width* chan_width_inf, const int num_segments, const t_segment_inf* segment_inf);

/**** EXPERIMENTAL ****/
#include <map>

class Wire_Counting {
  public:
    Wire_Counting() = default;

    /* number of wires in this wire group (here, wires are grouped by the number of switches they carry) */
    int num_wires = 0;
    /* map key is number of wires used. element represents how many times, over all possible configurations of the switches
     * in the channel, 'map key' wires from this wire group is used */
    std::map<int, long double> configs_used;

    /* the probabilistic expectation of how many wires will actually be used */
    float expectation_available = 0.;
};

typedef std::vector<std::vector<float> > t_xbar_matrix;

/* perform a probabilistic analysis on the compound crossbar formed by the input and output connection blocks */
void analyze_conn_blocks(const int***** opin_cb, const int***** ipin_cb, const t_type_ptr block_type, const int* Fc_array_out, const int* Fc_array_in, const t_chan_width* chan_width_inf);

/* make a poor cb pattern. */
void make_poor_cb_pattern(const e_pin_type pin_type, const t_type_ptr block_type, const int* Fc_array, const t_chan_width* chan_width_inf, int***** cb);

/**** END EXPERIMENTAL ****/

#endif /*CB_METRICS_H*/
