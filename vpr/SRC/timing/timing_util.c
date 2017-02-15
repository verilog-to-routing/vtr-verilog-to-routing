#include <fstream>

#include "vtr_log.h"
#include "vtr_assert.h"
#include "vtr_math.h"

#include "globals.h"
#include "timing_util.h"

double sec_to_nanosec(double seconds) { return 1e9*seconds; }

double sec_to_mhz(double seconds) { return (1. / seconds) / 1e6; }

PathInfo find_longest_critical_path_delay(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer) {
    PathInfo crit_path_info;

    auto cpds = find_critical_path_delays(constraints, setup_analyzer);

    //Record the maximum critical path accross all domain pairs
    for(const auto& path_info : cpds) {
        if(crit_path_info.path_delay < path_info.path_delay || std::isnan(crit_path_info.path_delay)) {
            crit_path_info = path_info;
        }
    }

    return crit_path_info;
}

PathInfo find_least_slack_critical_path_delay(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer) {
    PathInfo crit_path_info;

    auto cpds = find_critical_path_delays(constraints, setup_analyzer);

    //Record the maximum critical path accross all domain pairs
    for(const auto& path_info : cpds) {
        if(path_info.slack < crit_path_info.slack || std::isnan(crit_path_info.slack)) {
            crit_path_info = path_info;
        }
    }

    return crit_path_info;
}

std::vector<PathInfo> find_critical_path_delays(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer) {
    std::vector<PathInfo> cpds;

    //We calculate the critical path delay (CPD) for each pair of clock domains (which are connected to each other)
    //
    //   CPD = (Tlaunch_clock_delay + Tpropagation_delay) - (Tcapture_clock_delay)
    //       = Tdata_arrival - Tclock_capture
    //
    //Intuitively, CPD is the smallest period (maximum frequency) we can run the launch clock at while not violating
    //the constraint.
    //
    //Since we include the launch clock delay in the data arrival time, we only need to calculate the difference
    //with the caputre clock

    //To ensure we find the critical path delay, we look at the arrival times at all timing endpoints (i.e. logical_outputs())
    for(tatum::NodeId node : g_timing_graph->logical_outputs()) {

        //Look at each data arrival
        for(tatum::TimingTag data_tag : setup_analyzer.setup_tags(node, tatum::TagType::DATA_ARRIVAL)) {

            float data_arrival = data_tag.time().value();

            //And each clock arrival
            for(tatum::TimingTag clock_tag : setup_analyzer.setup_tags(node, tatum::TagType::CLOCK_CAPTURE)) {
                float clock_capture = clock_tag.time().value();



                //Provided the domain pair should be analyzed
                if(constraints.should_analyze(data_tag.launch_clock_domain(), clock_tag.capture_clock_domain())) {

                    float cpd = data_arrival - clock_capture;
                    VTR_ASSERT(!std::isnan(cpd));

                    float slack = find_node_setup_slack(setup_analyzer, node, data_tag.launch_clock_domain(), clock_tag.capture_clock_domain());
                    VTR_ASSERT(!std::isnan(slack));

                    //Record the path info
                    PathInfo path(cpd, slack,
                                  data_tag.origin_node(), node, 
                                  data_tag.launch_clock_domain(), clock_tag.capture_clock_domain());

                    //Find any existing path for this domain pair
                    auto cmp = [&path](const PathInfo& elem) {
                        return    elem.launch_domain == path.launch_domain
                               && elem.capture_domain == path.capture_domain;
                    };
                    auto iter = std::find_if(cpds.begin(), cpds.end(), cmp);

                    if(iter == cpds.end()) {
                        //New domain pair
                        cpds.push_back(path);
                    } else if(iter->path_delay < path.path_delay) {
                        //New max CPD
                        *iter = path;
                    }
                }
            }
        }
    }

    auto cmp = [&](const PathInfo& lhs, const PathInfo& rhs) {
        const auto& lhs_launch_clock_name = constraints.clock_domain_name(lhs.launch_domain);
        const auto& rhs_launch_clock_name = constraints.clock_domain_name(rhs.launch_domain);
        if(lhs_launch_clock_name < rhs_launch_clock_name) {
            //Sort by clock name first
            return true;
        } else if (lhs_launch_clock_name == rhs_launch_clock_name) {
            //Then so the intra-domain pair appear first
            return lhs.launch_domain == rhs.launch_domain; 
        }
        return false;
    };

    std::sort(cpds.begin(), cpds.end(), cmp);

    return cpds;
}

float find_setup_total_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer) {
    float tns = 0.;
    for(tatum::NodeId node : g_timing_graph->logical_outputs()) {
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
    for(tatum::NodeId node : g_timing_graph->logical_outputs()) {
        for(tatum::TimingTag tag : setup_analyzer.setup_slacks(node)) {
            float slack = tag.time().value();

            if(slack < 0.) {
                wns = std::min(wns, slack);
            }
        }
    }
    return wns;
}

float find_node_setup_slack(const tatum::SetupTimingAnalyzer& setup_analyzer, tatum::NodeId node, tatum::DomainId launch_domain, tatum::DomainId capture_domain) {
    for(const auto& tag : setup_analyzer.setup_slacks(node)) {
        if(tag.launch_clock_domain() == launch_domain &&
           tag.capture_clock_domain() == capture_domain) {
            return tag.time().value();
        }
    }

    return NAN;
}

std::vector<HistogramBucket> create_setup_slack_histogram(const tatum::SetupTimingAnalyzer& setup_analyzer, size_t num_bins) {
    std::vector<HistogramBucket> histogram;

    //Find the min and max slacks
    float min_slack = std::numeric_limits<float>::infinity();
    float max_slack = -std::numeric_limits<float>::infinity();
    for(tatum::NodeId node : g_timing_graph->logical_outputs()) {
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

    for(tatum::NodeId node : g_timing_graph->logical_outputs()) {
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

void print_setup_timing_summary(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer) {
    auto crit_paths = find_critical_path_delays(constraints, setup_analyzer);
    if(constraints.clock_domains().size() == 1) {
        //Single clock
        VTR_ASSERT(crit_paths.size() == 1);

        //Only makes sense to tal about Fmax in a single-clock circuit
        vtr::printf("Final critical path: %g ns,", sec_to_nanosec(crit_paths[0].path_delay));
        vtr::printf(" Fmax: %g MHz", sec_to_mhz(crit_paths[0].path_delay));
        vtr::printf("\n");

    } else {
        //Multi-clock

        //Periods per constraint
		vtr::printf_info("Minimum clock periods to meet each constraint:\n");
        for(const auto& path : crit_paths) {
            if(path.launch_domain != path.capture_domain) {
                //Indent inter-domain paths
                vtr::printf("    ");
            }

            vtr::printf("  %s to %s: %g ns (%g MHz)\n",
                        constraints.clock_domain_name(path.launch_domain).c_str(),
                        constraints.clock_domain_name(path.capture_domain).c_str(),
                        sec_to_nanosec(path.path_delay),
                        sec_to_mhz(path.path_delay));
        }

        //Slack per constraint
		vtr::printf_info("Worst setup slacks per constraint:\n");
        for(const auto& path : crit_paths) {
            if(path.launch_domain != path.capture_domain) {
                //Indent inter-domain paths
                vtr::printf("    ");
            }
            vtr::printf("  %s to %s: %g ns\n",
                        constraints.clock_domain_name(path.launch_domain).c_str(),
                        constraints.clock_domain_name(path.capture_domain).c_str(),
                        sec_to_nanosec(path.slack));
        }
        vtr::printf("\n");
    }
    vtr::printf("\n");

    vtr::printf("Setup Worst Negative Slack (sWNS): %g ns\n", sec_to_nanosec(find_setup_worst_negative_slack(setup_analyzer)));
    vtr::printf("Setup Total Negative Slack (sTNS): %g ns\n", sec_to_nanosec(find_setup_total_negative_slack(setup_analyzer)));
    vtr::printf("\n");

    vtr::printf_info("Setup slack histogram:\n");
    print_histogram(create_setup_slack_histogram(setup_analyzer));
    vtr::printf("\n");

    //Calculate the intra-domain (i.e. same launch and capture domain) geomean, and fanout-weighted periods
    std::vector<double> intra_domain_cpds;
    std::vector<double> fanout_weighted_intra_domain_cpds;
    double total_intra_domain_fanout = 0.;
    auto clock_fanouts = count_clock_fanouts(*g_timing_graph, setup_analyzer);
    for(const auto& path : crit_paths) {
        if(path.launch_domain == path.capture_domain) {
            intra_domain_cpds.push_back(path.path_delay);

            auto iter = clock_fanouts.find(path.launch_domain);
            VTR_ASSERT(iter != clock_fanouts.end());
            double fanout = iter->second;

            fanout_weighted_intra_domain_cpds.push_back(path.path_delay * fanout);
            total_intra_domain_fanout += fanout;
        }
    }

    if(intra_domain_cpds.size() > 0) {
        vtr::printf("\n");
        double geomean_intra_domain_cpd = vtr::geomean(intra_domain_cpds.begin(), intra_domain_cpds.end());
        vtr::printf("Geometric mean intra-domain period: %g ns (%g MHz)\n", 
                sec_to_nanosec(geomean_intra_domain_cpd), 
                sec_to_mhz(geomean_intra_domain_cpd));

        //Normalize weighted fanouts by total fanouts
        for(auto& weighted_cpd : fanout_weighted_intra_domain_cpds) {
            weighted_cpd /= total_intra_domain_fanout;
        }
        double fanout_weighted_geomean_intra_domain_cpd = vtr::geomean(fanout_weighted_intra_domain_cpds.begin(),
                                                                       fanout_weighted_intra_domain_cpds.end());
        vtr::printf("Fanout-weighted geomean intra-domain period: %g ns (%g MHz)\n", 
                sec_to_nanosec(fanout_weighted_geomean_intra_domain_cpd), 
                sec_to_mhz(fanout_weighted_geomean_intra_domain_cpd));
    }
    vtr::printf("\n");
}

std::map<tatum::DomainId,size_t> count_clock_fanouts(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& setup_analyzer) {
    std::map<tatum::DomainId,size_t> fanouts;
    for(auto node: timing_graph.logical_outputs()) {
        for(auto tag : setup_analyzer.setup_tags(node, tatum::TagType::CLOCK_CAPTURE)) {
            fanouts[tag.capture_clock_domain()] += 1;
        }
    }

    return fanouts;
}

void dump_atom_net_delays_tatum(std::string filename, const PlacementDelayCalculator& dc) {
    std::ofstream os(filename);

    for(AtomNetId net : g_atom_nl.nets()) {
        os << "Net: " << size_t(net) << " (" << g_atom_nl.net_name(net) << ")\n";
        AtomPinId driver = g_atom_nl.net_driver(net);
        os << "\tDriver: " << size_t(driver) << " (" << g_atom_nl.pin_name(driver) << ")\n";

        tatum::NodeId driver_tnode = g_atom_map.pin_tnode[driver];

        for(AtomPinId sink : g_atom_nl.net_sinks(net)) {
            tatum::NodeId sink_tnode = g_atom_map.pin_tnode[sink];

            //Find the corresponding edge
            tatum::EdgeId edge;
            for(tatum::EdgeId check_edge : g_timing_graph->node_out_edges(driver_tnode)) {
                if(g_timing_graph->edge_sink_node(check_edge) == sink_tnode) {
                    edge = check_edge;
                    break;
                }
            }
            VTR_ASSERT(edge);

            float delay = dc.max_edge_delay(*g_timing_graph, edge).value();
            os << "\tSink " << size_t(sink) << ": " << delay << " (" << g_atom_nl.pin_name(sink) << "): " "\n";

        }
    }
}

/*
 * Slack and criticality calculation utilities
 */

//Return the tag from the range [first,last) which has the lowest value
tatum::TimingTags::const_iterator find_minimum_tag(tatum::TimingTags::tag_range tags) {

    return std::min_element(tags.begin(), tags.end(), TimingTagValueComp()); 
}

//Return the tag from the range [first,last) which has the highest value
tatum::TimingTags::const_iterator find_maximum_tag(tatum::TimingTags::tag_range tags) {

    return std::max_element(tags.begin(), tags.end(), TimingTagValueComp()); 
}

//Returns the worst (maximum) criticality of the set of slack tags specified. Requires the maximum
//required time and worst slack for all domain pairs represent by the slack tags
//
// Criticality (in [0., 1.]) represents how timing-critical something is, 
// 0. is non-critical and 1. is most-critical.
//
// This returns 'relaxed per constraint' criticaly as defined in:
//
//     M. Wainberg and V. Betz, "Robust Optimization of Multiple Timing Constraints," 
//         IEEE CAD, vol. 34, no. 12, pp. 1942-1953, Dec. 2015. doi: 10.1109/TCAD.2015.2440316
//
// which handles the trade-off between different timing constraints in multi-clock circuits.
//
// Note that unlike in Wainberg, we calculate the relaxed criticality as a post-processing step.
float calc_relaxed_criticality(const std::map<DomainPair,float>& domains_max_req,
                               const std::map<DomainPair,float>& domains_worst_slack,
                               const tatum::TimingTags::tag_range tags) {
    //Allowable round-off tolerance during criticality calculation
    constexpr float CRITICALITY_ROUND_OFF_TOLERANCE = 1e-5;


    //Record the maximum criticality over all the tags
    float max_crit = 0.;
    for(const auto& tag : tags) {

        VTR_ASSERT_MSG(tag.type() == tatum::TagType::SLACK, "Tags must be slacks to calculate criticality");

        float slack = tag.time().value();

        auto domain_pair = DomainPair(tag.launch_clock_domain(), tag.capture_clock_domain());

        auto iter = domains_max_req.find(domain_pair);
        VTR_ASSERT_MSG(iter != domains_max_req.end(), "Require the maximum required time for clock domain pair");
        float max_req = iter->second;


        iter = domains_worst_slack.find(domain_pair);
        VTR_ASSERT_MSG(iter != domains_worst_slack.end(), "Require the worst slack for clock domain pair");
        float worst_slack = iter->second;

        if(worst_slack < 0.) {
            //We shift slacks and required time by the most negative slack 
            //**in the domain**, to ensure criticality is bounded within [0., 1.]
            //
            //This corresponds to the 'relaxed' criticality from Wainberg et. al.
            float shift = -worst_slack;
            VTR_ASSERT(shift > 0.);

            slack += shift;
            max_req += shift;
        }

        float crit = 1. - (slack / max_req);

        //Soft check for reasonable criticality values
        VTR_ASSERT_MSG(crit >= 0. - CRITICALITY_ROUND_OFF_TOLERANCE, "Criticality should never be negative");
        VTR_ASSERT_MSG(crit <= 1. + CRITICALITY_ROUND_OFF_TOLERANCE, "Criticality should never be greather than one");

        //Clamp criticality to [0., 1.] to correct round-off
        crit = std::max(0.f, crit);
        crit = std::min(1.f, crit);

        max_crit = std::max(max_crit, crit);
    }
    VTR_ASSERT_MSG(max_crit >= 0., "Criticality should never be negative");
    VTR_ASSERT_MSG(max_crit <= 1., "Criticality should never be greather than one");

    return max_crit;
}
