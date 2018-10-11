#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

#include "globals.h"

#include "vtr_assert.h"
#include "vtr_logic.h"
#include "vtr_version.h"
#include "vpr_error.h"

#include "atom_netlist_utils.h"
#include "netlist_writer.h"
#include "vpr_utils.h"

#include "fasm.h"


FasmWriterVisitor::FasmWriterVisitor(std::ostream& f) : os_(f) {}

void FasmWriterVisitor::visit_top_impl(const char* top_level_name) {
    auto& device_ctx = g_vpr_ctx.device();
    pb_graph_pin_lookup_from_index_by_type_.resize(device_ctx.num_block_types);
    for(int itype = 0; itype < device_ctx.num_block_types; itype++) {
        pb_graph_pin_lookup_from_index_by_type_.at(itype) = alloc_and_load_pb_graph_pin_lookup_from_index(&device_ctx.block_types[itype]);
    }
}

void FasmWriterVisitor::visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) {
    lut_mode_ = NO_LUT;
    lut_prefix_ = "";
    lut_parts_.resize(0);
    pb_route_ = nullptr;

    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    current_blk_id_ = blk_id;

    VTR_ASSERT(clb->pb_graph_node != nullptr);
    VTR_ASSERT(clb->pb_graph_node->pb_type);

    root_clb_ = clb->pb_graph_node;

    auto *pb_type = clb->pb_graph_node->pb_type;
    int x = place_ctx.block_locs[blk_id].x;
    int y = place_ctx.block_locs[blk_id].y;
    auto &grid_loc = device_ctx.grid[x][y];
    int x_offset = grid_loc.width_offset;
    int y_offset = grid_loc.height_offset;
    blk_type_ = grid_loc.type;

    current_blk_has_prefix_ = true;
    std::string grid_prefix;
    if(grid_loc.meta != nullptr && grid_loc.meta->has("fasm_prefix")) {
        grid_prefix = grid_loc.meta->get("fasm_prefix")->front().as_string();
    } else {
      current_blk_has_prefix_= false;
    }

    std::string pb_type_prefix;
    if (pb_type->meta != nullptr && pb_type->meta->has("fasm_prefix")) {
        pb_type_prefix = pb_type->meta->get("fasm_prefix")->front().as_string();
    } else {
      current_blk_has_prefix_ = false;
    }

    if(current_blk_has_prefix_) {
      blk_prefix_ = grid_prefix + "." + pb_type_prefix + ".";
    }
}

void FasmWriterVisitor::check_interconnect(const t_pb_route *pb_route, int inode) {
  if(!pb_route[inode].atom_net_id) {
    // Net is open.
    return;
  }

  /* No previous driver implies that this is either a top-level input pin
    * or a primitive output pin */
  int prev_node = pb_route[inode].driver_pb_pin_id;
  if(prev_node == OPEN) {
    return;
  }

  t_pb_graph_pin *cur_pin = pb_graph_pin_lookup_from_index_by_type_.at(blk_type_->index)[inode];
  t_pb_graph_pin *prev_pin = pb_graph_pin_lookup_from_index_by_type_.at(blk_type_->index)[prev_node];

  int prev_edge;
  for(prev_edge = 0; prev_edge < prev_pin->num_output_edges; prev_edge++) {
      VTR_ASSERT(prev_pin->output_edges[prev_edge]->num_output_pins == 1);
      if(prev_pin->output_edges[prev_edge]->output_pins[0]->pin_count_in_cluster == inode) {
          break;
      }
  }
  VTR_ASSERT(prev_edge < prev_pin->num_output_edges);

  auto *interconnect = prev_pin->output_edges[prev_edge]->interconnect;
  if(interconnect->meta != nullptr && interconnect->meta->has("fasm_mux")) {
    std::string fasm_mux = interconnect->meta->get("fasm_mux")->front().as_string();
    output_fasm_mux(fasm_mux, interconnect, prev_pin);
  }
}

std::string FasmWriterVisitor::build_clb_prefix(const t_pb_graph_node* pb_graph_node) const {
  if(pb_graph_node == root_clb_) {
    return "";
  }

  std::string clb_prefix = "";

  if(pb_graph_node->parent_pb_graph_node != root_clb_) {
    VTR_ASSERT(pb_graph_node->parent_pb_graph_node != nullptr);
    clb_prefix = build_clb_prefix(pb_graph_node->parent_pb_graph_node);
  }

  const auto *pb_type = pb_graph_node->pb_type;
  bool has_prefix = pb_type->meta != nullptr
      && pb_type->meta->has("fasm_prefix");

  if(!has_prefix) {
    return clb_prefix;
  }

  auto fasm_prefix_unsplit = pb_type->meta->get("fasm_prefix")->front().as_string();
  auto fasm_prefix = vtr::split(fasm_prefix_unsplit, " ");
  if(fasm_prefix.size() != pb_type->num_pb) {
    vpr_throw(VPR_ERROR_OTHER,
              __FILE__, __LINE__, "fasm_prefix = %s, num_pb = %d",
              fasm_prefix_unsplit.c_str(), pb_type->num_pb);
  }

  if(pb_graph_node->placement_index >= pb_type->num_pb) {
    vpr_throw(VPR_ERROR_OTHER,
              __FILE__, __LINE__,
              "pb_graph_node->placement_index = %d >= pb_type->num_pb = %d",
              fasm_prefix_unsplit.c_str(), pb_type->num_pb);
  }

  return clb_prefix + fasm_prefix.at(pb_graph_node->placement_index) + ".";

}

void FasmWriterVisitor::setup_split_lut(std::string fasm_lut) {
  auto fasm_lut_no_space = vtr::replace_all(vtr::replace_all(fasm_lut, " ", ""), "\n", "");

  auto lut_parts = vtr::split(fasm_lut_no_space, "=");
  if(lut_parts.size() != 2) {
    vpr_throw(VPR_ERROR_OTHER,
              __FILE__, __LINE__,
              "Split lut definition fasm_lut = %s does not parse.",
              fasm_lut.c_str());
  }

  lut_prefix_ = lut_parts[0];

  lut_parts_ = vtr::split(
      vtr::replace_all(
          vtr::replace_all(lut_parts[1], "(", ""),
                       ")", ""), ",");
  if(__builtin_popcount(lut_parts_.size()) != 1) {
    vpr_throw(VPR_ERROR_OTHER,
              __FILE__, __LINE__,
              "Number of lut splits must be power of two, found %d parts",
              lut_parts_.size());
  }
}

void FasmWriterVisitor::visit_all_impl(const t_pb_route *pb_route, const t_pb* pb,
        const t_pb_graph_node* pb_graph_node) {
  clb_prefix_ = build_clb_prefix(pb_graph_node);

  if(pb_graph_node && pb) {
    t_pb_type *pb_type = pb_graph_node->pb_type;
    auto *mode = &pb_type->modes[pb->mode];

    if(mode) {
      if(mode->meta != nullptr && mode->meta->has("fasm_features")) {
        output_fasm_features(mode->meta->get("fasm_features")->front().as_string());
      }

      if(mode->meta != nullptr && mode->meta->has("fasm_type")) {
        std::string fasm_type = mode->meta->get("fasm_type")->front().as_string();
        if(fasm_type == "LUT") {
          // Output LUT
          if(!mode->meta->has("fasm_lut")) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                      "pb_type->name %d has fasm_type = %s but is missing fasm_lut metadata.",
                      pb_type->name, fasm_type);
          }

          lut_mode_ = LUT;
          pb_route_ = pb_route;
          lut_prefix_ = mode->meta->get("fasm_lut")->front().as_string();

        } else if(fasm_type == "SPLIT_LUT") {
          // Output LUT
          if(!mode->meta->has("fasm_lut")) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                      "pb_type->name %d has fasm_type = %s but is missing fasm_lut metadata.",
                      pb_type->name, fasm_type);
          }

          lut_mode_ = SPLIT_LUT;
          pb_route_ = pb_route;
          setup_split_lut(mode->meta->get("fasm_lut")->front().as_string());
        } else {
          vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "Unknown fasm_type = %s\n",
                            fasm_type.c_str());
        }
      }
    }

    int port_index = 0;
    for (int i = 0; i < pb_type->num_ports; i++) {
      if (pb_type->ports[i].is_clock || pb_type->ports[i].type != IN_PORT) {
        continue;
      }
      for (int j = 0; j < pb_type->ports[i].num_pins; j++) {
        int inode = pb->pb_graph_node->input_pins[port_index][j].pin_count_in_cluster;
        check_interconnect(pb_route, inode);
      }
      port_index += 1;
    }
    port_index = 0;
    for (int i = 0; i < pb_type->num_ports; i++) {
      if (pb_type->ports[i].type != OUT_PORT) {
        continue;
      }
      for (int j = 0; j < pb_type->ports[i].num_pins; j++) {
        int inode = pb->pb_graph_node->output_pins[port_index][j].pin_count_in_cluster;
        check_interconnect(pb_route, inode);
      }
      port_index += 1;
    }
  }
}

void FasmWriterVisitor::visit_open_impl(const t_pb* atom) {
}

static AtomNetId _find_atom_input_logical_net(const t_pb* atom, const t_pb_route *pb_route, int atom_input_idx) {
    const t_pb_graph_node* pb_node = atom->pb_graph_node;
    const int cluster_pin_idx = pb_node->input_pins[0][atom_input_idx].pin_count_in_cluster;
    return pb_route[cluster_pin_idx].atom_net_id;
}

static LogicVec lut_outputs(const t_pb* atom_pb, const t_pb_route *pb_route) {
    auto& atom_ctx = g_vpr_ctx.atom();
    AtomBlockId block_id = atom_ctx.lookup.pb_atom(atom_pb);
    const t_model* model = atom_ctx.nlist.block_model(block_id);
    const auto& truth_table = atom_ctx.nlist.block_truth_table(block_id);
    auto ports = atom_ctx.nlist.block_input_ports(atom_ctx.lookup.pb_atom(atom_pb));

    if(ports.size() != 1) {
      if(ports.size() != 0) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "LUT port unexpected size is %d", ports.size());
      }

      return truth_table_to_lut_mask(truth_table, 0);
    }

    const int num_inputs = atom_pb->pb_graph_node->total_input_pins();
    const t_pb_graph_node* gnode = atom_pb->pb_graph_node;
    VTR_ASSERT(gnode->num_input_ports == 1);
    VTR_ASSERT(gnode->num_input_pins[0] == num_inputs);
    std::vector<int> permutation(num_inputs, OPEN);

    AtomPortId port_id = *ports.begin();

    for(size_t ipin = 0; ipin < num_inputs; ++ipin) {
        //The net currently connected to input j
        const AtomNetId impl_input_net_id = _find_atom_input_logical_net(atom_pb, pb_route, ipin);

        //Find the original pin index
        const t_pb_graph_pin* gpin = &gnode->input_pins[0][ipin];
        const BitIndex orig_index = atom_pb->atom_pin_bit_index(gpin);

        if(impl_input_net_id) {
            //If there is a valid net connected in the implementation
            AtomNetId logical_net_id = atom_ctx.nlist.port_net(port_id, orig_index);
            VTR_ASSERT(impl_input_net_id == logical_net_id);

            //Mark the permutation.
            //  The net originally located at orig_index in the atom netlist
            //  was moved to ipin in the implementation
            permutation[orig_index] = ipin;
        }
    }

    const auto permuted_truth_table = permute_truth_table(truth_table, num_inputs, permutation);
    LogicVec lut_mask = truth_table_to_lut_mask(permuted_truth_table, ports.size());
    return lut_mask;
}

int FasmWriterVisitor::find_lut_idx(t_pb_graph_node* pb_graph_node) {
  t_pb_graph_node* orig_pb_graph_node = pb_graph_node;
  while(pb_graph_node != nullptr) {
    VTR_ASSERT(pb_graph_node->pb_type != nullptr);

    auto string_at_node = vtr::string_fmt("%s[%d]", pb_graph_node->pb_type->name, pb_graph_node->placement_index);
    for(size_t idx = 0; idx < lut_parts_.size(); ++idx) {
      if(string_at_node == lut_parts_[idx]) {
        return idx;
      }
    }

    pb_graph_node = pb_graph_node->parent_pb_graph_node;
  }

  vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
            "Failed to find LUT index.");
}

void FasmWriterVisitor::visit_atom_impl(const t_pb* atom) {
    auto& atom_ctx = g_vpr_ctx.atom();

    auto atom_blk_id = atom_ctx.lookup.pb_atom(atom);
    if (atom_blk_id == AtomBlockId::INVALID()) {
        return;
    }

    const t_model* model = atom_ctx.nlist.block_model(atom_blk_id);
    if (model->name == std::string(MODEL_NAMES)) {
      if(lut_mode_ != LUT && lut_mode_ != SPLIT_LUT) {
          vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "LUT mode is %d\n",
                            lut_mode_);
      }

      LogicVec lut_mask = lut_outputs(atom, pb_route_);
      std::string lut_address_select;

      if(lut_mode_ == LUT) {
        lut_address_select = vtr::string_fmt("[%d:0]", lut_mask.size()-1);
      } else if(lut_mode_ == SPLIT_LUT) {
        // Get width of underlying LUT.
        // TODO: Test if this actually works!
        int width_of_lut = atom->pb_graph_node->pb_type->num_input_pins;
        int idx = find_lut_idx(atom->pb_graph_node);
        int start = (1 << width_of_lut)*idx;
        int end = start + lut_mask.size() -1;
        lut_address_select = vtr::string_fmt("[%d:%d]", end, start);
      } else {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "Unknown LUT mode %d\n",
                  lut_mode_);
      }

      std::stringstream ss("");
      ss << lut_prefix_ << lut_address_select << "=" << lut_mask ;

      output_fasm_features(ss.str());
    }
}

void FasmWriterVisitor::walk_routing() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& route_ctx = g_vpr_ctx.routing();

    for(const auto trace : route_ctx.trace_head) {
      const t_trace *head = trace;
      while(head != nullptr) {
        const auto &cur_node = device_ctx.rr_nodes[head->index];
        const t_trace *next = head->next;

        if(next != nullptr) {
          const auto next_inode = next->index;
          auto *meta = cur_node.edge_metadata(next_inode, head->iswitch, "fasm_features");
          if(meta != nullptr) {
            current_blk_has_prefix_ = false;
            output_fasm_features(meta->as_string());
          }
        }

        head = next;
      }
    }
}

void FasmWriterVisitor::finish_impl() {
    auto& device_ctx = g_vpr_ctx.device();
    for(int itype = 0; itype < device_ctx.num_block_types; itype++) {
        free_pb_graph_pin_lookup_from_index (pb_graph_pin_lookup_from_index_by_type_.at(itype));
    }

    walk_routing();
}

void parse_name_with_optional_index(const std::string in, std::string *name, int *index) {
  auto in_parts = vtr::split(in, "[]");

  if(in_parts.size() == 1) {
    *name = in;
    *index = 0;
  } else if(in_parts.size() == 2) {
    *name = in_parts[0];
    *index = vtr::atoi(in_parts[1]);
  } else {
    vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
              "Cannot parse %s.", in.c_str());
  }
}

void FasmWriterVisitor::output_fasm_mux(std::string fasm_mux,
                                        t_interconnect *interconnect,
                                        t_pb_graph_pin *mux_input_pin) {
    auto *pb_name = mux_input_pin->parent_node->pb_type->name;
    auto pb_index = mux_input_pin->parent_node->placement_index;
    auto *port_name = mux_input_pin->port->name;
    auto pin_index = mux_input_pin->pin_number;
    auto mux_inputs = vtr::split(fasm_mux, "\n");
    for(const auto &mux_input : mux_inputs) {
      auto mux_parts = vtr::split(vtr::replace_all(mux_input, " ", ""), "=");

      if(mux_parts.size() == 0) {
        // Swallow whitespace.
        continue;
      }

      if(mux_parts.size() != 2) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
            "fasm_mux line %s does not have 2 parts, has %d parts.\n",
            mux_input.c_str(), mux_parts.size());
      }

      auto vtr_parts = vtr::split(mux_parts[0], ".");
      if(vtr_parts.size() != 2) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
            "fasm_mux line %s does not have 2 parts, has %d parts.\n",
            mux_parts[0].c_str(), vtr_parts.size());
      }

      std::string mux_pb_name;
      int mux_pb_index;
      parse_name_with_optional_index(vtr_parts[0], &mux_pb_name, &mux_pb_index);
      std::string mux_port_name;
      int mux_pin_index;
      parse_name_with_optional_index(vtr_parts[1], &mux_port_name, &mux_pin_index);

      bool root_level_connection = interconnect->parent_mode->parent_pb_type ==
          mux_input_pin->parent_node->pb_type;

      if(root_level_connection) {
        // This connection is root level.  pb_index selects between
        // pb_type_prefixes_, not on the mux input.
        if(mux_pb_name == pb_name && mux_port_name == port_name && mux_pin_index == pin_index) {
          if(mux_parts[1] != "NULL") {
            output_fasm_features(mux_parts[1]);
          }
          return;
        }
      } else if(mux_pb_name == pb_name &&
                mux_pb_index == pb_index &&
                mux_port_name == port_name &&
                mux_pin_index == pin_index) {
        if(mux_parts[1] != "NULL") {
          output_fasm_features(mux_parts[1]);
        }
        return;
      }
    }

    vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
        "fasm_mux %s[%d].%s[%d] found no matches in:\n%s\n",
        pb_name, pb_index, port_name, pin_index, fasm_mux.c_str());
}

void FasmWriterVisitor::output_fasm_features(std::string features) {
  std::stringstream os(features);

  while(os) {
    std::string feature;
    os >> feature;
    if(os) {
      if(current_blk_has_prefix_) {
        os_ << blk_prefix_ << clb_prefix_;
      }
      os_ << feature << std::endl;
    }
  }
}
