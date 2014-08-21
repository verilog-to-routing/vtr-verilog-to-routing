#ifndef TIMINGGRAPH_HPP
#define TIMINGGRAPH_HPP
#include <boost/container.hpp>

#include "TimingEdge.hpp"
#include "TimingNode.hpp"


class TimingGraph {
    public:

    private:
        //stable_vector gives up element continuity in memory,
        //but instead ensures that references (and pointers) to
        //the elements do not change if the stable_vector is modified.
        //
        //This allows us to modify the timing graph without breaking
        //internal references!
        boost::container::stable_vector<TimingNode> tnodes_;

};

#endif
