#include <unordered_map>
#include <set>

#include "TimingGraph.hpp"

#include "atom_netlist_fwd.h"
#include "atom_map.h"
#include "physical_types.h"


class TimingGraphBuilder {
    public:
        TimingGraphBuilder(const AtomNetlist& netlist,
                           AtomMap& netlist_map)
            : netlist_(netlist) 
            , netlist_map_(netlist_map) {
            build();
            opt_memory_layout();
        }

        tatum::TimingGraph timing_graph();

    private:
        void build();
        void opt_memory_layout();

        void add_io_to_timing_graph(const AtomBlockId blk);
        void add_block_to_timing_graph(const AtomBlockId blk);
        void add_net_to_timing_graph(const AtomNetId net);

        void fix_comb_loops();
        tatum::EdgeId find_scc_edge_to_break(std::vector<tatum::NodeId> scc);

        void remap_ids(const tatum::GraphIdMaps& id_mapping);

        const t_pb_graph_pin* find_pb_graph_pin(const AtomPinId pin);
        const t_pb_graph_pin* find_associated_clock_pin(const AtomPinId pin);

        void mark_clustering_net_delays(float inter_cluster_net_delay);

    private:
        tatum::TimingGraph tg_;

        const AtomNetlist& netlist_;
        AtomMap& netlist_map_;
};

