#pragma once

#include "Time.hpp"
#include "timing_graph_fwd.hpp"

class TimingTag {
    public:
        TimingTag(const Time& new_value, const NodeId new_launch_node, const DomainId new_clock_domain)
            : value_(new_value)
            , clock_domain_(new_clock_domain)
            , launch_node_(new_launch_node)
        {}

        //Getters
        const Time& value() const { return value_; }
        DomainId clock_domain() const { return clock_domain_; }
        NodeId launch_node() const { return launch_node_; }

        //Setters
        void set_value(const Time& new_value) { value_ = new_value; }
        void set_clock_domain(const DomainId new_clock_domain) { clock_domain_ = new_clock_domain; }
        void set_launch_node(const NodeId new_launch_node) { launch_node_ = new_launch_node; }

        //Modifiers
        void max(const TimingTag& other) {
            if(other.value().value() > value_.value()) {
                //Other is larger so update ourselves
                *this = other;
            }
        }
        void min(const TimingTag& other) {
            if(other.value().value() < value_.value()) {
                //Other is smaller so update ourselves
                *this = other;
            }
        }


    private:
        Time value_;
        DomainId clock_domain_;
        NodeId launch_node_;
};

inline TimingTag operator+(TimingTag tag, const Time& delay) {
    tag.set_value(tag.value() + delay); 
    return tag;
}
inline TimingTag operator-(TimingTag tag, const Time& delay) {
    tag.set_value(tag.value() - delay); 
    return tag;
}
