#include "tatum/tags/TimingTags.hpp"
#include "tatum/TimingGraphFwd.hpp"
#include "tatum/error.hpp"
namespace tatum {

/*
 * Tag utilities
 */
//Return the tag from the range [first,last) which has the lowest value
TimingTags::const_iterator find_minimum_tag(TimingTags::tag_range tags) {

    return std::min_element(tags.begin(), tags.end(), TimingTagValueComp()); 
}

//Return the tag from the range [first,last) which has the highest value
TimingTags::const_iterator find_maximum_tag(TimingTags::tag_range tags) {

    return std::max_element(tags.begin(), tags.end(), TimingTagValueComp()); 
}

TimingTags::const_iterator find_tag(TimingTags::tag_range tags, 
                                           DomainId launch_domain, 
                                           DomainId capture_domain) {
    for(auto iter = tags.begin(); iter != tags.end(); ++iter) {
        if(iter->launch_clock_domain() == launch_domain && iter->capture_clock_domain() == capture_domain) {
            return iter;
        }
    }

    return tags.end();
}

//Returns true of the specified set of tags would constrain a node of type node_type
bool is_constrained(NodeType node_type, TimingTags::tag_range tags) {
    bool has_clock_launch = false;
    bool has_clock_capture = false;
    bool has_data_required = false;

    for(const auto& tag : tags) {
        if(tag.type() == TagType::CLOCK_LAUNCH) {
            has_clock_launch = true;
        } else if (tag.type() == TagType::CLOCK_CAPTURE) {
            has_clock_capture = true;
        } else if (tag.type() == TagType::DATA_REQUIRED) {
            has_data_required = true;
        }
    }

    bool constrained = false;

    if(node_type == NodeType::SINK) {
        //Constrained data nodes should have required times
        //
        //However, if a clock is generated on-chip (e.g. PLL) it may 
        //be driven off-chip so a primary-output sink may have only 
        //clock launch tags
        constrained = has_data_required || has_clock_launch;    

    } else if(node_type == NodeType::SOURCE) {
        //Constrained data nodes should have required times
        //
        //However, clock sources may have only clock tags
        constrained = has_data_required || (has_clock_launch && has_clock_capture);    

    } else if(node_type == NodeType::IPIN || node_type == NodeType::OPIN) {
        //Required time indicates a datapath node is constrained
        constrained = has_data_required;

    } else if(node_type == NodeType::CPIN) {
        //Clock pins should have both launch and capture to be constrained
        constrained = has_clock_launch && has_clock_capture;
    } else {
        throw Error("Unrecognized node type");
    }

    return constrained;
}

}
