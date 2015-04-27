#pragma once
#include <algorithm>
#include <cassert>

#include "timing_graph_fwd.hpp"

typedef int TagId;

#define INVALID_TAG_ID -1

class TimingTags {
    public:
        //Getters
        TagId num_tags() const { return times_.size(); }
        const Time& time(TagId tag_id) const { return times_[tag_id]; }
        DomainId clock_domain(TagId tag_id) const { return clock_domains_[tag_id]; }
        NodeId launch_node(TagId tag_id) const { return launch_nodes_[tag_id]; }

        //Setters
        void set_time(TagId tag_id, const Time& new_time) { times_[tag_id] = new_time; }
        void set_clock_domain(TagId tag_id, const DomainId new_clock_domain) { clock_domains_[tag_id] = new_clock_domain; }
        void set_launch_node(TagId tag_id, const NodeId new_launch_node) { launch_nodes_[tag_id] = new_launch_node; }

        //Modifiers
        void add_tag(const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {
            //Don't add invalid clock domains
            //Some sources like constant generators may yeild illegal clock domains
            if(new_clock_domain == INVALID_CLOCK_DOMAIN) {
                return;
            }

            times_.push_back(new_time);
            clock_domains_.push_back(new_clock_domain);
            launch_nodes_.push_back(new_launch_node);
        }
        void max_tag(const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {

            TagId tag_idx = find_clock_domain_tag(new_clock_domain);
            if(tag_idx == INVALID_TAG_ID) { 
                //First time we've seen this domain
                add_tag(new_time, new_clock_domain, new_launch_node);
            } else {
                //Need to max with existing value
                if(new_time.value() > times_[tag_idx].value()) {
                    //New value is larger, update max
                    times_[tag_idx] = new_time;
                    assert(clock_domains_[tag_idx] == new_clock_domain); //Domain must be the same
                    launch_nodes_[tag_idx] = new_launch_node;
                }
            }
        }
        void min_tag(const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node) {
            TagId tag_idx = find_clock_domain_tag(new_clock_domain);
            if(tag_idx == INVALID_TAG_ID) { 
                //First time we've seen this domain
                add_tag(new_time, new_clock_domain, new_launch_node);
            } else {
                //Need to max with existing value
                if(new_time.value() < times_[tag_idx].value()) {
                    //New value is smaller, update min
                    times_[tag_idx] = new_time;
                    assert(clock_domains_[tag_idx] == new_clock_domain); //Domain must be the same
                    launch_nodes_[tag_idx] = new_launch_node;
                }
            }
        }

        void clear() {
            times_.clear();
            clock_domains_.clear();
            launch_nodes_.clear();
        }


    private:
        TagId find_clock_domain_tag(DomainId domain_id) {
            auto pred = [domain_id](DomainId id){
                return id == domain_id;
            };
            auto iter = std::find_if(clock_domains_.begin(), clock_domains_.end(), pred);

            if(iter != clock_domains_.end()) {
                //Return the index
                return iter - clock_domains_.begin();
            } else {
                //Not found
                return INVALID_TAG_ID;
            }
        }

        //Struct of arrays layout
        std::vector<Time> times_;
        std::vector<DomainId> clock_domains_;
        std::vector<NodeId> launch_nodes_;
};
