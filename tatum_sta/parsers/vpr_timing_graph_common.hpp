#pragma once
#include "tatum_assert.hpp"
#include "tatum_strong_id.hpp"

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include "timing_graph_fwd.hpp"
#include "timing_constraints_fwd.hpp"


//Id for blocks (from VPR timing graph)
struct block_id_tag;
typedef tatum::util::StrongId<block_id_tag> BlockId;
inline std::ostream& operator<<(std::ostream& os, BlockId block_id) {
    return os << "Block(" << size_t(block_id) << ")";
}



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

enum class VprNodeType {
	INPAD_SOURCE, //Driver of an input I/O pad
	INPAD_OPIN, //Output pin of an input I/O pad
	OUTPAD_IPIN, //Input pin of an output I/O pad
	OUTPAD_SINK, //Sink of an output I/O pad
	PRIMITIVE_IPIN, //Input pin to a primitive (e.g. LUT)
	PRIMITIVE_OPIN, //Output pin from a primitive (e.g. LUT)
	FF_IPIN, //Input pin to a flip-flop - goes to FF_SINK
	FF_OPIN, //Output pin from a flip-flop - comes from FF_SOURCE
	FF_SINK, //Sink (D) pin of flip-flop
	FF_SOURCE, //Source (Q) pin of flip-flop
	FF_CLOCK, //Clock pin of flip-flop
    CLOCK_SOURCE, //A clock generator such as a PLL
    CLOCK_OPIN, //Output pin from an on-chip clock source - comes from CLOCK_SOURCE
	CONSTANT_GEN_SOURCE, //Source of a constant logic 1 or 0
    UNKOWN //Unrecognized type, if encountered this is almost certainly an error
};

typedef struct node_s {
    int node_id;
    tatum::NodeType type;
    VprNodeType vpr_type;
    int ipin;
    int iblk;
    int domain;
    int is_clk_src;
    bool is_const_gen;
    float skew;
    float iodelay;
    std::vector<edge_t>* out_edges;
} node_t;

struct domain_header_t {
    int src_domain;
    int sink_domain;
};

struct VprFfInfo {
    std::multimap<BlockId,tatum::NodeId> logical_block_FF_clocks;
    std::multimap<BlockId,tatum::NodeId> logical_block_FF_sources;
    std::multimap<BlockId,tatum::NodeId> logical_block_FF_sinks;
};

class VprArrReqTimes {
    public:
        void set_num_nodes(int nnodes) { num_nodes = nnodes; }
        void add_arr_time(const tatum::DomainId clock_id, const tatum::NodeId node_id, float val) {
            resize(clock_id);
            TATUM_ASSERT(arr.find(clock_id) != arr.end());
            float curr_val = arr[clock_id][size_t(node_id)];
            if(!isnan(curr_val) && isnan(val)) {
                //Don't over-write real values with NAN
                return;
            }
            if(isnan(curr_val) || curr_val < val) {
                //Max
                arr[clock_id][size_t(node_id)] = val;
            }
        }

        void add_req_time(const tatum::DomainId clock_id, const tatum::NodeId node_id, float val) {
            resize(clock_id);
            TATUM_ASSERT(req.find(clock_id) != req.end());
            float curr_val = req[clock_id][size_t(node_id)];
            if(!isnan(curr_val) && isnan(val)) {
                //Don't over-write real values with NAN
                return;
            }
            if(isnan(curr_val) || val < curr_val) {
                //Min
                req[clock_id][size_t(node_id)] = val;
            }
        }

        float get_arr_time(const tatum::DomainId clock_id, const tatum::NodeId node_id) const {
            auto arr_iter = arr.find(clock_id);
            TATUM_ASSERT(arr_iter != arr.end());
            TATUM_ASSERT(size_t(node_id)  < arr_iter->second.size());
            return arr_iter->second[size_t(node_id)];
        }

        float get_req_time(const tatum::DomainId clock_id, const tatum::NodeId node_id) const {
            auto req_iter = req.find(clock_id);
            TATUM_ASSERT(req_iter != req.end());
            TATUM_ASSERT(size_t(node_id)  < req_iter->second.size());
            return req_iter->second[size_t(node_id)];
        }

        int get_num_clocks() const { return (int) arr.size(); }
        std::vector<tatum::DomainId> clocks() const {
            std::vector<tatum::DomainId> clocks_to_return;
            for(auto kv : arr) {
                clocks_to_return.push_back(kv.first);
            }
            return clocks_to_return;
        }

        int get_num_nodes() const { return (int) num_nodes; }

        void resize(const tatum::DomainId clock_id) {
            if(arr.find(clock_id) == arr.end()) {
                arr[clock_id] = std::vector<float>(num_nodes, NAN);
                req[clock_id] = std::vector<float>(num_nodes, NAN);
            }
        }

        void print() const {
            for(auto arr_iter : arr) {
                tatum::DomainId clock_id = arr_iter.first;
                auto req_iter = req.find(clock_id);

                std::cout << "Clock " << clock_id << std::endl;
                for(int i = 0; i < num_nodes; i++) {
                    std::cout << "Arr: " << arr_iter.second[i];
                    std::cout << " Req: " << req_iter->second[i];
                    std::cout << std::endl;
                }
            }
        }

        std::set<tatum::DomainId> domains() {
            std::set<tatum::DomainId> domain_values;
            for(const auto& pair : arr) {
                domain_values.insert(pair.first);
            }
            return domain_values;
        }
    private:

        int num_nodes;
        std::map<tatum::DomainId,std::vector<float>> arr;
        std::map<tatum::DomainId,std::vector<float>> req;

};

extern int yyparse(tatum::TimingGraph& tg, VprArrReqTimes& arr_req_times, tatum::TimingConstraints& tc, VprFfInfo& ff_info, std::vector<float>& edge_delays);
extern FILE *yyin;

