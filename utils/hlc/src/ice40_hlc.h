#ifndef ICE40_HLC_H
#define ICE40_HLC_H

#include <list>
#include <map>
#include <unordered_set>
#include <string>
#include <vector>

#include "netlist_walker.h"

#include "hlc.h"

class ICE40HLCWriterVisitor : public NetlistVisitor {
    public:
        ICE40HLCWriterVisitor(std::ostream& f);

    private:
        void visit_top_impl(const char* top_level_name) override;
        void visit_open_impl(const t_pb* atom) override;
        void visit_atom_impl(const t_pb* atom) override;
        void visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) override;
        void visit_all_impl(const t_pb_routes &top_pb_route, const t_pb* pb,
            const t_pb_graph_node* pb_graph_node) override;
        void finish_impl() override;

    private:
        void process_ports(const t_pb_routes &top_pb_route, const int num_ports,
            const int *num_pins, const t_pb_graph_pin *const *pins);
        void process_route(const t_pb_routes &top_pb_route, const t_pb_route *pb_route,
            const t_pb_graph_pin *pin);
        void close_tile();

        void set_tile(t_hlc_coord pos, std::string name);
        void set_cell(std::string name, int index);

    private:
        std::ostream& os_;

        ClusterBlockId current_blk_id_;
        t_hlc_file output_;
        t_hlc_tile* current_tile_;
        t_hlc_cell* current_cell_;
};

#endif
