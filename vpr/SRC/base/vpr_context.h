#ifndef VPR_CONTEXT_H
#define VPR_CONTEXT_H
#include <unordered_map>
#include <memory>

#include "vtr_matrix.h"
#include "vtr_ndmatrix.h"
#include "netlist.h"
#include "atom_netlist.h"
#include "rr_node.h"
#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "power.h"
#include "power_components.h"

//A Context is collection of state relating to a particular part of VPR
//
//This is a base class who's only purpose is to disable copying of contexts.
//This ensures that attempting to use a context by value (instead of by reference)
//will result in a compilation error.
//
//No data or member functions should be defined in this class!
struct Context {
    //Contexts are non-copyable
    Context() = default;
    Context(Context&) = delete;
    Context& operator=(Context&) = delete;
};

//State relating to the atom-level netlist
//
//This should contain only data structures related to user specified netlist
//being implemented by VPR onto the target device.
struct AtomContext : public Context {
    /********************************************************************
     Atom Netlist
     ********************************************************************/
    /* Atom netlist */
    AtomNetlist nlist;

    /* Mappings to/from the Atom Netlist */
    AtomLookup lookup;
};

//State relating to timing
//
//This should contain only data structures related to timing analysis, 
//or related timing analysis algorithmic state.
struct TimingContext : public Context {
    /********************************************************************
     Timing
     ********************************************************************/

    //The current timing graph. 
    //This represents the timing dependencies between pins of the atom netlist
    std::shared_ptr<tatum::TimingGraph> graph;

    //The current timing constraints, as loaded from an SDC file (or set by default).
    //These specify how timing analysis is performed (e.g. target clock periods)
    std::shared_ptr<tatum::TimingConstraints> constraints;

    struct timing_analysis_profile_info {
        double timing_analysis_wallclock_time() const {
            return sta_wallclock_time + slack_wallclock_time;
        }

        double old_timing_analysis_wallclock_time() const {
            return old_sta_wallclock_time + old_delay_annotation_wallclock_time;
        }

        double sta_wallclock_time = 0.;
        double slack_wallclock_time = 0.;
        size_t num_full_updates = 0;

        double old_sta_wallclock_time = 0.;
        double old_delay_annotation_wallclock_time = 0.;
        size_t num_old_sta_full_updates = 0;
    };
    timing_analysis_profile_info stats;


    //Legacy timing analyzer globals (TODO: remove)
    t_timing_constraints* sdc;
    int num_tnodes; /* Number of nodes (pins) in the timing graph */
    t_tnode* tnodes; /* [0..num_tnodes - 1] nodes in the timing graph */
    int num_tnode_levels; /* Number of levels in the timing graph. */
    std::vector<std::vector<int>> tnodes_at_level; /* [0..num__tnode_levels - 1].  Count and list of tnodes at each level of    
                                   * the timing graph, to make topological searches easier. Level-0 nodes are
                                   * sources to the timing graph (types TN_FF_SOURCE, TN_INPAD_SOURCE
                                   * and TN_CONSTANT_GEN_SOURCE). Level-N nodes are in the immediate fanout of 
                                   * nodes with level at most N-1. */

};

//State relating the device
//
//This should contain only data structures describing the targeted device.
struct DeviceContext : public Context {
    /*********************************************************************
     Physical FPGA architecture 
     *********************************************************************/
    /* x and y dimensions of the FPGA itself, the core of the FPGA is from [1..nx][1..ny], the I/Os form a perimeter surrounding the core */
    int nx, ny;
    vtr::Matrix<t_grid_tile> grid; /* FPGA complex blocks grid [0..nx+1][0..ny+1] */

    /* Special pointers to identify special blocks on an FPGA: I/Os, unused, and default */
    t_type_ptr IO_TYPE;
    t_type_ptr EMPTY_TYPE;
    t_type_ptr FILL_TYPE;

    /* block_types are blocks that can be moved by the placer
     such as: I/Os, CLBs, memories, multipliers, etc
     Different types of physical block are contained in type descriptors
     */
    int num_block_types;
    t_type_descriptor *block_types;

    /*******************************************************************
     Routing related
     ********************************************************************/

    /* chan_width is for x|y-directed channels; i.e. between rows */
    t_chan_width chan_width;

    /* Structures to define the routing architecture of the FPGA.           */
    int num_rr_nodes;
    t_rr_node *rr_nodes; /* [0..num_rr_nodes-1]          */

    int num_rr_indexed_data;
    t_rr_indexed_data *rr_indexed_data; /* [0 .. num_rr_indexed_data-1] */

    //The indicies of rr nodes of a given type at a specific x,y grid location
    t_rr_node_indices rr_node_indices; //[0..NUM_RR_TYPES-1][0..nx+1][0..ny+1][0..size-1]

    int num_rr_switches;
    t_rr_switch_inf *rr_switch_inf; /* autogenerated in build_rr_graph based on switch fan-in. [0..(num_rr_switches-1)] */

    int num_arch_switches;
    t_arch_switch_inf *arch_switch_inf; /* [0..(num_arch_switches-1)] */

    /*
     * switch_fanin_remap is only used for printing out switch fanin stats (the -switch_stats option)
     * array index: [0..(num_arch_switches-1)]; 
     * map key: num of all possible fanin of that type of switch on chip
     * map value: remapped switch index (index in rr_switch_inf)
     */
    map<int, int> *switch_fanin_remap; 

    /*******************************************************************
     Clock Network
     ********************************************************************/
    t_clock_arch * clock_arch;

    /*******************************************************************
     Timing related
     ********************************************************************/
    //TODO: Remove these max_internal_delay globals when the classic timing analyzer is removed
    float pb_max_internal_delay; /* biggest internal delay of block */
    const t_pb_type *pbtype_max_internal_delay; /* block type with highest internal delay */
};

//State relating to power analysis
//
//This should contain only data structures related to power analysis, 
//or related power analysis algorithmic state.
struct PowerContext : public Context {
    /*******************************************************************
     Power
     ********************************************************************/
    t_solution_inf solution_inf;
    t_power_output* output;
    t_power_commonly_used* commonly_used;
    t_power_tech* tech;
    t_power_arch* arch;
    t_net_power * clb_net_power;


    /* Atom net power info */
    std::unordered_map<AtomNetId,t_net_power> atom_net_power;
    t_power_components by_component;
};

//State relating to clustering
//
//This should contain only data structures that describe the current 
//clustering/packing, or related clusterer/packer algorithmic state.
struct ClusteringContext : public Context {
    /********************************************************************
     CLB Netlist
     ********************************************************************/
    /* blocks in the clustered netlist */
    int num_blocks;
    t_block *blocks; //[0..num_blocks-1]

    /* External-to-complex blocks, post-packed netlist [NETS ONLY]*/
    t_netlist clbs_nlist;
};


//State relating to placement
//
//This should contain only data structures that describe the current placement, 
//or related placer algorithm state.
struct PlacementContext : public Context {
    //Clustered block placement locations
    std::vector<t_block_loc> block_locs; //[0..cluster_ctx.num_blocks-1]

    //Clustered block associated with each grid location (i.e. inverse of block_locs)
    //std::vector<std::vector<t_grid_blocks>> grid_blocks; //[0..device_ctx.nx+1][0..device_ctx.ny+1]
    vtr::Matrix<t_grid_blocks> grid_blocks; //[0..device_ctx.nx+1][0..device_ctx.ny+1]

    //SHA256 digest of the .place file (used for unique identification and consistency checking)
    std::string placement_id;
};

//State relating to routing
//
//This should contain only data structures that describe the current routing implementation, 
//or related router algorithmic state.
struct RoutingContext : public Context {
    /* [0..num_nets-1] of linked list start pointers.  Defines the routing.  */
    t_trace **trace_head, **trace_tail;

    t_rr_node_state* rr_node_state; /* [0..num_rr_nodes-1] */

    int **net_rr_terminals; /* [0..num_nets-1][0..num_pins-1] */

    int **rr_blk_source; /* [0..num_blocks-1][0..num_class-1] */

    t_rr_node_route_inf *rr_node_route_inf; /* [0..device_ctx.num_rr_nodes-1] */

    //Limits area in which each net must be routed.
    t_bb* route_bb = NULL; /* [0..cluster_ctx.clbs_nlist.net.size()-1]*/

    //SHA256 digest of the .route file (used for unique identification and consistency checking)
    std::string routing_id;
};

//This object encapsulates VPR's state. There is typically a single instance which is
//accessed via the global variable g_vpr_ctx (see globals.h/.cpp).
//
//It is divided up into separate sub-contexts of logically related data structures.
//
//Each sub-context can be accessed via member functions which return a reference to the sub-context:
//  * The default the member function (e.g. device()) return an const (immutable) reference providing 
//    read-only access to the context. This should be the preferred form, as the compiler will detect
//    unintentional state changes.
//  * The 'mutable' member function (e.g. mutable_device()) will return a non-const (mutable) reference 
//    allowing modification of the context. This should only be used on an as-needed basis.
//
//Typical usage in VPR would be to call the appropriate accessor to get a reference to the context of
//interest, and then operate on it.
//
//
//For example if we were performing an action which required access to the current placement, we would do:
//
//      void my_analysis_algorithm() {
//          //Get read-only access to the placement
//          auto& place_ctx = g_vpr_ctx.placement();
//
//          //Do something that depends on (but does not change)
//          //the current placement...
//
//      }
//
//If we needed to modify the placement (e.g. we were implementing another placement algorithm) we would do:
//
//      void my_placement_algorithm() {
//          //Get read-write access to the placement
//          auto& place_ctx = g_vpr_ctx.mutable_placement();
//
//          //Do something that modifies the placement
//          //...
//      }
//
//Note that the returned contexts are not copyable, so they must be taken by reference.
class VprContext : public Context {
    public:
        const AtomContext& atom() const { return atom_; }
        AtomContext& mutable_atom() { return atom_; }

        const DeviceContext& device() const { return device_; }
        DeviceContext& mutable_device() { return device_; }

        const TimingContext& timing() const { return timing_; }
        TimingContext& mutable_timing() { return timing_; }

        const PowerContext& power() const { return power_; }
        PowerContext& mutable_power() { return power_; }

        const ClusteringContext& clustering() const { return clustering_; }
        ClusteringContext& mutable_clustering() { return clustering_; }

        const PlacementContext& placement() const { return placement_; }
        PlacementContext& mutable_placement() { return placement_; }

        const RoutingContext& routing() const { return routing_; }
        RoutingContext& mutable_routing() { return routing_; }
    private:
        DeviceContext device_;

        AtomContext atom_;

        TimingContext timing_;
        PowerContext power_;

        ClusteringContext clustering_;
        PlacementContext placement_;
        RoutingContext routing_;
};

#endif
