#include <fstream>
#include <sstream>
#include <utility>

#include "tatum/timing_paths.hpp"
#include "vtr_log.h"
#include "vtr_assert.h"
#include "vtr_math.h"

#include "globals.h"
#include "timing_info.h"
#include "timing_util.h"

#include "tatum/report/graphviz_dot_writer.hpp"

double sec_to_nanosec(double seconds) {
    return 1e9 * seconds;
}

double sec_to_mhz(double seconds) {
    return (1. / seconds) / 1e6;
}

/*
 * Setup-time related
 */
tatum::TimingPathInfo find_longest_critical_path_delay(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer) {
    tatum::TimingPathInfo crit_path_info;

    auto& timing_ctx = g_vpr_ctx.timing();

    auto cpds = tatum::find_critical_paths(*timing_ctx.graph, constraints, setup_analyzer);

    //Record the maximum critical path across all domain pairs
    for (const auto& path_info : cpds) {
        if (crit_path_info.delay() < path_info.delay() || std::isnan(crit_path_info.delay())) {
            crit_path_info = path_info;
        }
    }

    return crit_path_info;
}

tatum::TimingPathInfo find_least_slack_critical_path_delay(const tatum::TimingConstraints& constraints, const tatum::SetupTimingAnalyzer& setup_analyzer) {
    tatum::TimingPathInfo crit_path_info;

    auto& timing_ctx = g_vpr_ctx.timing();

    auto cpds = tatum::find_critical_paths(*timing_ctx.graph, constraints, setup_analyzer);

    //Record the maximum critical path across all domain pairs
    for (const auto& path_info : cpds) {
        if (path_info.slack() < crit_path_info.slack() || std::isnan(crit_path_info.slack())) {
            crit_path_info = path_info;
        }
    }

    return crit_path_info;
}

float find_setup_total_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer) {
    auto& timing_ctx = g_vpr_ctx.timing();

    float tns = 0.;
    for (tatum::NodeId node : timing_ctx.graph->logical_outputs()) {
        for (tatum::TimingTag tag : setup_analyzer.setup_slacks(node)) {
            float slack = tag.time().value();
            if (slack < 0.) {
                tns += slack;
            }
        }
    }
    return tns;
}

float find_setup_worst_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer) {
    auto& timing_ctx = g_vpr_ctx.timing();

    float wns = 0.;
    for (tatum::NodeId node : timing_ctx.graph->logical_outputs()) {
        for (tatum::TimingTag tag : setup_analyzer.setup_slacks(node)) {
            float slack = tag.time().value();

            if (slack < 0.) {
                wns = std::min(wns, slack);
            }
        }
    }
    return wns;
}

float find_node_setup_slack(const tatum::SetupTimingAnalyzer& setup_analyzer, tatum::NodeId node, tatum::DomainId launch_domain, tatum::DomainId capture_domain) {
    for (const auto& tag : setup_analyzer.setup_slacks(node)) {
        if (tag.launch_clock_domain() == launch_domain && tag.capture_clock_domain() == capture_domain) {
            return tag.time().value();
        }
    }

    return NAN;
}

std::vector<HistogramBucket> create_setup_slack_histogram(const tatum::SetupTimingAnalyzer& setup_analyzer, size_t num_bins) {
    auto& timing_ctx = g_vpr_ctx.timing();

    std::vector<HistogramBucket> histogram;

    //Find the min and max slacks
    float min_slack = std::numeric_limits<float>::infinity();
    float max_slack = -std::numeric_limits<float>::infinity();
    for (tatum::NodeId node : timing_ctx.graph->logical_outputs()) {
        for (tatum::TimingTag tag : setup_analyzer.setup_slacks(node)) {
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
    for (size_t ibucket = 0; ibucket < num_bins; ++ibucket) {
        float bucket_max = bucket_min + bin_size;

        histogram.emplace_back(bucket_min, bucket_max);

        bucket_min = bucket_max;
    }

    //To avoid round-off errors we force the max value of the last bucket equal to the max slack
    histogram[histogram.size() - 1].max_value = max_slack;

    //Count the slacks into the buckets
    auto comp = [](const HistogramBucket& bucket, float slack) {
        return bucket.max_value < slack;
    };

    for (tatum::NodeId node : timing_ctx.graph->logical_outputs()) {
        for (tatum::TimingTag tag : setup_analyzer.setup_slacks(node)) {
            float slack = tag.time().value();

            //Find the bucket who's max is less than the current slack

            auto iter = std::lower_bound(histogram.begin(), histogram.end(), slack, comp);
            VTR_ASSERT(iter != histogram.end());

            iter->count++;
        }
    }

    return histogram;
}

std::vector<HistogramBucket> create_criticality_histogram(const Netlist<>& net_list,
                                                          const SetupTimingInfo& setup_timing,
                                                          const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                                          bool is_flat,
                                                          size_t num_bins) {
    std::vector<HistogramBucket> histogram;
    float step = 1. / num_bins;

    //Create the bins
    float curr_div = 0.;
    for (size_t i = 0; i < num_bins; ++i) {
        float min = curr_div;
        float max = curr_div + step;
        histogram.emplace_back(min, max);

        curr_div += step;
    }

    //To avoid round-off errors we force the max value of the last bucket equal to the max criticalty
    histogram[histogram.size() - 1].max_value = 1.;

    auto cmp = [](const HistogramBucket& bucket, float crit) {
        return bucket.max_value < crit;
    };

    //Count the criticalities into the buckets
    for (auto net_id : net_list.nets()) {
        auto sinks = net_list.net_sinks(net_id);

        for (auto pin : sinks) {
            float crit = calculate_clb_net_pin_criticality(setup_timing, netlist_pin_lookup, pin, is_flat);

            auto iter = std::lower_bound(histogram.begin(), histogram.end(), crit, cmp);
            VTR_ASSERT(iter != histogram.end());

            iter->count++;
        }
    }

    return histogram;
}

void TimingStats::writeHuman(std::ostream& output) const {
    output << prefix << "critical path delay (least slack): " << least_slack_cpd_delay << " ns";

    //Fmax is only meaningful for a single-clock circuit
    output << ", Fmax: " << fmax << " MHz";
    output << "\n";

    output << prefix << "setup Worst Negative Slack (sWNS): " << setup_worst_neg_slack << " ns\n";
    output << prefix << "setup Total Negative Slack (sTNS): " << setup_total_neg_slack << " ns\n";
    output << "\n";
}
void TimingStats::writeJSON(std::ostream& output) const {
    output << "{\n";

    output << "  \"cpd\": " << least_slack_cpd_delay << ",\n";
    output << "  \"fmax\": " << fmax << ",\n";

    output << "  \"swns\": " << setup_worst_neg_slack << ",\n";
    output << "  \"stns\": " << setup_total_neg_slack << "\n";
    output << "}\n";
}

void TimingStats::writeXML(std::ostream& output) const {
    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    output << "<timing_summary_report>\n";

    output << "  <cpd value=\"" << least_slack_cpd_delay << "\" unit=\"ns\" description=\"Final critical path delay\"></nets>\n";
    output << "  <fmax value=\"" << fmax << "\" unit=\"MHz\" description=\"Max circuit frequency\"></fmax>\n";
    output << "  <swns value=\"" << setup_worst_neg_slack << "\" unit=\"ns\" description=\"setup Worst Negative Slack (sWNS)\"></swns>\n";
    output << "  <stns value=\"" << setup_total_neg_slack << "\" unit=\"ns\" description=\"setup Total Negative Slack (sTNS)\"></stns>\n";

    output << "</block_usage_report>\n";
}

TimingStats::TimingStats(std::string pref, double cpd, double f_max, double swns, double stns) {
    least_slack_cpd_delay = cpd;
    fmax = f_max;
    setup_worst_neg_slack = swns;
    setup_total_neg_slack = stns;
    prefix = std::move(pref);
}

void TimingStats::write(OutputFormat fmt, std::ostream& output) const {
    switch (fmt) {
        case HumanReadable:
            writeHuman(output);
            break;
        case JSON:
            writeJSON(output);
            break;
        case XML:
            writeXML(output);
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Unknown extension on in timing summary file");
            break;
    }
}

void write_setup_timing_summary(std::string_view timing_summary_filename, const TimingStats& stats) {
    if (!timing_summary_filename.empty()) {
        TimingStats::OutputFormat fmt;

        if (vtr::check_file_name_extension(timing_summary_filename.data(), ".json")) {
            fmt = TimingStats::OutputFormat::JSON;
        } else if (vtr::check_file_name_extension(timing_summary_filename.data(), ".xml")) {
            fmt = TimingStats::OutputFormat::XML;
        } else if (vtr::check_file_name_extension(timing_summary_filename.data(), ".txt")) {
            fmt = TimingStats::OutputFormat::HumanReadable;
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_PACK, "Unknown extension on output %s", timing_summary_filename.data());
        }

        std::fstream fp;

        fp.open(timing_summary_filename.data(), std::fstream::out | std::fstream::trunc);
        stats.write(fmt, fp);
        fp.close();
    }
}

void print_setup_timing_summary(const tatum::TimingConstraints& constraints,
                                const tatum::SetupTimingAnalyzer& setup_analyzer,
                                std::string_view prefix,
                                std::string_view timing_summary_filename) {
    auto& timing_ctx = g_vpr_ctx.timing();

    auto crit_paths = tatum::find_critical_paths(*timing_ctx.graph, constraints, setup_analyzer);

    auto least_slack_cpd = find_least_slack_critical_path_delay(constraints, setup_analyzer);

    double least_slack_cpd_delay = sec_to_nanosec(least_slack_cpd.delay());
    double fmax = sec_to_mhz(least_slack_cpd.delay());
    double setup_worst_neg_slack = sec_to_nanosec(find_setup_worst_negative_slack(setup_analyzer));
    double setup_total_neg_slack = sec_to_nanosec(find_setup_total_negative_slack(setup_analyzer));

    const auto stats = TimingStats(prefix.data(), least_slack_cpd_delay, fmax,
                                   setup_worst_neg_slack, setup_total_neg_slack);
    if (!timing_summary_filename.empty())
        write_setup_timing_summary(timing_summary_filename, stats);

    VTR_LOG("%scritical path delay (least slack): %g ns", prefix.data(), least_slack_cpd_delay);

    if (crit_paths.size() == 1) {
        //Fmax is only meaningful for a single-clock circuit
        VTR_LOG(", Fmax: %g MHz", fmax);
    }
    VTR_LOG("\n");

    VTR_LOG("%ssetup Worst Negative Slack (sWNS): %g ns\n", prefix.data(), setup_worst_neg_slack);
    VTR_LOG("%ssetup Total Negative Slack (sTNS): %g ns\n", prefix.data(), setup_total_neg_slack);
    VTR_LOG("\n");

    VTR_LOG("%ssetup slack histogram:\n", prefix.data());
    print_histogram(create_setup_slack_histogram(setup_analyzer));

    if (crit_paths.size() > 1) {
        //Multi-clock
        VTR_LOG("\n");

        //Periods per constraint
        VTR_LOG("%sintra-domain critical path delays (CPDs):\n", prefix.data());
        for (const auto& path : crit_paths) {
            if (path.launch_domain() == path.capture_domain()) {
                VTR_LOG("  %s to %s CPD: %g ns (%g MHz)\n",
                        constraints.clock_domain_name(path.launch_domain()).c_str(),
                        constraints.clock_domain_name(path.capture_domain()).c_str(),
                        sec_to_nanosec(path.delay()),
                        sec_to_mhz(path.delay()));
            }
        }
        VTR_LOG("\n");

        VTR_LOG("%sinter-domain critical path delays (CPDs):\n", prefix.data());
        for (const auto& path : crit_paths) {
            if (path.launch_domain() != path.capture_domain()) {
                VTR_LOG("  %s to %s CPD: %g ns (%g MHz)\n",
                        constraints.clock_domain_name(path.launch_domain()).c_str(),
                        constraints.clock_domain_name(path.capture_domain()).c_str(),
                        sec_to_nanosec(path.delay()),
                        sec_to_mhz(path.delay()));
            }
        }
        VTR_LOG("\n");

        //Slack per constraint
        VTR_LOG("%sintra-domain worst setup slacks per constraint:\n", prefix.data());
        for (const auto& path : crit_paths) {
            if (path.launch_domain() == path.capture_domain()) {
                VTR_LOG("  %s to %s worst setup slack: %g ns\n",
                        constraints.clock_domain_name(path.launch_domain()).c_str(),
                        constraints.clock_domain_name(path.capture_domain()).c_str(),
                        sec_to_nanosec(path.slack()));
            }
        }
        VTR_LOG("\n");

        VTR_LOG("%sinter-domain worst setup slacks per constraint:\n", prefix.data());
        for (const auto& path : crit_paths) {
            if (path.launch_domain() != path.capture_domain()) {
                VTR_LOG("  %s to %s worst setup slack: %g ns\n",
                        constraints.clock_domain_name(path.launch_domain()).c_str(),
                        constraints.clock_domain_name(path.capture_domain()).c_str(),
                        sec_to_nanosec(path.slack()));
            }
        }
    }

    //Calculate the intra-domain (i.e. same launch and capture domain) non-virtual geomean, and fanout-weighted periods
    VTR_LOG("\n");
    std::vector<double> intra_domain_cpds;
    std::vector<double> fanout_weighted_intra_domain_cpds;
    double total_intra_domain_fanout = 0.;
    auto clock_fanouts = count_clock_fanouts(*timing_ctx.graph, setup_analyzer);
    for (const auto& path : crit_paths) {
        if (path.launch_domain() == path.capture_domain() && !constraints.is_virtual_clock(path.launch_domain())) {
            if (path.delay() == 0.) {
                VTR_LOG_WARN("%s%s to %s CPD is %g, skipping in geomean and fanout-weighted CPDs\n",
                             prefix.data(),
                             constraints.clock_domain_name(path.launch_domain()).c_str(),
                             constraints.clock_domain_name(path.capture_domain()).c_str(),
                             sec_to_nanosec(path.delay()));
                continue;
            }

            intra_domain_cpds.push_back(path.delay());

            auto iter = clock_fanouts.find(path.launch_domain());
            VTR_ASSERT(iter != clock_fanouts.end());
            double fanout = iter->second;

            fanout_weighted_intra_domain_cpds.push_back(path.delay() * fanout);
            total_intra_domain_fanout += fanout;
        }
    }

    //Print multi-clock geomeans
    double geomean_intra_domain_cpd = std::numeric_limits<double>::quiet_NaN();
    if (!intra_domain_cpds.empty()) {
        geomean_intra_domain_cpd = vtr::geomean(intra_domain_cpds.begin(), intra_domain_cpds.end());
    }
    VTR_LOG("%sgeomean non-virtual intra-domain period: %g ns (%g MHz)\n",
            prefix.data(),
            sec_to_nanosec(geomean_intra_domain_cpd),
            sec_to_mhz(geomean_intra_domain_cpd));

    //Normalize weighted fanouts by total fanouts
    for (auto& weighted_cpd : fanout_weighted_intra_domain_cpds) {
        weighted_cpd /= total_intra_domain_fanout;
    }

    double fanout_weighted_geomean_intra_domain_cpd = std::numeric_limits<double>::quiet_NaN();
    if (!fanout_weighted_intra_domain_cpds.empty()) {
        fanout_weighted_geomean_intra_domain_cpd = vtr::geomean(fanout_weighted_intra_domain_cpds.begin(),
                                                                fanout_weighted_intra_domain_cpds.end());
    }

    VTR_LOG("%sfanout-weighted geomean non-virtual intra-domain period: %g ns (%g MHz)\n",
            prefix.data(),
            sec_to_nanosec(fanout_weighted_geomean_intra_domain_cpd),
            sec_to_mhz(fanout_weighted_geomean_intra_domain_cpd));

    VTR_LOG("\n");

    /* If the terminate_if_timing fails option is on, this checks if slack is negative and if user wants VPR
     * flow to fail if their design doesn't meet timing constraints. If both conditions are true, the function
     * adds details about the negative slack to a string that will be printed when VPR throws an error.
     */
    if (timing_ctx.terminate_if_timing_fails && (setup_worst_neg_slack < 0 || setup_total_neg_slack < 0) && prefix == "Final ") {
        std::string msg = "\nDesign did not meet timing constraints.\nTiming failed and terminate_if_timing_fails set -- exiting";
        VPR_FATAL_ERROR(VPR_ERROR_TIMING, msg.c_str());
    }
}

/*
 * Hold-time related statistics
 */
float find_hold_total_negative_slack(const tatum::HoldTimingAnalyzer& hold_analyzer) {
    auto& timing_ctx = g_vpr_ctx.timing();

    float tns = 0.;
    for (tatum::NodeId node : timing_ctx.graph->logical_outputs()) {
        for (tatum::TimingTag tag : hold_analyzer.hold_slacks(node)) {
            float slack = tag.time().value();
            if (slack < 0.) {
                tns += slack;
            }
        }
    }
    return tns;
}

tatum::NodeId find_origin_node_for_hold_slack(const tatum::TimingTags::tag_range arrival_tags,
                                              const tatum::TimingTags::tag_range required_tags,
                                              float slack) {
    /*Given the slack, arrival, and required tags of a certain timing node,
     * its origin node is found*/
    for (tatum::TimingTag arrival_tag : arrival_tags) {
        for (tatum::TimingTag required_tag : required_tags) {
            if (arrival_tag.time().value() - required_tag.time().value() == slack) {
                tatum::NodeId origin_node = arrival_tag.origin_node();
                VTR_ASSERT(origin_node);
                return origin_node;
            }
        }
    }
    return tatum::NodeId::INVALID();
}

float find_total_negative_slack_within_clb_blocks(const tatum::HoldTimingAnalyzer& hold_analyzer) {
    /*Some negative slack are found within short paths in clb blocks. This cannot be optimized
     * by the router. This function is used to measure the effectiveness of the router's
     * hold time optimizer*/
    auto& timing_ctx = g_vpr_ctx.timing();
    auto& atom_ctx = g_vpr_ctx.atom();

    float slack_in_block = 0;
    for (tatum::NodeId node : timing_ctx.graph->logical_outputs()) {
        for (tatum::TimingTag tag : hold_analyzer.hold_slacks(node)) {
            float slack = tag.time().value();

            auto arrival_tags = hold_analyzer.hold_tags(node, tatum::TagType::DATA_ARRIVAL);
            auto required_tags = hold_analyzer.hold_tags(node, tatum::TagType::DATA_REQUIRED);
            tatum::NodeId origin_node = find_origin_node_for_hold_slack(arrival_tags, required_tags, slack);

            VTR_ASSERT(origin_node);

            /*Retrieve the source and sink clb blocks corresponding to these timing nodes*/
            AtomPinId origin_pin = atom_ctx.lookup().tnode_atom_pin(origin_node);
            AtomPinId pin = atom_ctx.lookup().tnode_atom_pin(node);
            VTR_ASSERT(origin_pin);
            VTR_ASSERT(pin);

            AtomBlockId atom_src_block = atom_ctx.netlist().pin_block(origin_pin);
            AtomBlockId atom_sink_block = atom_ctx.netlist().pin_block(pin);

            ClusterBlockId clb_src_block = atom_ctx.lookup().atom_clb(atom_src_block);
            VTR_ASSERT(clb_src_block);
            ClusterBlockId clb_sink_block = atom_ctx.lookup().atom_clb(atom_sink_block);
            VTR_ASSERT(clb_sink_block);

            const t_pb_graph_pin* sink_gpin = atom_ctx.lookup().atom_pin_pb_graph_pin(pin);
            VTR_ASSERT(sink_gpin);

            int sink_pb_route_id = sink_gpin->pin_count_in_cluster;
            ClusterNetId net_id;
            int sink_block_pin_index, sink_net_pin_index;
            std::tie(net_id, sink_block_pin_index, sink_net_pin_index) = find_pb_route_clb_input_net_pin(clb_sink_block, sink_pb_route_id);

            if (net_id == ClusterNetId::INVALID() && sink_block_pin_index == -1 && sink_net_pin_index == -1) {
                /*Does not go out of the cluster*/
                if (clb_sink_block == clb_src_block) {
                    if (slack < 0.) {
                        slack_in_block += slack;
                    }
                }
            }
        }
    }
    return slack_in_block;
}

float find_hold_worst_negative_slack(const tatum::HoldTimingAnalyzer& hold_analyzer) {
    auto& timing_ctx = g_vpr_ctx.timing();

    float wns = 0.;
    for (tatum::NodeId node : timing_ctx.graph->logical_outputs()) {
        for (tatum::TimingTag tag : hold_analyzer.hold_slacks(node)) {
            float slack = tag.time().value();

            if (slack < 0.) {
                wns = std::min(wns, slack);
            }
        }
    }
    return wns;
}

float find_hold_worst_slack(const tatum::HoldTimingAnalyzer& hold_analyzer, const tatum::DomainId launch, const tatum::DomainId capture) {
    auto& timing_ctx = g_vpr_ctx.timing();

    float worst_slack = std::numeric_limits<float>::infinity();
    for (tatum::NodeId node : timing_ctx.graph->logical_outputs()) {
        for (tatum::TimingTag tag : hold_analyzer.hold_slacks(node)) {
            if (tag.launch_clock_domain() == launch && tag.capture_clock_domain() == capture) {
                float slack = tag.time().value();

                worst_slack = std::min(worst_slack, slack);
            }
        }
    }
    return worst_slack;
}

std::vector<HistogramBucket> create_hold_slack_histogram(const tatum::HoldTimingAnalyzer& hold_analyzer, size_t num_bins) {
    auto& timing_ctx = g_vpr_ctx.timing();

    std::vector<HistogramBucket> histogram;

    //Find the min and max slacks
    float min_slack = std::numeric_limits<float>::infinity();
    float max_slack = -std::numeric_limits<float>::infinity();
    for (tatum::NodeId node : timing_ctx.graph->logical_outputs()) {
        for (tatum::TimingTag tag : hold_analyzer.hold_slacks(node)) {
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
    for (size_t ibucket = 0; ibucket < num_bins; ++ibucket) {
        float bucket_max = bucket_min + bin_size;

        histogram.emplace_back(bucket_min, bucket_max);

        bucket_min = bucket_max;
    }

    //To avoid round-off errors we force the max value of the last bucket equal to the max slack
    histogram[histogram.size() - 1].max_value = max_slack;

    //Count the slacks into the buckets
    auto comp = [](const HistogramBucket& bucket, float slack) {
        return bucket.max_value < slack;
    };

    for (tatum::NodeId node : timing_ctx.graph->logical_outputs()) {
        for (tatum::TimingTag tag : hold_analyzer.hold_slacks(node)) {
            float slack = tag.time().value();

            //Find the bucket who's max is less than the current slack
            auto iter = std::lower_bound(histogram.begin(), histogram.end(), slack, comp);
            VTR_ASSERT(iter != histogram.end());

            //Add to bucket
            iter->count++;
        }
    }

    return histogram;
}

void print_hold_timing_summary(const tatum::TimingConstraints& constraints,
                               const tatum::HoldTimingAnalyzer& hold_analyzer,
                               std::string_view prefix) {
    auto& timing_ctx = g_vpr_ctx.timing();

    auto hold_worst_neg_slack = sec_to_nanosec(find_hold_worst_negative_slack(hold_analyzer));
    auto hold_total_neg_slack = sec_to_nanosec(find_hold_total_negative_slack(hold_analyzer));

    VTR_LOG("%shold Worst Negative Slack (hWNS): %g ns\n", prefix.data(), hold_worst_neg_slack);
    VTR_LOG("%shold Total Negative Slack (hTNS): %g ns\n", prefix.data(), hold_total_neg_slack);

    /*For testing*/
    //VTR_LOG("Hold Total Negative Slack within clbs: %g ns\n", sec_to_nanosec(find_total_negative_slack_within_clb_blocks(hold_analyzer)));
    VTR_LOG("\n");

    VTR_LOG("%shold slack histogram:\n", prefix.data());
    print_histogram(create_hold_slack_histogram(hold_analyzer));

    if (constraints.clock_domains().size() > 1) {
        //Multi-clock
        VTR_LOG("\n");

        //Slack per constraint
        VTR_LOG("%sintra-domain worst hold slacks per constraint:\n", prefix.data());
        for (const auto& domain : constraints.clock_domains()) {
            float worst_slack = find_hold_worst_slack(hold_analyzer, domain, domain);

            if (worst_slack == std::numeric_limits<float>::infinity()) continue; //No path

            VTR_LOG("  %s to %s worst hold slack: %g ns\n",
                    constraints.clock_domain_name(domain).c_str(),
                    constraints.clock_domain_name(domain).c_str(),
                    sec_to_nanosec(worst_slack));
        }
        VTR_LOG("\n");

        VTR_LOG("%sinter-domain worst hold slacks per constraint:\n", prefix.data());
        for (const auto& launch_domain : constraints.clock_domains()) {
            for (const auto& capture_domain : constraints.clock_domains()) {
                if (launch_domain != capture_domain) {
                    float worst_slack = find_hold_worst_slack(hold_analyzer, launch_domain, capture_domain);

                    if (worst_slack == std::numeric_limits<float>::infinity()) continue; //No path

                    VTR_LOG("  %s to %s worst hold slack: %g ns\n",
                            constraints.clock_domain_name(launch_domain).c_str(),
                            constraints.clock_domain_name(capture_domain).c_str(),
                            sec_to_nanosec(worst_slack));
                }
            }
        }
    }
    VTR_LOG("\n");

    /* If the terminate_if_timing fails option is on, this checks if slack is negative and if user wants VPR
     * flow to fail if their design doesn't meet timing constraints. If both conditions are true, the function
     * adds details about the negative slack to a string that will be printed when VPR throws an error.
     */
    if (timing_ctx.terminate_if_timing_fails && (hold_worst_neg_slack < 0 || hold_total_neg_slack < 0) && prefix == "Final ") {
        std::string msg = "\nDesign did not meet timing constraints.\nTiming failed and terminate_if_timing_fails set -- exiting";
        VPR_FATAL_ERROR(VPR_ERROR_TIMING, msg.c_str());
    }
}

/*
 * General utilities
 */
std::map<tatum::DomainId, size_t> count_clock_fanouts(const tatum::TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& setup_analyzer) {
    std::map<tatum::DomainId, size_t> fanouts;
    for (tatum::NodeId node : timing_graph.nodes()) {
        tatum::NodeType type = timing_graph.node_type(node);
        if (type == tatum::NodeType::SOURCE || type == tatum::NodeType::SINK) {
            for (auto tag : setup_analyzer.setup_tags(node, tatum::TagType::DATA_ARRIVAL)) {
                fanouts[tag.launch_clock_domain()] += 1;
            }
            for (auto tag : setup_analyzer.setup_tags(node, tatum::TagType::DATA_REQUIRED)) {
                fanouts[tag.launch_clock_domain()] += 1;
            }
        }
    }

    return fanouts;
}

/*
 * Criticalities and setup slacks calculation utilities
 */

/**
 * @brief Returns the criticality of a net's pin in the CLB netlist.
 *        Assumes that the timing graph is correct and up to date.
 */
float calculate_clb_net_pin_criticality(const SetupTimingInfo& timing_info,
                                        const ClusteredPinAtomPinsLookup& pin_lookup,
                                        const ParentPinId pin_id,
                                        bool is_flat) {
    float pin_crit = 0.;
    if (is_flat) {
        pin_crit = timing_info.setup_pin_criticality(convert_to_atom_pin_id(pin_id));
    } else {
        //There may be multiple atom netlist pins connected to this CLB pin
        for (const AtomPinId atom_pin : pin_lookup.connected_atom_pins(convert_to_cluster_pin_id(pin_id))) {
            //Take the maximum of the atom pin criticality as the CLB pin criticality
            pin_crit = std::max(pin_crit, timing_info.setup_pin_criticality(atom_pin));
        }
    }

    return pin_crit;
}

/**
 * @brief Returns the raw setup slack of a net's pin in the CLB netlist.
 *        Assumes that the timing graph is correct and up to date.
 */
float calculate_clb_net_pin_setup_slack(const SetupTimingInfo& timing_info, const ClusteredPinAtomPinsLookup& pin_lookup, ClusterPinId clb_pin) {
    //There may be multiple atom netlist pins connected to this CLB pin
    float clb_pin_setup_slack = std::numeric_limits<float>::infinity();
    for (const auto atom_pin : pin_lookup.connected_atom_pins(clb_pin)) {
        //Take the worst/minimum of the atom pin slack as the CLB pin slack
        clb_pin_setup_slack = std::min(clb_pin_setup_slack, timing_info.setup_pin_slack(atom_pin));
    }

    return clb_pin_setup_slack;
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

float calc_relaxed_criticality(const std::map<DomainPair, float>& domains_max_req,
                               const std::map<DomainPair, float>& domains_worst_slack,
                               const tatum::TimingTags::tag_range tags) {
    //Allowable round-off tolerance during criticality calculation
    constexpr float CRITICALITY_ROUND_OFF_TOLERANCE = 5e-4;

    //Record the maximum criticality over all the tags
    float max_crit = 0.;
    for (const auto& tag : tags) {
        VTR_ASSERT_MSG(tag.type() == tatum::TagType::SLACK, "Tags must be slacks to calculate criticality");

        float slack = tag.time().value();

        auto domain_pair = DomainPair(tag.launch_clock_domain(), tag.capture_clock_domain());

        auto iter = domains_max_req.find(domain_pair);
        VTR_ASSERT_MSG(iter != domains_max_req.end(), "Require the maximum required time for clock domain pair");
        float max_req = iter->second;

        iter = domains_worst_slack.find(domain_pair);
        VTR_ASSERT_MSG(iter != domains_worst_slack.end(), "Require the worst slack for clock domain pair");
        float worst_slack = iter->second;

        VTR_ASSERT_SAFE_MSG(!std::isnan(worst_slack), "Worst slack should not be nan");
        VTR_ASSERT_SAFE_MSG(std::isfinite(worst_slack), "Worst slack should not be infinite");

        if (worst_slack < 0.) {
            //We shift slacks and required time by the most negative slack
            //**in the domain**, to ensure criticality is bounded within [0., 1.]
            //
            //This corresponds to the 'relaxed' criticality from Wainberg et. al.
            float shift = -worst_slack;
            VTR_ASSERT(shift > 0.);

            slack += shift;
            max_req += shift;
        }
        VTR_ASSERT_SAFE_MSG(!std::isnan(slack), "Slack should not be nan");
        VTR_ASSERT_SAFE_MSG(!std::isnan(max_req), "Max required time should not be nan");
        VTR_ASSERT_SAFE_MSG(std::isfinite(slack), "Slack should not be infinite");
        VTR_ASSERT_SAFE_MSG(std::isfinite(max_req), "Max required time should not be infinite");

        float crit = std::numeric_limits<float>::quiet_NaN();
        if (max_req > 0.) {
            //Standard case
            crit = 1. - (slack / max_req);

        } else if (max_req == 0. && slack == 0.) {
            //Special case to avoid divide by zero
            crit = 1.;
        } else {
            std::string msg = vtr::string_fmt("Invalid maximum required time %g (expected >= 0). Shifted slack was %g.", max_req, slack);
            VPR_ERROR(VPR_ERROR_TIMING, msg.c_str());
        }

        //Soft check for reasonable criticality values
        VTR_ASSERT_SAFE_MSG(!std::isnan(crit), "Criticality not be nan");
        VTR_ASSERT_SAFE_MSG(std::isfinite(crit), "Criticality should not be infinite");
        VTR_ASSERT_MSG(crit >= 0. - CRITICALITY_ROUND_OFF_TOLERANCE, "Criticality should never be negative");
        VTR_ASSERT_MSG(crit <= 1. + CRITICALITY_ROUND_OFF_TOLERANCE, "Criticality should never be greater than one");

        //Clamp criticality to [0., 1.] to correct round-off
        crit = std::max(0.f, crit);
        crit = std::min(1.f, crit);

        max_crit = std::max(max_crit, crit);
    }
    VTR_ASSERT_MSG(max_crit >= 0., "Criticality should never be negative");
    VTR_ASSERT_MSG(max_crit <= 1., "Criticality should never be greater than one");

    return max_crit;
}

void print_tatum_cpds(const std::vector<tatum::TimingPathInfo>& cpds) {
    for (auto path : cpds) {
        VTR_LOG("Tatum   %zu -> %zu: least_slack=%g cpd=%g\n", size_t(path.launch_domain()), size_t(path.capture_domain()), float(path.slack()), float(path.delay()));
    }
}

tatum::NodeId id_or_pin_name_to_tnode(const std::string& pin_name_or_tnode) {
    std::istringstream ss(pin_name_or_tnode);
    int id;
    if (ss >> id) { //Successfully converted

        if (id < 0) {
            return tatum::NodeId::INVALID();
        } else {
            return tatum::NodeId(id);
        }
    }

    return pin_name_to_tnode(pin_name_or_tnode);
}

tatum::NodeId pin_name_to_tnode(const std::string& pin_name) {
    auto& atom_ctx = g_vpr_ctx.atom();

    AtomPinId pin = atom_ctx.netlist().find_pin(pin_name);

    if (!pin) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Failed to find pin named '%s'\n", pin_name.c_str());
    }

    tatum::NodeId tnode = atom_ctx.lookup().atom_pin_tnode(pin);

    if (!tnode) {
        VPR_THROW(VPR_ERROR_TIMING, "Failed to find tnode for pin '%s' (pin: %zu)\n", pin_name.c_str(), size_t(pin));
    }

    return tnode;
}

void write_setup_timing_graph_dot(const std::string& filename, const SetupTimingInfo& timing_info, tatum::NodeId debug_node) {
    auto& timing_graph = *timing_info.timing_graph();

    auto dot_writer = tatum::make_graphviz_dot_writer(timing_graph, *timing_info.delay_calculator());

    std::vector<tatum::NodeId> nodes;
    if (debug_node) {
        //Transitive fanin/fanout
        nodes = tatum::find_transitively_connected_nodes(timing_graph, {debug_node});
    } else {
        //All
        auto tg_nodes = timing_graph.nodes();
        nodes = std::vector<tatum::NodeId>(tg_nodes.begin(), tg_nodes.end());
    }
    dot_writer.set_nodes_to_dump(nodes);

    dot_writer.write_dot_file(filename, *timing_info.setup_analyzer());
}

void write_hold_timing_graph_dot(const std::string& filename, HoldTimingInfo& timing_info, tatum::NodeId debug_node) {
    auto& timing_graph = *timing_info.timing_graph();

    auto dot_writer = tatum::make_graphviz_dot_writer(timing_graph, *timing_info.delay_calculator());

    std::vector<tatum::NodeId> nodes;
    if (debug_node) {
        //Transitive fanin/fanout
        nodes = tatum::find_transitively_connected_nodes(timing_graph, {debug_node});
    } else {
        //All
        auto tg_nodes = timing_graph.nodes();
        nodes = std::vector<tatum::NodeId>(tg_nodes.begin(), tg_nodes.end());
    }
    dot_writer.set_nodes_to_dump(nodes);

    dot_writer.write_dot_file(filename, *timing_info.hold_analyzer());
}
