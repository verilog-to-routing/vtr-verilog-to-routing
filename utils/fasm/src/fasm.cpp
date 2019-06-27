#include "fasm.h"

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

#include "rr_metadata.h"

#include "vtr_assert.h"
#include "vtr_logic.h"
#include "vtr_version.h"
#include "vpr_error.h"

#include "atom_netlist_utils.h"
#include "netlist_writer.h"
#include "vpr_utils.h"
#include "route_tree_timing.h"

#include "fasm_utils.h"

namespace fasm {

FasmWriterVisitor::FasmWriterVisitor(std::ostream& f) : os_(f) {}

void FasmWriterVisitor::visit_top_impl(const char* top_level_name) {
    (void)top_level_name;
    auto& device_ctx = g_vpr_ctx.device();
    pb_graph_pin_lookup_from_index_by_type_.resize(device_ctx.num_block_types);
    for(int itype = 0; itype < device_ctx.num_block_types; itype++) {
        pb_graph_pin_lookup_from_index_by_type_.at(itype) = alloc_and_load_pb_graph_pin_lookup_from_index(&device_ctx.block_types[itype]);
    }
}

void FasmWriterVisitor::visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    current_blk_id_ = blk_id;

    VTR_ASSERT(clb->pb_graph_node != nullptr);
    VTR_ASSERT(clb->pb_graph_node->pb_type);

    root_clb_ = clb->pb_graph_node;

    int x = place_ctx.block_locs[blk_id].loc.x;
    int y = place_ctx.block_locs[blk_id].loc.y;
    int z = place_ctx.block_locs[blk_id].loc.z;
    auto &grid_loc = device_ctx.grid[x][y];
    blk_type_ = grid_loc.type;

    current_blk_has_prefix_ = true;
    std::string grid_prefix;
    if(grid_loc.meta != nullptr && grid_loc.meta->has("fasm_prefix")) {
      auto* value = grid_loc.meta->get("fasm_prefix");
      VTR_ASSERT(value != nullptr);
      std::string prefix_unsplit = value->front().as_string();
      std::vector<std::string> fasm_prefixes = vtr::split(prefix_unsplit, " \t\n");
      if(fasm_prefixes.size() != static_cast<size_t>(blk_type_->capacity)) {
        vpr_throw(VPR_ERROR_OTHER,
                  __FILE__, __LINE__,
                  "number of fasm_prefix (%s) options (%d) for block (%s) must match capacity(%d)",
                  prefix_unsplit.c_str(), fasm_prefixes.size(), blk_type_->name, blk_type_->capacity);
      }
      grid_prefix = fasm_prefixes[z];
    } else {
      current_blk_has_prefix_= false;
    }

    if(current_blk_has_prefix_) {
      blk_prefix_ = grid_prefix + ".";
    }
}

void FasmWriterVisitor::check_interconnect(const t_pb_routes &pb_routes, int inode) {
  auto iter = pb_routes.find(inode);
  if(iter == pb_routes.end() || !iter->second.atom_net_id) {
    // Net is open.
    return;
  }

  /* No previous driver implies that this is either a top-level input pin
    * or a primitive output pin */
  int prev_node = iter->second.driver_pb_pin_id;
  if(prev_node == OPEN) {
    return;
  }

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
  if(interconnect->meta.has("fasm_mux")) {
    auto* value = interconnect->meta.get("fasm_mux"); 
    VTR_ASSERT(value != nullptr);
    std::string fasm_mux = value->front().as_string();
    output_fasm_mux(fasm_mux, interconnect, prev_pin);
  }
}

static std::string handle_fasm_prefix(const t_metadata_dict *meta,
        const t_pb_graph_node *pb_graph_node, const t_pb_type *pb_type) {
  bool has_prefix = meta != nullptr && meta->has("fasm_prefix");
  if(!has_prefix) {
      return "";
  }

  auto* value = meta->one("fasm_prefix");
  VTR_ASSERT(value != nullptr);
  auto fasm_prefix_unsplit = value->as_string();
  auto fasm_prefix = vtr::split(fasm_prefix_unsplit, " \t\n");
  VTR_ASSERT(pb_type->num_pb >= 0);
  if(fasm_prefix.size() != static_cast<size_t>(pb_type->num_pb)) {
    vpr_throw(VPR_ERROR_OTHER,
              __FILE__, __LINE__,
              "number of fasm_prefix (%s) options (%d) for block (%s) must match capacity(%d)",
              fasm_prefix_unsplit.c_str(), fasm_prefix.size(), pb_type->name, pb_type->num_pb);
  }

  if(pb_graph_node->placement_index >= pb_type->num_pb) {
    vpr_throw(VPR_ERROR_OTHER,
              __FILE__, __LINE__,
              "pb_graph_node->placement_index = %d >= pb_type->num_pb = %d",
              fasm_prefix_unsplit.c_str(), pb_type->num_pb);
  }

  return fasm_prefix.at(pb_graph_node->placement_index) + ".";
}

std::string FasmWriterVisitor::build_clb_prefix(const t_pb *pb, const t_pb_graph_node* pb_graph_node, bool* is_parent_pb_null) const {
  std::string clb_prefix = "";

  const t_pb *pb_for_graph_node = nullptr;

  // If not t_pb, mode_index is always 0.
  int mode_index = 0;
  if(root_clb_ != pb_graph_node && pb_graph_node->parent_pb_graph_node != root_clb_) {
    VTR_ASSERT(pb_graph_node->parent_pb_graph_node != nullptr);
    if(pb != nullptr) {
      while(pb_for_graph_node == nullptr) {
        pb_for_graph_node = pb->find_pb(pb_graph_node);

        if(pb_for_graph_node == nullptr) {
          if(pb->parent_pb == nullptr) {
            *is_parent_pb_null = true;
            break;
          }
          pb = pb->parent_pb;
        }
      }

      if(pb_for_graph_node != nullptr) {
        mode_index = pb_for_graph_node->mode;
      }
    }

    clb_prefix = build_clb_prefix(pb, pb_graph_node->parent_pb_graph_node, is_parent_pb_null);
  }

  const auto *pb_type = pb_graph_node->pb_type;

  clb_prefix += handle_fasm_prefix(&pb_type->meta, pb_graph_node, pb_type);

  if(pb_type->modes != nullptr) {
    VTR_ASSERT(mode_index < pb_type->num_modes);

    clb_prefix += handle_fasm_prefix(&pb_type->modes[mode_index].meta,
                                     pb_graph_node, pb_type);
  } else {
    VTR_ASSERT(mode_index == 0);
  }

  return clb_prefix;
}

static const t_pb_graph_pin* is_node_used(const t_pb_routes &top_pb_route, const t_pb_graph_node* pb_graph_node) {
    // Is the node used at all?
    const t_pb_graph_pin* pin = nullptr;
    for(int port_index = 0; port_index < pb_graph_node->num_output_ports; ++port_index) {
        for(int pin_index = 0; pin_index < pb_graph_node->num_output_pins[port_index]; ++pin_index) {
            pin = &pb_graph_node->output_pins[port_index][pin_index];
            if (top_pb_route.count(pin->pin_count_in_cluster) > 0 && top_pb_route[pin->pin_count_in_cluster].atom_net_id != AtomNetId::INVALID()) {
                return pin;
            }
        }
    }
    for(int port_index = 0; port_index < pb_graph_node->num_input_ports; ++port_index) {
        for(int pin_index = 0; pin_index < pb_graph_node->num_input_pins[port_index]; ++pin_index) {
            pin = &pb_graph_node->input_pins[port_index][pin_index];
            if (top_pb_route.count(pin->pin_count_in_cluster) > 0 && top_pb_route[pin->pin_count_in_cluster].atom_net_id != AtomNetId::INVALID()) {
                return pin;
            }
        }
    }
    return nullptr;
}

void FasmWriterVisitor::check_features(const t_metadata_dict *meta) const {
  if(meta == nullptr) {
    return;
  }

  if(!meta->has("fasm_features")) {
    return;
  }

  auto* value = meta->one("fasm_features");
  VTR_ASSERT(value != nullptr);

  output_fasm_features(value->as_string());
}

void FasmWriterVisitor::visit_all_impl(const t_pb_routes &pb_routes, const t_pb* pb) {
  VTR_ASSERT(pb != nullptr);
  VTR_ASSERT(pb->pb_graph_node != nullptr);

  const t_pb_graph_node *pb_graph_node = pb->pb_graph_node;

  // Check if this PB is `open` and has to be skipped
  bool is_parent_pb_null = false;
  std::string clb_prefix = build_clb_prefix(pb, pb_graph_node, &is_parent_pb_null);
  if (is_parent_pb_null == true) {
    return;
  }
  clb_prefix_ = clb_prefix;

  t_pb_type *pb_type = pb_graph_node->pb_type;
  auto *mode = &pb_type->modes[pb->mode];

  check_features(&pb_type->meta);
  if(mode != nullptr) {
    check_features(&mode->meta);
  }

  if(mode != nullptr && std::string(mode->name) == "wire") {
    auto io_pin = is_node_used(pb_routes, pb_graph_node);
    if(io_pin != nullptr) {
      const auto& route = pb_routes.at(io_pin->pin_count_in_cluster);
      const int num_inputs = *route.pb_graph_pin->parent_node->num_input_pins;
      const auto *lut_definition = find_lut(route.pb_graph_pin->parent_node);
      VTR_ASSERT(lut_definition->num_inputs == num_inputs);

      output_fasm_features(lut_definition->CreateWire(route.pb_graph_pin->pin_number));
    }
  }

  int port_index = 0;
  for (int i = 0; i < pb_type->num_ports; i++) {
    if (pb_type->ports[i].is_clock || pb_type->ports[i].type != IN_PORT) {
      continue;
    }
    for (int j = 0; j < pb_type->ports[i].num_pins; j++) {
      int inode = pb->pb_graph_node->input_pins[port_index][j].pin_count_in_cluster;
      check_interconnect(pb_routes, inode);
    }
    port_index += 1;
  }

  port_index = 0;
  for (int i = 0; i < pb_type->num_ports; i++) {
    if (!pb_type->ports[i].is_clock || pb_type->ports[i].type != IN_PORT) {
      continue;
    }
    for (int j = 0; j < pb_type->ports[i].num_pins; j++) {
      int inode = pb->pb_graph_node->clock_pins[port_index][j].pin_count_in_cluster;
      check_interconnect(pb_routes, inode);
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
      check_interconnect(pb_routes, inode);
    }
    port_index += 1;
  }
}

void FasmWriterVisitor::visit_route_through_impl(const t_pb* atom) {
  check_for_lut(atom);
  check_for_param(atom);
}

static AtomNetId _find_atom_input_logical_net(const t_pb* atom, const t_pb_routes &pb_route, int atom_input_idx) {
    const t_pb_graph_node* pb_node = atom->pb_graph_node;
    const int cluster_pin_idx = pb_node->input_pins[0][atom_input_idx].pin_count_in_cluster;
    if(pb_route.count(cluster_pin_idx) > 0) {
        return pb_route[cluster_pin_idx].atom_net_id;
    } else {
        return AtomNetId::INVALID();
    }
}

static LogicVec lut_outputs(const t_pb* atom_pb, size_t num_inputs, const t_pb_routes &pb_route) {
    auto& atom_ctx = g_vpr_ctx.atom();
    AtomBlockId block_id = atom_ctx.lookup.pb_atom(atom_pb);
    const auto& truth_table = atom_ctx.nlist.block_truth_table(block_id);
    auto ports = atom_ctx.nlist.block_input_ports(atom_ctx.lookup.pb_atom(atom_pb));

    const t_pb_graph_node* gnode = atom_pb->pb_graph_node;

    if(ports.size() != 1) {
      if(ports.size() != 0) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "LUT port unexpected size is %d", ports.size());
      }

      Lut lut(num_inputs);
      if(truth_table.size() == 1) {
        VTR_ASSERT(truth_table[0].size() == 1);
        lut.SetConstant(truth_table[0][0]);
      } else if(truth_table.size() == 0) {
        lut.SetConstant(vtr::LogicValue::FALSE);
      } else {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "LUT truth table unexpected size is %d", truth_table.size());
      }

      return lut.table();
    }

    VTR_ASSERT(gnode->num_input_ports == 1);
    //VTR_ASSERT(gnode->num_input_pins[0] >= num_inputs);
    std::vector<vtr::LogicValue> inputs(num_inputs, vtr::LogicValue::DONT_CARE);
    std::vector<int> permutation(num_inputs, -1);

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

    // truth_table a vector of truth table rows.  The last value in each row is
    // the output value, and the other values are the input values.  Open pins
    // are don't cares, and should be -1 in the permutation table.
    Lut lut(num_inputs);
    for(const auto& row : truth_table) {
        std::fill(std::begin(inputs), std::end(inputs), vtr::LogicValue::DONT_CARE);
        for(size_t i = 0; i < row.size() - 1; i++) {
            int permuted_idx = permutation[i];
            if(permuted_idx != -1) {
              inputs[permuted_idx] = row[i];
            }
        }

        lut.SetOutput(inputs, row[row.size() - 1]);
    }

    return lut.table();
}

static const t_metadata_dict *get_fasm_type(const t_pb_graph_node* pb_graph_node, std::string target_type) {
  if(pb_graph_node == nullptr) {
    return nullptr;
  }

  if(pb_graph_node->pb_type == nullptr) {
    return nullptr;
  }

  const t_metadata_dict *meta = nullptr;
  if(pb_graph_node->pb_type->meta.has("fasm_type")) {
    meta = &pb_graph_node->pb_type->meta;
  }

  if(pb_graph_node->pb_type->parent_mode != nullptr) {
    VTR_ASSERT(pb_graph_node->pb_type->parent_mode->parent_pb_type != nullptr);
    const t_pb_type *pb_type = pb_graph_node->pb_type->parent_mode->parent_pb_type;
    if(pb_graph_node->pb_type->parent_mode->meta.has("fasm_type")) {
      meta = &pb_graph_node->pb_type->parent_mode->meta;
    } else if(pb_type->num_modes <= 1) {
      if(pb_type->meta.has("fasm_type")) {
        meta = &pb_type->meta;
      }
    }
  }

  if(meta != nullptr) {
    auto* value = meta->one("fasm_type");
    VTR_ASSERT(value != nullptr);
    if(value->as_string() == target_type) {
      return meta;
    }
  }

  return nullptr;
}

const LutOutputDefinition* FasmWriterVisitor::find_lut(const t_pb_graph_node* pb_graph_node) {
  while(pb_graph_node != nullptr) {
    VTR_ASSERT(pb_graph_node->pb_type != nullptr);

    auto iter = lut_definitions_.find(pb_graph_node->pb_type);
    if(iter == lut_definitions_.end()) {
      const t_metadata_dict *meta = get_fasm_type(pb_graph_node, "LUT");
      if(meta != nullptr) {
        VTR_ASSERT(meta->has("fasm_lut"));
        auto* value = meta->one("fasm_lut");
        VTR_ASSERT(value != nullptr);

        std::vector<std::pair<std::string, LutOutputDefinition>> luts;
        luts.push_back(std::make_pair(
            vtr::string_fmt("%s[0]", pb_graph_node->pb_type->name),
            LutOutputDefinition(value->as_string())));

        auto insert_result = lut_definitions_.insert(
            std::make_pair(pb_graph_node->pb_type, luts));
        VTR_ASSERT(insert_result.second);
        iter = insert_result.first;
      }

      meta = get_fasm_type(pb_graph_node, "SPLIT_LUT");
      if(meta != nullptr) {
        VTR_ASSERT(meta->has("fasm_lut"));
        auto* value = meta->one("fasm_lut");
        VTR_ASSERT(value != nullptr);
        std::string fasm_lut = value->as_string();
        auto lut_parts = split_fasm_entry(fasm_lut, "\n", "\t ");
        if(__builtin_popcount(lut_parts.size()) != 1) {
          vpr_throw(VPR_ERROR_OTHER,
                    __FILE__, __LINE__,
                    "Number of lut splits must be power of two, found %d parts",
                    lut_parts.size());
        }

        std::vector<std::pair<std::string, LutOutputDefinition>> luts;
        luts.reserve(lut_parts.size());
        for(const auto &part : lut_parts) {
          auto parts = vtr::split(part, "=");
          if(parts.size() != 2) {
            vpr_throw(VPR_ERROR_OTHER,
                      __FILE__, __LINE__,
                      "Split lut definition fasm_lut = %s does not parse.",
                      fasm_lut.c_str());
          }

          luts.push_back(std::make_pair(
              parts[1], LutOutputDefinition(parts[0])));
        }

        auto insert_result = lut_definitions_.insert(
            std::make_pair(pb_graph_node->pb_type,
                           luts));
        VTR_ASSERT(insert_result.second);
        iter = insert_result.first;
      }
    }

    if(iter != lut_definitions_.end()) {
      auto string_at_node = vtr::string_fmt("%s[%d]", pb_graph_node->pb_type->name, pb_graph_node->placement_index);
      for(const auto &lut : iter->second) {
        if(lut.first == string_at_node) {
          return &lut.second;
        }
      }
    }

    pb_graph_node = pb_graph_node->parent_pb_graph_node;
  }

  vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
            "Failed to find LUT output definition.");
  return nullptr;
}

static const t_pb_routes &find_pb_route(const t_pb* pb) {
  const t_pb* parent = pb->parent_pb;
  while(parent != nullptr) {
    pb = parent;
    parent = pb->parent_pb;
  }
  return pb->pb_route;
}

void FasmWriterVisitor::check_for_param(const t_pb *atom) {
    auto& atom_ctx = g_vpr_ctx.atom();

    auto atom_blk_id = atom_ctx.lookup.pb_atom(atom);
    if (atom_blk_id == AtomBlockId::INVALID()) {
        return;
    }

    if(atom->pb_graph_node == nullptr ||
       atom->pb_graph_node->pb_type == nullptr) {
        return;
    }

    const auto *meta = &atom->pb_graph_node->pb_type->meta;
    if(!meta->has("fasm_params")) {
        return;
    }

    auto iter = parameters_.find(atom->pb_graph_node->pb_type);

    if(iter == parameters_.end()) {
        Parameters params;
        auto* value = meta->one("fasm_params");
        VTR_ASSERT(value != nullptr);

        std::string fasm_params = value->as_string();
        for(const auto param : vtr::split(fasm_params, "\n")) {
          auto param_parts = split_fasm_entry(param, "=", "\t ");
            if(param_parts.size() == 0) {
                continue;
            }
            VTR_ASSERT(param_parts.size() == 2);

            params.AddParameter(param_parts[1], param_parts[0]);
        }

        auto ret = parameters_.insert(std::make_pair(
                    atom->pb_graph_node->pb_type,
                    params));

        VTR_ASSERT(ret.second);
        iter = ret.first;
    }

    auto &params = iter->second;

    for(auto param : atom_ctx.nlist.block_params(atom_blk_id)) {
        auto feature = params.EmitFasmFeature(param.first, param.second);

        if(feature.size() > 0) {
            output_fasm_features(feature);
        }
    }
}

void FasmWriterVisitor::check_for_lut(const t_pb* atom) {
    auto& atom_ctx = g_vpr_ctx.atom();

    auto atom_blk_id = atom_ctx.lookup.pb_atom(atom);
    if (atom_blk_id == AtomBlockId::INVALID()) {
        return;
    }

    const t_model* model = atom_ctx.nlist.block_model(atom_blk_id);
    if (model->name == std::string(MODEL_NAMES)) {
      VTR_ASSERT(atom->pb_graph_node != nullptr);
      const auto *lut_definition = find_lut(atom->pb_graph_node);
      VTR_ASSERT(lut_definition->num_inputs == *atom->pb_graph_node->num_input_pins);

      const t_pb_routes &pb_route = find_pb_route(atom);
      LogicVec lut_mask = lut_outputs(atom, lut_definition->num_inputs, pb_route);
      output_fasm_features(lut_definition->CreateInit(lut_mask));
    }
}

void FasmWriterVisitor::visit_atom_impl(const t_pb* atom) {
  check_for_lut(atom);
  check_for_param(atom);
}

void FasmWriterVisitor::walk_route_tree(const t_rt_node *root) {
    for (t_linked_rt_edge* edge = root->u.child_list; edge != nullptr; edge = edge->next) {
        auto *meta = vpr::rr_edge_metadata(root->inode, edge->child->inode, edge->iswitch, "fasm_features");
        if(meta != nullptr) {
            current_blk_has_prefix_ = false;
            output_fasm_features(meta->as_string());
        }

        walk_route_tree(edge->child);
    }
}

void FasmWriterVisitor::walk_routing() {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    for(const auto &trace : route_ctx.trace) {
      t_trace *head = trace.head;
      t_rt_node* root = traceback_to_route_tree(head);
      walk_route_tree(root);
      free_route_tree(root);
    }
}


void FasmWriterVisitor::finish_impl() {
    auto& device_ctx = g_vpr_ctx.device();
    for(int itype = 0; itype < device_ctx.num_block_types; itype++) {
        free_pb_graph_pin_lookup_from_index (pb_graph_pin_lookup_from_index_by_type_.at(itype));
    }

    walk_routing();
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
      auto mux_parts = split_fasm_entry(mux_input, "=:", "\t ");

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

      auto fasm_features = vtr::join(vtr::split(mux_parts[1], ","), "\n");

      if(root_level_connection) {
        // This connection is root level.  pb_index selects between
        // pb_type_prefixes_, not on the mux input.
        if(mux_pb_name == pb_name && mux_port_name == port_name && mux_pin_index == pin_index) {
          if(mux_parts[1] != "NULL") {
            output_fasm_features(fasm_features);
          }
          return;
        }
      } else if(mux_pb_name == pb_name &&
                mux_pb_index == pb_index &&
                mux_port_name == port_name &&
                mux_pin_index == pin_index) {
        if(mux_parts[1] != "NULL") {
          output_fasm_features(fasm_features);
        }
        return;
      }
    }

    vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
        "fasm_mux %s[%d].%s[%d] found no matches in:\n%s\n",
        pb_name, pb_index, port_name, pin_index, fasm_mux.c_str());
}

void FasmWriterVisitor::output_fasm_features(std::string features) const {
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

} // namespace fasm
