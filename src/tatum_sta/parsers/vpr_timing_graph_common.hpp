#pragma once
#include "assert.hpp"

#include <vector>
#include <cmath>


class TimingGraph; //Forward Declaration


typedef struct domain_skew_iodelay_s {
    int domain;
    float skew;
    float iodelay;
} domain_skew_iodelay_t;

typedef struct edge_s {
    int to_node;
    float delay;
} edge_t;

typedef struct node_arr_req_s {
    int node_id;
    float T_arr;
    float T_req;
} node_arr_req_t;

typedef struct timing_graph_level_s {
    int level;
    std::vector<int>* node_ids;
} timing_graph_level_t;

enum class TN_Type; //Forward declaration

typedef struct node_s {
    int node_id;
    TN_Type type;
    int ipin;
    int iblk;
    int domain;
    float skew;
    float iodelay;
    std::vector<edge_t>* out_edges;
} node_t;

struct domain_header_t {
    int src_domain;
    int sink_domain;
};


class VprArrReqTimes {
    public:
        void set_num_nodes(int nnodes) { num_nodes = nnodes; }
        void add_arr_time(int clock_id, int node_id, float val) {
            resize(clock_id);
            float curr_val = arr[clock_id][node_id];
            if(!isnan(curr_val) && isnan(val)) {
                //Don't over-write real values with NAN
                return;
            }
            arr[clock_id][node_id] = val;
        }

        void add_req_time(int clock_id, int node_id, float val) {
            resize(clock_id);
            float curr_val = req[clock_id][node_id];
            if(!isnan(curr_val) && isnan(val)) {
                //Don't over-write real values with NAN
                return;
            }
            req[clock_id][node_id] = val;
        }

        float get_arr_time(int clock_id, int node_id) const {
            VERIFY(clock_id < (int) arr.size());
            VERIFY(node_id  < (int) arr[clock_id].size());
            return arr[clock_id][node_id];
        }
        float get_req_time(int clock_id, int node_id) const {
            VERIFY(clock_id < (int) req.size());
            VERIFY(node_id  < (int) req[clock_id].size());
            return req[clock_id][node_id];
        }
        int get_num_clocks() const { return (int) arr.size(); }
        int get_num_nodes() const { return (int) num_nodes; }
    private:
        void resize(int clock_id) {
            while(clock_id >= (int) arr.size()) {
                arr.push_back(std::vector<float>(num_nodes, NAN));
                req.push_back(std::vector<float>(num_nodes, NAN));
            }
            VERIFY(arr.size() == req.size());
            VERIFY(clock_id < (int) arr.size());
        }

        int num_nodes;
        std::vector<std::vector<float>> arr;
        std::vector<std::vector<float>> req;

};

extern int yyparse(TimingGraph& tg, VprArrReqTimes& arr_req_times);
extern FILE *yyin;

