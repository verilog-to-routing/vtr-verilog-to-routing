#ifndef FASM_H
#define FASM_H

#include <iostream>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "netlist_walker.h"

class FasmWriterVisitor : public NetlistVisitor {
    public:
        FasmWriterVisitor(std::ostream& f);

    private:
        void visit_top_impl(const char* top_level_name) override;
        void visit_open_impl(const t_pb* atom) override;
        void visit_atom_impl(const t_pb* atom) override;
        void visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) override;
        void visit_all_impl(const t_pb_route *top_pb_route, const t_pb* pb,
            const t_pb_graph_node* pb_graph_node) override;
        void finish_impl() override;

    private:

        std::ostream& os_;

        ClusterBlockId current_blk_id_;
};

#endif  // FASM_H
