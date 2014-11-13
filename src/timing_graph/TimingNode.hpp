#ifndef TIMINGNODE_HPP
#define TIMINGNODE_HPP
#include <vector>

#include "TimingEdge.hpp"
#include "Time.hpp"

enum class TN_Type {
	INPAD_SOURCE, //Input to an input I/O pad 
	INPAD_OPIN, //Output from an input I/O pad 
	OUTPAD_IPIN, //Input to an output I/O pad 
	OUTPAD_SINK, //Output from an output I/O pad 
	PRIMITIVE_IPIN, //Input pin to a primitive (e.g. a LUT) 
	PRIMITIVE_OPIN, //Output pin from a primitive (e.g. a LUT) 
	FF_IPIN, //Input pin to a flip-flop - goes to FF_SINK 
	FF_OPIN, //Output pin from a flip-flop - comes from FF_SOURCE 
	FF_SINK, //Sink (D) pin of flip-flop 
	FF_SOURCE, //Source (Q) pin of flip-flop 
	FF_CLOCK, //Clock pin of flip-flop 
    CLOCK_SOURCE, //An on-chip clock generator such as a pll 
    CLOCK_OPIN, //Output pin from an on-chip clock source - comes from CLOCK_SOURCE 
	CONSTANT_GEN_SOURCE, //Source of a constant logic 1 or 0 
    UNKOWN //Unrecognized type, this is almost certainly an error
};

std::ostream& operator<<(std::ostream& os, const TN_Type type);
std::istream& operator>>(std::istream& os, TN_Type& type);

class TimingNode {
    public:
        TimingNode() {}
        TimingNode(TN_Type new_type): type_(new_type) {}

        int num_out_edges() const { return out_edges_.size(); }

        TimingEdge& out_edge(int idx) { return out_edges_[idx]; }
        const TimingEdge& out_edge(int idx) const { return out_edges_[idx]; }
        void add_out_edge(const TimingEdge& edge) { out_edges_.push_back(edge); }

        TN_Type type() const { return type_; }

        Time arrival_time() const { return T_arr_; }
        void set_arrival_time(Time new_arrival_time) { T_arr_ = new_arrival_time; }

        Time required_time() const { return T_req_; }
        void set_required_time(Time new_required_time) { T_req_ = new_required_time; }

        void print() {
            for(auto& edge : out_edges_) {
                edge.print();
            }
        }
    private:
        Time T_arr_; //Data arrival time at this node
        Time T_req_; //Data required arrival time at this node
        Time T_clock_delay_; //Clock arrival time at this node, only valid for FF types

        TN_Type type_;

        std::vector<TimingEdge> out_edges_; //Timing edges driven by this node

};

#endif
