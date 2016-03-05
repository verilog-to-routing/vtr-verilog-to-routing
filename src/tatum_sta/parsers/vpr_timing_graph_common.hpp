#pragma once
#include "assert.hpp"

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include "timing_graph_fwd.hpp"


//Forward Declarations
class TimingGraph;
class TimingConstraints;


typedef struct domain_skew_iodelay_s {
    int domain;
    float skew;
    float iodelay;
} domain_skew_iodelay_t;

typedef struct edge_s {
    int src_node;
    int sink_node;
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
    int is_clk_src;
    float skew;
    float iodelay;
    std::vector<edge_t>* out_edges;
} node_t;

struct domain_header_t {
    int src_domain;
    int sink_domain;
};

/*
 *struct union_type {
 *    char* strVal;
 *    double floatVal;
 *    int intVal;
 *    domain_skew_iodelay_t domainSkewIodelayVal;
 *    edge_t edgeVal;
 *    node_arr_req_t nodeArrReqVal;
 *    timing_graph_level_t timingGraphLevelVal;
 *    node_t nodeVal;
 *    TN_Type nodeTypeVal;
 *    domain_header_t domain_header;
 *};
 *
 *#define YYSTYPE union_type
 */

class VprArrReqTimes {
    public:
        void set_num_nodes(int nnodes) { num_nodes = nnodes; }
        void add_arr_time(int clock_id, int node_id, float val) {
            resize(clock_id);
            VERIFY(arr.find(clock_id) != arr.end());
            float curr_val = arr[clock_id][node_id];
            if(!isnan(curr_val) && isnan(val)) {
                //Don't over-write real values with NAN
                return;
            }
            if(isnan(curr_val) || curr_val < val) {
                //Max
                arr[clock_id][node_id] = val;
            }
        }

        void add_req_time(int clock_id, int node_id, float val) {
            resize(clock_id);
            VERIFY(req.find(clock_id) != req.end());
            float curr_val = req[clock_id][node_id];
            if(!isnan(curr_val) && isnan(val)) {
                //Don't over-write real values with NAN
                return;
            }
            if(isnan(curr_val) || val < curr_val) {
                //Min
                req[clock_id][node_id] = val;
            }
        }

        float get_arr_time(int clock_id, int node_id) const {
            auto arr_iter = arr.find(clock_id);
            VERIFY(arr_iter != arr.end());
            VERIFY(node_id  < (int) arr_iter->second.size());
            return arr_iter->second[node_id];
        }

        float get_req_time(int clock_id, int node_id) const {
            auto req_iter = req.find(clock_id);
            VERIFY(req_iter != req.end());
            VERIFY(node_id  < (int) req_iter->second.size());
            return req_iter->second[node_id];
        }

        int get_num_clocks() const { return (int) arr.size(); }
        std::vector<int> clocks() const {
            std::vector<int> clocks_to_return;
            for(auto kv : arr) {
                clocks_to_return.push_back(kv.first);
            }
            return clocks_to_return;
        }

        int get_num_nodes() const { return (int) num_nodes; }

        void resize(int clock_id) {
            if(arr.find(clock_id) == arr.end()) {
                arr[clock_id] = std::vector<float>(num_nodes, NAN);
                req[clock_id] = std::vector<float>(num_nodes, NAN);
            }
        }

        void print() const {
            for(auto arr_iter : arr) {
                int clock_id = arr_iter.first;
                auto req_iter = req.find(clock_id);

                std::cout << "Clock " << clock_id << std::endl;
                for(int i = 0; i < num_nodes; i++) {
                    std::cout << "Arr: " << arr_iter.second[i];
                    std::cout << " Req: " << req_iter->second[i];
                    std::cout << std::endl;
                }
            }
        }

        std::set<int> domains() {
            std::set<int> domain_values;
            for(const auto& pair : arr) {
                domain_values.insert(pair.first);
            }
            return domain_values;
        }
    private:

        int num_nodes;
        std::map<int,std::vector<float>> arr;
        std::map<int,std::vector<float>> req;

};

extern int yyparse(TimingGraph& tg, VprArrReqTimes& arr_req_times, TimingConstraints& tc, std::vector<BlockId>& node_logical_blocks, std::vector<float>& edge_delays);
extern FILE *yyin;

