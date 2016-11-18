#ifndef TATUM_STA_GOLDEN_REFERENCE
#define TATUM_STA_GOLDEN_REFERENCE
#include <map>

#include "timing_graph_fwd.hpp"
#include "tatumparse.hpp"

struct TagResult {
    TagResult(tatum::NodeId node_id, tatum::DomainId domain_id, float arr_val, float req_val): node(node_id), domain(domain_id), arr(arr_val), req(req_val) {}
    tatum::NodeId node;
    tatum::DomainId domain;
    float arr;
    float req;
};

class GoldenReference {
    public:

        void set_result(tatum::NodeId node, tatumparse::TagType tag_type, tatum::DomainId domain, float arr, float req) {
            auto key = std::make_pair(node, tag_type);
            auto res = results[key].insert(std::make_pair(domain, TagResult(node, domain, arr, req)));

            TATUM_ASSERT_MSG(res.second, "Was inserted");
        }

        const std::map<tatum::DomainId,TagResult>& get_result(tatum::NodeId node, tatumparse::TagType tag_type) {
            auto key = std::make_pair(node, tag_type);
            return results[key];
        }

        size_t num_tags() {
            size_t cnt = 0;
            for(auto& kv : results) {
                cnt += kv.second.size();
            }

            return cnt;
        }

        void remap_nodes(const tatum::util::linear_map<tatum::NodeId,tatum::NodeId>& node_id_map) {
            std::map<Key,Value> remapped_results; 

            for(const auto& kv : results) {
                auto& old_key = kv.first;
                auto new_key = std::make_pair(node_id_map[old_key.first], old_key.second);

                remapped_results[new_key] = kv.second;
            }

            results = std::move(remapped_results);
        }
        

    private:
        //TODO: this is not very memory efficient....
        typedef std::pair<tatum::NodeId,tatumparse::TagType> Key;
        typedef std::map<tatum::DomainId,TagResult> Value;

        std::map<Key,Value> results;
};


#endif
