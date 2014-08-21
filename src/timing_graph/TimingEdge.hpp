#ifndef TIMINGEDGE_HPP
#define TIMINGEDGE_HPP

class TimingEdge {
    public:
        TimingNode* to_node() {return to_node_;}
        void set_to_node(TimingNode* node) {to_node_ = node;}
    private:
        TimingNode* to_node_;
};

#endif
