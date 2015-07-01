#include <ctime>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "assert.hpp"
#include "sta_util.hpp"

using std::cout;
using std::endl;

void identify_constant_gen_fanout_helper(const TimingGraph& tg, const NodeId node_id, std::set<NodeId>& const_gen_fanout_nodes);
void identify_clock_gen_fanout_helper(const TimingGraph& tg, const NodeId node_id, std::set<NodeId>& clock_gen_fanout_nodes);

float time_sec(struct timespec start, struct timespec end) {
    float time = end.tv_sec - start.tv_sec;

    time += (end.tv_nsec - start.tv_nsec) * 1e-9;
    return time;
}

void print_histogram(const std::vector<float>& values, int nbuckets) {
    nbuckets = std::min(values.size(), (size_t) nbuckets);
    int values_per_bucket = ceil((float) values.size() / nbuckets);

    std::vector<float> buckets(nbuckets);

    //Sum up each bucket
    for(size_t i = 0; i < values.size(); i++) {
        int ibucket = i / values_per_bucket;

        buckets[ibucket] += values[i];
    }

    //Normalize to get average value
    for(int i = 0; i < nbuckets; i++) {
        buckets[i] /= values_per_bucket;
    }

    float max_bucket_val = *std::max_element(buckets.begin(), buckets.end());

    //Print the histogram
    std::ios_base::fmtflags saved_flags = cout.flags();
    std::streamsize prec = cout.precision();
    std::streamsize width = cout.width();

    std::streamsize int_width = ceil(log10(values.size()));
    std::streamsize float_prec = 1;

    int histo_char_width = 60;

    //cout << "\t" << endl;
    for(int i = 0; i < nbuckets; i++) {
        cout << std::setw(int_width) << i*values_per_bucket << ":" << std::setw(int_width) << (i+1)*values_per_bucket - 1;
        cout << " " <<  std::scientific << std::setprecision(float_prec) << buckets[i];
        cout << " ";

        for(int j = 0; j < histo_char_width*(buckets[i]/max_bucket_val); j++) {
            cout << "*";
        }
        cout << endl;
    }

    cout.flags(saved_flags);
    cout.precision(prec);
    cout.width(width);
}

float relative_error(float A, float B) {
    if (A == B) {
        return 0.;
    }

    if (fabs(B) > fabs(A)) {
        return fabs((A - B) / B);
    } else {
        return fabs((A - B) / A);
    }
}

void print_level_histogram(const TimingGraph& tg, int nbuckets) {
    cout << "Levels Width Histogram" << endl;

    std::vector<float> level_widths;
    for(int i = 0; i < tg.num_levels(); i++) {
        level_widths.push_back(tg.level(i).size());
    }
    print_histogram(level_widths, nbuckets);
}

void print_node_fanin_histogram(const TimingGraph& tg, int nbuckets) {
    cout << "Node Fan-in Histogram" << endl;

    std::vector<float> fanin;
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        fanin.push_back(tg.num_node_in_edges(i));
    }

    std::sort(fanin.begin(), fanin.end(), std::greater<float>());
    print_histogram(fanin, nbuckets);
}

void print_node_fanout_histogram(const TimingGraph& tg, int nbuckets) {
    cout << "Node Fan-out Histogram" << endl;

    std::vector<float> fanout;
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        fanout.push_back(tg.num_node_out_edges(i));
    }

    std::sort(fanout.begin(), fanout.end(), std::greater<float>());
    print_histogram(fanout, nbuckets);
}


void print_timing_graph(const TimingGraph& tg) {
    for(NodeId node_id = 0; node_id < tg.num_nodes(); node_id++) {
        cout << "Node: " << node_id;
        cout << " Type: " << tg.node_type(node_id);
        cout << " Out Edges: " << tg.num_node_out_edges(node_id);
        cout << " is_clk_src: " << tg.node_is_clock_source(node_id);
        cout << endl;
        for(int out_edge_idx = 0; out_edge_idx < tg.num_node_out_edges(node_id); out_edge_idx++) {
            EdgeId edge_id = tg.node_out_edge(node_id, out_edge_idx);
            ASSERT(tg.edge_src_node(edge_id) == node_id);

            NodeId sink_node_id = tg.edge_sink_node(edge_id);

            cout << "\tEdge src node: " << node_id << " sink node: " << sink_node_id << endl;
            //<< " Delay: " << tg.edge_delay(edge_id).value() << endl;
        }
    }
}

void print_levelization(const TimingGraph& tg) {
    cout << "Num Levels: " << tg.num_levels() << endl;
    for(int ilevel = 0; ilevel < tg.num_levels(); ilevel++) {
        const auto& level = tg.level(ilevel);
        cout << "Level " << ilevel << ": " << level.size() << " nodes" << endl;
        cout << "\t";
        for(auto node_id : level) {
            cout << node_id << " ";
        }
        cout << endl;
    }
}



std::set<NodeId> identify_constant_gen_fanout(const TimingGraph& tg) {
    //Walk the timing graph and identify nodes that are in the fanout of a constant generator
    std::set<NodeId> const_gen_fanout_nodes;
    for(NodeId node_id : tg.primary_inputs()) {
        if(tg.node_type(node_id) == TN_Type::CONSTANT_GEN_SOURCE) {
            identify_constant_gen_fanout_helper(tg, node_id, const_gen_fanout_nodes);
        }
    }
    return const_gen_fanout_nodes;
}

void identify_constant_gen_fanout_helper(const TimingGraph& tg, const NodeId node_id, std::set<NodeId>& const_gen_fanout_nodes) {
    if(const_gen_fanout_nodes.count(node_id) == 0) {
        //Haven't seen this node before
        const_gen_fanout_nodes.insert(node_id);
        for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
            EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);
            identify_constant_gen_fanout_helper(tg, tg.edge_sink_node(edge_id), const_gen_fanout_nodes);
        }
    }
}

std::set<NodeId> identify_clock_gen_fanout(const TimingGraph& tg) {
    std::set<NodeId> clock_gen_fanout_nodes;
    for(NodeId node_id : tg.primary_inputs()) {
        if(tg.node_type(node_id) == TN_Type::CLOCK_SOURCE) {
            identify_clock_gen_fanout_helper(tg, node_id, clock_gen_fanout_nodes);
        }
    }
    return clock_gen_fanout_nodes;
}

void identify_clock_gen_fanout_helper(const TimingGraph& tg, const NodeId node_id, std::set<NodeId>& clock_gen_fanout_nodes) {
    if(clock_gen_fanout_nodes.count(node_id) == 0) {
        //Haven't seen this node before
        clock_gen_fanout_nodes.insert(node_id);
        for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
            EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);
            identify_clock_gen_fanout_helper(tg, tg.edge_sink_node(edge_id), clock_gen_fanout_nodes);
        }
    }

}

/*
 *void write_dot_file_setup(std::ostream& os, const TimingGraph& tg, const std::shared_ptr<TimingAnalyzer<SetupHoldAnalysis>> analyzer) {
 *    //Write out a dot file of the timing graph
 *    os << "digraph G {" << endl;
 *    os << "\tnode[shape=record]" << endl;
 *
 *    for(int inode = 0; inode < tg.num_nodes(); inode++) {
 *        os << "\tnode" << inode;
 *        os << "[label=\"";
 *        os << "{#" << inode << " (" << tg.node_type(inode) << ")";
 *        const TimingTags& data_tags = analyzer->setup_data_tags(inode);
 *        if(data_tags.num_tags() > 0) {
 *            for(const TimingTag& tag : data_tags) {
 *                os << " | {";
 *                os << "DATA - clk: " << tag.clock_domain();
 *                os << " launch: " << tag.launch_node();
 *                os << "\\n";
 *                os << " arr: " << tag.arr_time().value();
 *                os << " req: " << tag.req_time().value();
 *                os << "}";
 *            }
 *        }
 *        const TimingTags& clock_tags = analyzer->setup_clock_tags(inode);
 *        if(clock_tags.num_tags() > 0) {
 *            for(const TimingTag& tag : clock_tags) {
 *                os << " | {";
 *                os << "CLOCK - clk: " << tag.clock_domain();
 *                os << " launch: " << tag.launch_node();
 *                os << "\\n";
 *                os << " arr: " << tag.arr_time().value();
 *                os << " req: " << tag.req_time().value();
 *                os << "}";
 *            }
 *        }
 *        os << "}\"]";
 *        os << endl;
 *    }
 *
 *    //Force drawing to be levelized
 *    for(int ilevel = 0; ilevel < tg.num_levels(); ilevel++) {
 *        os << "\t{rank = same;";
 *
 *        for(NodeId node_id : tg.level(ilevel)) {
 *            os << " node" << node_id <<";";
 *        }
 *        os << "}" << endl;
 *    }
 *
 *    for(int ilevel = 0; ilevel < tg.num_levels(); ilevel++) {
 *        for(NodeId node_id : tg.level(ilevel)) {
 *            for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
 *                EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);
 *
 *                NodeId sink_node_id = tg.edge_sink_node(edge_id);
 *
 *                os << "\tnode" << node_id << " -> node" << sink_node_id;
 *                os << " [ label=\"" << tg.edge_delay(edge_id) << "\" ]";
 *                os << ";" << endl;
 *            }
 *        }
 *    }
 *
 *    os << "}" << endl;
 *}
 *
 *void write_dot_file_hold(std::ostream& os, const TimingGraph& tg, const std::shared_ptr<TimingAnalyzer<SetupHoldAnalysis>> analyzer) {
 *    //Write out a dot file of the timing graph
 *    os << "digraph G {" << endl;
 *    os << "\tnode[shape=record]" << endl;
 *
 *    //Declare nodes and annotate tags
 *    for(int inode = 0; inode < tg.num_nodes(); inode++) {
 *        os << "\tnode" << inode;
 *        os << "[label=\"";
 *        os << "{#" << inode << " (" << tg.node_type(inode) << ")";
 *        const TimingTags& data_tags = analyzer->hold_data_tags(inode);
 *        if(data_tags.num_tags() > 0) {
 *            for(const TimingTag& tag : data_tags) {
 *                os << " | {";
 *                os << "DATA - clk: " << tag.clock_domain();
 *                os << " launch: " << tag.launch_node();
 *                os << "\\n";
 *                os << " arr: " << tag.arr_time().value();
 *                os << " req: " << tag.req_time().value();
 *                os << "}";
 *            }
 *        }
 *        const TimingTags& clock_tags = analyzer->hold_clock_tags(inode);
 *        if(clock_tags.num_tags() > 0) {
 *            for(const TimingTag& tag : clock_tags) {
 *                os << " | {";
 *                os << "CLOCK - clk: " << tag.clock_domain();
 *                os << " launch: " << tag.launch_node();
 *                os << "\\n";
 *                os << " arr: " << tag.arr_time().value();
 *                os << " req: " << tag.req_time().value();
 *                os << "}";
 *            }
 *        }
 *        os << "}\"]";
 *        os << endl;
 *    }
 *
 *    //Force drawing to be levelized
 *    for(int ilevel = 0; ilevel < tg.num_levels(); ilevel++) {
 *        os << "\t{rank = same;";
 *
 *        for(NodeId node_id : tg.level(ilevel)) {
 *            os << " node" << node_id <<";";
 *        }
 *        os << "}" << endl;
 *    }
 *
 *    //Add edges with delays annoated
 *    for(int ilevel = 0; ilevel < tg.num_levels(); ilevel++) {
 *        for(NodeId node_id : tg.level(ilevel)) {
 *            for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
 *                EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);
 *
 *                NodeId sink_node_id = tg.edge_sink_node(edge_id);
 *
 *                os << "\tnode" << node_id << " -> node" << sink_node_id;
 *                os << " [ label=\"" << tg.edge_delay(edge_id) << "\" ]";
 *                os << ";" << endl;
 *            }
 *        }
 *    }
 *
 *    os << "}" << endl;
 *}
 */


void add_ff_clock_to_source_sink_edges(TimingGraph& tg, std::vector<float>& edge_delays) {
    //We represent the dependancies between the clock and data paths
    //As edges in the graph from FF_CLOCK pins to FF_SOURCES (launch path)
    //and FF_SINKS (capture path)
    //
    //We use the information about the logical blocks associated with each
    //timing node to infer these edges.  That is, we look for FF_SINKs and FF_SOURCEs
    //that share the same logical block as an FF_CLOCK.
    //
    //TODO: Currently this just works for VPR's timing graph where only one FF_CLOCK exists
    //      per basic logic block.  This will need to be generalized.

    //Build a map from logical block id to the tnodes we care about
    std::map<BlockId,std::vector<NodeId>> logical_block_FF_clocks;
    std::map<BlockId,std::vector<NodeId>> logical_block_FF_sources;
    std::map<BlockId,std::vector<NodeId>> logical_block_FF_sinks;

    for(NodeId node_id = 0; node_id < tg.num_nodes(); node_id++) {
        if(tg.node_type(node_id) == TN_Type::FF_CLOCK) {
            logical_block_FF_clocks[tg.node_logical_block(node_id)].push_back(node_id);
        } else if (tg.node_type(node_id) == TN_Type::FF_SOURCE) {
            logical_block_FF_sources[tg.node_logical_block(node_id)].push_back(node_id);
        } else if (tg.node_type(node_id) == TN_Type::FF_SINK) {
            logical_block_FF_sinks[tg.node_logical_block(node_id)].push_back(node_id);
        }
    }

    //Loop through each FF_CLOCK and add edges to FF_SINKs and FF_SOURCEs
    for(const auto clock_kv : logical_block_FF_clocks) {
        BlockId logical_block_id = clock_kv.first;
        VERIFY(clock_kv.second.size() == 1);
        NodeId ff_clock_node_id = clock_kv.second[0];

        //Check for FF_SOURCEs associated with this FF_CLOCK pin
        auto src_iter = logical_block_FF_sources.find(logical_block_id);
        if(src_iter != logical_block_FF_sources.end()) {
            //Go through each assoicated source and add an edge to it
            for(NodeId ff_src_node_id : src_iter->second) {
                tg.add_edge(ff_clock_node_id, ff_src_node_id);

                //Mark edge as having zero delay
                edge_delays.push_back(0.);
            }
        }

        //Check for FF_SINKs associated with this FF_CLOCK pin
        auto sink_iter = logical_block_FF_sinks.find(logical_block_id);
        if(sink_iter != logical_block_FF_sinks.end()) {
            //Go through each assoicated sink and add an edge to it
            for(NodeId ff_sink_node_id : sink_iter->second) {
                tg.add_edge(ff_clock_node_id, ff_sink_node_id);

                //Mark edge as having zero delay
                edge_delays.push_back(0.);
            }
        }
    }
}
