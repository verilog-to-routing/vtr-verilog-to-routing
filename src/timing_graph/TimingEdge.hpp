#ifndef TIMINGEDGE_HPP
#define TIMINGEDGE_HPP
#include <vector>

#include "Time.hpp"

class TimingEdge {
    public:
        int to_node() const {return to_node_;}
        void set_to_node(int node) {to_node_ = node;}

        Time delay() const {return delay_;}
        void set_delay(Time delay_val) {delay_ = delay_val;}

        void print() {
            printf("    Edge to %d, Delay %g\n", to_node_, delay_.value());
        }
    private:
        int to_node_;
        Time delay_;
};

#endif
