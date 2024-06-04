#ifndef VPR_CONTEXT_H
#define VPR_CONTEXT_H
#include <unordered_map>
#include <memory>
#include <vector>
#include <mutex>

#include "vpr_types.h"
#include "vtr_ndmatrix.h"
#include "vtr_optional.h"
#include "vtr_vector.h"
#include "atom_netlist.h"
#include "clustered_netlist.h"
#include "rr_graph_view.h"
#include "rr_graph_storage.h"
#include "rr_graph_builder.h"
#include "rr_node.h"
#include "rr_rc_data.h"
#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "power.h"
#include "power_components.h"
#include "device_grid.h"
#include "clock_network_builders.h"
#include "clock_connection_builders.h"
#include "route_tree.h"
#include "router_lookahead.h"
#include "place_macro.h"
#include "compressed_grid.h"
#include "metadata_storage.h"
#include "vpr_constraints.h"
#include "noc_storage.h"
#include "noc_traffic_flows.h"
#include "noc_routing.h"
#include "tatum/report/TimingPath.hpp"

#ifndef NO_SERVER

#include "gateio.h"
#include "taskresolver.h"

class SetupHoldTimingInfo;
class PostClusterDelayCalculator;

#endif /* NO_SERVER */

/**
 * @brief A Context is collection of state relating to a particular part of VPR
 *
 * This is a base class who's only purpose is to disable copying of contexts.
 * This ensures that attempting to use a context by value (instead of by reference)
 * will result in a compilation error.
 *
 * No data or member functions should be defined in this class!
 */
struct Context {
    //Contexts are non-copyable
    Context() = default;
    Context(Context&) = delete;
    Context& operator=(Context&) = delete;
    virtual ~Context() = default;
};

/**
 * @brief State relating to the atom-level netlist
 *
 * This should contain only data structures related to user specified netlist
 * being implemented by VPR onto the target device.
 */
struct AtomContext : public Context {
    /********************************************************************
     * Atom Netlist
     ********************************************************************/
    /**
     * @brief constructor
     *
     * In the constructor initialize the list of pack molecules to nullptr and defines a custom deletor for it
     */
    AtomContext()
        : list_of_pack_molecules(nullptr, free_pack_molecules) {}

    ///@brief Atom netlist
    AtomNetlist nlist;

    ///@brief Mappings to/from the Atom Netlist to physically described .blif models
    AtomLookup lookup;

    /**
     * @brief The molecules associated with each atom block.
     *
     * This map is loaded in the pre-packing stage and freed at the very end of vpr flow run.
     * The pointers in this multimap is shared with list_of_pack_molecules.
     */
    std::multimap<AtomBlockId, t_pack_molecule*> atom_molecules;

    /**
     * @brief A linked list of all the packing molecules that are loaded in pre-packing stage.
     *
     * Is is useful in freeing the pack molecules at the destructor of the Atom context using free_pack_molecules.
     */
    std::unique_ptr<t_pack_molecule, decltype(&free_pack_molecules)> list_of_pack_molecules;
};

/**
 * @brief State relating to timing
 *
 * This should contain only data structures related to timing analysis,
 * or related timing analysis algorithmic state.
 */
struct TimingContext : public Context {
    /********************************************************************
     * Timing
     ********************************************************************/

    /**
     * @brief The current timing graph.
     *
     * This represents the timing dependencies between pins of the atom netlist
     */
    std::shared_ptr<tatum::TimingGraph> graph;

    /**
     * @brief The current timing constraints, as loaded from an SDC file (or set by default).
     *
     * These specify how timing analysis is performed (e.g. target clock periods)
     */
    std::shared_ptr<tatum::TimingConstraints> constraints;

    t_timing_analysis_profile_info stats;

    /* Represents whether or not VPR should fail if timing constraints aren't met. */
    bool terminate_if_timing_fails = false;
};

namespace std {
template<>
struct hash<std::tuple<int, int, short>> {
    std::size_t operator()(const std::tuple<int, int, short>& ok) const noexcept {
        std::size_t seed = std::hash<int>{}(std::get<0>(ok));
        vtr::hash_combine(seed, std::get<1>(ok));
        vtr::hash_combine(seed, std::get<2>(ok));
        return seed;
    }
};
} // namespace std

/**
 * @brief State relating the device
 *
 * This should contain only data structures describing the targeted device.
 */
struct DeviceContext : public Context {
    /*********************************************************************
     * Physical FPGA architecture
     *********************************************************************/

    /**
     * @brief The device grid
     *
     * This represents the physical layout of the device. To get the physical tile at each location (layer_num, x, y) the helper functions in this data structure should be used.
     */
    DeviceGrid grid;
    /*
     * Empty types
     */

    t_physical_tile_type_ptr EMPTY_PHYSICAL_TILE_TYPE;
    t_logical_block_type_ptr EMPTY_LOGICAL_BLOCK_TYPE;

    /*
     * block_types are blocks that can be moved by the placer
     * such as: I/Os, CLBs, memories, multipliers, etc
     * Different types of physical block are contained in type descriptors
     */

    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    /*
     * Keep which layer in multi-die FPGA require inter-cluster programmable routing resources [0..number_of_layers-1]
     * If a layer doesn't require inter-cluster programmable routing resources,
     * RRGraph generation will ignore building SBs and CBs for that specific layer.
     */
    std::vector<bool> inter_cluster_prog_routing_resources;

    /**
     * @brief Boolean that indicates whether the architecture implements an N:M
     *        physical tiles to logical blocks mapping
     */
    bool has_multiple_equivalent_tiles;

    /*******************************************************************
     * Routing related
     ********************************************************************/

    ///@brief chan_width is for x|y-directed channels; i.e. between rows
    t_chan_width chan_width;

    /*
     * Structures to define the routing architecture of the FPGA.
     */

    vtr::vector<RRIndexedDataId, t_rr_indexed_data> rr_indexed_data; // [0 .. num_rr_indexed_data-1]

    ///@brief Fly-weighted Resistance/Capacitance data for RR Nodes
    std::vector<t_rr_rc_data> rr_rc_data;

    ///@brief Sets of non-configurably connected nodes
    std::vector<std::vector<RRNodeId>> rr_non_config_node_sets;

    ///@brief Reverse look-up from RR node to non-configurably connected node set (index into rr_non_config_node_sets)
    std::unordered_map<RRNodeId, int> rr_node_to_non_config_node_set;

    /* A writeable view of routing resource graph to be the ONLY database
     * for routing resource graph builder functions.
     */
    RRGraphBuilder rr_graph_builder{};

    /* A read-only view of routing resource graph to be the ONLY database
     * for client functions: GUI, placer, router, timing analyzer etc.
     */
    RRGraphView rr_graph{rr_graph_builder.rr_nodes(), rr_graph_builder.node_lookup(), rr_graph_builder.rr_node_metadata(), rr_graph_builder.rr_edge_metadata(), rr_indexed_data, rr_rc_data, rr_graph_builder.rr_segments(), rr_graph_builder.rr_switch(), rr_graph_builder.node_in_edge_storage(), rr_graph_builder.node_ptc_storage()};

    /* Track ids for each rr_node in the rr_graph.
     * This is used by drawer for tileable routing resource graph
     */
    std::map<RRNodeId, std::vector<size_t>> rr_node_track_ids;

    std::vector<t_arch_switch_inf> arch_switch_inf; // [0..(num_arch_switches-1)]

    std::map<int, t_arch_switch_inf> all_sw_inf;

    int delayless_switch_idx = OPEN;

    bool rr_graph_is_flat = false;

    /*
     * Clock Networks
     */

    std::vector<std::unique_ptr<ClockNetwork>> clock_networks;
    std::vector<std::unique_ptr<ClockConnection>> clock_connections;

    /**
     * @brief rr_node idx that connects to the input of all clock network wires
     *
     * Useful for two stage clock routing
     * XXX: currently only one place to source the clock networks so only storing
     *      a single value
     */
    int virtual_clock_network_root_idx;
    /**
     * @brief switch_fanin_remap is only used for printing out switch fanin stats
     *        (the -switch_stats option)
     *
     * array index: [0..(num_arch_switches-1)];
     * map key: num of all possible fanin of that type of switch on chip
     * map value: remapped switch index (index in rr_switch_inf)
     */
    std::vector<std::map<int, int>> switch_fanin_remap;

    /*******************************************************************
     * Architecture
     ********************************************************************/
    const t_arch* arch;

    /*******************************************************************
     * Clock Network
     ********************************************************************/
    t_clock_arch* clock_arch;

    /**
     * @brief Name of rrgraph file read (if any).
     *
     * Used to determine when reading rrgraph if file is already loaded.
     */
    std::string read_rr_graph_filename;

    /*******************************************************************
     * Place Related
     *******************************************************************/
    enum e_pad_loc_type pad_loc_type;
};

/**
 * @brief State relating to power analysis
 *
 * This should contain only data structures related to power analysis,
 * or related power analysis algorithmic state.
 */
struct PowerContext : public Context {
    /*******************************************************************
     * Power
     ********************************************************************/
    t_solution_inf solution_inf;
    t_power_output* output;
    t_power_commonly_used* commonly_used;
    t_power_tech* tech;
    t_power_arch* arch;
    vtr::vector<ClusterNetId, t_net_power> clb_net_power;

    ///@brief Atom net power info
    std::unordered_map<AtomNetId, t_net_power> atom_net_power;
    t_power_components by_component;
};

/**
 * @brief State relating to clustering
 *
 * This should contain only data structures that describe the current
 * clustering/packing, or related clusterer/packer algorithmic state.
 */
struct ClusteringContext : public Context {
    /********************************************************************
     * CLB Netlist
     ********************************************************************/

    ///@brief New netlist class derived from Netlist
    ClusteredNetlist clb_nlist;

    /* Database for nets of each clb block pin after routing stage
     * - post_routing_clb_pin_nets:
     *     mapping of pb_type pins to clustered net ids
     * - pre_routing_net_pin_mapping:
     *     a copy of mapping for current pb_route index to previous pb_route index
     *     Record the previous pin mapping for finding the correct pin index during timing analysis
     */
    std::map<ClusterBlockId, std::map<int, ClusterNetId>> post_routing_clb_pin_nets;
    std::map<ClusterBlockId, std::map<int, int>> pre_routing_net_pin_mapping;
};

/**
 * @brief State relating to helper data structure using in the clustering stage
 *
 * This should contain helper data structures that are useful in the clustering/packing stage.
 * They are encapsulated here as they are useful in clustering and reclustering algorithms that may be used
 * in packing or placement stages.
 */
struct ClusteringHelperContext : public Context {
    // A map used to save the number of used instances from each logical block type.
    std::map<t_logical_block_type_ptr, size_t> num_used_type_instances;

    // Stats keeper for placement information during packing/clustering
    t_cluster_placement_stats* cluster_placement_stats;

    // total number of models in the architecture
    int num_models;

    int max_cluster_size;
    t_pb_graph_node** primitives_list;

    bool enable_pin_feasibility_filter;
    int feasible_block_array_size;

    // total number of CLBs
    int total_clb_num;

    // A vector of routing resource nodes within each of logic cluster_ctx.blocks types [0 .. num_logical_block_type-1]
    std::vector<t_lb_type_rr_node>* lb_type_rr_graphs;

    // the utilization of external input/output pins during packing (between 0 and 1)
    t_ext_pin_util_targets target_external_pin_util;

    // During clustering, a block is related to un-clustered primitives with nets.
    // This relation has three types: low fanout, high fanout, and trasitive
    // high_fanout_thresholds stores the threshold for nets to a block type to be considered high fanout
    t_pack_high_fanout_thresholds high_fanout_thresholds;

    // A vector of unordered_sets of AtomBlockIds that are inside each clustered block [0 .. num_clustered_blocks-1]
    // unordered_set for faster insertion/deletion during the iterative improvement process of packing
    vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>> atoms_lookup;

    /** Stores the NoC group ID of each atom block. Atom blocks that belong
     * to different NoC groups can't be clustered with each other into the
     * same clustered block.*/
    vtr::vector<AtomBlockId, NocGroupId> atom_noc_grp_id;

    ~ClusteringHelperContext() {
        delete[] primitives_list;
    }
};

/**
 * @brief State relating to packing multithreading
 *
 * This contain data structures to synchronize multithreading of packing iterative improvement.
 */
struct PackingMultithreadingContext : public Context {
    vtr::vector<ClusterBlockId, bool> clb_in_flight;
    vtr::vector<ClusterBlockId, std::mutex> mu;
};

/**
 * @brief State relating to placement
 *
 * This should contain only data structures that describe the current placement,
 * or related placer algorithm state.
 */
struct PlacementContext : public Context {
    ///@brief Clustered block placement locations
    vtr::vector_map<ClusterBlockId, t_block_loc> block_locs;

    ///@brief Clustered pin placement mapping with physical pin
    vtr::vector_map<ClusterPinId, int> physical_pins;

    ///@brief Clustered block associated with each grid location (i.e. inverse of block_locs)
    GridBlock grid_blocks;

    ///@brief The pl_macros array stores all the placement macros (usually carry chains).
    std::vector<t_pl_macro> pl_macros;

    /**
     * @brief Compressed grid space for each block type
     *
     * Used to efficiently find logically 'adjacent' blocks of the same
     * block type even though the may be physically far apart
     * Indexed with logical block type index: [0...num_logical_block_types-1] -> logical block compressed grid
     */
    t_compressed_block_grids compressed_block_grids;

    /**
     * @brief SHA256 digest of the .place file
     *
     * Used for unique identification and consistency checking
     */
    std::string placement_id;

    /**
     * Use during placement to print extra debug information. It is set to true based on the number assigned to
     * placer_debug_net or placer_debug_block parameters in the command line.
     */
    bool f_placer_debug = false;

    /**
     * Set this variable to ture if the type of the bounding box used in placement is of the type cube. If it is false,
     * it would mean that per-layer bounding box is used. For the 2D architecture, the cube bounding box would be used.
     */
    bool cube_bb = false;
};

/**
 * @brief State relating to routing
 *
 * This should contain only data structures that describe the current routing implementation,
 * or related router algorithmic state.
 */
struct RoutingContext : public Context {
    /* [0..num_nets-1] of linked list start pointers.  Defines the routing.  */
    vtr::vector<ParentNetId, vtr::optional<RouteTree>> route_trees;

    vtr::vector<ParentNetId, std::unordered_set<int>> trace_nodes;

    vtr::vector<ParentNetId, std::vector<RRNodeId>> net_rr_terminals; /* [0..num_nets-1][0..num_pins-1] */

    vtr::vector<ParentNetId, uint8_t> is_clock_net; /* [0..num_nets-1] */

    vtr::vector<ParentBlockId, std::vector<RRNodeId>> rr_blk_source; /* [0..num_blocks-1][0..num_class-1] */

    vtr::vector<RRNodeId, t_rr_node_route_inf> rr_node_route_inf; /* [0..device_ctx.num_rr_nodes-1] */

    vtr::vector<ParentNetId, std::vector<std::vector<int>>> net_terminal_groups;

    vtr::vector<ParentNetId, std::vector<int>> net_terminal_group_num;

    /**
     * @brief Information about whether a node is part of a non-configurable set
     *
     * (i.e. connected to others with non-configurable edges like metal shorts that can't be disabled)
     * Stored in a single bit per rr_node for efficiency
     * bit value 0: node is not part of a non-configurable set
     * bit value 1: node is part of a non-configurable set
     * Initialized once when RoutingContext is initialized, static throughout invocation of router
     */
    vtr::dynamic_bitset<RRNodeId> non_configurable_bitset; /*[0...device_ctx.num_rr_nodes] */

    ///@brief Information about current routing status of each net
    t_net_routing_status net_status;

    ///@brief Limits area within which each net must be routed.
    vtr::vector<ParentNetId, t_bb> route_bb; /* [0..cluster_ctx.clb_nlist.nets().size()-1]*/

    t_clb_opins_used clb_opins_used_locally; //[0..cluster_ctx.clb_nlist.blocks().size()-1][0..num_class-1]

    /**
     * @brief SHA256 digest of the .route file
     *
     * Used for unique identification and consistency checking
     */
    std::string routing_id;

    // Cache key used to create router lookahead
    std::tuple<e_router_lookahead, std::string, std::vector<t_segment_inf>> router_lookahead_cache_key_;

    /**
     * @brief Cache of router lookahead object.
     *
     * Cache key: (lookahead type, read lookahead (if any), segment definitions).
     */
    vtr::Cache<std::tuple<e_router_lookahead, std::string, std::vector<t_segment_inf>>,
               RouterLookahead>
        cached_router_lookahead_;

    /**
     * @brief Routing constraints, read only
     */
    VprConstraints constraints;
};

/**
 * @brief State relating to VPR's floorplanning constraints
 *
 * This should contain only data structures related to constraining blocks
 * to certain regions on the chip.
 */
struct FloorplanningContext : public Context {
    /**
     * @brief Stores groups of constrained atoms, areas where the atoms are constrained to
     *
     * Provides all information needed about floorplanning constraints, including
     * which atoms are constrained and the regions they are constrained to.
     *
     * The constraints are input into vpr and do not change.
     */
    VprConstraints constraints;

    /**
     * @brief Constraints for each cluster
     *
     * Each cluster will have a PartitionRegion specifying its regions constraints
     * according to the constrained atoms packed into it. This structure allows the floorplanning
     * constraints for a given cluster to be found easily given its ClusterBlockId.
     *
     * The constraints on each cluster are computed during the clustering process and can change.
     */
    vtr::vector<ClusterBlockId, PartitionRegion> cluster_constraints;

    std::vector<Region> overfull_regions;
};

/**
 * @brief State of the Network on Chip (NoC)
 *
 * This should only contain data structures related to describing the
 * NoC within the device.
 */
struct NocContext : public Context {
    /**
     * @brief A model of the NoC
     *
     * Contains all the routers and links that make up the NoC. The routers contain
     * information regarding the physical tile positions they represent. The links
     * define the connections between every router (topology) and also metrics that describe its
     * "usage".
     *
     *
     * The NoC model is created once from the architecture file description.
     */
    NocStorage noc_model;

    /**
     * @brief Stores all the communication happening between routers in the NoC
     *
     * Contains all of the traffic flows that describe which pairs of logical routers are communicating and also some metrics and constraints on the data transfer between the two routers.
     * 
     *
     * This is created from a user supplied .flows file.
     */
    NocTrafficFlows noc_traffic_flows_storage;

    /**
     * @brief Contains the packet routing algorithm used by the NoC.
     *
     * This should be used to route traffic flows within the NoC.
     *
     * This is created from a user supplied command line option "--noc_routing_algorithm"
     */
    std::unique_ptr<NocRouting> noc_flows_router;
};

#ifndef NO_SERVER
/**
 * @brief State relating to server mode
 *
 * This should contain only data structures that
 * related to server state.
 */
class ServerContext : public Context {
  public:
    const server::GateIO& gateIO() const { return gate_io_; }
    server::GateIO& mutable_gateIO() { return gate_io_; }

    const server::TaskResolver& task_resolver() const { return task_resolver_; }
    server::TaskResolver& mutable_task_resolver() { return task_resolver_; }

    void set_crit_paths(std::vector<tatum::TimingPath>&& crit_paths) { crit_paths_ = std::move(crit_paths); }
    const std::vector<tatum::TimingPath>& crit_paths() const { return crit_paths_; }

    void clear_crit_path_elements() { crit_path_element_indexes_.clear(); }
    void set_crit_path_elements(const std::map<std::size_t, std::set<std::size_t>>& crit_path_element_indexes) { crit_path_element_indexes_ = crit_path_element_indexes; }
    std::map<std::size_t, std::set<std::size_t>> crit_path_element_indexes() const { return crit_path_element_indexes_; }

    void set_draw_crit_path_contour(bool draw_crit_path_contour) { draw_crit_path_contour_ = draw_crit_path_contour; }
    bool draw_crit_path_contour() const { return draw_crit_path_contour_; }

    void set_timing_info(const std::shared_ptr<SetupHoldTimingInfo>& timing_info) { timing_info_ = timing_info; }
    const std::shared_ptr<SetupHoldTimingInfo>& timing_info() const { return timing_info_; }

    void set_routing_delay_calc(const std::shared_ptr<PostClusterDelayCalculator>& routing_delay_calc) { routing_delay_calc_ = routing_delay_calc; }
    const std::shared_ptr<PostClusterDelayCalculator>& routing_delay_calc() const { return routing_delay_calc_; }

  private:
    server::GateIO gate_io_;
    server::TaskResolver task_resolver_;

    /**
     * @brief Stores the critical path items.
     *
     * This value is used when rendering the critical path by the selected index.
     * Once calculated upon request, it provides the value for a specific critical path
     * to be rendered upon user request.
     */
    std::vector<tatum::TimingPath> crit_paths_;

    /**
     * @brief Stores the selected critical path elements.
     *
     * This value is used to render the selected critical path elements upon client request.
     * The std::map key plays role of path index, where the element indexes are stored as std::set.
     */
    std::map<std::size_t, std::set<std::size_t>> crit_path_element_indexes_;

    /**
     * @brief Stores the flag indicating whether to draw the critical path contour.
     *
     * If the flag is set to true, the non-selected critical path elements will be drawn as a contour, while selected elements will be drawn as usual.
     */
    bool draw_crit_path_contour_ = false;

    /**
     * @brief Reference to the SetupHoldTimingInfo calculated during the routing stage.
     */
    std::shared_ptr<SetupHoldTimingInfo> timing_info_;

    /**
     * @brief Reference to the PostClusterDelayCalculator calculated during the routing stage.
     */
    std::shared_ptr<PostClusterDelayCalculator> routing_delay_calc_;
};
#endif /* NO_SERVER */

/**
 * @brief This object encapsulates VPR's state.
 *
 * There is typically a single instance which is
 * accessed via the global variable g_vpr_ctx (see globals.h/.cpp).
 *
 * It is divided up into separate sub-contexts of logically related data structures.
 *
 * Each sub-context can be accessed via member functions which return a reference to the sub-context:
 *  - The default the member function (e.g. device()) return an const (immutable) reference providing
 *    read-only access to the context. This should be the preferred form, as the compiler will detect
 *     unintentional state changes.
 *  - The 'mutable' member function (e.g. mutable_device()) will return a non-const (mutable) reference
 *    allowing modification of the context. This should only be used on an as-needed basis.
 *
 * Typical usage in VPR would be to call the appropriate accessor to get a reference to the context of
 * interest, and then operate on it.
 *
 *
 * For example if we were performing an action which required access to the current placement, we would do:
 *
 * ```
 *    void my_analysis_algorithm() {
 *        //Get read-only access to the placement
 *        auto& place_ctx = g_vpr_ctx.placement();
 *
 *        //Do something that depends on (but does not change)
 *        //the current placement...
 *
 *    }
 * ```
 * <!-- NOTE: Markdown ``` are used to display the code properly in the documentation -->
 *
 * If we needed to modify the placement (e.g. we were implementing another placement algorithm) we would do:
 *
 * ```
 *    void my_placement_algorithm() {
 *        //Get read-write access to the placement
 *        auto& place_ctx = g_vpr_ctx.mutable_placement();
 *
 *        //Do something that modifies the placement
 *        //...
 *    }
 * ```
 *
 *
 * @note The returned contexts are not copyable, so they must be taken by reference.
 */
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

    const ClusteringHelperContext& cl_helper() const { return helper_; }
    ClusteringHelperContext& mutable_cl_helper() { return helper_; }

    const PlacementContext& placement() const { return placement_; }
    PlacementContext& mutable_placement() { return placement_; }

    const RoutingContext& routing() const { return routing_; }
    RoutingContext& mutable_routing() { return routing_; }

    const FloorplanningContext& floorplanning() const { return constraints_; }
    FloorplanningContext& mutable_floorplanning() { return constraints_; }

    const NocContext& noc() const { return noc_; }
    NocContext& mutable_noc() { return noc_; }

    const PackingMultithreadingContext& packing_multithreading() const { return packing_multithreading_; }
    PackingMultithreadingContext& mutable_packing_multithreading() { return packing_multithreading_; }

#ifndef NO_SERVER
    const ServerContext& server() const { return server_; }
    ServerContext& mutable_server() { return server_; }
#endif /* NO_SERVER */

  private:
    DeviceContext device_;

    AtomContext atom_;

    TimingContext timing_;
    PowerContext power_;

    ClusteringContext clustering_;
    ClusteringHelperContext helper_;

    PlacementContext placement_;
    RoutingContext routing_;
    FloorplanningContext constraints_;
    NocContext noc_;

#ifndef NO_SERVER
    ServerContext server_;
#endif /* NO_SERVER */

    PackingMultithreadingContext packing_multithreading_;
};

#endif
