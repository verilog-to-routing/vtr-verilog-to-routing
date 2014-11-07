#ifndef TIMINGEDGE_HPP
#define TIMINGEDGE_HPP
#include "Time.hpp"

class TimingEdge {
    public:
        int to_node() const {return to_node_;}
        void set_to_node(tnode_id_t node) {to_node_ = node;}

        Time delay() const {return delay_;}
        void set_delay(Time delay_val) {delay_ = delay_val;}
    private:
        int to_node_;
        Time delay_;
};

#endif
