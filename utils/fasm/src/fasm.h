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
#include "netlist_writer.h"
#include "lut.h"
#include "parameters.h"

namespace fasm {

class FasmWriterVisitor : public NetlistVisitor {

  public:
      FasmWriterVisitor(std::ostream& f);

  private:
      void visit_top_impl(const char* top_level_name) override;
      void visit_open_impl(const t_pb* atom) override;
      void visit_atom_impl(const t_pb* atom) override;
      // clb in visit_clb_impl stands for complex logic block.
      // visit_clb_impl is called on each top-level pb_type used in the design.
      void visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) override;
      void visit_all_impl(const t_pb_route *top_pb_route, const t_pb* pb,
          const t_pb_graph_node* pb_graph_node) override;
      void finish_impl() override;

  private:
      void output_fasm_features(std::string features) const;
      void check_features(t_metadata_dict *meta) const;
      void check_interconnect(const t_pb_route *pb_route, int inode);
      void check_for_lut(const t_pb* atom);
      void output_fasm_mux(std::string fasm_mux, t_interconnect *interconnect, t_pb_graph_pin *mux_input_pin);
      void walk_routing();
      std::string build_clb_prefix(const t_pb_graph_node* pb_graph_node) const;
      const LutOutputDefinition* find_lut(const t_pb_graph_node* pb_graph_node);
      void check_for_param(const t_pb *atom);

      std::ostream& os_;

      t_pb_graph_node *root_clb_;
      bool current_blk_has_prefix_;
      t_type_ptr blk_type_;
      std::string blk_prefix_;
      std::string clb_prefix_;
      ClusterBlockId current_blk_id_;
      std::vector<t_pb_graph_pin**> pb_graph_pin_lookup_from_index_by_type_;
      std::map<const t_pb_type*, std::vector<std::pair<std::string, LutOutputDefinition>>> lut_definitions_;
      std::map<const t_pb_type*, Parameters> parameters_;
};

} // namespace fasm

#endif  // FASM_H
