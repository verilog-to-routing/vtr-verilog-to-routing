#include <boost/pool/object_pool.hpp>

#include "TimingTags.hpp"


//Modifiers
void TimingTags::add_tag(boost::pool<>& tag_pool, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {
    //Don't add invalid clock domains
    //Some sources like constant generators may yeild illegal clock domains
    if(new_clock_domain == INVALID_CLOCK_DOMAIN) {
        return;
    }

    if(num_tags_ == 0) {
        //Store it as the head
        head_tag_ = TimingTag(new_time, new_clock_domain, new_launch_node);
    } else {
        //Store it in a linked list from head

        //Allocate form a central storage pool
        assert(tag_pool.get_requested_size() == sizeof(TimingTag)); //Make sure the pool is the correct size
        TimingTag* tag = new(tag_pool.malloc()) TimingTag(new_time, new_clock_domain, new_launch_node);

        //Insert one-after head in O(1) time
        TimingTag* next_tag = head_tag_.next(); //Save next link (may be nullptr)
        head_tag_.set_next(tag); //Tag is now second in list
        tag->set_next(next_tag); //Attach tail
    }
    num_tags_++;
}

void TimingTags::max_tag(boost::pool<>& tag_pool, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {
    TimingTagIterator iter = find_tag_by_clock_domain(new_clock_domain);
    if(iter == end()) { 
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

void TimingTags::min_tag(boost::pool<>& tag_pool, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {
    TimingTagIterator iter = find_tag_by_clock_domain(new_clock_domain);
    if(iter == end()) { 
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
    num_tags_ = 0;
}

TimingTagIterator TimingTags::find_tag_by_clock_domain(DomainId domain_id) {
    auto pred = [domain_id](const TimingTag& tag){
        return tag.clock_domain() == domain_id;
    };

    return std::find_if(begin(), end(), pred);
}
