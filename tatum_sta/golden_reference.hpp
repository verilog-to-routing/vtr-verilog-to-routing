#ifndef TATUM_STA_GOLDEN_REFERENCE
#define TATUM_STA_GOLDEN_REFERENCE

#include "timing_graph_fwd.hpp"
#include "tatumparse.hpp"

class GoldenReference {
    public:
        float get_arr(tatumparse::TagType type, tatum::NodeId node, tatum::DomainId domain) {
            return arr_[make_key(type, node, domain)];
        }

        float get_req(tatumparse::TagType type, tatum::NodeId node, tatum::DomainId domain) {
            return req_[make_key(type, node, domain)];
        }

        void set_arr(tatumparse::TagType type, tatum::NodeId node, tatum::DomainId domain, float val) {
            arr_[make_key(type, node, domain)] = val;
        }

        void set_req(tatumparse::TagType type, tatum::NodeId node, tatum::DomainId domain, float val) {
            req_[make_key(type, node, domain)] = val;
        }

    private:
        typedef std::tuple<tatumparse::TagType,tatum::NodeId,tatum::DomainId> Key;

        Key make_key(tatumparse::TagType type, tatum::NodeId node, tatum::DomainId domain) {
            return std::make_tuple(type, node, domain);
        }

        std::map<Key,float> arr_;
        std::map<Key,float> req_;
};

#endif
