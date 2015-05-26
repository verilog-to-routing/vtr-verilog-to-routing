#include "verify.hpp"
#include <iostream>
#include "sta_util.hpp"

#define RELATIVE_EPSILON 1.e-5
#define ABSOLUTE_EPSILON 1.e-13

using std::cout;
using std::endl;

bool verify_arr_tag(float arr_time, float vpr_arr_time, NodeId node_id, int domain, const std::set<NodeId>& clock_gen_fanout_nodes, std::streamsize num_width) {
    bool error = false;
    float arr_abs_err = fabs(arr_time - vpr_arr_time);
    float arr_rel_err = relative_error(arr_time, vpr_arr_time);
    if(isnan(arr_time) && isnan(arr_time) != isnan(vpr_arr_time)) {
        error = true;
        cout << "Node: " << node_id << " Clk: " << domain;
        cout << " Calc_Arr: " << std::setw(num_width) << arr_time;
        cout << " VPR_Arr: " << std::setw(num_width) << vpr_arr_time << endl;
        cout << "\tERROR Calculated arrival time was nan and didn't match VPR." << endl;
    } else if (!isnan(arr_time) && isnan(vpr_arr_time)) {
        if(clock_gen_fanout_nodes.count(node_id)) {
            //Pass, clock gen fanout can be NAN in VPR but have a value here,
            //since (unlike VPR) we explictly track clock arrivals as tags
        } else {
            error = true;
            cout << "Node: " << node_id << " Clk: " << domain;
            cout << " Calc_Arr: " << std::setw(num_width) << arr_time;
            cout << " VPR_Arr: " << std::setw(num_width) << vpr_arr_time << endl;
            cout << "\tERROR Calculated arrival time was not nan but VPR expected nan." << endl;
        }
    } else if (isnan(arr_time) && isnan(vpr_arr_time)) {
        //They agree, pass
    } else if(arr_rel_err > RELATIVE_EPSILON && arr_abs_err > ABSOLUTE_EPSILON) {
        error = true;
        cout << "Node: " << node_id << " Clk: " << domain;
        cout << " Calc_Arr: " << std::setw(num_width) << arr_time;
        cout << " VPR_Arr: " << std::setw(num_width) << vpr_arr_time << endl;
        cout << "\tERROR arrival time abs, rel errs: " << std::setw(num_width) << arr_abs_err;
        cout << ", " << std::setw(num_width) << arr_rel_err << endl;
    } else {
        VERIFY(!isnan(arr_rel_err) && !isnan(arr_abs_err));
        VERIFY(arr_rel_err < RELATIVE_EPSILON || arr_abs_err < ABSOLUTE_EPSILON);
    }
    return error;
}

bool verify_req_tag(float req_time, float vpr_req_time, NodeId node_id, int domain, const std::set<NodeId>& const_gen_fanout_nodes, std::streamsize num_width) {
    bool error = false;
    float req_abs_err = fabs(req_time - vpr_req_time);
    float req_rel_err = relative_error(req_time, vpr_req_time);
    if(isnan(req_time) && isnan(req_time) != isnan(vpr_req_time)) {
        error = true;
        cout << "Node: " << node_id << " Clk: " << domain;
        cout << " Calc_Req: " << std::setw(num_width) << req_time;
        cout << " VPR_Req: " << std::setw(num_width) << vpr_req_time << endl;
        cout << "\tERROR Calculated required time was nan and didn't match VPR." << endl;
    } else if (!isnan(req_time) && isnan(vpr_req_time)) {
        if (const_gen_fanout_nodes.count(node_id)) {
            //VPR doesn't propagate required times along paths sourced by constant generators
            //but we do, so ignore such errors
#if 0
            cout << "Node: " << node_id << " Clk: " << domain;
            cout << " Calc_Req: " << std::setw(num_width) << req_time;
            cout << " VPR_Req: " << std::setw(num_width) << vpr_req_time << endl;
            cout << "\tOK since " << node_id << " in fanout of Constant Generator" << endl;
#endif
        } else {
            error = true;
            cout << "Node: " << node_id << " Clk: " << domain;
            cout << " Calc_Req: " << std::setw(num_width) << req_time;
            cout << " VPR_Req: " << std::setw(num_width) << vpr_req_time << endl;
            cout << "\tERROR Calculated required time was not nan but VPR expected nan." << endl;
        }

    } else if (isnan(req_time) && isnan(vpr_req_time)) {
        //They agree, pass
    } else if(req_rel_err > RELATIVE_EPSILON && req_abs_err > ABSOLUTE_EPSILON) {
        error = true;
        cout << "Node: " << node_id << " Clk: " << domain;
        cout << " Calc_Req: " << std::setw(num_width) << req_time;
        cout << " VPR_Req: " << std::setw(num_width) << vpr_req_time << endl;
        cout << "\tERROR required time abs, rel errs: " << std::setw(num_width) << req_abs_err;
        cout << ", " << std::setw(num_width) << req_rel_err << endl;
    } else {
        VERIFY(!isnan(req_rel_err) && !isnan(req_abs_err));
        VERIFY(req_rel_err < RELATIVE_EPSILON || req_abs_err < ABSOLUTE_EPSILON);
    }
    return error;
}
