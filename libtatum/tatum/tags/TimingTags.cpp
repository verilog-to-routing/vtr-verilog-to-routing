#include "tatum/tags/TimingTags.hpp"
#include "tatum/TimingGraphFwd.hpp"
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

}
