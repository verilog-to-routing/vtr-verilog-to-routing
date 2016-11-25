#include <iostream>
#include "verify.hpp"
#include "TimingTags.hpp"
#include "TimingTag.hpp"
#include "timing_analyzers.hpp"
#include "util.hpp"

using namespace tatum;

using std::cout;
using std::endl;

constexpr float RELATIVE_EPSILON = 1.e-5;
constexpr float ABSOLUTE_EPSILON = 1.e-13;

size_t verify_node_tags(const NodeId node, TimingTags::tag_range analyzer_tags, const std::map<DomainId,TagResult>& ref_results, std::string type);
bool verify_tag(const TimingTag& tag, const TagResult& ref_result);
bool verify_time(NodeId node, DomainId domain, float analyzer_time, float reference_time);

size_t verify_analyzer(const TimingGraph& tg, std::shared_ptr<TimingAnalyzer> analyzer, GoldenReference& gr) {
    size_t tags_checked = 0;

    auto setup_analyzer = std::dynamic_pointer_cast<SetupTimingAnalyzer>(analyzer);
    auto hold_analyzer = std::dynamic_pointer_cast<HoldTimingAnalyzer>(analyzer);

    for(LevelId level : tg.levels()) {

        for(NodeId node : tg.level_nodes(level)) {


            if(setup_analyzer) {
                tags_checked += verify_node_tags(node, setup_analyzer->setup_tags(node, TagType::DATA_ARRIVAL), gr.get_result(node, tatumparse::TagType::SETUP_DATA_ARRIVAL), "setup_data_arrival");
                tags_checked += verify_node_tags(node, setup_analyzer->setup_tags(node, TagType::DATA_REQUIRED), gr.get_result(node, tatumparse::TagType::SETUP_DATA_REQUIRED), "setup_data_required");
                tags_checked += verify_node_tags(node, setup_analyzer->setup_tags(node, TagType::CLOCK_LAUNCH), gr.get_result(node, tatumparse::TagType::SETUP_LAUNCH_CLOCK), "setup_launch_clock");
                tags_checked += verify_node_tags(node, setup_analyzer->setup_tags(node, TagType::CLOCK_CAPTURE), gr.get_result(node, tatumparse::TagType::SETUP_CAPTURE_CLOCK), "setup_capture_clock");
            }
            if(hold_analyzer) {
                tags_checked += verify_node_tags(node, hold_analyzer->hold_tags(node, TagType::DATA_ARRIVAL), gr.get_result(node, tatumparse::TagType::HOLD_DATA_ARRIVAL), "hold_data_arrival");
                tags_checked += verify_node_tags(node, hold_analyzer->hold_tags(node, TagType::DATA_REQUIRED), gr.get_result(node, tatumparse::TagType::HOLD_DATA_REQUIRED), "hold_data_required");
                tags_checked += verify_node_tags(node, hold_analyzer->hold_tags(node, TagType::CLOCK_LAUNCH), gr.get_result(node, tatumparse::TagType::HOLD_LAUNCH_CLOCK), "hold_launch_clock");
                tags_checked += verify_node_tags(node, hold_analyzer->hold_tags(node, TagType::CLOCK_CAPTURE), gr.get_result(node, tatumparse::TagType::HOLD_CAPTURE_CLOCK), "hold_capture_clock");
            }
        }
    }

    return tags_checked;
}

size_t verify_node_tags(const NodeId node, TimingTags::tag_range analyzer_tags, const std::map<DomainId,TagResult>& ref_results, std::string type) {

    //Check that every tag in the analyzer matches the reference results
    size_t tags_verified = 0;
    for(const TimingTag& tag : analyzer_tags) {
        auto iter = ref_results.find(tag.clock_domain());
        if(iter == ref_results.end()) {
            cout << "Node: " << node << " Type: " << type << endl;
            cout << "\tERROR No reference tag found for clock domain " << tag.clock_domain() << endl;
            std::exit(1);
        } else if (verify_tag(tag, iter->second)) {
            ++tags_verified;
        } else {
            cout << "ERROR Failed tag verification!" << endl;
            std::exit(1);
        }
    }

    //See if there were any reference results we did not check
    if(tags_verified != ref_results.size()) {
        cout << "Node: " << node << " Type: " << type << endl;
        cout << "\tERROR Tags checked (" << tags_verified << ") does not match number of reference results (" << ref_results.size() << ")" << endl;

        cout << "\t\tCalc Tags:" << endl;
        for(const TimingTag& tag : analyzer_tags) {
            cout << "\t\t\tTag " << tag.clock_domain() << " time: " << tag.time().value() << endl;
        }

        cout << "\t\tRef Tags:" << endl;
        for(const auto& kv : ref_results) {
            const auto& ref = kv.second;
            cout << "\t\t\tTag " << ref.domain << " time: " << ref.time << endl;
        }
        std::exit(1);
    }

    return tags_verified;
}

bool verify_tag(const TimingTag& tag, const TagResult& ref_result) {
    TATUM_ASSERT(tag.clock_domain() == ref_result.domain);

    bool valid = true;
    valid &= verify_time(ref_result.node, ref_result.domain, tag.time().value(), ref_result.time);

    return valid;
}

bool verify_time(NodeId node, DomainId domain, float analyzer_time, float reference_time) {
    float arr_abs_err = fabs(analyzer_time - reference_time);
    float arr_rel_err = relative_error(analyzer_time, reference_time);
    if(isnan(analyzer_time) && isnan(analyzer_time) != isnan(reference_time)) {
        cout << "Node: " << node << " Clk: " << domain;
        cout << " Calc: " << analyzer_time;
        cout << " Ref: " << reference_time << endl;
        cout << "\tERROR Calculated time was nan and didn't match golden reference." << endl;
        return false;
    } else if (!isnan(analyzer_time) && isnan(reference_time)) {
        //We allow tatum results to be non-NAN when VPR is NAN
        //
        //This occurs in some cases (such as applying clock tags to primary outputs)
        //which are cuased by the differeing analysis methods
    } else if (isnan(analyzer_time) && isnan(reference_time)) {
        //They agree, pass
    } else if(arr_rel_err > RELATIVE_EPSILON && arr_abs_err > ABSOLUTE_EPSILON) {
        cout << "Node: " << node << " Clk: " << domain;
        cout << " Calc_: " << analyzer_time;
        cout << " Ref_: " << reference_time << endl;
        cout << "\tERROR time abs, rel errs: " << arr_abs_err;
        cout << ", " << arr_rel_err << endl;
        return false;
    } else {
        TATUM_ASSERT(!isnan(arr_rel_err) && !isnan(arr_abs_err));
        TATUM_ASSERT(arr_rel_err < RELATIVE_EPSILON || arr_abs_err < ABSOLUTE_EPSILON);
    }
    return true;
}
