#include <boost/pool/object_pool.hpp>

#include "TimingTags.hpp"


//Modifiers
void TimingTags::add_tag(boost::object_pool<TimingTag>& tag_pool, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {
    //Don't add invalid clock domains
    //Some sources like constant generators may yeild illegal clock domains
    if(new_clock_domain == INVALID_CLOCK_DOMAIN) {
        return;
    }

    tags_.push_front(*(tag_pool.construct(TimingTag(new_time, new_clock_domain, new_launch_node))));
    //tags_.push_front(*(new TimingTag(new_time, new_clock_domain, new_launch_node)));
}

void TimingTags::max_tag(boost::object_pool<TimingTag>& tag_pool, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {
    TagList::iterator iter = find_tag_by_clock_domain(new_clock_domain);
    if(iter == tags_.end()) { 
        //First time we've seen this domain
        add_tag(tag_pool, new_time, new_clock_domain, new_launch_node);
    } else {
        TimingTag& matched_tag = *iter;

        //Need to max with existing value
        if(new_time.value() > matched_tag.time().value()) {
            //New value is larger
            //Update max
            matched_tag.set_time(new_time);
            assert(matched_tag.clock_domain() == new_clock_domain); //Domain must be the same
            matched_tag.set_launch_node(new_launch_node);
        }
    }
}

void TimingTags::min_tag(boost::object_pool<TimingTag>& tag_pool, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {
    TagList::iterator iter = find_tag_by_clock_domain(new_clock_domain);
    if(iter == tags_.end()) { 
        //First time we've seen this domain
        add_tag(tag_pool, new_time, new_clock_domain, new_launch_node);
    } else {
        TimingTag& matched_tag = *iter;

        //Need to min with existing value
        if(new_time.value() < matched_tag.time().value()) {
            //New value is smaller
            //Update min
            matched_tag.set_time(new_time);
            assert(matched_tag.clock_domain() == new_clock_domain); //Domain must be the same
            matched_tag.set_launch_node(new_launch_node);
        }
    }
}

void TimingTags::clear() {
    tags_.clear();
}

TimingTags::TagList::iterator TimingTags::find_tag_by_clock_domain(DomainId domain_id) {
    auto pred = [domain_id](TimingTag tag){
        return tag.clock_domain() == domain_id;
    };
    return std::find_if(tags_.begin(), tags_.end(), pred);
}
