#include <ctime>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <numeric>
#include "tatum/util/tatum_assert.hpp"
#include "tatum/util/OsFormatGuard.hpp"
#include "tatum/base/sta_util.hpp"

using std::cout;
using std::endl;

namespace tatum {

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

void print_level_histogram(const TimingGraph& tg, int nbuckets) {
    cout << "Levels Width Histogram" << endl;

    std::vector<float> level_widths;
    for(const LevelId level_id : tg.levels()) {
        level_widths.push_back(tg.level_nodes(level_id).size());
    }
    print_histogram(level_widths, nbuckets);
}

void print_node_fanin_histogram(const TimingGraph& tg, int nbuckets) {
    cout << "Node Fan-in Histogram" << endl;

    std::vector<float> fanin;
    for(const NodeId node_id : tg.nodes()) {
        fanin.push_back(tg.node_in_edges(node_id).size());
    }

    std::sort(fanin.begin(), fanin.end(), std::greater<float>());
    print_histogram(fanin, nbuckets);
}

void print_node_fanout_histogram(const TimingGraph& tg, int nbuckets) {
    cout << "Node Fan-out Histogram" << endl;

    std::vector<float> fanout;
    for(const NodeId node_id : tg.nodes()) {
        fanout.push_back(tg.node_out_edges(node_id).size());
    }

    std::sort(fanout.begin(), fanout.end(), std::greater<float>());
    print_histogram(fanout, nbuckets);
}


void print_timing_graph(std::shared_ptr<const TimingGraph> tg) {
    for(const NodeId node_id : tg->nodes()) {
        cout << "Node: " << node_id;
        cout << " Type: " << tg->node_type(node_id);
        cout << " Out Edges: " << tg->node_out_edges(node_id).size();
        cout << "\n";
        for(EdgeId edge_id : tg->node_out_edges(node_id)) {
            TATUM_ASSERT(tg->edge_src_node(edge_id) == node_id);

            NodeId sink_node_id = tg->edge_sink_node(edge_id);

            cout << "\tEdge src node: " << node_id << " sink node: " << sink_node_id << "\n";
        }
    }
}

void print_levelization(std::shared_ptr<const TimingGraph> tg) {
    cout << "Num Levels: " << tg->levels().size() << "\n";
    for(const LevelId level_id : tg->levels()) {
        const auto& level = tg->level_nodes(level_id);
        cout << "Level " << level_id << ": " << level.size() << " nodes" << "\n";
        cout << "\t";
        for(auto node_id : level) {
            cout << node_id << " ";
        }
        cout << "\n";
    }
}



void print_setup_tags_histogram(const TimingGraph& tg, const SetupTimingAnalyzer& analyzer) {
    OsFormatGuard format_guard(std::cout);

    const int int_width = 8;
    const int flt_width = 2;

    auto totaler = [](int total, const std::map<int,int>::value_type& kv) {
        return total + kv.second;
    };

    std::cout << "Node Total Setup Tag Count Histogram:" << std::endl;
    std::map<int,int> total_tag_cnts;
    for(const NodeId i : tg.nodes()) {
        total_tag_cnts[analyzer.setup_tags(i).size()]++;
    }

    int total_tags = std::accumulate(total_tag_cnts.begin(), total_tag_cnts.end(), 0, totaler);
    for(const auto& kv : total_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_tags << ")" << std::endl;
    }

    std::cout << "Node Data Arrival Setup Tag Count Histogram:" << std::endl;
    std::map<int,int> data_arrival_tag_cnts;
    for(const NodeId i : tg.nodes()) {
        data_arrival_tag_cnts[analyzer.setup_tags(i, TagType::DATA_ARRIVAL).size()]++;
    }

    int total_data_arrival_tags = std::accumulate(data_arrival_tag_cnts.begin(), data_arrival_tag_cnts.end(), 0, totaler);
    for(const auto& kv : data_arrival_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_data_arrival_tags << ")" << std::endl;
    }

    std::cout << "Node Data Required Setup Tag Count Histogram:" << std::endl;
    std::map<int,int> data_required_tag_cnts;
    for(const NodeId i : tg.nodes()) {
        data_required_tag_cnts[analyzer.setup_tags(i, TagType::DATA_REQUIRED).size()]++;
    }

    int total_data_tags = std::accumulate(data_required_tag_cnts.begin(), data_required_tag_cnts.end(), 0, totaler);
    for(const auto& kv : data_required_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_data_tags << ")" << std::endl;
    }

    std::cout << "Node Clock Launch Setup Tag Count Histogram:" << std::endl;
    std::map<int,int> clock_launch_tag_cnts;
    for(const NodeId i : tg.nodes()) {
        clock_launch_tag_cnts[analyzer.setup_tags(i, TagType::CLOCK_LAUNCH).size()]++;
    }

    int total_clock_launch_tags = std::accumulate(clock_launch_tag_cnts.begin(), clock_launch_tag_cnts.end(), 0, totaler);
    for(const auto& kv : clock_launch_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_clock_launch_tags << ")" << std::endl;
    }

    std::cout << "Node Clock Capture Setup Tag Count Histogram:" << std::endl;
    std::map<int,int> clock_capture_tag_cnts;
    for(const NodeId i : tg.nodes()) {
        clock_capture_tag_cnts[analyzer.setup_tags(i, TagType::CLOCK_CAPTURE).size()]++;
    }

    int total_clock_capture_tags = std::accumulate(clock_capture_tag_cnts.begin(), clock_capture_tag_cnts.end(), 0, totaler);
    for(const auto& kv : clock_capture_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_clock_capture_tags << ")" << std::endl;
    }

}

void print_hold_tags_histogram(const TimingGraph& tg, const HoldTimingAnalyzer& analyzer) {
    OsFormatGuard format_guard(std::cout);
    const int int_width = 8;
    const int flt_width = 2;

    auto totaler = [](int total, const std::map<int,int>::value_type& kv) {
        return total + kv.second;
    };

    std::cout << "Node Total Hold Tag Count Histogram:" << std::endl;
    std::map<int,int> total_tag_cnts;
    for(const NodeId i : tg.nodes()) {
        total_tag_cnts[analyzer.hold_tags(i).size()]++;
    }

    int total_tags = std::accumulate(total_tag_cnts.begin(), total_tag_cnts.end(), 0, totaler);
    for(const auto& kv : total_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_tags << ")" << std::endl;
    }

    std::cout << "Node Data Hold Tag Count Histogram:" << std::endl;
    std::map<int,int> data_tag_cnts;
    for(const NodeId i : tg.nodes()) {
        data_tag_cnts[analyzer.hold_tags(i).size()]++;
    }

    int total_data_tags = std::accumulate(data_tag_cnts.begin(), data_tag_cnts.end(), 0, totaler);
    for(const auto& kv : data_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_data_tags << ")" << std::endl;
    }

    std::cout << "Node Clock Launch Setup Tag Count Histogram:" << std::endl;
    std::map<int,int> clock_launch_tag_cnts;
    for(const NodeId i : tg.nodes()) {
        clock_launch_tag_cnts[analyzer.hold_tags(i, TagType::CLOCK_LAUNCH).size()]++;
    }

    int total_clock_launch_tags = std::accumulate(clock_launch_tag_cnts.begin(), clock_launch_tag_cnts.end(), 0, totaler);
    for(const auto& kv : clock_launch_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_clock_launch_tags << ")" << std::endl;
    }

    std::cout << "Node Clock Capture Setup Tag Count Histogram:" << std::endl;
    std::map<int,int> clock_capture_tag_cnts;
    for(const NodeId i : tg.nodes()) {
        clock_capture_tag_cnts[analyzer.hold_tags(i, TagType::CLOCK_CAPTURE).size()]++;
    }

    int total_clock_capture_tags = std::accumulate(clock_capture_tag_cnts.begin(), clock_capture_tag_cnts.end(), 0, totaler);
    for(const auto& kv : clock_capture_tag_cnts) {
        std::cout << "\t" << kv.first << " Tags: " << std::setw(int_width) << kv.second << " (" << std::setw(flt_width) << std::fixed << (float) kv.second / total_clock_capture_tags << ")" << std::endl;
    }
}

void print_setup_tags(const TimingGraph& tg, const SetupTimingAnalyzer& analyzer) {
    OsFormatGuard flag_guard(std::cout);

    std::cout << std::endl;
    std::cout << "Setup Tags:" << std::endl;
    std::cout << std::scientific;
    for(const LevelId level_id : tg.levels()) {
        std::cout << "Level: " << level_id << std::endl;
        for(NodeId node_id : tg.level_nodes(level_id)) {
            std::cout << "Node: " << node_id << " (" << tg.node_type(node_id) << ")" << std::endl;;
            for(const TimingTag& tag : analyzer.setup_tags(node_id)) {
                std::cout << "\t" << tag.type() << ": ";
                std::cout << "  launch : " << tag.launch_clock_domain();
                std::cout << "  capture : " << tag.capture_clock_domain();
                std::cout << "  time: " << tag.time().value();
                std::cout << std::endl;
            }
        }
    }
    std::cout << std::endl;
}

void print_hold_tags(const TimingGraph& tg, const HoldTimingAnalyzer& analyzer) {
    OsFormatGuard flag_guard(std::cout);

    std::cout << std::endl;
    std::cout << "Hold Tags:" << std::endl;
    std::cout << std::scientific;
    for(const LevelId level_id : tg.levels()) {
        std::cout << "Level: " << level_id << std::endl;
        for(NodeId node_id : tg.level_nodes(level_id)) {
            std::cout << "Node: " << node_id << " (" << tg.node_type(node_id) << ")" << std::endl;;
            for(const TimingTag& tag : analyzer.hold_tags(node_id)) {
                std::cout << "\t" << tag.type() << ": ";
                std::cout << "  launch : " << tag.launch_clock_domain();
                std::cout << "  capture : " << tag.capture_clock_domain();
                std::cout << "  time: " << tag.time().value();
                std::cout << std::endl;
            }
        }
    }
    std::cout << std::endl;
}


void dump_level_times(std::string fname, const TimingGraph& timing_graph, std::map<std::string,float> serial_prof_data, std::map<std::string,float> parallel_prof_data) {
    //Per-level speed-up
    //cout << "Level Speed-Ups by width:" << endl;
    std::map<int,std::vector<LevelId>,std::greater<int>> widths_to_levels;
    for(const LevelId level_id : timing_graph.levels()) {
        int width = timing_graph.level_nodes(level_id).size();
        widths_to_levels[width].push_back(level_id);
    }

    std::ofstream of(fname);
    of << "Width,Level,serial_fwd,serial_bck,parallel_fwd,parallel_bck"<< endl;
    for(auto kv : widths_to_levels) {
        int width = kv.first;
        for(const auto ilevel : kv.second) {
            std::stringstream key_fwd;
            std::stringstream key_bck;
            key_fwd << "fwd_level_" << ilevel;
            key_bck << "bck_level_" << ilevel;
            of << width << "," << ilevel << ",";
            of << serial_prof_data[key_fwd.str()] << "," << serial_prof_data[key_bck.str()] << ",";
            of << parallel_prof_data[key_fwd.str()] << "," << parallel_prof_data[key_bck.str()];
            of << endl;
        }
    }
}

} //namepsace
