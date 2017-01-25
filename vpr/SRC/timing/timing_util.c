#include "timing_util.h"
#include "vtr_assert.h"

#include "globals.h"

float find_critical_path_delay(const tatum::SetupTimingAnalyzer& setup_analyzer) {

    //Look at all the edges to sink nodes in the graph and find the one with least slack
    tatum::NodeId least_slack_node;
    tatum::TimingTag least_slack_tag;
    for(tatum::NodeId node : g_timing_graph.logical_outputs()) { //All sink nodes
        for(tatum::EdgeId edge : g_timing_graph.node_in_edges(node)) { //All incoming edges
            for(tatum::TimingTag tag : setup_analyzer.setup_slacks(edge)) { //All slacks on edge
                if(!least_slack_node //Uninitialized
                   || least_slack_tag.time() > tag.time()) //Smaller
                {
                    least_slack_node = node;
                    least_slack_tag = tag;
                }
            }
        }
    }

    //Find the associated least-slack tag on the least-slack node
    float cpd = NAN;
    for(tatum::TimingTag tag : setup_analyzer.setup_tags(least_slack_node, tatum::TagType::DATA_ARRIVAL)) {
        if(tag.launch_clock_domain() == least_slack_tag.launch_clock_domain()) {
            cpd = tag.time().value();
            break;
        }
    }
    VTR_ASSERT(!std::isnan(cpd));

    return cpd;
}

float find_setup_total_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer) {
    float tns = 0.;
    for(tatum::NodeId node : g_timing_graph.logical_outputs()) {
        for(tatum::TimingTag tag : setup_analyzer.setup_slacks(node)) {
            float slack = tag.time().value();
            if(slack < 0.) {
                tns += slack;
            }
        }
    }
    return tns;
}

float find_setup_worst_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer) {
    float wns = 0.;
    for(tatum::NodeId node : g_timing_graph.logical_outputs()) {
        for(tatum::TimingTag tag : setup_analyzer.setup_slacks(node)) {
            float slack = tag.time().value();

            if(slack < 0.) {
                wns = std::min(wns, slack);
            }
        }
    }
    return wns;
}

std::vector<HistogramBucket> find_setup_slack_histogram(const tatum::SetupTimingAnalyzer& setup_analyzer, size_t num_bins) {
    std::vector<HistogramBucket> histogram;

    //Find the min and max slacks
    float min_slack = std::numeric_limits<float>::infinity();
    float max_slack = -std::numeric_limits<float>::infinity();
    for(tatum::NodeId node : g_timing_graph.logical_outputs()) {
        for(tatum::TimingTag tag : setup_analyzer.setup_slacks(node)) {
            float slack = tag.time().value();
            
            min_slack = std::min(min_slack, slack);
            max_slack = std::max(max_slack, slack);
        }
    }

    //Determine the bin size
    float range = max_slack - min_slack;
    float bin_size = range / num_bins;

    //Create the buckets
    float bucket_min = min_slack;
    for(size_t ibucket = 0; ibucket < num_bins; ++ibucket) {
        float bucket_max = bucket_min + bin_size;

        histogram.emplace_back(bucket_min, bucket_max);

        bucket_min = bucket_max;
    }

    //To avoid round-off errors we force the max value of the last bucket equal to the max slack
    histogram[histogram.size()-1].max_value = max_slack;

    //Count the slacks into the buckets
    auto comp = [](const HistogramBucket& bucket, float slack) {
        return bucket.max_value < slack;
    };

    for(tatum::NodeId node : g_timing_graph.logical_outputs()) {
        for(tatum::TimingTag tag : setup_analyzer.setup_slacks(node)) {
            float slack = tag.time().value();
            
            //Find the bucket who's max is less than the current slack

            auto iter = std::lower_bound(histogram.begin(), histogram.end(), slack, comp);
            VTR_ASSERT(iter != histogram.end());
            
            iter->count++;
        }
    }

    return histogram;
}

