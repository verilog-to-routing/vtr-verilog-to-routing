#include <unordered_map>

#include "vtr_linear_map.h"

#include "TimingGraph.hpp"
#include "atom_netlist_fwd.h"
#include "atom_map.h"
#include "physical_types.h"


class TimingGraphBuilder {
    public:
        TimingGraphBuilder(const AtomNetlist& netlist,
                           AtomMap& netlist_map,
                           const std::unordered_map<AtomBlockId,t_pb_graph_node*>& blk_to_pb_gnode_map)
            : netlist_(netlist) 
            , netlist_map_(netlist_map)
            , blk_to_pb_gnode_(blk_to_pb_gnode_map) {}

        tatum::TimingGraph build_timing_graph();

    private:
        void add_io_to_timing_graph(const AtomBlockId blk);
        void add_comb_block_to_timing_graph(const AtomBlockId blk);
        void add_seq_block_to_timing_graph(const AtomBlockId blk);
        void add_net_to_timing_graph(const AtomNetId net);

        const t_pb_graph_pin* find_pb_graph_pin(const AtomPinId pin);
        const t_pb_graph_pin* find_associated_clock_pin(const AtomPinId pin);

    private:
        tatum::TimingGraph tg_;

        const AtomNetlist& netlist_;
        AtomMap& netlist_map_;
        const std::unordered_map<AtomBlockId,t_pb_graph_node*>& blk_to_pb_gnode_;
};

