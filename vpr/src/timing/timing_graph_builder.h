#include <memory>

#include "tatum/TimingGraphFwd.hpp"

#include "atom_netlist_fwd.h"
#include "atom_lookup.h"

/*
 * Class for constructing a Timing Graph (a tatum::TimingGraph, for use with the Tatum 
 * STA engine) from the provided AtomNetlist. It also updates the provided AtomLookup 
 * with the mapping from netlist elements to timing graph nodes.
 *
 * To construct a timing graph initialize this class with the netlist and lookup 
 * (via constructor), and call the timing_graph() method.
 *
 * For details on how the timing graph is constructed from the netlist, see the comments 
 * in the associated .cpp implementation file.
 */
class TimingGraphBuilder {
  public:
    TimingGraphBuilder(const AtomNetlist& netlist,
                       AtomLookup& netlist_lookup);

    std::unique_ptr<tatum::TimingGraph> timing_graph(bool allow_dangling_combinational_nodes);

  private:
    void build(bool allow_dangling_combinational_nodes);
    void opt_memory_layout();

    void add_io_to_timing_graph(const AtomBlockId blk);
    void add_block_to_timing_graph(const AtomBlockId blk);
    void add_net_to_timing_graph(const AtomNetId net);

    //Helper functions for add_block_to_timing_graph()
    std::set<tatum::NodeId> create_block_timing_nodes(const AtomBlockId blk);
    void create_block_internal_data_timing_edges(const AtomBlockId blk, const std::set<tatum::NodeId>& clock_generator_tnodes);
    void create_block_internal_clock_timing_edges(const AtomBlockId blk, const std::set<tatum::NodeId>& clock_generator_tnodes);

    void fix_comb_loops();
    tatum::EdgeId find_scc_edge_to_break(std::vector<tatum::NodeId> scc);

    void remap_ids(const tatum::GraphIdMaps& id_mapping);

    const t_pb_graph_pin* find_pb_graph_pin(const AtomPinId pin);
    const t_pb_graph_pin* find_associated_clock_pin(const AtomPinId pin);

    void mark_clustering_net_delays(float inter_cluster_net_delay);

    bool is_netlist_clock_source(const AtomPinId pin) const;

    bool validate_netlist_timing_graph_consistency() const;

  private:
    std::unique_ptr<tatum::TimingGraph> tg_;

    const AtomNetlist& netlist_;
    AtomLookup& netlist_lookup_;

    std::set<AtomPinId> netlist_clock_drivers_;
};
