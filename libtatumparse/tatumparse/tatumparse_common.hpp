#ifndef TATUMPARSE_COMMON_HPP
#define TATUMPARSE_COMMON_HPP

#include "tatumparse.hpp"

namespace tatumparse {

    struct NodeTag {
        NodeTag(int domain, float arr_val, float req_val)
            : domain_id(domain), arr(arr_val), req(req_val) {}

        int domain_id;
        float arr;
        float req;
    };

    struct NodeResult {
        int id;
        std::vector<NodeTag> tags;
    };

}

#endif
