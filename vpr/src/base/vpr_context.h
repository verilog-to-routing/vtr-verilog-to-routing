#ifndef VPR_CONTEXT_H
#define VPR_CONTEXT_H
#include <unordered_map>
#include <memory>
#include <vector>
#include <mutex>

#include "prepack.h"
#include "vpr_types.h"
#include "vtr_ndmatrix.h"
#include "vtr_optional.h"
#include "vtr_vector.h"
#include "vtr_vector_map.h"
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
#include "blk_loc_registry.h"

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
    /// @brief Atom netlist
    AtomNetlist nlist;

    /// @brief Mappings to/from the Atom Netlist to physically described .blif models
    AtomLookup lookup;
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
    RRGraphView rr_graph{rr_graph_builder.rr_nodes(), rr_graph_builder.node_lookup(), rr_graph_builder.rr_node_metadata(), rr_graph_builder.rr_edge_metadata(), rr_indexed_data, rr_rc_data, rr_graph_builder.rr_segments(), rr_graph_builder.rr_switch()};
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

    /// @brief New netlist class derived from Netlist
    ClusteredNetlist clb_nlist;

    /// @brief Database for nets of each clb block pin after routing stage.
    ///  - post_routing_clb_pin_nets:
    ///     mapping of pb_type pins to clustered net ids.
    ///  - pre_routing_net_pin_mapping:
    ///     a copy of mapping for current pb_route index to previous pb_route index
    ///     Record the previous pin mapping for finding the correct pin index during
    ///     timing analysis.
    std::map<ClusterBlockId, std::map<int, ClusterNetId>> post_routing_clb_pin_nets;
    std::map<ClusterBlockId, std::map<int, int>> pre_routing_net_pin_mapping;

    /// @brief A vector of unordered_sets of AtomBlockIds that are inside each
    ///        clustered block [0 .. num_clustered_blocks-1]
    /// This is populated when the packing is loaded.
    vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>> atoms_lookup;
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
  private:
    /**
     * Determines if blk_loc_registry_ can be accessed by calling getter methods.
     * This flag should be set to false at the beginning of the placement stage,
     * and set to true at the end of placement. This ensures that variables that
     * are subject to change during placement are kept local to the placement stage.
     */
    bool loc_vars_are_accessible_ = true;

    /**
     * @brief Stores block location information, which is subject to change during the
     * placement stage.
     */
    BlkLocRegistry blk_loc_registry_;

  public:

    const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs() const { VTR_ASSERT_SAFE(loc_vars_are_accessible_); return blk_loc_registry_.block_locs(); }
    vtr::vector_map<ClusterBlockId, t_block_loc>& mutable_block_locs() { VTR_ASSERT_SAFE(loc_vars_are_accessible_); return blk_loc_registry_.mutable_block_locs(); }
    const GridBlock& grid_blocks() const { VTR_ASSERT_SAFE(loc_vars_are_accessible_); return blk_loc_registry_.grid_blocks(); }
    GridBlock& mutable_grid_blocks() { VTR_ASSERT_SAFE(loc_vars_are_accessible_); return blk_loc_registry_.mutable_grid_blocks(); }
    vtr::vector_map<ClusterPinId, int>& mutable_physical_pins() { VTR_ASSERT_SAFE(loc_vars_are_accessible_); return blk_loc_registry_.mutable_physical_pins(); }
    const vtr::vector_map<ClusterPinId, int>& physical_pins() const { VTR_ASSERT_SAFE(loc_vars_are_accessible_); return blk_loc_registry_.physical_pins(); }
    BlkLocRegistry& mutable_blk_loc_registry() { VTR_ASSERT_SAFE(loc_vars_are_accessible_); return blk_loc_registry_; }
    const BlkLocRegistry& blk_loc_registry() const { VTR_ASSERT_SAFE(loc_vars_are_accessible_); return blk_loc_registry_; }

    /**
     * @brief Makes blk_loc_registry_ inaccessible through the getter methods.
     *
     * This method should be called at the beginning of the placement stage to
     * guarantee that the placement stage code does not access block location variables
     * stored in the global state.
     */
    void lock_loc_vars() { VTR_ASSERT_SAFE(loc_vars_are_accessible_); loc_vars_are_accessible_ = false; }

    /**
     * @brief Makes blk_loc_registry_ accessible through the getter methods.
     *
     * This method should be called at the end of the placement stage to
     * make the block location information accessible for subsequent stages.
     */
    void unlock_loc_vars() { VTR_ASSERT_SAFE(!loc_vars_are_accessible_); loc_vars_are_accessible_ = true; }

    ///@brief The pl_macros array stores all the placement macros (usually carry chains).
    std::vector<t_pl_macro> pl_macros;

    ///@brief Stores ClusterBlockId of all movable clustered blocks (blocks that are not locked down to a single location)
    std::vector<ClusterBlockId> movable_blocks;

    ///@brief Stores ClusterBlockId of all movable clustered of each block type
    std::vector<std::vector<ClusterBlockId>> movable_blocks_per_type;

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
     * @brief User specified routing constraints
     */
    UserRouteConstraints constraints;
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
    UserPlaceConstraints constraints;

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

    /**
     * @brief Floorplanning constraints in the compressed grid coordinate system.
     *
     * Indexing -->  [0..grid.num_layers-1][0..numClusters-1]
     *
     * Each clustered block has a logical type with a corresponding compressed grid.
     * Compressed floorplanning constraints are calculated by translating the grid locations
     * of floorplanning regions to compressed grid locations. To ensure regions do not enlarge:
     * - The bottom left corner is rounded up to the nearest compressed location.
     * - The top right corner is rounded down to the nearest compressed location.
     *
     * When the floorplanning constraint spans across multiple layers, a compressed
     * constraints is created for each a layer that the original constraint includes.
     * This is because blocks of the same type might have different (x, y) locations
     * in different layers, and as result, their compressed locations in each layer
     * may correspond to a different physical (x, y) location.
     *
     */
    std::vector<vtr::vector<ClusterBlockId, PartitionRegion>> compressed_cluster_constraints;

    std::vector<PartitionRegion> overfull_partition_regions;
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
 * relate to the vpr server state.
 */
struct ServerContext : public Context {
    /**
     * @brief \ref server::GateIO.
     */
    server::GateIO gate_io;

    /**
     * @brief \ref server::TaskResolver.
     */
    server::TaskResolver task_resolver;

    /**
     * @brief Stores the critical path items.
     *
     * This value is used when rendering the critical path by the selected index.
     * Once calculated upon request, it provides the value for a specific critical path
     * to be rendered upon user request.
     */
    std::vector<tatum::TimingPath> crit_paths;

    /**
     * @brief Stores the selected critical path elements.
     *
     * This value is used to render the selected critical path elements upon client request.
     * The std::map key plays role of path index, where the element indexes are stored as std::set.
     */
    std::map<std::size_t, std::set<std::size_t>> crit_path_element_indexes;

    /**
     * @brief Stores the flag indicating whether to draw the critical path contour.
     *
     * If True, the entire path will be rendered with some level of transparency, regardless of the selection of path elements. However, selected path elements will be drawn in full color.
     * This feature is helpful in visual debugging, to see how the separate path elements are mapped into the whole path.
     */
    bool draw_crit_path_contour = false;

    /**
     * @brief Reference to the SetupHoldTimingInfo calculated during the routing stage.
     */
    std::shared_ptr<SetupHoldTimingInfo> timing_info;

    /**
     * @brief Reference to the PostClusterDelayCalculator calculated during the routing stage.
     */
    std::shared_ptr<PostClusterDelayCalculator> routing_delay_calc;
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
