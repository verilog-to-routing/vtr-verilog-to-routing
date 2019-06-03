#include <set>

#include "vtr_log.h"
#include "vtr_linear_map.h"

#include "timing_graph_builder.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "atom_netlist.h"
#include "atom_netlist_utils.h"

#include "tatum/TimingGraph.hpp"

using tatum::EdgeId;
using tatum::NodeId;
using tatum::NodeType;
using tatum::TimingGraph;

template<class K, class V>
tatum::util::linear_map<K, V> remap_valid(const tatum::util::linear_map<K, V>& data, const tatum::util::linear_map<K, K>& id_map) {
    tatum::util::linear_map<K, V> new_data;

    for (size_t i = 0; i < data.size(); ++i) {
        tatum::EdgeId old_edge(i);
        tatum::EdgeId new_edge = id_map[old_edge];

        if (new_edge) {
            new_data.insert(new_edge, data[old_edge]);
        }
    }

    return new_data;
}

TimingGraphBuilder::TimingGraphBuilder(const AtomNetlist& netlist,
                                       AtomLookup& netlist_lookup)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup)
    , netlist_clock_drivers_(find_netlist_logical_clock_drivers(netlist_)) {
    //pass
}

std::unique_ptr<TimingGraph> TimingGraphBuilder::timing_graph() {
    build();
    opt_memory_layout();

    VTR_ASSERT(tg_);

    tg_->validate();
    return std::move(tg_);
}

void TimingGraphBuilder::build() {
    tg_ = std::make_unique<tatum::TimingGraph>();

    for (AtomBlockId blk : netlist_.blocks()) {
        AtomBlockType blk_type = netlist_.block_type(blk);

        if (blk_type == AtomBlockType::INPAD || blk_type == AtomBlockType::OUTPAD) {
            add_io_to_timing_graph(blk);
        } else if (blk_type == AtomBlockType::BLOCK) {
            add_block_to_timing_graph(blk);
        } else {
            VPR_THROW(VPR_ERROR_TIMING, "Unrecognized atom block type while constructing timing graph");
        }
    }

    for (AtomNetId net : netlist_.nets()) {
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
    if (netlist_.block_type(blk) == AtomBlockType::INPAD) {
        node_type = NodeType::SOURCE;

        if (netlist_.block_pins(blk).size() == 1) {
            pin = *netlist_.block_pins(blk).begin();
        } else {
            //Un-swept disconnected input
            VTR_ASSERT(netlist_.block_pins(blk).size() == 0);
            return;
        }

    } else {
        VTR_ASSERT(netlist_.block_type(blk) == AtomBlockType::OUTPAD);
        node_type = NodeType::SINK;

        if (netlist_.block_pins(blk).size() == 1) {
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
    std::set<std::string> output_ports_used_as_combinational_sinks;

    //Create the input pins
    for (AtomPinId input_pin : netlist_.block_input_pins(blk)) {
        AtomPortId input_port = netlist_.pin_port(input_pin);

        //Inspect the port model to determine pin type
        const t_model_ports* model_port = netlist_.port_model(input_port);

        NodeId tnode;
        VTR_ASSERT(!model_port->is_clock);
        if (model_port->clock.empty()) {
            //No clock => combinational input
            tnode = tg_->add_node(NodeType::IPIN);

            //A combinational pin is really both internal and external, mark it internal here
            //and external in the default case below
            netlist_lookup_.set_atom_pin_tnode(input_pin, tnode, BlockTnode::INTERNAL);
        } else {
            tnode = tg_->add_node(NodeType::SINK);

            if (!model_port->combinational_sink_ports.empty()) {
                //There is an internal combinational connection starting at this sequential input

                //Create the internal source
                NodeId internal_tnode = tg_->add_node(NodeType::SOURCE);
                netlist_lookup_.set_atom_pin_tnode(input_pin, internal_tnode, BlockTnode::INTERNAL);
            }
        }

        //Record any output port which will be used as a combinational sink
        output_ports_used_as_combinational_sinks.insert(model_port->combinational_sink_ports.begin(),
                                                        model_port->combinational_sink_ports.end());

        //Save the pin to external tnode mapping
        netlist_lookup_.set_atom_pin_tnode(input_pin, tnode);
    }

    //Create the clock pins
    for (AtomPinId clock_pin : netlist_.block_clock_pins(blk)) {
        AtomPortId clock_port = netlist_.pin_port(clock_pin);

        //Inspect the port model to determine pin type
        const t_model_ports* model_port = netlist_.port_model(clock_port);

        VTR_ASSERT(model_port->is_clock);
        VTR_ASSERT(model_port->clock.empty());

        NodeId tnode = tg_->add_node(NodeType::CPIN);

        netlist_lookup_.set_atom_pin_tnode(clock_pin, tnode);
    }

    //Create the output pins
    std::set<tatum::NodeId> clock_generator_tnodes;
    for (AtomPinId output_pin : netlist_.block_output_pins(blk)) {
        AtomPortId output_port = netlist_.pin_port(output_pin);

        //Inspect the port model to determine pin type
        const t_model_ports* model_port = netlist_.port_model(output_port);

        NodeId tnode;
        if (is_netlist_clock_source(output_pin)) {
            //A generated clock source
            tnode = tg_->add_node(NodeType::SOURCE);

            clock_generator_tnodes.insert(tnode);

            if (!model_port->is_clock) {
                //An implicit clock source, possibly clock derived from data

                AtomNetId clock_net = netlist_.pin_net(output_pin);
                VTR_LOG_WARN("Inferred implicit clock source %s for netlist clock %s (possibly data used as clock)\n",
                             netlist_.pin_name(output_pin).c_str(), netlist_.net_name(clock_net).c_str());

                //This type of situation often requires cutting paths between the implicit clock source and
                //it's inputs which can cause dangling combinational nodes. Do not error if this occurs.
                tg_->set_allow_dangling_combinational_nodes(true);
            }
        } else {
            VTR_ASSERT_MSG(!model_port->is_clock, "Primitive data output can not be a clock generator");

            if (model_port->clock.empty()) {
                //No clock => combinational output
                tnode = tg_->add_node(NodeType::OPIN);

                //A combinational pin is really both internal and external, mark it internal here
                //and external in the default case below
                netlist_lookup_.set_atom_pin_tnode(output_pin, tnode, BlockTnode::INTERNAL);

            } else {
                VTR_ASSERT(!model_port->clock.empty());
                //Has an associated clock => sequential output
                tnode = tg_->add_node(NodeType::SOURCE);

                if (output_ports_used_as_combinational_sinks.count(model_port->name)) {
                    //There is a combinational path within the primitive terminating at this sequential output

                    //Create the internal sink node
                    NodeId internal_tnode = tg_->add_node(NodeType::SINK);
                    netlist_lookup_.set_atom_pin_tnode(output_pin, internal_tnode, BlockTnode::INTERNAL);
                }
            }
        }

        netlist_lookup_.set_atom_pin_tnode(output_pin, tnode);
    }

    //Connect the clock pins to the sources and sinks
    for (AtomPinId pin : netlist_.block_pins(blk)) {
        for (auto blk_tnode_type : {BlockTnode::EXTERNAL, BlockTnode::INTERNAL}) {
            NodeId tnode = netlist_lookup_.atom_pin_tnode(pin, blk_tnode_type);
            if (!tnode) continue;

            if (clock_generator_tnodes.count(tnode)) continue; //Clock sources don't have incomming clock pin connections

            auto node_type = tg_->node_type(tnode);

            if (node_type == NodeType::SOURCE || node_type == NodeType::SINK) {
                //Look-up the clock name on the port model
                AtomPortId port = netlist_.pin_port(pin);
                const t_model_ports* model_port = netlist_.port_model(port);

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
                if (node_type == NodeType::SOURCE) {
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

    //Connect the combinational edges
    for (AtomPinId src_pin : netlist_.block_input_pins(blk)) {
        //Combinational edges go between IPINs and OPINs for combinational blocks
        //and between the internal SOURCEs and SINKS for sequential blocks
        //
        //We have already marked all internal SOURCE/SINK nodes internal, and all IPIN/OPIN nodes as internal
        //so we only need to look at tnode's of that class
        NodeId src_tnode = netlist_lookup_.atom_pin_tnode(src_pin, BlockTnode::INTERNAL);

        if (src_tnode) {
            auto src_type = tg_->node_type(src_tnode);

            //Look-up the combinationally connected sink ports name on the port model
            AtomPortId src_port = netlist_.pin_port(src_pin);
            const t_model_ports* model_port = netlist_.port_model(src_port);

            for (const std::string& sink_port_name : model_port->combinational_sink_ports) {
                AtomPortId sink_port = netlist_.find_port(blk, sink_port_name);
                if (!sink_port) continue; //Port may not be connected

                //We now need to create edges between the source pin, and all the pins in the
                //output port
                for (AtomPinId sink_pin : netlist_.port_pins(sink_port)) {
                    //Get the tnode of the sink
                    NodeId sink_tnode = netlist_lookup_.atom_pin_tnode(sink_pin, BlockTnode::INTERNAL);

                    if (!sink_tnode) {
                        //No tnode found, either a combinational clock generator or an error

                        //Try again looking for an external tnode
                        sink_tnode = netlist_lookup_.atom_pin_tnode(sink_pin, BlockTnode::EXTERNAL);

                        //Is the sink a clock generator?
                        if (sink_tnode && clock_generator_tnodes.count(sink_tnode)) {
                            //Do not create the edge
                            VTR_LOG_WARN("Timing edge from %s to %s will not be created since %s has been identified as a clock generator\n",
                                         netlist_.pin_name(src_pin).c_str(), netlist_.pin_name(sink_pin).c_str(), netlist_.pin_name(sink_pin).c_str());
                        } else {
                            //Unknown
                            VPR_THROW(VPR_ERROR_TIMING, "Unable to find matching sink tnode for timing edge from %s to %s",
                                      netlist_.pin_name(src_pin).c_str(), netlist_.pin_name(src_pin).c_str());
                        }

                    } else {
                        //Valid tnode create the edge
                        auto sink_type = tg_->node_type(sink_tnode);

                        VTR_ASSERT_MSG((src_type == NodeType::IPIN && sink_type == NodeType::OPIN)
                                           || (src_type == NodeType::SOURCE && sink_type == NodeType::SINK)
                                           || (src_type == NodeType::SOURCE && sink_type == NodeType::OPIN)
                                           || (src_type == NodeType::IPIN && sink_type == NodeType::SINK),
                                       "Internal primitive combinational edges must be between {IPIN, SOURCE} and {OPIN, SINK}");

                        //Add the edge between the pins
                        tg_->add_edge(tatum::EdgeType::PRIMITIVE_COMBINATIONAL, src_tnode, sink_tnode);
                    }
                }
            }
        }
    }
}

void TimingGraphBuilder::add_net_to_timing_graph(const AtomNetId net) {
    //Create edges from the driver to sink tnodes

    AtomPinId driver_pin = netlist_.net_driver(net);

    if (!driver_pin) {
        //Undriven nets have no timing dependencies, and hence no edges
        VTR_LOG_WARN("Net %s has no driver and will be ignored for timing purposes\n", netlist_.net_name(net).c_str());
        return;
    }

    NodeId driver_tnode = netlist_lookup_.atom_pin_tnode(driver_pin);
    VTR_ASSERT(driver_tnode);

    for (AtomPinId sink_pin : netlist_.net_sinks(net)) {
        NodeId sink_tnode = netlist_lookup_.atom_pin_tnode(sink_pin);
        VTR_ASSERT(sink_tnode);

        tg_->add_edge(tatum::EdgeType::INTERCONNECT, driver_tnode, sink_tnode);
    }
}

void TimingGraphBuilder::fix_comb_loops() {
    auto sccs = tatum::identify_combinational_loops(*tg_);

    //For non-simple loops (i.e. SCCs with multiple loops) we may need to break
    //multiple edges, so repeatedly break edges until there are no SCCs left
    while (!sccs.empty()) {
        VTR_LOG_WARN("Detected %zu strongly connected component(s) forming combinational loop(s) in timing graph\n", sccs.size());
        for (const auto& scc : sccs) {
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
    for (tatum::NodeId src_node : scc) {
        AtomPinId src_pin = netlist_lookup_.tnode_atom_pin(src_node);
        for (tatum::EdgeId edge : tg_->node_out_edges(src_node)) {
            if (tg_->edge_disabled(edge)) continue;

            tatum::NodeId sink_node = tg_->edge_sink_node(edge);
            AtomPinId sink_pin = netlist_lookup_.tnode_atom_pin(sink_node);

            if (scc_set.count(sink_node)) {
                VTR_LOG_WARN("Arbitrarily disabling timing graph edge %zu (%s -> %s) to break combinational loop\n",
                             edge, netlist_.pin_name(src_pin).c_str(), netlist_.pin_name(sink_pin).c_str());
                return edge;
            }
        }
    }
    return tatum::EdgeId::INVALID();
}

void TimingGraphBuilder::remap_ids(const tatum::GraphIdMaps& id_mapping) {
    //Update the pin-tnode mapping
    vtr::linear_map<tatum::NodeId, AtomPinId> new_tnode_atom_pin;
    for (auto kv : netlist_lookup_.tnode_atom_pins()) {
        tatum::NodeId old_tnode = kv.first;
        AtomPinId pin = kv.second;
        tatum::NodeId new_tnode = id_mapping.node_id_map[old_tnode];

        new_tnode_atom_pin.emplace(new_tnode, pin);
    }

    for (auto kv : new_tnode_atom_pin) {
        tatum::NodeId tnode = kv.first;
        AtomPinId pin = kv.second;
        netlist_lookup_.set_atom_pin_tnode(pin, tnode);
    }
}

bool TimingGraphBuilder::is_netlist_clock_source(const AtomPinId pin) const {
    return netlist_clock_drivers_.count(pin);
}
