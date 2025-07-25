#include "fasm.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

#include "globals.h"

#include "rr_metadata.h"

#include "vtr_assert.h"
#include "vtr_logic.h"
#include "vpr_error.h"

#include "atom_netlist_utils.h"

#include "fasm_utils.h"

namespace fasm {

FasmWriterVisitor::FasmWriterVisitor(vtr::string_internment *strings, std::ostream& f, bool is_flat) : strings_(strings), os_(f),
    pb_graph_pin_lookup_from_index_by_type_(g_vpr_ctx.device().logical_block_types),
    fasm_lut(strings->intern_string(vtr::string_view("fasm_lut"))),
    fasm_features(strings->intern_string(vtr::string_view("fasm_features"))),
    fasm_params(strings->intern_string(vtr::string_view("fasm_params"))),
    fasm_prefix(strings->intern_string(vtr::string_view("fasm_prefix"))),
    fasm_placeholders(strings->intern_string(vtr::string_view("fasm_placeholders"))),
    fasm_type(strings->intern_string(vtr::string_view("fasm_type"))),
    fasm_mux(strings->intern_string(vtr::string_view("fasm_mux"))),
    is_flat_(is_flat){
}

void FasmWriterVisitor::visit_top_impl(const char* top_level_name) {
    (void)top_level_name;
}

void FasmWriterVisitor::visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& block_locs = g_vpr_ctx.placement().block_locs();

    current_blk_id_ = blk_id;

    VTR_ASSERT(clb->pb_graph_node != nullptr);
    VTR_ASSERT(clb->pb_graph_node->pb_type);

    root_clb_ = clb->pb_graph_node;

    int x = block_locs[blk_id].loc.x;
    int y = block_locs[blk_id].loc.y;
    int layer_num = block_locs[blk_id].loc.layer;
    int sub_tile = block_locs[blk_id].loc.sub_tile;
    physical_tile_ = device_ctx.grid.get_physical_type({x, y, layer_num});
    logical_block_ = cluster_ctx.clb_nlist.block_type(blk_id);
    const t_metadata_dict* grid_meta = device_ctx.grid.get_metadata({x, y, layer_num});

    blk_prefix_ = "";
    clb_prefix_ = "";
    clb_prefix_map_.clear();

    // Get placeholder list (if provided)
    tags_.clear();
    if(grid_meta != nullptr && grid_meta->has(fasm_placeholders)) {
      const std::vector<t_metadata_value>* value = grid_meta->get(fasm_placeholders);
      VTR_ASSERT(value != nullptr);

      // Parse placeholder definition
      std::vector<std::string> tag_defs = vtr::StringToken(value->front().as_string().get(strings_)).split("\n");
      for (std::string& tag_def: tag_defs) {
        std::vector<std::string> parts = split_fasm_entry(tag_def, "=:", "\t ");
        if (parts.empty()) {
          continue;
        }

        VTR_ASSERT(parts.size() == 2);

        VTR_ASSERT(tags_.count(parts.at(0)) == 0);

        // When the value is "NULL" then substitute empty string
        if (parts.at(1) == "NULL") {
            tags_[parts.at(0)] = "";
        }
        else {
            tags_[parts.at(0)] = parts.at(1);
        }
      }
    }

    std::string grid_prefix;
    if(grid_meta != nullptr && grid_meta->has(fasm_prefix)) {
      const std::vector<t_metadata_value>* value = grid_meta->get(fasm_prefix);
      VTR_ASSERT(value != nullptr);
      std::string prefix_unsplit = value->front().as_string().get(strings_);
      std::vector<std::string> fasm_prefixes = vtr::StringToken(prefix_unsplit).split(" \t\n");
      if(fasm_prefixes.size() != static_cast<size_t>(physical_tile_->capacity)) {
        vpr_throw(VPR_ERROR_OTHER,
                  __FILE__, __LINE__,
                  "number of fasm_prefix (%s) options (%d) for block (%s) must match capacity(%d)",
                  prefix_unsplit.c_str(), fasm_prefixes.size(), physical_tile_->name.c_str(), physical_tile_->capacity);
      }
      grid_prefix = fasm_prefixes[sub_tile];
      blk_prefix_ = grid_prefix + ".";
    }
    else {
      blk_prefix_ = "";
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

  const t_pb_graph_pin *prev_pin = pb_graph_pin_lookup_from_index_by_type_.pb_gpin(logical_block_->index, prev_node);

  int prev_edge;
  for(prev_edge = 0; prev_edge < prev_pin->num_output_edges; prev_edge++) {
      VTR_ASSERT(prev_pin->output_edges[prev_edge]->num_output_pins == 1);
      if(prev_pin->output_edges[prev_edge]->output_pins[0]->pin_count_in_cluster == inode) {
          break;
      }
  }
  VTR_ASSERT(prev_edge < prev_pin->num_output_edges);

  auto *interconnect = prev_pin->output_edges[prev_edge]->interconnect;
  if(interconnect->meta.has(fasm_mux)) {
    auto* value = interconnect->meta.get(fasm_mux);
    VTR_ASSERT(value != nullptr);
    std::string fasm_mux_str = value->front().as_string().get(strings_);
    output_fasm_mux(fasm_mux_str, interconnect, prev_pin);
  }
}

std::string FasmWriterVisitor::handle_fasm_prefix(const t_metadata_dict *meta,
        const t_pb_graph_node *pb_graph_node, const t_pb_type *pb_type) const {
  bool has_prefix = meta != nullptr && meta->has(fasm_prefix);
  if(!has_prefix) {
      return "";
  }

  auto* value = meta->one(fasm_prefix);
  VTR_ASSERT(value != nullptr);
  auto fasm_prefix_unsplit = value->as_string().get(strings_);
  auto fasm_prefixes = vtr::StringToken(fasm_prefix_unsplit).split(" \t\n");
  VTR_ASSERT(pb_type->num_pb >= 0);
  if(fasm_prefixes.size() != static_cast<size_t>(pb_type->num_pb)) {
    vpr_throw(VPR_ERROR_OTHER,
              __FILE__, __LINE__,
              "number of fasm_prefix (%s) options (%d) for block (%s) must match capacity(%d)",
              fasm_prefix_unsplit.c_str(), fasm_prefixes.size(), pb_type->name, pb_type->num_pb);
  }

  if(pb_graph_node->placement_index >= pb_type->num_pb) {
    vpr_throw(VPR_ERROR_OTHER,
              __FILE__, __LINE__,
              "pb_graph_node->placement_index = %d >= pb_type->num_pb = %d",
              fasm_prefix_unsplit.c_str(), pb_type->num_pb);
  }

  return fasm_prefixes.at(pb_graph_node->placement_index) + ".";
}

std::string FasmWriterVisitor::build_clb_prefix(const t_pb *pb, const t_pb_graph_node* pb_graph_node, bool* is_parent_pb_null) const {
  std::string clb_prefix;

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

  const t_pb_type* pb_type = pb_graph_node->pb_type;

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

/**
 * @brief Returns true if the given pin is used (i.e. is not "open").
 */
static bool is_pin_used(const t_pb_graph_pin* pin, const t_pb_routes &top_pb_route) {
    // A pin is used if it has a pb_route that is connected to an atom net.
    if (top_pb_route.count(pin->pin_count_in_cluster) == 0)
        return false;
    if (!top_pb_route[pin->pin_count_in_cluster].atom_net_id.is_valid())
        return false;
    return true;
}

/**
 * @brief Returns the input pin for the given wire.
 *
 * Wires in VPR are a special primitive which is a LUT which acts like a wire
 * pass-through. Only one input of this LUT should be used.
 *
 *  @param top_pb_route
 *      The top pb route for the cluster that contains the wire.
 *  @param pb_graph_node
 *      The pb_graph_node of the wire primitive that we are getting the input
 *      pin for.
 */
static const t_pb_graph_pin* get_wire_input_pin(const t_pb_routes &top_pb_route, const t_pb_graph_node* pb_graph_node) {
    const t_pb_graph_pin* wire_input_pin = nullptr;
    for(int port_index = 0; port_index < pb_graph_node->num_input_ports; ++port_index) {
        for(int pin_index = 0; pin_index < pb_graph_node->num_input_pins[port_index]; ++pin_index) {
            const t_pb_graph_pin* pin = &pb_graph_node->input_pins[port_index][pin_index];
            if (is_pin_used(pin, top_pb_route)) {
                VTR_ASSERT_MSG(wire_input_pin == nullptr,
                               "Wire found with more than 1 used input");
                wire_input_pin = pin;
            }
        }
    }
    return wire_input_pin;
}

/**
 * @brief Returns true if the given wire is used.
 *
 * A wire is used if it has a used output pin.
 *
 *  @param top_pb_route
 *      The top pb route for the cluster that contains the wire.
 *  @param pb_graph_node
 *      The pb_graph_node of the wire primitive that we are checking is used.
 */
static bool is_wire_used(const t_pb_routes &top_pb_route, const t_pb_graph_node* pb_graph_node) {
    // A wire is used if it has a used output pin.
    const t_pb_graph_pin* wire_output_pin = nullptr;
    for(int port_index = 0; port_index < pb_graph_node->num_output_ports; ++port_index) {
        for(int pin_index = 0; pin_index < pb_graph_node->num_output_pins[port_index]; ++pin_index) {
            const t_pb_graph_pin* pin = &pb_graph_node->output_pins[port_index][pin_index];
            if (is_pin_used(pin, top_pb_route)) {
                VTR_ASSERT_MSG(wire_output_pin == nullptr,
                               "Wire found with more than 1 used output");
                wire_output_pin = pin;
            }
        }
    }

    if (wire_output_pin != nullptr)
        return true;

    return false;
}

void FasmWriterVisitor::check_features(const t_metadata_dict *meta) const {
  if(meta == nullptr) {
    return;
  }

  if(!meta->has(fasm_features)) {
    return;
  }

  auto* value = meta->one(fasm_features);
  VTR_ASSERT(value != nullptr);

  output_fasm_features(value->as_string().get(strings_));
}

void FasmWriterVisitor::visit_all_impl(const t_pb_routes &pb_routes, const t_pb* pb) {
  VTR_ASSERT(pb != nullptr);
  VTR_ASSERT(pb->pb_graph_node != nullptr);

  const t_pb_graph_node *pb_graph_node = pb->pb_graph_node;

  // Check if this PB is `open` and has to be skipped
  bool is_parent_pb_null = false;
  std::string clb_prefix = build_clb_prefix(pb, pb_graph_node, &is_parent_pb_null);
  clb_prefix_map_.insert(std::make_pair(pb_graph_node, clb_prefix));
  clb_prefix_ = clb_prefix;
  if (is_parent_pb_null) {
    return;
  }

  t_pb_type* pb_type = pb_graph_node->pb_type;
  t_mode* mode = &pb_type->modes[pb->mode];

  check_features(&pb_type->meta);
  if(mode != nullptr) {
    check_features(&mode->meta);
  }

  if(mode != nullptr && std::string(mode->name) == "wire") {
    // Check if the wire is used. If the wire is unused (i.e. it does not connect
    // to anything), it does not need to be created.
    if (is_wire_used(pb_routes, pb_graph_node)) {
      // Get the input pin of the LUT that feeds the wire. There should be one
      // and only one.
      const t_pb_graph_pin* wire_input_pin = get_wire_input_pin(pb_routes, pb_graph_node);
      VTR_ASSERT_MSG(wire_input_pin != nullptr,
                     "Wire found with no used input pins");

      // Get the route going into this pin.
      const auto& route = pb_routes.at(wire_input_pin->pin_count_in_cluster);

      // Find the lut definition for the parent of this wire.
      const int num_inputs = *route.pb_graph_pin->parent_node->num_input_pins;
      const auto *lut_definition = find_lut(route.pb_graph_pin->parent_node);
      VTR_ASSERT(lut_definition->num_inputs == num_inputs);

      // Create a wire implementation for the LUT.
      output_fasm_features(lut_definition->CreateWire(route.pb_graph_pin->pin_number));
    } else {
      // If the wire is not used, ensure that the inputs to the wire are also
      // unused. This is just a sanity check to ensure that all wires are
      // either completely unused or have one input and one output.
      VTR_ASSERT_MSG(get_wire_input_pin(pb_routes, pb_graph_node) == nullptr,
                     "Wire found with a used input pin, but no used output pin");
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
    AtomBlockId block_id = atom_ctx.lookup().atom_pb_bimap().pb_atom(atom_pb);
    const auto& truth_table = atom_ctx.netlist().block_truth_table(block_id);
    auto ports = atom_ctx.netlist().block_input_ports(atom_ctx.lookup().atom_pb_bimap().pb_atom(atom_pb));

    const t_pb_graph_node* gnode = atom_pb->pb_graph_node;

    if(ports.size() != 1) {
      if(ports.size() != 0) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "LUT port unexpected size is %d", ports.size());
      }

      Lut lut(num_inputs);
      if(truth_table.size() == 1) {
        VTR_ASSERT(truth_table[0].size() == 1);
        lut.SetConstant(truth_table[0][0]);
      } else if(truth_table.empty()) {
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
            AtomNetId logical_net_id = atom_ctx.netlist().port_net(port_id, orig_index);
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

const t_metadata_dict *FasmWriterVisitor::get_fasm_type(const t_pb_graph_node* pb_graph_node, std::string_view target_type) const {
  if(pb_graph_node == nullptr) {
    return nullptr;
  }

  if(pb_graph_node->pb_type == nullptr) {
    return nullptr;
  }

  const t_metadata_dict *meta = nullptr;
  if(pb_graph_node->pb_type->meta.has(fasm_type)) {
    meta = &pb_graph_node->pb_type->meta;
  }

  if(pb_graph_node->pb_type->parent_mode != nullptr) {
    VTR_ASSERT(pb_graph_node->pb_type->parent_mode->parent_pb_type != nullptr);
    const t_pb_type *pb_type = pb_graph_node->pb_type->parent_mode->parent_pb_type;
    if(pb_graph_node->pb_type->parent_mode->meta.has(fasm_type)) {
      meta = &pb_graph_node->pb_type->parent_mode->meta;
    } else if(pb_type->num_modes <= 1) {
      if(pb_type->meta.has(fasm_type)) {
        meta = &pb_type->meta;
      }
    }
  }

  if(meta != nullptr) {
    auto* value = meta->one(fasm_type);
    VTR_ASSERT(value != nullptr);
    if(value->as_string().get(strings_) == target_type) {
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
        VTR_ASSERT(meta->has(fasm_lut));
        auto* value = meta->one(fasm_lut);
        VTR_ASSERT(value != nullptr);

        std::vector<std::pair<std::string, LutOutputDefinition>> luts;
        luts.emplace_back(vtr::string_fmt("%s[0]", pb_graph_node->pb_type->name),
                          LutOutputDefinition(value->as_string().get(strings_)));

        auto insert_result = lut_definitions_.insert(
            std::make_pair(pb_graph_node->pb_type, luts));
        VTR_ASSERT(insert_result.second);
        iter = insert_result.first;
      }

      meta = get_fasm_type(pb_graph_node, "SPLIT_LUT");
      if(meta != nullptr) {
        VTR_ASSERT(meta->has(fasm_lut));
        auto* value = meta->one(fasm_lut);
        VTR_ASSERT(value != nullptr);
        std::string fasm_lut_str = value->as_string().get(strings_);
        auto lut_parts = split_fasm_entry(fasm_lut_str, "\n", "\t ");
        if(__builtin_popcount(lut_parts.size()) != 1) {
          vpr_throw(VPR_ERROR_OTHER,
                    __FILE__, __LINE__,
                    "Number of lut splits must be power of two, found %d parts",
                    lut_parts.size());
        }

        std::vector<std::pair<std::string, LutOutputDefinition>> luts;
        luts.reserve(lut_parts.size());
        for(const auto &part : lut_parts) {
          auto parts = vtr::StringToken(part).split("=");
          if(parts.size() != 2) {
            vpr_throw(VPR_ERROR_OTHER,
                      __FILE__, __LINE__,
                      "Split lut definition fasm_lut = %s does not parse.",
                      fasm_lut_str.c_str());
          }

          luts.emplace_back(parts[1], LutOutputDefinition(parts[0]));
        }

        auto insert_result = lut_definitions_.insert(
            std::make_pair(pb_graph_node->pb_type,
                           luts));
        VTR_ASSERT(insert_result.second);
        iter = insert_result.first;
      }
    }

    if(iter != lut_definitions_.end()) {
      std::string string_at_node = vtr::string_fmt("%s[%d]", pb_graph_node->pb_type->name, pb_graph_node->placement_index);
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

    AtomBlockId atom_blk_id = atom_ctx.lookup().atom_pb_bimap().pb_atom(atom);
    if (atom_blk_id == AtomBlockId::INVALID()) {
        return;
    }

    if(atom->pb_graph_node == nullptr ||
       atom->pb_graph_node->pb_type == nullptr) {
        return;
    }

    const t_metadata_dict* meta = &atom->pb_graph_node->pb_type->meta;
    if(!meta->has(fasm_params)) {
        return;
    }

    auto iter = parameters_.find(atom->pb_graph_node->pb_type);

    if(iter == parameters_.end()) {
        Parameters params;
        auto* value = meta->one(fasm_params);
        VTR_ASSERT(value != nullptr);

        std::string fasm_params_str = value->as_string().get(strings_);
        for(const std::string& param : vtr::StringToken(fasm_params_str).split("\n")) {
          std::vector<std::string> param_parts = split_fasm_entry(param, "=", "\t ");
            if(param_parts.empty()) {
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

    for(const auto& param : atom_ctx.netlist().block_params(atom_blk_id)) {
        auto feature = params.EmitFasmFeature(param.first, param.second);

        if(!feature.empty()) {
            output_fasm_features(feature);
        }
    }
}

void FasmWriterVisitor::check_for_lut(const t_pb* atom) {
    auto& atom_ctx = g_vpr_ctx.atom();
    const LogicalModels& models = g_vpr_ctx.device().arch->models;

    auto atom_blk_id = atom_ctx.lookup().atom_pb_bimap().pb_atom(atom);
    if (atom_blk_id == AtomBlockId::INVALID()) {
        return;
    }

    LogicalModelId names_model_id = models.get_model_by_name(LogicalModels::MODEL_NAMES);
    LogicalModelId model_id = atom_ctx.netlist().block_model(atom_blk_id);
    if (model_id == names_model_id) {
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

void FasmWriterVisitor::walk_route_tree(const RRGraphBuilder& rr_graph_builder, const RouteTreeNode& root) {
    for(const RouteTreeNode& child: root.child_nodes()){
        const t_metadata_value* meta = vpr::rr_edge_metadata(rr_graph_builder, size_t(root.inode), size_t(child.inode), size_t(child.parent_switch), fasm_features);

        if(meta != nullptr) {
            output_fasm_features(meta->as_string().get(strings_), "", "");
        }

        walk_route_tree(rr_graph_builder, child);
    }
}

void FasmWriterVisitor::walk_routing() {
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    const auto& device_ctx = g_vpr_ctx.device();

    for(const auto &tree : route_ctx.route_trees) {
        if (!tree) continue;
        walk_route_tree(device_ctx.rr_graph_builder, tree.value().root());
    }
}


void FasmWriterVisitor::finish_impl() {
    walk_routing();
}

void FasmWriterVisitor::find_clb_prefix(const t_pb_graph_node *node,
        bool *have_prefix, std::string *clb_prefix) const {

    *have_prefix = false;
    *clb_prefix  = "";

    while(node != nullptr) {
        auto clb_prefix_itr = clb_prefix_map_.find(node);
        *have_prefix = clb_prefix_itr != clb_prefix_map_.end();
        if(*have_prefix) {
            *clb_prefix = clb_prefix_itr->second;
            return;
        }

        node = node->parent_pb_graph_node;
    }
}

void FasmWriterVisitor::output_fasm_mux(std::string_view fasm_mux_str,
                                        t_interconnect *interconnect,
                                        const t_pb_graph_pin *mux_input_pin) {
    char* pb_name = mux_input_pin->parent_node->pb_type->name;
    int pb_index = mux_input_pin->parent_node->placement_index;
    char* port_name = mux_input_pin->port->name;
    int pin_index = mux_input_pin->pin_number;
    std::vector<std::string> mux_inputs = vtr::StringToken(fasm_mux_str).split("\n");

    bool have_prefix = false;
    std::string clb_prefix;

    // If the MUX input is an input port, this MUX is in the same pb_type as the
    // port, therefore we find the prefix up to the parent pb_graph node of the
    // MUX input.
    //
    // If the MUX input is an output port, this MUX is in the pb_type on level
    // above the port. In this case, we find the prefix up to the grandparent
    // pb_graph node of the MUX input.
    if (mux_input_pin->port->type == IN_PORT) {
        find_clb_prefix(mux_input_pin->parent_node, &have_prefix, &clb_prefix);
    } else {
        VTR_ASSERT(mux_input_pin->port->type == OUT_PORT);
        find_clb_prefix(mux_input_pin->parent_node->parent_pb_graph_node, &have_prefix, &clb_prefix);
    }

    for(const std::string& mux_input : mux_inputs) {
      std::vector<std::string> mux_parts = split_fasm_entry(mux_input, "=:", "\t ");

      if(mux_parts.empty()) {
        // Swallow whitespace.
        continue;
      }

      if(mux_parts.size() != 2) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
            "fasm_mux line %s does not have 2 parts, has %d parts.\n",
            mux_input.c_str(), mux_parts.size());
      }

      std::vector<std::string> vtr_parts = vtr::StringToken(mux_parts[0]).split(".");
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

      std::string fasm_features_str = vtr::join(vtr::StringToken(mux_parts[1]).split(","), "\n");


      if(root_level_connection) {
        // This connection is root level.  pb_index selects between
        // pb_type_prefixes_, not on the mux input.
        if(mux_pb_name == pb_name && mux_port_name == port_name && mux_pin_index == pin_index) {
          if(mux_parts[1] != "NULL") {
            output_fasm_features(fasm_features_str, clb_prefix, blk_prefix_);
          }
          return;
        }
      } else if(mux_pb_name == pb_name &&
                mux_pb_index == pb_index &&
                mux_port_name == port_name &&
                mux_pin_index == pin_index) {
        if(mux_parts[1] != "NULL") {
          output_fasm_features(fasm_features_str, clb_prefix, blk_prefix_);
        }
        return;
      }
    }

    vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
        "fasm_mux %s[%d].%s[%d] found no matches in:\n%s\n",
        pb_name, pb_index, port_name, pin_index, fasm_mux_str.data());
}

void FasmWriterVisitor::output_fasm_features(const std::string& features) const {
  output_fasm_features(features, clb_prefix_, blk_prefix_);
}

void FasmWriterVisitor::output_fasm_features(const std::string& features, std::string_view clb_prefix, std::string_view blk_prefix) const {
  std::istringstream os(features);

  while(os) {
    std::string feature;
    os >> feature;
    if(os) {
      std::string out_feature;
      out_feature += blk_prefix;
      out_feature += clb_prefix;
      out_feature += feature;
      // Substitute tags
      os_ << substitute_tags(out_feature, tags_) << std::endl;
    }
  }

}

} // namespace fasm
