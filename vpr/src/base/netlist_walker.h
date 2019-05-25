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
    void walk_blocks(const t_pb_routes& pb_route, const t_pb* pb);

  private:
    NetlistVisitor& visitor_;
};

class NetlistVisitor {
  public:
    virtual ~NetlistVisitor() = default;
    void start() { start_impl(); }
    void visit_top(const char* top_level_name) { visit_top_impl(top_level_name); }
    void visit_clb(ClusterBlockId blk_id, const t_pb* clb) { visit_clb_impl(blk_id, clb); }

    // visit_atom is called on leaf pb nodes that map to a netlist element.
    void visit_atom(const t_pb* atom) { visit_atom_impl(atom); }

    // visit_route_through is called on leaf pb nodes that do not map to a
    // netlist element.  This is generally used for route-through nodes.
    void visit_route_through(const t_pb* atom) {
        visit_route_through_impl(atom);
    }

    // visit_all is called on all t_pb nodes that are in use for any
    // reason.
    //
    // top_pb_route is the pb_route for the cluster being visited.
    // pb is the current element in the cluster being visited.
    void visit_all(const t_pb_routes& top_pb_route, const t_pb* pb) {
        visit_all_impl(top_pb_route, pb);
    }
    void finish() { finish_impl(); }

  protected:
    //All implementation methods are no-ops in this base class
    virtual void start_impl();
    virtual void visit_top_impl(const char* top_level_name);
    virtual void visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb);
    virtual void visit_atom_impl(const t_pb* atom);
    virtual void visit_route_through_impl(const t_pb* atom);
    virtual void visit_all_impl(const t_pb_routes& top_pb_route, const t_pb* pb);

    virtual void finish_impl();
};
#endif
