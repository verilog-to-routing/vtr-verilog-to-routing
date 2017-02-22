#ifndef TATUM_TIMING_PATH_HPP
#define TATUM_TIMING_PATH_HPP

#include "tatum/util/tatum_assert.hpp"

namespace tatum {

//A component/point along a timing path
class TimingPathElem {
    public:
        TimingPathElem(tatum::TimingTag tag_v,
                       tatum::NodeId node_v,
                       tatum::EdgeId incomming_edge_v)
            : tag(tag_v)
            , node(node_v)
            , incomming_edge(incomming_edge_v) {
            //pass
        }

        tatum::TimingTag tag;
        tatum::NodeId node;
        tatum::EdgeId incomming_edge;
};


//The type of a timing path
enum class TimingPathType {
    SETUP,
    HOLD,
    UNKOWN
};

//A collection of timing path elements which form
//a timing path. Used during reporting.
class TimingPath {
    public:
        tatum::DomainId launch_domain;
        tatum::DomainId capture_domain;

        tatum::NodeId endpoint() const { 
            TATUM_ASSERT(data_launch.size() > 0);
            return data_launch[data_launch.size()-1].node;
        }
        tatum::NodeId startpoint() const { 
            TATUM_ASSERT(data_launch.size() > 0);
            return data_launch[0].node; 
        }

        std::vector<TimingPathElem> clock_launch;
        std::vector<TimingPathElem> data_launch;
        std::vector<TimingPathElem> clock_capture;
        tatum::TimingTag slack_tag;
        TimingPathType type;
};

} //namespace

#endif
