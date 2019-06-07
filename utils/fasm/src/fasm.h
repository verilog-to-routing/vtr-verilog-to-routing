// Core of the FPGA assembly (FASM) output code.
//
// The netlist walker implements the core walking of the netlist and routing
// graph with the intention of emitting the complete implemented design to the
// output FASM file.  Once output, the FASM file is expected to completely
// describe the features required to implement the design in a hardware
// bitstream.
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
#include "route_tree_type.h"

namespace fasm {

// FASM netlist visitor.
//
// This netlist visitor emits FASM features to the provided ostream using
// FASM metadata tags.  See the FASM documentation in docs/utils/fasm.rst
// for a complete list of tags.
//
// High level methodology:
//  - At each top level pb_type, root tile FASM prefix is checked.
//  - At each pb_type, the CLB prefix is build by walking from the child to
//    the root pb_type.
//  - All LUTs are visited and emitted via the fasm_type/fasm_lut tags.
//  - All route through wires are emitted via the same mechanism as LUTs.
//  - All atom's (e.g. leaf pb_types) are visited and have their parameters
//    emitted via fasm_params tags.
//  - All pb_type interconnect's are visited and are emitted via fasm_mux tags.
//  - All pb_type and pb_type mode's are checked for static feature emission
//    via the fasm_features tag.
//
// Once the netlist visitor is done, in finish_impl, the routing graph is
// walked and static features are emitted via the fasm_features tag.
//
// Note that FASM features are incrementally written the ostream as they are
// seen, so the order of FASM output is arbitrary based on the walk order of
// the netlist and routing graph.
class FasmWriterVisitor : public NetlistVisitor {

  public:
      FasmWriterVisitor(std::ostream& f);

  private:
      void visit_top_impl(const char* top_level_name) override;
      void visit_route_through_impl(const t_pb* atom) override;
      void visit_atom_impl(const t_pb* atom) override;
      // clb in visit_clb_impl stands for complex logic block.
      // visit_clb_impl is called on each top-level pb_type used in the design.
      void visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) override;
      void visit_all_impl(const t_pb_routes &top_pb_route, const t_pb* pb) override;
      void finish_impl() override;

  private:
      void output_fasm_features(std::string features) const;
      void check_features(const t_metadata_dict *meta) const;
      void check_interconnect(const t_pb_routes &pb_route, int inode);
      void check_for_lut(const t_pb* atom);
      void output_fasm_mux(std::string fasm_mux, t_interconnect *interconnect, t_pb_graph_pin *mux_input_pin);
      void walk_routing();
      void walk_route_tree(const t_rt_node *root);
      std::string build_clb_prefix(const t_pb *pb, const t_pb_graph_node* pb_graph_node, bool* is_parent_pb_null) const;
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
