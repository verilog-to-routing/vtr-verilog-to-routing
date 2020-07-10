#include <iostream>
#include <memory>

#include "verify.hpp"
#include "tatum/tags/TimingTags.hpp"
#include "tatum/tags/TimingTag.hpp"
#include "tatum/timing_analyzers.hpp"
#include "tatum/report/graphviz_dot_writer.hpp"
#include "util.hpp"

using namespace tatum;

using std::cout;
using std::endl;

constexpr float RELATIVE_EPSILON = 1.e-5;
constexpr float ABSOLUTE_EPSILON = 1.e-13;

//Verify against golden reference results
std::pair<size_t,bool> verify_node_tags(const NodeId node, TimingTags::tag_range analyzer_tags, const std::map<std::pair<DomainId,DomainId>,TagResult>& ref_results, std::string type);
bool verify_tag(const TimingTag& tag, const TagResult& ref_result, std::string type);

//Verify against another analyzer
std::pair<size_t,bool> verify_node_tags(const NodeId node, TimingTags::tag_range check_tags, TimingTags::tag_range ref_tags, std::string type);
bool verify_tag(const TimingTag& ref_tag, const TimingTag& check_tag, NodeId node, std::string type);

bool verify_time(NodeId node, DomainId launch_domain, DomainId capture_domain, float analyzer_time, float reference_time, std::string type);

std::pair<size_t,bool> verify_analyzer(const TimingGraph& tg, std::shared_ptr<TimingAnalyzer> analyzer, GoldenReference& gr) {
    bool error = false;
    size_t tags_checked = 0;

    auto setup_analyzer = std::dynamic_pointer_cast<SetupTimingAnalyzer>(analyzer);
    auto hold_analyzer = std::dynamic_pointer_cast<HoldTimingAnalyzer>(analyzer);

    for(LevelId level : tg.levels()) {

        for(NodeId node : tg.level_nodes(level)) {


            if(setup_analyzer) {
                auto res = verify_node_tags(node, setup_analyzer->setup_tags(node, TagType::DATA_ARRIVAL), gr.get_result(node, tatumparse::TagType::SETUP_DATA_ARRIVAL), "setup_data_arrival");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, setup_analyzer->setup_tags(node, TagType::DATA_REQUIRED), gr.get_result(node, tatumparse::TagType::SETUP_DATA_REQUIRED), "setup_data_required");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, setup_analyzer->setup_tags(node, TagType::CLOCK_LAUNCH), gr.get_result(node, tatumparse::TagType::SETUP_LAUNCH_CLOCK), "setup_launch_clock");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, setup_analyzer->setup_tags(node, TagType::CLOCK_CAPTURE), gr.get_result(node, tatumparse::TagType::SETUP_CAPTURE_CLOCK), "setup_capture_clock");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, setup_analyzer->setup_tags(node, TagType::SLACK), gr.get_result(node, tatumparse::TagType::SETUP_SLACK), "setup_slack");
                tags_checked += res.first;
                error |= res.second;
            }
            if(hold_analyzer) {
                auto res = verify_node_tags(node, hold_analyzer->hold_tags(node, TagType::DATA_ARRIVAL), gr.get_result(node, tatumparse::TagType::HOLD_DATA_ARRIVAL), "hold_data_arrival");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, hold_analyzer->hold_tags(node, TagType::DATA_REQUIRED), gr.get_result(node, tatumparse::TagType::HOLD_DATA_REQUIRED), "hold_data_required");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, hold_analyzer->hold_tags(node, TagType::CLOCK_LAUNCH), gr.get_result(node, tatumparse::TagType::HOLD_LAUNCH_CLOCK), "hold_launch_clock");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, hold_analyzer->hold_tags(node, TagType::CLOCK_CAPTURE), gr.get_result(node, tatumparse::TagType::HOLD_CAPTURE_CLOCK), "hold_capture_clock");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, hold_analyzer->hold_tags(node, TagType::SLACK), gr.get_result(node, tatumparse::TagType::HOLD_SLACK), "hold_slack");
                tags_checked += res.first;
                error |= res.second;
            }
        }
    }

    return {tags_checked,!error};
}


std::pair<size_t,bool> verify_node_tags(const NodeId node, TimingTags::tag_range analyzer_tags, const std::map<std::pair<DomainId,DomainId>,TagResult>& ref_results, std::string type) {
    bool error = false;

    //Check that every tag in the analyzer matches the reference results
    size_t tags_verified = 0;
    for(const TimingTag& tag : analyzer_tags) {
        auto iter = ref_results.find({tag.launch_clock_domain(), tag.capture_clock_domain()});
        if(iter == ref_results.end()) {
            cout << "Node: " << node << " Type: " << type << endl;
            cout << "\tERROR No reference tag found for clock domain pair " << tag.launch_clock_domain() << ", " << tag.capture_clock_domain() << endl;
            error = true;
        } else {
            if(!verify_tag(tag, iter->second, type)) {
                error = true;
            }
            ++tags_verified;
        /*
         *} else {
         *    cout << "ERROR Failed tag verification!" << endl;
         *    error = true;
         */
        }
    }

    //See if there were any reference results we did not check
    if(tags_verified != ref_results.size()) {
        cout << "Node: " << node << " Type: " << type << endl;
        cout << "\tERROR Tags checked (" << tags_verified << ") does not match number of reference results (" << ref_results.size() << ")" << endl;

        cout << "\t\tCalc Tags:" << endl;
        for(const TimingTag& tag : analyzer_tags) {
            cout << "\t\t\tTag " << tag.launch_clock_domain() << " to " << tag.capture_clock_domain() << " time: " << tag.time().value() << endl;
        }

        cout << "\t\tRef Tags:" << endl;
        for(const auto& kv : ref_results) {
            const auto& ref = kv.second;
            cout << "\t\t\tTag " << ref.launch_domain << " to " << ref.capture_domain << " time: " << ref.time << endl;
        }
        error = true;
    }

    return {tags_verified, error};
}

bool verify_tag(const TimingTag& tag, const TagResult& ref_result, std::string type) {
    TATUM_ASSERT(tag.launch_clock_domain() == ref_result.launch_domain && tag.capture_clock_domain() == ref_result.capture_domain);

    bool valid = true;
    valid &= verify_time(ref_result.node, ref_result.launch_domain, ref_result.capture_domain, tag.time().value(), ref_result.time, type);

    return valid;
}


std::pair<size_t,bool> verify_equivalent_analysis(const TimingGraph& tg, const DelayCalculator& dc, std::shared_ptr<TimingAnalyzer> ref_analyzer,  std::shared_ptr<TimingAnalyzer> check_analyzer) {
    bool error = false;
    size_t tags_checked = 0;

    auto setup_ref_analyzer = std::dynamic_pointer_cast<SetupTimingAnalyzer>(ref_analyzer);
    auto hold_ref_analyzer = std::dynamic_pointer_cast<HoldTimingAnalyzer>(ref_analyzer);
    auto setup_check_analyzer = std::dynamic_pointer_cast<SetupTimingAnalyzer>(check_analyzer);
    auto hold_check_analyzer = std::dynamic_pointer_cast<HoldTimingAnalyzer>(check_analyzer);

    auto dot_writer = make_graphviz_dot_writer(tg, dc);

    NodeId error_node;


    for(LevelId level : tg.levels()) {

        for(NodeId node : tg.level_nodes(level)) {


            if(setup_ref_analyzer) {
                TATUM_ASSERT(setup_check_analyzer);
                auto res = verify_node_tags(node, setup_check_analyzer->setup_tags(node, TagType::DATA_ARRIVAL), setup_ref_analyzer->setup_tags(node, TagType::DATA_ARRIVAL), "setup_data_arrival");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, setup_check_analyzer->setup_tags(node, TagType::DATA_REQUIRED), setup_ref_analyzer->setup_tags(node, TagType::DATA_REQUIRED), "setup_data_required");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, setup_check_analyzer->setup_tags(node, TagType::CLOCK_LAUNCH), setup_ref_analyzer->setup_tags(node, TagType::CLOCK_LAUNCH), "setup_launch_clock");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, setup_check_analyzer->setup_tags(node, TagType::CLOCK_CAPTURE), setup_ref_analyzer->setup_tags(node, TagType::CLOCK_CAPTURE), "setup_capture_clock");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, setup_check_analyzer->setup_slacks(node), setup_ref_analyzer->setup_slacks(node), "setup_slack");
                tags_checked += res.first;
                error |= res.second;
            }

            if(hold_ref_analyzer) {
                TATUM_ASSERT(hold_check_analyzer);

                auto res = verify_node_tags(node, hold_check_analyzer->hold_tags(node, TagType::DATA_ARRIVAL), hold_ref_analyzer->hold_tags(node, TagType::DATA_ARRIVAL), "hold_data_arrival");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, hold_check_analyzer->hold_tags(node, TagType::DATA_REQUIRED), hold_ref_analyzer->hold_tags(node, TagType::DATA_REQUIRED), "hold_data_required");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, hold_check_analyzer->hold_tags(node, TagType::CLOCK_LAUNCH), hold_ref_analyzer->hold_tags(node, TagType::CLOCK_LAUNCH), "hold_launch_clock");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, hold_check_analyzer->hold_tags(node, TagType::CLOCK_CAPTURE), hold_ref_analyzer->hold_tags(node, TagType::CLOCK_CAPTURE), "hold_capture_clock");
                tags_checked += res.first;
                error |= res.second;

                res = verify_node_tags(node, hold_check_analyzer->hold_slacks(node), hold_ref_analyzer->hold_slacks(node), "hold_slack");
                tags_checked += res.first;
                error |= res.second;
            }



        }
    }

    if (error) {
        std::cout << "Dumping dot\n";
#if 0
        std::vector<tatum::NodeId> nodes = find_transitively_connected_nodes(tg, {NodeId(4630)});
#else
        const auto& tg_nodes = tg.nodes();
        std::vector<tatum::NodeId> nodes(tg_nodes.begin(), tg_nodes.end());
#endif
        dot_writer.set_nodes_to_dump(nodes);

        if (setup_check_analyzer) dot_writer.write_dot_file("check_tg_setup_annotated.dot", *setup_check_analyzer);
        if (setup_ref_analyzer) dot_writer.write_dot_file("ref_tg_setup_annotated.dot", *setup_ref_analyzer);
        if (hold_check_analyzer) dot_writer.write_dot_file("check_tg_hold_annotated.dot", *hold_check_analyzer);
        if (hold_ref_analyzer) dot_writer.write_dot_file("ref_tg_hold_annotated.dot", *hold_ref_analyzer);
    }

    return {tags_checked,!error};
}

std::pair<size_t,bool> verify_node_tags(const NodeId node, TimingTags::tag_range check_tags, TimingTags::tag_range ref_tags, std::string type) {
    bool error = false;

    //Check that every tag in the analyzer matches the reference results
    size_t tags_verified = 0;
    for(const TimingTag& ref_tag : ref_tags) {

        auto match_tag = [&](const TimingTag& tag) {
            return ref_tag.launch_clock_domain() == tag.launch_clock_domain() 
                   && ref_tag.capture_clock_domain() == tag.capture_clock_domain();
        };

        auto iter = std::find_if(check_tags.begin(), check_tags.end(), match_tag);

        if(iter == check_tags.end()) {
            cout << "Node: " << node << " Type: " << type << endl;
            cout << "\tERROR No check tag found for clock domain pair " << ref_tag.launch_clock_domain() << ", " << ref_tag.capture_clock_domain() << endl;
            error = true;
        } else {
            if(!verify_tag(*iter, ref_tag, node, type)) {
                error = true;
            }
            ++tags_verified;
        }
    }

    //See if there were any reference results we did not check
    if(tags_verified != ref_tags.size()) {
        cout << "Node: " << node << " Type: " << type << endl;
        cout << "\tERROR Tags checked (" << tags_verified << ") does not match number of reference tags (" << ref_tags.size() << ")" << endl;

        cout << "\t\tCalc Tags:" << endl;
        for(const TimingTag& tag : check_tags) {
            cout << "\t\t\tTag " << tag.launch_clock_domain() << " to " << tag.capture_clock_domain() << " time: " << tag.time().value() << endl;
        }

        cout << "\t\tRef Tags:" << endl;
        for(const TimingTag& tag : ref_tags) {
            cout << "\t\t\tTag " << tag.launch_clock_domain() << " to " << tag.capture_clock_domain() << " time: " << tag.time().value() << endl;
        }
        error = true;
    }

    return {tags_verified, error};
}

bool verify_tag(const TimingTag& check_tag, const TimingTag& ref_tag, const NodeId node, std::string type) {
    TATUM_ASSERT(ref_tag.launch_clock_domain() == check_tag.launch_clock_domain()
                 && ref_tag.capture_clock_domain() == check_tag.capture_clock_domain());

    bool valid = true;
    valid &= verify_time(node, check_tag.launch_clock_domain(), check_tag.capture_clock_domain(), ref_tag.time().value(), check_tag.time().value(), type);

    return valid;
}

bool verify_time(NodeId node, DomainId launch_domain, DomainId capture_domain, float analyzer_time, float reference_time, std::string type) {
    float arr_abs_err = fabs(analyzer_time - reference_time);
    float arr_rel_err = relative_error(analyzer_time, reference_time);
    if(std::isnan(analyzer_time) && (std::isnan(analyzer_time) != std::isnan(reference_time))) {
        cout << "Node: " << node << " Launch Clk: " << launch_domain << " Capture Clk: " << capture_domain;
        cout << " Calc: " << analyzer_time;
        cout << " Ref: " << reference_time << endl;
        cout << "\tERROR Calculated time was nan and didn't match golden reference." << endl;
        return false;
    } else if (!std::isnan(analyzer_time) && std::isnan(reference_time)) {
        //We allow tatum results to be non-NAN when reference is NAN
        //
        //This occurs in some cases (such as applying clock tags to primary outputs)
        //which are cuased by the differeing analysis methods
    } else if ((std::isnan(analyzer_time) && std::isnan(reference_time)) //Both NaN
               || (std::isinf(analyzer_time) && std::isinf(reference_time) //Both inf with same sign
                   && (std::signbit(analyzer_time) == std::signbit(reference_time)))) {
        //They agree, pass
    } else if(arr_rel_err > RELATIVE_EPSILON && arr_abs_err > ABSOLUTE_EPSILON) {
        cout << "Node: " << node << " " << type << " Launch Clk: " << launch_domain << " Capture Clk: " << capture_domain;
        cout << " Calc: " << analyzer_time;
        cout << " Ref: " << reference_time << endl;
        cout << "\tERROR time abs, rel errs: " << arr_abs_err;
        cout << ", " << arr_rel_err << endl;
        return false;
    } else {
        TATUM_ASSERT(!std::isnan(arr_rel_err) && !std::isnan(arr_abs_err));
        TATUM_ASSERT(arr_rel_err < RELATIVE_EPSILON || arr_abs_err < ABSOLUTE_EPSILON);
    }
    return true;
}
