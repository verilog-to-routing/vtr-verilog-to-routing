#pragma once
#include <array>
#include <algorithm>
#include <cassert>

#include "timing_graph_fwd.hpp"
#include "Time.hpp"

typedef int TagId;

#define INVALID_TAG_ID -1
#define NUM_FLAT_TAGS 1

class TimingTags {
    public:
        //Getters
        TagId num_tags() const;
        const Time& time(TagId tag_id) const;
        DomainId clock_domain(TagId tag_id) const;
        NodeId launch_node(TagId tag_id) const;

        //Setters
        void set_time(TagId tag_id, const Time& new_time);
        void set_clock_domain(TagId tag_id, const DomainId new_clock_domain);
        void set_launch_node(TagId tag_id, const NodeId new_launch_node);

        //Modifiers
        void add_tag(const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node);
        void set_tag(TagId tag_id, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node);
        void max_tag(const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node);
        void min_tag(const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node);
        void clear();


    private:
        TagId find_clock_domain_tag(DomainId domain_id);

        //Struct of arrays layout

        //In the general case we expect very few tags per node
        //as a memory layout optimization if we have fewer than 
        //NUM_FLAT_TAGS, those tags are stored in the object
        //those beyond are stored in externally allocated memory
        TagId num_tags_;
        std::array<Time,NUM_FLAT_TAGS> local_times_;
        std::array<DomainId,NUM_FLAT_TAGS> local_clock_domains_;
        std::array<NodeId,NUM_FLAT_TAGS> local_launch_nodes_;

        std::vector<Time> external_times_;
        std::vector<DomainId> external_clock_domains_;
        std::vector<NodeId> external_launch_nodes_;
};
