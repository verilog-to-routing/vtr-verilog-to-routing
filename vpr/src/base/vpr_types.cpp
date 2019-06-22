#include <cmath>
#include "vpr_types.h"

t_ext_pin_util_targets::t_ext_pin_util_targets(float default_in_util, float default_out_util) {
    defaults_.input_pin_util = default_in_util;
    defaults_.output_pin_util = default_out_util;
}

t_ext_pin_util t_ext_pin_util_targets::get_pin_util(std::string block_type_name) const {
    auto itr = overrides_.find(block_type_name);
    if (itr != overrides_.end()) {
        return itr->second;
    }
    return defaults_;
}

void t_ext_pin_util_targets::set_block_pin_util(std::string block_type_name, t_ext_pin_util target) {
    overrides_[block_type_name] = target;
}

void t_ext_pin_util_targets::set_default_pin_util(t_ext_pin_util default_target) {
    defaults_ = default_target;
}

t_pack_high_fanout_thresholds::t_pack_high_fanout_thresholds(int threshold)
    : default_(threshold) {}

void t_pack_high_fanout_thresholds::set_default(int threshold) {
    default_ = threshold;
}

void t_pack_high_fanout_thresholds::set(std::string block_type_name, int threshold) {
    overrides_[block_type_name] = threshold;
}

int t_pack_high_fanout_thresholds::get_threshold(std::string block_type_name) const {
    auto itr = overrides_.find(block_type_name);
    if (itr != overrides_.end()) {
        return itr->second;
    }
    return default_;
}

/*
 * t_pb structure function definitions
 */

int t_pb::get_num_child_types() const {
    if (child_pbs != nullptr && has_modes()) {
        return pb_graph_node->pb_type->modes[mode].num_pb_type_children;
    } else {
        return 0;
    }
}

int t_pb::get_num_children_of_type(int type_index) const {
    t_mode* mode_ptr = get_mode();
    if (mode_ptr) {
        return mode_ptr->pb_type_children[type_index].num_pb;
    }
    return 0; //No mode
}

t_mode* t_pb::get_mode() const {
    if (has_modes()) {
        return &pb_graph_node->pb_type->modes[mode];
    } else {
        return nullptr;
    }
}

//Returns the t_pb associated with the specified gnode which is contained
//within the current pb
const t_pb* t_pb::find_pb(const t_pb_graph_node* gnode) const {
    //Base case
    if (pb_graph_node == gnode) {
        return this;
    }

    //Search recursively
    for (int ichild_type = 0; ichild_type < get_num_child_types(); ++ichild_type) {
        if (child_pbs[ichild_type] == nullptr) continue;

        for (int ipb = 0; ipb < get_num_children_of_type(ichild_type); ++ipb) {
            const t_pb* child_pb = &child_pbs[ichild_type][ipb];

            const t_pb* found_pb = child_pb->find_pb(gnode);
            if (found_pb != nullptr) {
                VTR_ASSERT(found_pb->pb_graph_node == gnode);
                return found_pb; //Found
            }
        }
    }
    return nullptr; //Not found
}

const t_pb* t_pb::find_pb_for_model(const std::string& blif_model) const {
    //Base case
    const t_model* model = pb_graph_node->pb_type->model;
    if (model && model->name == blif_model) {
        return this;
    }

    //Search recursively
    for (int ichild_type = 0; ichild_type < get_num_child_types(); ++ichild_type) {
        if (child_pbs[ichild_type] == nullptr) continue;

        for (int ipb = 0; ipb < get_num_children_of_type(ichild_type); ++ipb) {
            const t_pb* child_pb = &child_pbs[ichild_type][ipb];

            const t_pb* matching_pb = child_pb->find_pb_for_model(blif_model);
            if (matching_pb) {
                return this;
            }
        }
    }
    return nullptr; //Not found
}

//Returns the root pb containing this pb
const t_pb* t_pb::root_pb() const {
    const t_pb* curr_pb = this;
    while (!curr_pb->is_root()) {
        curr_pb = curr_pb->parent_pb;
    }

    return curr_pb;
}

std::string t_pb::hierarchical_type_name() const {
    std::vector<std::string> names;

    for (const t_pb* curr = this; curr != nullptr; curr = curr->parent_pb) {
        std::string type_name;

        //get name and type of physical block
        if (curr->pb_graph_node) {
            type_name = curr->pb_graph_node->pb_type->name;
            type_name += "[" + std::to_string(curr->pb_graph_node->placement_index) + "]";

            //get the mode of the physical block
            if (!curr->is_primitive()) {
                // primitives have no modes
                std::string mode_name = curr->pb_graph_node->pb_type->modes[curr->mode].name;
                type_name += "[" + mode_name + "]";
            }
        }

        names.push_back(type_name);
    }

    //We walked up from the leaf to root, so we join in reverse order
    return vtr::join(names.rbegin(), names.rend(), "/");
}

//Returns the bit index into the AtomPort for the specified primitive
//pb_graph_pin, considering any pin rotations which have been applied to logically
//equivalent pins
BitIndex t_pb::atom_pin_bit_index(const t_pb_graph_pin* gpin) const {
    VTR_ASSERT_MSG(is_primitive(), "Atom pin indicies can only be looked up from primitives");

    auto iter = pin_rotations_.find(gpin);

    if (iter != pin_rotations_.end()) {
        //Return the original atom pin index
        return iter->second;
    } else {
        //No re-mapping, return the index directly
        return gpin->pin_number;
    }
}

//For a given gpin, sets the mapping to the original atom netlist pin's bit index in
//it's AtomPort.  This is used to record any pin rotations which have been applied to
//logically equivalent pins
void t_pb::set_atom_pin_bit_index(const t_pb_graph_pin* gpin, BitIndex atom_pin_bit_idx) {
    pin_rotations_[gpin] = atom_pin_bit_idx;
}
