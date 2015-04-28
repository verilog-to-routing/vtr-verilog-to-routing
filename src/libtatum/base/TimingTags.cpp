#include "TimingTags.hpp"

//Getters
TagId TimingTags::num_tags() const {
    return num_tags_; 
}

const Time& TimingTags::time(TagId tag_id) const { 
    if(tag_id < NUM_FLAT_TAGS) {
        return local_times_[tag_id]; 
    } else {
        return external_times_[tag_id - NUM_FLAT_TAGS];
    }
}

DomainId TimingTags::clock_domain(TagId tag_id) const { 
    if(tag_id < NUM_FLAT_TAGS) {
        return local_clock_domains_[tag_id];
    } else {
        return external_clock_domains_[tag_id - NUM_FLAT_TAGS];
    }
}

NodeId TimingTags::launch_node(TagId tag_id) const { 
    if(tag_id < NUM_FLAT_TAGS) {
        return local_launch_nodes_[tag_id]; 
    } else {
        return external_launch_nodes_[tag_id]; 
    }
}

//Setters
void TimingTags::set_time(TagId tag_id, const Time& new_time) { 
    if(tag_id < NUM_FLAT_TAGS) {
        local_times_[tag_id] = new_time; 
    } else {
        external_times_[tag_id - NUM_FLAT_TAGS] = new_time; 
    }
}

void TimingTags::set_clock_domain(TagId tag_id, const DomainId new_clock_domain) { 
    if(tag_id < NUM_FLAT_TAGS) {
        local_clock_domains_[tag_id] = new_clock_domain; 
    } else {
        external_clock_domains_[tag_id] = new_clock_domain; 
    }
}

void TimingTags::set_launch_node(TagId tag_id, const NodeId new_launch_node) { 
    if(tag_id < NUM_FLAT_TAGS) {
        local_launch_nodes_[tag_id] = new_launch_node; 
    } else {
        external_launch_nodes_[tag_id] = new_launch_node; 
    }
}

//Modifiers
void TimingTags::add_tag(const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {
    //Don't add invalid clock domains
    //Some sources like constant generators may yeild illegal clock domains
    if(new_clock_domain == INVALID_CLOCK_DOMAIN) {
        return;
    }

    if(num_tags() < NUM_FLAT_TAGS) {
        local_times_[num_tags()] = new_time;
        local_clock_domains_[num_tags()] = new_clock_domain;
        local_launch_nodes_[num_tags()] = new_launch_node;
    } else {
        external_times_.push_back(new_time);
        external_clock_domains_.push_back(new_clock_domain);
        external_launch_nodes_.push_back(new_launch_node);
    }
    //Added the tag
    num_tags_++;
}

void TimingTags::set_tag(TagId tag_id, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {
    if(tag_id < NUM_FLAT_TAGS) {
        local_times_[tag_id] = new_time;
        local_clock_domains_[tag_id] = new_clock_domain;
        local_launch_nodes_[tag_id] = new_launch_node;
    } else {
        external_times_[tag_id - NUM_FLAT_TAGS] = new_time;
        external_clock_domains_[tag_id - NUM_FLAT_TAGS] = new_clock_domain;
        external_launch_nodes_[tag_id - NUM_FLAT_TAGS] = new_launch_node;
    }
    
}

void TimingTags::max_tag(const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {
    TagId tag_idx = find_clock_domain_tag(new_clock_domain);
    if(tag_idx == INVALID_TAG_ID) { 
        //First time we've seen this domain
        add_tag(new_time, new_clock_domain, new_launch_node);
    } else {
        //Need to max with existing value
        if(new_time.value() > time(tag_idx).value()) {
            //New value is larger
            assert(clock_domain(tag_idx) == new_clock_domain); //Domain must be the same

            //Update max
            set_tag(tag_idx, new_time, new_clock_domain, new_launch_node);
        }
    }
}

void TimingTags::min_tag(const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {
    TagId tag_idx = find_clock_domain_tag(new_clock_domain);
    if(tag_idx == INVALID_TAG_ID) { 
        //First time we've seen this domain
        add_tag(new_time, new_clock_domain, new_launch_node);
    } else {
        //Need to max with existing value
        if(new_time.value() < time(tag_idx).value()) {
            //New value is smaller
            assert(clock_domain(tag_idx) == new_clock_domain); //Domain must be the same

            //Update min
            set_tag(tag_idx, new_time, new_clock_domain, new_launch_node);
        }
    }
}

void TimingTags::clear() {
    num_tags_ = 0;
    external_times_.clear();
    external_clock_domains_.clear();
    external_launch_nodes_.clear();
}

TagId TimingTags::find_clock_domain_tag(DomainId domain_id) {
    auto pred = [domain_id](DomainId id){
        return id == domain_id;
    };
    auto local_iter = std::find_if(local_clock_domains_.begin(), local_clock_domains_.end(), pred);

    if(local_iter != local_clock_domains_.end()) {
        //Found in the local data
        return local_iter - local_clock_domains_.begin();
    } else {
        //Look up in external data
        auto external_iter = std::find_if(external_clock_domains_.begin(), external_clock_domains_.end(), pred);
        if(external_iter != external_clock_domains_.end()) {
            return NUM_FLAT_TAGS + external_iter - external_clock_domains_.begin();
        } else {
            return INVALID_TAG_ID;
        }
    }
}
