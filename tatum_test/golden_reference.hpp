#ifndef TATUM_STA_GOLDEN_REFERENCE
#define TATUM_STA_GOLDEN_REFERENCE
#include <map>

#include "tatum/TimingGraphFwd.hpp"
#include "tatum/util/tatum_linear_map.hpp"
#include "tatumparse.hpp"

struct TagResult {
    TagResult(tatum::NodeId node_id, tatum::DomainId launch_domain_id, tatum::DomainId capture_domain_id, float time_val)
        : node(node_id)
        , launch_domain(launch_domain_id)
        , capture_domain(capture_domain_id)
        , time(time_val) {}
    tatum::NodeId node;
    tatum::DomainId launch_domain;
    tatum::DomainId capture_domain;
    float time;
};

class GoldenReference {
    public:

        void set_result(tatum::NodeId node, tatumparse::TagType tag_type, tatum::DomainId launch_domain, tatum::DomainId capture_domain, float time) {
            auto key = std::make_pair(node, tag_type);
            auto res = results[key].insert(std::make_pair(std::make_pair(launch_domain, capture_domain), TagResult(node, launch_domain, capture_domain, time)));

            TATUM_ASSERT_MSG(res.second, "Was inserted");
        }

        const std::map<std::pair<tatum::DomainId,tatum::DomainId>,TagResult>& get_result(tatum::NodeId node, tatumparse::TagType tag_type) {
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
        typedef std::map<std::pair<tatum::DomainId,tatum::DomainId>,TagResult> Value;

        std::map<Key,Value> results;
};


#endif
