/*
 * Timing Graph Builder
 *
 * This file implements the TimingGraphBuilder class which constructs the timing graph
 * from the provided AtomNetlist.
 *
 * The Timing Graph is a directed acyclic graph (DAG) consisting of nodes and edges:
 *    - Nodes: represent netlist pins (tatum::IPIN, tatum::OPIN) and logical sources/
 *      sinks like primary inputs/outpus, flip-flops and clock generators (tatum::SOURCE,
 *      tatum::SINK). 
 *
 *      Note that tatum::SOURCE/tatum::SINK represent the start/end of
 *      a timing path. As a result tatum::SOURCE's hould never have input edges (except
 *      perhaps from a tatum::CPIN if it is a sequential source), and tatum::SINKS's 
 *      should never have output edges.
 *
 *    - Edges: represent the timing dependences between nodes in the timing graph. 
 *      These correspond to net connections, combinational paths inside primitives, 
 *      clock-to-q, and setup/hold constraints for sequential elements.
 *
 * The timing graph is constructed by first creating timing sub-graphs corresponding to 
 * each primitive block in the AtomNetlist. Each sub-graph contains the timing graph 
 * nodes required to represent the primitive, and all internal timing graph edges (i.e.
 * those completely contained within the primitive).
 *
 * Next timing graph edges corresponding to nets in the netlist (i.e. connections between 
 * netlist primitives) are created to "stitch" the sub-graph together. This results in a
 * timing graph corresponding to the netlist.
 *
 *
 * Modelling Primitive Combinational and Sequential Logic as a Timing Graph
 * -----------------------------------------------------------------------
 *
 * Consider the architectural primitive block below, which contains two 
 * sequential elements 'A' and 'B' (controlled by the primitive input 
 * pin 'clk'), and two clouds of combinational logic 'C' and 'D'.
 *
 * The combinational logic 'D' is driven by primtive input 'e' and 
 * drives primitive output pin 'g'.
 *
 * The combinational logic 'C' is driven by the sequential element 'A' and
 * primitive input pin 'e'; it then drives the input of the sequential 
 * element 'B'.
 *
 * Sequential element 'A' is driven by primitive input 'f', and sequential 
 * element 'B' drives primitive output pin 'h'.
 *
 *           +---------------------------------------+
 *           |                                       |
 *           |                         ---           |
 *           |                        /   \          |
 *     e --->-----------------+----->|  D  |-------->|--> g
 *           |                |       \   /          |   
 *           |                |        ---           |
 *           |                v         ^            |
 *           |               ---        |            |
 *           |   +---+      /   \       |    +---+   |
 *     f --->|-->| A |---->|  C  |------+--->| B |-->|--> h
 *           |   |   |      \   /            |   |   |
 *           |   |   |       ---             |   |   |
 *           |   +-^-+                       +-^-+   |
 *           |     |                           |     |
 *           |     |                           |     |
 *   clk --->|---------------------------------+     |
 *           |                                       |
 *           +---------------------------------------+
 *
 *  As a result there are the following "timing sub-paths" within this primitive:
 *
 *  1) 'e' -> 'D' -> 'g'            (combinational propogation delay)
 *  2) 'e' -> 'C' -> 'B'            (combinational propogation delay + sequentialsetup/hold check)
 *  3) 'f' -> 'A'                   (sequential setup/hold check)
 *  4) 'A' -> 'C' -> 'D' -> 'g'     (sequential clock-to-q + combinational propogataion delay)
 *  5) 'B' -> 'h'                   (sequential clock-to-q)
 *  6) 'clk' -> 'A'                 (sequential clock arrival)
 *  7) 'clk' -> 'B'                 (sequential clock arrival)
 *
 *  and one fully contained timing path:
 *
 *  8) 'A' -> 'C' -> 'B' (clock-to-q + combinational propogation delay
 *                       + sequential setup/hold check)
 *
 * which all must be modelled by the timing graph:
 *
 * Modelling combinational logic is simple, and only requires that there is 
 * a timing graph edge (tedge) between the corresponding timing graph nodes 
 * (STA only considers the topological structure of the netlist, not it's 
 * logic functionality).
 *
 * For instance, to model the combinational logic 'D', we use two tnodes as
 * follows:
 *
 *              +--------+        +--------+
 *          --->| IPIN e |------->| OPIN g |--->
 *              +--------+        +--------+
 *
 * where 'IPIN e' and 'OPIN g' respectively corresponds to the primitive 
 * input 'e' and output 'g' and the edge between them the timing dependency
 * through the combinational logic 'D'.
 *
 *
 * To model a sequential element like 'B', we use three timing graph 
 * nodes as follows:
 *
 *              +--------+        +--------+
 *          --->| SINK B |        | SRC B  |--->
 *              +--------+        +--------+
 *                   ^                ^
 *                   |                |
 *                   +-----+    +-----+
 *                         |    |
 *                       +--------+
 *                   --->| CPIN   |
 *                       +--------+
 *
 * Where 'SINK B' represents the data input of sequential element 'B' (e.g. 
 * Flip-Flop D pin), 'SRC B' the data output (SOURCE) of sequential element 
 * 'B' (e.g. Flip-Flop Q pin), and 'CPIN' the clock input pin of sequential 
 * element 'B'.
 *
 *
 * Following the above transformations, we arrive at the following timing graph 
 * structure, which corresponds to the architectural primitive described above:
 *
 *       +--------+                                             +--------+
 *   --->| IPIN e |-------------------------------------------->| OPIN g |--->
 *       +--------+                                             +--------+
 *            |                                                     ^
 *            |                                                     |
 *            |                +------------------------------------+
 *            |                |
 *            |                |
 *            +----------------)-------------------+
 *                             |                   |
 *                             |                   v
 *       +--------+        +--------+         +--------+        +--------+
 *   --->| SINK A |        | SRC A  |-------->| SINK B |        | SRC B  |--->
 *       +--------+        +--------+         +--------+        +--------+
 *            ^                ^                   ^                ^
 *            |                |                   |                |
 *         +--+                |                   |                |
 *         |      +------------+                   |                |
 *         |      |                                |                |
 *       +----------+                              |                |
 *       |          |------------------------------+                |
 *   --->| CPIN clk |                                               |
 *       |          |-----------------------------------------------+
 *       +----------+                        
 *
 * Note that we have also used only a single CPIN for both 'A' and 'B' (since 
 * they are controlled by the same clock).
 *
 *
 *
 * Building the Timing Graph From VPR's Data Structures
 * ----------------------------------------------------
 *
 * VPR does not directly model the intenals of netlist primitives (e.g. internal
 * sequential elements like 'A' or 'B' above). Instead, various attributes are 
 * tagged on the pins of the primitive which indicate:
 *     - whether a pin is sequential, combinational or a clock
 *     - whether the pin is combinationally connected to another pin 
 *       within the primitive.
 * 
 * Mostly there is a one-to-one correspondance between netlist pins and tnodes,
 * the only exception is for sequential-sequential connections within a primtive
 * (e.g. the fully internal 'A' to 'B' timing path above).
 *
 * As a result we make a distinction between tnodes which are strictly "internal"
 * to a primitive, and those which are "external" (i.e. connect to tnodes outside
 * the primitive). Note that this distinction is just a labelling which is only 
 * relevant to how VPR tracks the relation between tnodes and netlist pins. It 
 * has no effect on the timing analysis result (which only depends on the 
 * structure of the timing graph). Most of VPR doesn't care about the internal 
 * timing paths, as they are typically uneffected by any of VPR's optimization
 * choices. Therefore most of VPR only considers "external" tnodes when mapping 
 * between netlist pins and tnodes. Of course we record both so the parts of VPR
 * which *do* care about them (e.g. the code here, and in the delay calculator)
 * can figure out the correct mapping.
 *
 * As a result the timing graph we build (for the primitive example above) is 
 * described within VPR as:
 *
 *       +------------+                                            +------------+
 *   --->|   IPIN e   |------------------------------------------->|   OPIN g   |--->
 *       | (external/ |                                            | (external/ |
 *       |  internal) |                                            |  internal) |
 *       +------------+                                            +------------+
 *            |                                                           ^
 *            |                                                           |
 *            |                   +---------------------------------------+
 *            |                   |
 *            |                   |
 *            +-------------------)--------------------+
 *                                |                    |
 *                                |                    v
 *       +------------+     +------------+       +------------+    +------------+
 *   --->|   SINK f   |     |   SRC f    |------>|   SINK h   |    |   SRC h    |--->
 *       | (external) |     | (internal) |       | (internal) |    | (external) |
 *       +------------+     +------------+       +------------+    +------------+
 *             ^                  ^                    ^                 ^
 *             |                  |                    |                 |
 *         +---+                  |                    |                 |
 *         |        +-------------+                    |                 |
 *         |        |                                  |                 |
 *       +------------+                                |                 |
 *       |  CPIN clk  |--------------------------------+                 |
 *   --->|            |                                                  |
 *       | (external) |--------------------------------------------------+
 *       +------------+                        
 *
 * Where each pin in the netlist corresponds to an "external" tnode, but the 
 * sequential netlist pins 'f' and 'h' have additional "internal" tnodes 
 * corresponding to their respective data outputs/inputs within the primitive.
 * Also note that combinational pins are makred as both internal and external 
 * for convenience (i.e. both map to the same tnode).
 *
 */
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

std::unique_ptr<TimingGraph> TimingGraphBuilder::timing_graph(bool allow_dangling_combinational_nodes) {
    build(allow_dangling_combinational_nodes);
    opt_memory_layout();

    VTR_ASSERT(tg_);

    tg_->validate();
    validate_netlist_timing_graph_consistency();

    return std::move(tg_);
}

//Constructs the timing graph from the netlist
void TimingGraphBuilder::build(bool allow_dangling_combinational_nodes) {
    tg_ = std::make_unique<tatum::TimingGraph>();

    // Optionally allow dangling combinational nodes.
    // Set by `--allow_dangling_combinational_nodes on`. Default value is false
    tg_->set_allow_dangling_combinational_nodes(allow_dangling_combinational_nodes);

    //Walk through the netlist blocks creating the timing sub-graphs corresponding to
    //each block (i.e. the timing nodes and internal edges of the block)
    //
    //Note that this does not add timing graph edges which are external to each block.
    for (AtomBlockId blk : netlist_.blocks()) {
        AtomBlockType blk_type = netlist_.block_type(blk);

        if (blk_type == AtomBlockType::INPAD || blk_type == AtomBlockType::OUTPAD) {
            add_io_to_timing_graph(blk);
        } else if (blk_type == AtomBlockType::BLOCK) {
            add_block_to_timing_graph(blk);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_TIMING, "Unrecognized atom block type while constructing timing graph");
        }
    }

    //Walk through the netlist nets adding the edges representing each net to
    //the timiing graph. This connects the timing graph nodes of each netlist
    //block together.
    for (AtomNetId net : netlist_.nets()) {
        add_net_to_timing_graph(net);
    }

    //Break any combinational loops (i.e. if the graph is not a DAG)
    fix_comb_loops();

    //Levelize the graph (i.e. determine its topological ordering and record
    //the level of each node) for the timing analyzer
    tg_->levelize();
}

//Re-order the timing graph to be ordered in a traversal friendly manner
void TimingGraphBuilder::opt_memory_layout() {
    //For more details see the comments in tatum/TimingGraph.cpp, and the
    //discussion in Section III-A of K. E. Murray et al., "Tatum: Parallel
    //Timing Analysis for Faster Design Cycles and Improved Optimization",
    //FPT 2018
    auto id_map = tg_->optimize_layout();

    //Now that tatum has re-ordered the graph to be more cache friendly,
    //the original tatum NodeId's stored in VPR are no longer valid.
    //We must now update the node reference to reflect the changes.
    remap_ids(id_map);
}

//Creates the timing graph nodes for the associated primary I/O
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

    netlist_lookup_.set_atom_pin_tnode(pin, tnode, BlockTnode::EXTERNAL);
}

//Creates the timing graph nodes and internal edges for a netlist block
void TimingGraphBuilder::add_block_to_timing_graph(const AtomBlockId blk) {
    /*
     * How the code builds the primtive timing sub-graph
     * -------------------------------------------------
     *
     * The code below builds the timing sub-graph corresponding corresponding to the
     * current netlist primitive/block. This is accomplished by walking through
     * the primitive's input, clock and output pins and creating the corresponding
     * tnodes (note that if internal sequentail paths exist within the primitive 
     * this also creates the appropriate internal tnodes).
     *
     * Once all nodes have been created the edges are added between them according
     * to what was specified in the architecture file models.
     *
     * Note that to minimize the size of the timing graph we only create tnodes and 
     * edges where they actually exist within the netlist. This means we do not create 
     * tnodes or tedges to/from pins which are disconnected in the netlist (even if 
     * they exist in the archtiecture).
     *
     *
     * Clock Generators
     * ----------------
     * An additional wrinkle in the above process is the presense of clock generators,
     * such as PLLs, which may define new clocks at their output (in contrast with a
     * primary input which is always a SOURCE type tnode).
     *
     * If a primitive output pin is marked as a clock then we create a SOURCE
     * type node instead of the regular OPIN.
     *
     * Additionally, in some instances designs use 'data' signals as clocks (usually
     * bad practise, but it happens). We detect such cases and also create a
     * SOURCE (and leave any combinational inputs to that node disconnected).
     */

    auto clock_generator_tnodes = create_block_timing_nodes(blk);
    create_block_internal_data_timing_edges(blk, clock_generator_tnodes);
    create_block_internal_clock_timing_edges(blk, clock_generator_tnodes);
}

//Constructs the timing graph nodes for the specified block
//
//Returns the set of created tnodese which are clock generators
std::set<tatum::NodeId> TimingGraphBuilder::create_block_timing_nodes(const AtomBlockId blk) {
    std::set<std::string> output_ports_used_as_combinational_sinks;

    //Create the tnodes corresponding to input pins
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
            //This is a sequential data input (i.e. a sequential data capture point/timing path end-point)
            tnode = tg_->add_node(NodeType::SINK);

            if (!model_port->combinational_sink_ports.empty()) {
                //There is an internal combinational connection starting at this sequential input
                //pin. This is a new timing path and hence we must create a new SOURCE node.

                //Create the internal source
                NodeId internal_tnode = tg_->add_node(NodeType::SOURCE);
                netlist_lookup_.set_atom_pin_tnode(input_pin, internal_tnode, BlockTnode::INTERNAL);
            }
        }

        //Record any output port which will be used as a combinational sink
        // This is used to determine (when creating output pin tnodes) whether we also need to
        // create an internal SINK tnode.
        output_ports_used_as_combinational_sinks.insert(model_port->combinational_sink_ports.begin(),
                                                        model_port->combinational_sink_ports.end());

        //Save the pin to external tnode mapping
        netlist_lookup_.set_atom_pin_tnode(input_pin, tnode, BlockTnode::EXTERNAL);
    }

    //Create the clock pins
    for (AtomPinId clock_pin : netlist_.block_clock_pins(blk)) {
        AtomPortId clock_port = netlist_.pin_port(clock_pin);

        //Inspect the port model to determine pin type
        const t_model_ports* model_port = netlist_.port_model(clock_port);

        VTR_ASSERT(model_port->is_clock);
        VTR_ASSERT(model_port->clock.empty());

        NodeId tnode = tg_->add_node(NodeType::CPIN);

        netlist_lookup_.set_atom_pin_tnode(clock_pin, tnode, BlockTnode::EXTERNAL);
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
            VTR_ASSERT_MSG(!model_port->is_clock, "Primitive data output (i.e. non-clock source output pin) should not be marked as a clock generator");

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

        //Record as external tnode
        netlist_lookup_.set_atom_pin_tnode(output_pin, tnode, BlockTnode::EXTERNAL);
    }

    return clock_generator_tnodes;
}

void TimingGraphBuilder::create_block_internal_clock_timing_edges(const AtomBlockId blk, const std::set<tatum::NodeId>& clock_generator_tnodes) {
    //Connect the clock pins to the sources and sinks
    for (AtomPinId pin : netlist_.block_pins(blk)) {
        for (auto blk_tnode_type : {BlockTnode::EXTERNAL, BlockTnode::INTERNAL}) {
            NodeId tnode = netlist_lookup_.atom_pin_tnode(pin, blk_tnode_type);
            if (!tnode) continue;

            if (clock_generator_tnodes.count(tnode)) continue; //Clock sources don't have incoming clock pin connections

            auto node_type = tg_->node_type(tnode);

            if (node_type != NodeType::SOURCE && node_type != NodeType::SINK) continue;

            VTR_ASSERT_SAFE(node_type == NodeType::SOURCE || node_type == NodeType::SINK);

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
            VTR_ASSERT(clk_tnode);

            //Determine the type of edge to create
            //This corresponds to how the clock (clk_tnode) relates
            //to the data (tnode). So a SOURCE data tnode should be
            //a PRIMTIVE_CLOCK_LAUNCH (clock launches data), while
            //a SINK data tnode should be a PRIMITIVE_CLOCK_CAPTURE
            //(clock captures data).
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

    //Connect the combinational edges from clock input pins
    //
    //These are typically used to represent clock buffers
    for (AtomPinId src_clock_pin : netlist_.block_clock_pins(blk)) {
        NodeId src_tnode = netlist_lookup_.atom_pin_tnode(src_clock_pin, BlockTnode::EXTERNAL);

        if (!src_tnode) continue;

        //Look-up the combinationally connected sink ports name on the port model
        AtomPortId src_port = netlist_.pin_port(src_clock_pin);
        const t_model_ports* model_port = netlist_.port_model(src_port);

        for (const std::string& sink_port_name : model_port->combinational_sink_ports) {
            AtomPortId sink_port = netlist_.find_port(blk, sink_port_name);
            if (!sink_port) continue; //Port may not be connected

            //We now need to create edges between the source pin, and all the pins in the
            //output port
            for (AtomPinId sink_pin : netlist_.port_pins(sink_port)) {
                //Get the tnode of the sink
                NodeId sink_tnode = netlist_lookup_.atom_pin_tnode(sink_pin, BlockTnode::EXTERNAL);

                tg_->add_edge(tatum::EdgeType::PRIMITIVE_COMBINATIONAL, src_tnode, sink_tnode);
                VTR_LOG("Adding edge from '%s' (tnode: %zu) -> '%s' (tnode: %zu) to allow clocks to propagate\n", netlist_.pin_name(src_clock_pin).c_str(), size_t(src_tnode), netlist_.pin_name(sink_pin).c_str(), size_t(sink_tnode));
            }
        }
    }
}

void TimingGraphBuilder::create_block_internal_data_timing_edges(const AtomBlockId blk, const std::set<tatum::NodeId>& clock_generator_tnodes) {
    //Connect the combinational edges from data input pins
    //
    //These edges may represent an intermediate (combinational) sub-path of a
    //timing path (i.e. between IPINs and OPINs), the start of a timing path (i.e. SOURCE
    //to OPIN), the end of a timing path (i.e. IPIN to SINK), or an internal timing path
    //(i.e. SOURCE to SINK).
    //
    //Note that the creation of these edges is driven by the 'combinationl_sink_ports' specified
    //in the architecture primitive model
    for (AtomPinId src_pin : netlist_.block_input_pins(blk)) {
        //Note that we have already created all the relevant nodes, and appropriately labelled them as
        //internal/external. As a result, we only need to consider the 'internal' tnodes when creating
        //the edges within the current block.
        NodeId src_tnode = netlist_lookup_.atom_pin_tnode(src_pin, BlockTnode::INTERNAL);

        if (!src_tnode) continue;

        auto src_type = tg_->node_type(src_tnode);

        //Look-up the combinationally connected sink ports name on the port model
        AtomPortId src_port = netlist_.pin_port(src_pin);
        const t_model_ports* model_port = netlist_.port_model(src_port);
        VTR_ASSERT_SAFE(model_port != nullptr);

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
                        VPR_FATAL_ERROR(VPR_ERROR_TIMING, "Unable to find matching sink tnode for timing edge from %s to %s",
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
    //Loops correspond to Strongly Connected Components (SCCs) in the graph
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
    for (BlockTnode type : {BlockTnode::EXTERNAL, BlockTnode::INTERNAL}) {
        for (auto kv : netlist_lookup_.atom_pin_tnodes(type)) {
            AtomPinId pin = kv.first;
            tatum::NodeId old_tnode = kv.second;

            if (!old_tnode) continue;

            tatum::NodeId new_tnode = id_mapping.node_id_map[old_tnode];

            netlist_lookup_.set_atom_pin_tnode(pin, new_tnode, type);
        }
    }
}

bool TimingGraphBuilder::is_netlist_clock_source(const AtomPinId pin) const {
    return netlist_clock_drivers_.count(pin);
}

bool TimingGraphBuilder::validate_netlist_timing_graph_consistency() const {
    for (AtomPinId pin : netlist_.pins()) {
        tatum::NodeId ext_tnode = netlist_lookup_.atom_pin_tnode(pin, BlockTnode::EXTERNAL);
        if (!ext_tnode) VPR_ERROR(VPR_ERROR_TIMING, "Found no external tnode for atom pin '%zu'", size_t(pin));

        tatum::NodeId int_tnode = netlist_lookup_.atom_pin_tnode(pin, BlockTnode::INTERNAL);

        /*
         * Sanity check look-up consistency
         */
        AtomPinId ext_tnode_pin = netlist_lookup_.tnode_atom_pin(ext_tnode);
        if (ext_tnode_pin != pin) {
            VPR_ERROR(VPR_ERROR_TIMING, "Inconsistent external tnode -> atom pin lookup: atom pin %zu -> tnode %zu, but tnode %zu -> atom pin %zu",
                      size_t(pin), size_t(ext_tnode), size_t(ext_tnode), size_t(ext_tnode_pin));
        }
        if (int_tnode) {
            AtomPinId int_tnode_pin = netlist_lookup_.tnode_atom_pin(int_tnode);
            if (int_tnode_pin != pin) {
                VPR_ERROR(VPR_ERROR_TIMING, "Inconsistent internal tnode -> atom pin lookup: atom pin %zu -> tnode %zu, but tnode %zu -> atom pin %zu",
                          size_t(pin), size_t(int_tnode), size_t(int_tnode), size_t(int_tnode_pin));
            }
        }

        /*
         * Sanity check internal/external tnode types
         */
        tatum::NodeType ext_tnode_type = tg_->node_type(ext_tnode);
        if (ext_tnode_type == tatum::NodeType::IPIN || ext_tnode_type == tatum::NodeType::OPIN) {
            if (!int_tnode) VPR_ERROR(VPR_ERROR_TIMING, "Missing expected internal tnode for combinational atom pin %zu", size_t(pin));
            if (int_tnode != ext_tnode) VPR_ERROR(VPR_ERROR_TIMING, "Mismatch  external/internal tnodes (%zu vs %zu) for combinational atom pin %zu",
                                                  size_t(ext_tnode), size_t(int_tnode), size_t(pin));
        } else if (ext_tnode_type == tatum::NodeType::CPIN) {
            if (int_tnode) VPR_ERROR(VPR_ERROR_TIMING, "Unexpected internal tnode (%zu) for clock pin: atom pin %zu ,external tnode %zu",
                                     size_t(int_tnode), size_t(pin), size_t(ext_tnode));
        } else if (ext_tnode_type == tatum::NodeType::SOURCE) {
            if (int_tnode && tg_->node_type(int_tnode) != tatum::NodeType::SINK) {
                VPR_ERROR(VPR_ERROR_TIMING, "Found internal tnode (%zu) associated with atom pin %zu, but it is not a SINK (external tnode %zu was a SOURCE)",
                          size_t(int_tnode), size_t(pin), size_t(ext_tnode));
            }

        } else if (ext_tnode_type == tatum::NodeType::SINK) {
            if (int_tnode && tg_->node_type(int_tnode) != tatum::NodeType::SOURCE) {
                VPR_ERROR(VPR_ERROR_TIMING, "Found internal tnode (%zu) associated with atom pin %zu, but it is not a SOURCE (external tnode %zu was a SINK)",
                          size_t(int_tnode), size_t(pin), size_t(ext_tnode));
            }
        } else {
            VPR_ERROR(VPR_ERROR_TIMING, "Unexpected timing node type for atom pin %zu", size_t(pin));
        }
    }

    return true; //success
}
