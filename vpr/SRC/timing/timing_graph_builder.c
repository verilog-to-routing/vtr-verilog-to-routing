#include <set>

#include "vtr_log.h"
#include "vtr_linear_map.h"

#include "timing_graph_builder.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "atom_netlist.h"

#include "tatum/TimingGraph.hpp"

using tatum::TimingGraph;
using tatum::NodeId;
using tatum::NodeType;
using tatum::EdgeId;

template<class K, class V>
tatum::util::linear_map<K,V> remap_valid(const tatum::util::linear_map<K,V>& data, const tatum::util::linear_map<K,K>& id_map) {
    tatum::util::linear_map<K,V> new_data;

    for(size_t i = 0; i < data.size(); ++i) {
        tatum::EdgeId old_edge(i);
        tatum::EdgeId new_edge = id_map[old_edge];

        if(new_edge) {
            new_data.insert(new_edge, data[old_edge]);
        }
    }

    return new_data;
}

std::unique_ptr<TimingGraph> TimingGraphBuilder::timing_graph() {
    build();
    opt_memory_layout();

    VTR_ASSERT(tg_);
    tg_->validate();
    return std::move(tg_);
}

void TimingGraphBuilder::build() {
    tg_ = std::unique_ptr<tatum::TimingGraph>(new tatum::TimingGraph());

    for(AtomBlockId blk : netlist_.blocks()) {

        AtomBlockType blk_type = netlist_.block_type(blk);

        if(blk_type == AtomBlockType::INPAD || blk_type == AtomBlockType::OUTPAD) {
            add_io_to_timing_graph(blk);
        } else if (blk_type == AtomBlockType::BLOCK) {
            add_block_to_timing_graph(blk);
        } else {
            VPR_THROW(VPR_ERROR_TIMING, "Unrecognized atom block type while constructing timing graph");
        }
    }

    for(AtomNetId net : netlist_.nets()) {
        add_net_to_timing_graph(net);
    }

    fix_comb_loops();

    tg_->levelize();
}

void TimingGraphBuilder::opt_memory_layout() {
    auto id_map = tg_->optimize_layout();


    remap_ids(id_map);

}

void TimingGraphBuilder::add_io_to_timing_graph(const AtomBlockId blk) {

    NodeType node_type;
    AtomPinId pin;
    if(netlist_.block_type(blk) == AtomBlockType::INPAD) {
        node_type = NodeType::SOURCE;

        if(netlist_.block_pins(blk).size() == 1) {
            pin = *netlist_.block_pins(blk).begin();
        } else {
            //Un-swept disconnected input
            VTR_ASSERT(netlist_.block_pins(blk).size() == 0);
            return;
        }

    } else {
        VTR_ASSERT(netlist_.block_type(blk) == AtomBlockType::OUTPAD);
        node_type = NodeType::SINK;

        if(netlist_.block_pins(blk).size() == 1) {
            pin = *netlist_.block_pins(blk).begin();
        } else {
            //Un-swept disconnected output
            VTR_ASSERT(netlist_.block_pins(blk).size() == 0);
            return;
        }
    }

    NodeId tnode = tg_->add_node(node_type);

    netlist_lookup_.set_atom_pin_tnode(pin, tnode);
}

void TimingGraphBuilder::add_block_to_timing_graph(const AtomBlockId blk) {
    std::set<std::string> output_ports_requiring_internal_sinks;

    //Create the input pins
    for(AtomPinId input_pin : netlist_.block_input_pins(blk)) {
        AtomPortId input_port = netlist_.pin_port(input_pin);

        //Inspect the port model to determine pin type
        const t_model_ports* model_port = netlist_.port_model(input_port);

        NodeId tnode;
        VTR_ASSERT(!model_port->is_clock);
        if(model_port->clock.empty()) {
            //No clock => combinational input
            tnode = tg_->add_node(NodeType::IPIN);

            //A combinational pin is really both internal and external, mark it internal here
            //and external iin the default case below
            netlist_lookup_.set_atom_pin_tnode(input_pin, tnode, BlockTnode::INTERNAL);
        } else {
            tnode = tg_->add_node(NodeType::SINK);

            if(!model_port->combinational_sink_ports.empty()) {
                //There is an internal sequential connection within this primitive

                //Create the internal source
                NodeId internal_tnode = tg_->add_node(NodeType::SOURCE);
                netlist_lookup_.set_atom_pin_tnode(input_pin, internal_tnode, BlockTnode::INTERNAL);

                //Record the output port which will need an internal sink created
                output_ports_requiring_internal_sinks.insert(model_port->combinational_sink_ports.begin(),
                                                             model_port->combinational_sink_ports.end());
            }
        }

        netlist_lookup_.set_atom_pin_tnode(input_pin, tnode);
    }

    //Create the clock pins
    for(AtomPinId clock_pin : netlist_.block_clock_pins(blk)) {
        AtomPortId clock_port = netlist_.pin_port(clock_pin);

        //Inspect the port model to determine pin type
        const t_model_ports* model_port = netlist_.port_model(clock_port);

        VTR_ASSERT(model_port->is_clock);
        VTR_ASSERT(model_port->clock.empty());

        NodeId tnode = tg_->add_node(NodeType::CPIN);

        netlist_lookup_.set_atom_pin_tnode(clock_pin, tnode);
    }

    //Create the output pins
    for(AtomPinId output_pin : netlist_.block_output_pins(blk)) {
        AtomPortId output_port = netlist_.pin_port(output_pin);

        //Inspect the port model to determine pin type
        const t_model_ports* model_port = netlist_.port_model(output_port);

        NodeId tnode;
        if(model_port->is_clock) {
            //A generated clock source
            tnode = tg_->add_node(NodeType::SOURCE);

        } else if(model_port->clock.empty()) {
            //No clock => combinational output
            tnode = tg_->add_node(NodeType::OPIN);

            //A combinational pin is really both internal and external, mark it internal here
            //and external iin the default case below
            netlist_lookup_.set_atom_pin_tnode(output_pin, tnode, BlockTnode::INTERNAL);

        } else {
            VTR_ASSERT(!model_port->is_clock);
            VTR_ASSERT(!model_port->clock.empty());
            //Has an associated clock => sequential output
            tnode = tg_->add_node(NodeType::SOURCE);


            if(output_ports_requiring_internal_sinks.count(model_port->name)) {
                NodeId internal_tnode = tg_->add_node(NodeType::SINK);
                netlist_lookup_.set_atom_pin_tnode(output_pin, internal_tnode, BlockTnode::INTERNAL);
            }
        }

        netlist_lookup_.set_atom_pin_tnode(output_pin, tnode);
    }


    //Connect the clock pins to the sources and sinks
    for(AtomPinId pin : netlist_.block_pins(blk)) {

        for(auto blk_tnode_type : {BlockTnode::EXTERNAL, BlockTnode::INTERNAL}) {
            NodeId tnode = netlist_lookup_.atom_pin_tnode(pin, blk_tnode_type);

            if (tnode) {
                auto node_type = tg_->node_type(tnode);

                if(node_type == NodeType::SOURCE || node_type == NodeType::SINK) {
                    //Look-up the clock name on the port model
                    AtomPortId port = netlist_.pin_port(pin);
                    const t_model_ports* model_port = netlist_.port_model(port);

                    if(model_port->is_clock && model_port->dir == OUT_PORT) {
                        //This pin is a clock generator, so there is no clock pin
                        //connection
                        continue; 
                    }

                    VTR_ASSERT_MSG(!model_port->clock.empty(), "Sequential pins must have a clock");

                    //Find the clock pin in the netlist
                    AtomPortId clk_port = netlist_.find_port(blk, model_port->clock);
                    VTR_ASSERT(clk_port);
                    VTR_ASSERT_MSG(netlist_.port_width(clk_port) == 1, "Primitive clock ports can only contain one pin");

                    AtomPinId clk_pin = netlist_.port_pin(clk_port, 0);
                    VTR_ASSERT(clk_pin);

                    //Convert the pin to it's tnode
                    NodeId clk_tnode = netlist_lookup_.atom_pin_tnode(clk_pin);

                    tatum::EdgeType type;
                    if(node_type == NodeType::SOURCE) {
                        type = tatum::EdgeType::PRIMITIVE_CLOCK_LAUNCH;
                    } else {
                        VTR_ASSERT(node_type == NodeType::SINK);
                        type = tatum::EdgeType::PRIMITIVE_CLOCK_CAPTURE;
                    }

                    //Add the edge from the clock to the source/sink
                    tg_->add_edge(type, clk_tnode, tnode);
                }
            }
        }
    }

    //Connect the combinational edges
    for(AtomPinId src_pin : netlist_.block_input_pins(blk)) {
        //Combinational edges go between IPINs and OPINs for combinational blocks
        //and between the internal SOURCEs and SINKS for sequential blocks
        for(auto blk_tnode_type : {BlockTnode::EXTERNAL, BlockTnode::INTERNAL}) {
            NodeId src_tnode = netlist_lookup_.atom_pin_tnode(src_pin, blk_tnode_type);

            if(src_tnode) {
                auto src_type = tg_->node_type(src_tnode);
                bool is_internal_source = (   blk_tnode_type == BlockTnode::INTERNAL 
                                           && src_type == NodeType::SOURCE);

                if(src_type == NodeType::IPIN || is_internal_source) {

                    //Look-up the combinationally connected sink ports name on the port model
                    AtomPortId src_port = netlist_.pin_port(src_pin);
                    const t_model_ports* model_port = netlist_.port_model(src_port);

                    for(std::string sink_port_name : model_port->combinational_sink_ports) {
                        AtomPortId sink_port = netlist_.find_port(blk, sink_port_name); 
                        VTR_ASSERT(sink_port);

                        //We now need to create edges between the source pin, and all the pins in the
                        //output port
                        for(AtomPinId sink_pin : netlist_.port_pins(sink_port)) {
                            NodeId sink_tnode = netlist_lookup_.atom_pin_tnode(sink_pin, blk_tnode_type);
                            VTR_ASSERT(sink_tnode);

                            auto sink_type = tg_->node_type(sink_tnode);
                            bool is_internal_sink = (   blk_tnode_type == BlockTnode::INTERNAL 
                                                     && sink_type == NodeType::SINK);
                            VTR_ASSERT_MSG(sink_type == NodeType::OPIN
                                           || is_internal_sink, 
                                           "Internal primitive combinational edges must be between IPINs and OPINs,"
                                           " or internal SINKs and internal SOURCEs");

                            //Add the edge between the pins
                            tg_->add_edge(tatum::EdgeType::PRIMITIVE_COMBINATIONAL, src_tnode, sink_tnode);
                        }
                    }
                }
            }
        }
    }
}

void TimingGraphBuilder::add_net_to_timing_graph(const AtomNetId net) {
    //Create edges from the driver to sink tnodes

    AtomPinId driver_pin = netlist_.net_driver(net);
    NodeId driver_tnode = netlist_lookup_.atom_pin_tnode(driver_pin);
    VTR_ASSERT(driver_tnode);
    
    for(AtomPinId sink_pin : netlist_.net_sinks(net)) {
        NodeId sink_tnode = netlist_lookup_.atom_pin_tnode(sink_pin);
        VTR_ASSERT(sink_tnode);

        tg_->add_edge(tatum::EdgeType::INTERCONNECT, driver_tnode, sink_tnode);
    }
}

void TimingGraphBuilder::fix_comb_loops() {
    auto sccs = tatum::identify_combinational_loops(*tg_);

    //For non-simple loops (i.e. SCCs with multiple loops) we may need to break
    //multiple edges, so repeatedly break edges until there are no SCCs left
    while(!sccs.empty()) {
        vtr::printf_warning(__FILE__, __LINE__, "Detected %zu strongly connected component(s) forming combinational loop(s) in timing graph\n", sccs.size());
        for(auto scc : sccs) {
            EdgeId edge_to_break = find_scc_edge_to_break(scc);
            VTR_ASSERT(edge_to_break);

            tg_->disable_edge(edge_to_break);
        }

        sccs = tatum::identify_combinational_loops(*tg_);
    }
}

tatum::EdgeId TimingGraphBuilder::find_scc_edge_to_break(std::vector<tatum::NodeId> scc) {
    //Find an edge which is part of the SCC and remove it from the graph,
    //in the hope of breaking the SCC
    std::set<tatum::NodeId> scc_set(scc.begin(), scc.end());
    for(tatum::NodeId src_node : scc) {
        AtomPinId src_pin = netlist_lookup_.tnode_atom_pin(src_node);
        for(tatum::EdgeId edge : tg_->node_out_edges(src_node)) {
            if(tg_->edge_disabled(edge)) continue;

            tatum::NodeId sink_node = tg_->edge_sink_node(edge);
            AtomPinId sink_pin = netlist_lookup_.tnode_atom_pin(sink_node);

            if(scc_set.count(sink_node)) {
                vtr::printf_warning(__FILE__, __LINE__, "Arbitrarily disabling timing graph edge %zu (%s -> %s) to break combinational loop\n", 
                                                         edge, netlist_.pin_name(src_pin).c_str(), netlist_.pin_name(sink_pin).c_str());
                return edge;
            }
        }
    }
    return tatum::EdgeId::INVALID();
}

void TimingGraphBuilder::remap_ids(const tatum::GraphIdMaps& id_mapping) {
    //Update the pin-tnode mapping
    vtr::linear_map<tatum::NodeId,AtomPinId> new_tnode_atom_pin;
    for(auto kv : netlist_lookup_.tnode_atom_pins()) {
        tatum::NodeId old_tnode = kv.first;
        AtomPinId pin = kv.second;
        tatum::NodeId new_tnode = id_mapping.node_id_map[old_tnode];

        new_tnode_atom_pin.emplace(new_tnode, pin);
    }

    for(auto kv : new_tnode_atom_pin) {
        tatum::NodeId tnode = kv.first;
        AtomPinId pin = kv.second;
        netlist_lookup_.set_atom_pin_tnode(pin, tnode);
    }
}
