#include <ctime>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include "sta_util.hpp"

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
    std::ios_base::fmtflags saved_flags = std::cout.flags();
    std::streamsize prec = std::cout.precision();
    std::streamsize width = std::cout.width();

    std::streamsize int_width = ceil(log10(values.size()));
    std::streamsize float_prec = 1;

    int histo_char_width = 60;

    //std::cout << "\t" << std::endl;
    for(int i = 0; i < nbuckets; i++) {
        std::cout << std::setw(int_width) << i*values_per_bucket << ":" << std::setw(int_width) << (i+1)*values_per_bucket - 1;
        std::cout << " " <<  std::scientific << std::setprecision(float_prec) << buckets[i];
        std::cout << " ";

        for(int j = 0; j < histo_char_width*(buckets[i]/max_bucket_val); j++) {
            std::cout << "*";
        }
        std::cout << std::endl;
    }

    std::cout.flags(saved_flags);
    std::cout.precision(prec);
    std::cout.width(width);
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
    std::cout << "Levels Width Histogram" << std::endl;

    std::vector<float> level_widths;
    for(int i = 0; i < tg.num_levels(); i++) {
        level_widths.push_back(tg.level(i).size());
    }
    print_histogram(level_widths, nbuckets);
}

void print_node_fanin_histogram(const TimingGraph& tg, int nbuckets) {
    std::cout << "Node Fan-in Histogram" << std::endl;

    std::vector<float> fanin;
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        fanin.push_back(tg.num_node_in_edges(i));
    }

    std::sort(fanin.begin(), fanin.end(), std::greater<float>());
    print_histogram(fanin, nbuckets);
}

void print_node_fanout_histogram(const TimingGraph& tg, int nbuckets) {
    std::cout << "Node Fan-out Histogram" << std::endl;

    std::vector<float> fanout;
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        fanout.push_back(tg.num_node_out_edges(i));
    }

    std::sort(fanout.begin(), fanout.end(), std::greater<float>());
    print_histogram(fanout, nbuckets);
}


void print_timing_graph(const TimingGraph& tg) {
    for(NodeId node_id = 0; node_id < tg.num_nodes(); node_id++) {
        std::cout << "Node: " << node_id << " Type: " << tg.node_type(node_id) <<  " Out Edges: " << tg.num_node_out_edges(node_id) << std::endl;
        for(int out_edge_idx = 0; out_edge_idx < tg.num_node_out_edges(node_id); out_edge_idx++) {
            EdgeId edge_id = tg.node_out_edge(node_id, out_edge_idx);
            NodeId from_node_id = tg.edge_src_node(edge_id);
            assert(from_node_id == node_id);

            NodeId sink_node_id = tg.edge_sink_node(edge_id);

            std::cout << "\tEdge src node: " << node_id << " sink node: " << sink_node_id << " Delay: " << tg.edge_delay(edge_id).value() << std::endl;
        }
    }
}
