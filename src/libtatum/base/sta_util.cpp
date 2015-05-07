#include <ctime>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "assert.hpp"
#include "sta_util.hpp"


using std::cout;
using std::endl;

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
        cout << "Node: " << node_id << " Type: " << tg.node_type(node_id) <<  " Out Edges: " << tg.num_node_out_edges(node_id) << endl;
        for(int out_edge_idx = 0; out_edge_idx < tg.num_node_out_edges(node_id); out_edge_idx++) {
            EdgeId edge_id = tg.node_out_edge(node_id, out_edge_idx);
            ASSERT(tg.edge_src_node(edge_id) == node_id);

            NodeId sink_node_id = tg.edge_sink_node(edge_id);

            cout << "\tEdge src node: " << node_id << " sink node: " << sink_node_id << " Delay: " << tg.edge_delay(edge_id).value() << endl;
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


void print_timing_tags_histogram(const TimingGraph& tg, SerialTimingAnalyzer& analyzer, int nbuckets) {
    const int int_width = 8;
    const int flt_width = 2;

    cout << "Node Arrival Tag Count Histogram:" << endl;
    std::map<int,int> arr_tag_cnts;
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        arr_tag_cnts[analyzer.arrival_tags(i).num_tags()]++;
    }

    auto totaler = [](int total, const std::map<int,int>::value_type& kv) {
        return total + kv.second;
    };

    int total_arr_tags = std::accumulate(arr_tag_cnts.begin(), arr_tag_cnts.end(), 0, totaler);
    for(const auto& kv : arr_tag_cnts) {
        cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_arr_tags << ")" << endl;
    }

    cout << "Node Required Tag Count Histogram:" << endl;
    std::map<int,int> req_tag_cnts;
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        req_tag_cnts[analyzer.required_tags(i).num_tags()]++;
    }
    int total_req_tags = std::accumulate(req_tag_cnts.begin(), req_tag_cnts.end(), 0, totaler);
    for(const auto& kv : req_tag_cnts) {
        cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_req_tags << ")" << endl;
    }
}

void print_timing_tags(const TimingGraph& tg, SerialTimingAnalyzer& analyzer) {
    cout << std::scientific;
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        for(const TimingTag& tag : analyzer.arrival_tags(i)) {
            cout << "Node " << i << ": clk: " << tag.clock_domain() << " Arr: " << tag.time().value() << endl;
        }
    }
    for(NodeId i = 0; i < tg.num_nodes(); i++) {
        for(const TimingTag& tag : analyzer.required_tags(i)) {
            cout << "Node " << i << ": clk: " << tag.clock_domain() << " Req: " << tag.time().value() << endl;
        }
    }
}


