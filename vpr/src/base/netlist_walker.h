#ifndef NETLIST_WALKER_H
#define NETLIST_WALKER_H
#include "vpr_types.h"

class NetlistVisitor;

class NetlistWalker {

    public:
        NetlistWalker(NetlistVisitor& netlist_visitor)
            : visitor_(netlist_visitor) {}

        void walk();

    private:
        void walk_blocks(const t_pb_routes &pb_route, const t_pb *pb, const t_pb_graph_node *pb_graph_node);

    private:
        NetlistVisitor& visitor_;
};

class NetlistVisitor {

    public:
        virtual ~NetlistVisitor() = default;
        void start() { start_impl(); }
        void visit_top(const char* top_level_name) { visit_top_impl(top_level_name); }
        void visit_clb(ClusterBlockId blk_id, const t_pb* clb) { visit_clb_impl(blk_id, clb); }
        void visit_atom(const t_pb* atom) { visit_atom_impl(atom); }
        void visit_open(const t_pb* atom) { visit_open_impl(atom); }
        void visit_all(const t_pb_routes &top_pb_route, const t_pb* pb, const t_pb_graph_node* pb_graph_node) {
                VTR_ASSERT(pb == nullptr || pb_graph_node == pb->pb_graph_node);
                VTR_ASSERT(pb_graph_node != nullptr);
                visit_all_impl(top_pb_route, pb, pb_graph_node);
        }
        void finish() { finish_impl(); }

    protected:
        //All implementation methods are no-ops in this base class
        virtual void start_impl();
        virtual void visit_top_impl(const char* top_level_name);
        virtual void visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb);
        virtual void visit_atom_impl(const t_pb* atom);
        virtual void visit_open_impl(const t_pb* atom);
        virtual void visit_all_impl(const t_pb_routes &top_pb_route, const t_pb* pb, const t_pb_graph_node* pb_graph_node);
        virtual void finish_impl();
};
#endif
