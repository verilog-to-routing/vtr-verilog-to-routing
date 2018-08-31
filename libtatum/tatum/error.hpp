#ifndef TATUM_ERROR_HPP
#define TATUM_ERROR_HPP
#include <stdexcept>

#include "tatum/TimingGraphFwd.hpp"

namespace tatum {
class Error : public std::runtime_error {

    public:
        //String only
        explicit Error(const std::string& what_str)
            : std::runtime_error(what_str) {}

        //String and node
        explicit Error(const std::string& what_str, const NodeId n)
            : std::runtime_error(what_str)
            , node(n) {}

        //String and edge
        explicit Error(const std::string& what_str, const EdgeId e)
            : std::runtime_error(what_str)
            , edge(e) {}

        //String, node and edge
        explicit Error(const std::string& what_str, const NodeId n, const EdgeId e)
            : std::runtime_error(what_str)
            , node(n)
            , edge(e) {}

        NodeId node; //The related node, may be invalid if unspecified
        EdgeId edge; //The related edge, may be invalid if unspecified
};

}
#endif
