#ifndef VPR_CLB_DELAY_CALC_H
#define VPR_CLB_DELAY_CALC_H

#include "netlist.h"

class CachingClbDelayCalc; //Forward declaration

//Delay Calculator for routing internal to a clustered logic block
class ClbDelayCalc {
    public:
        ClbDelayCalc();

        //Delay from a CLB input (driven outside the CLB) to specified internal sink pin
        float clb_input_to_internal_sink_delay(const t_net_pin* clb_input_pin, int internal_sink_pin) const;

        //Delay from an internal driver to the CLB's output pin
        float internal_src_to_clb_output_delay(int internal_src_pin, const t_net_pin* clb_output_pin) const;

        //Delay from an internal driver to an internal sink within the same CLB
        float internal_src_to_internal_sink_delay(int clb, int internal_src_pin, int internal_sink_pin) const;

        //Delay of a route-through connection from a CLB input to output
        float clb_input_to_clb_output_delay(const t_net_pin* clb_input_pin, const t_net_pin* clb_output_pin) const;

    private:
        friend CachingClbDelayCalc;
        float trace_max_delay(int clb, int src_pb_route_pin, int sink_pb_route_pin) const;

        float pb_route_max_delay(int clb, int pb_route_idx) const;
        const t_pb_graph_edge* find_pb_graph_edge(int clb_block, int pb_route_idx) const;
        const t_pb_graph_edge* find_pb_graph_edge(const t_pb_graph_pin* driver, const t_pb_graph_pin* sink) const;

    private:
        IntraLbPbPinLookup intra_lb_pb_pin_lookup_;
};

#include "clb_delay_calc.inl"

#endif
