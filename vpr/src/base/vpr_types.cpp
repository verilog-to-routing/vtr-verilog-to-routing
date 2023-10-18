#include <cmath>
#include "vpr_types.h"
#include "globals.h"

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

/**
 * @brief Returns the read-only t_pb associated with the specified gnode which is contained
 *        within the current pb
 */
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

/**
 * @brief Returns the mutable t_pb associated with the specified gnode which is contained
 *        within the current pb
 */
t_pb* t_pb::find_mutable_pb(const t_pb_graph_node* gnode) {
    //Base case
    if (pb_graph_node == gnode) {
        return this;
    }

    //Search recursively
    for (int ichild_type = 0; ichild_type < get_num_child_types(); ++ichild_type) {
        if (child_pbs[ichild_type] == nullptr) continue;

        for (int ipb = 0; ipb < get_num_children_of_type(ichild_type); ++ipb) {
            t_pb* child_pb = &child_pbs[ichild_type][ipb];

            t_pb* found_pb = child_pb->find_mutable_pb(gnode);
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

/**
 * @brief Returns the root pb containing this pb
 */
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

/**
 * @brief Returns the bit index into the AtomPort for the specified primitive
 *        pb_graph_pin, considering any pin rotations which have been applied to logically
 *        equivalent pins
 */
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

/**
 * @brief For a given gpin, sets the mapping to the original atom netlist pin's bit index in
 *        it's AtomPort.
 *
 * This is used to record any pin rotations which have been applied to
 * logically equivalent pins
 */
void t_pb::set_atom_pin_bit_index(const t_pb_graph_pin* gpin, BitIndex atom_pin_bit_idx) {
    pin_rotations_[gpin] = atom_pin_bit_idx;
}

void free_pack_molecules(t_pack_molecule* list_of_pack_molecules) {
    t_pack_molecule* cur_pack_molecule = list_of_pack_molecules;
    while (cur_pack_molecule != nullptr) {
        cur_pack_molecule = list_of_pack_molecules->next;
        delete list_of_pack_molecules;
        list_of_pack_molecules = cur_pack_molecule;
    }
}

/**
 * Free linked lists found in cluster_placement_stats_list
 */
void free_cluster_placement_stats(t_cluster_placement_stats* cluster_placement_stats_list) {
    auto& device_ctx = g_vpr_ctx.device();

    for (const auto& type : device_ctx.logical_block_types) {
        int index = type.index;
        cluster_placement_stats_list[index].free_primitives();
    }
    delete[] cluster_placement_stats_list;
}

void t_cluster_placement_stats::move_inflight_to_tried() {
    tried.insert(*in_flight.begin());
    in_flight.clear();
}

void t_cluster_placement_stats::invalidate_primitive_and_increment_iterator(int pb_type_index, std::unordered_multimap<int, t_cluster_placement_primitive*>::iterator& it) {
    invalid.insert(*it);
    valid_primitives[pb_type_index].erase(it++);
}

void t_cluster_placement_stats::move_primitive_to_inflight(int pb_type_index, std::unordered_multimap<int, t_cluster_placement_primitive*>::iterator& it) {
    in_flight.insert(*it);
    valid_primitives[pb_type_index].erase(it);
}

/**
 * @brief Put primitive back on the correct location of valid primitives vector based on the primitive pb type
 *
 * @note that valid status is not changed because if the primitive is not valid, it will get properly collected later
 */
void t_cluster_placement_stats::insert_primitive_in_valid_primitives(std::pair<int, t_cluster_placement_primitive*> cluster_placement_primitive) {
    int i;
    bool success = false;
    int null_index = OPEN;
    t_cluster_placement_primitive* input_cluster_placement_primitive = cluster_placement_primitive.second;

    for (i = 0; i < num_pb_types && !success; i++) {
        if (valid_primitives[i].empty()) {
            null_index = i;
            continue;
        }
        t_cluster_placement_primitive* cur_cluster_placement_primitive = valid_primitives[i].begin()->second;
        if (input_cluster_placement_primitive->pb_graph_node->pb_type
            == cur_cluster_placement_primitive->pb_graph_node->pb_type) {
            success = true;
            valid_primitives[i].insert(cluster_placement_primitive);
        }
    }
    if (!success) {
        VTR_ASSERT(null_index != OPEN);
        valid_primitives[null_index].insert(cluster_placement_primitive);
    }
}

void t_cluster_placement_stats::flush_queue(std::unordered_multimap<int, t_cluster_placement_primitive*>& queue) {
    for (auto& it : queue) {
        insert_primitive_in_valid_primitives(it);
    }
    queue.clear();
}

void t_cluster_placement_stats::flush_intermediate_queues() {
    flush_queue(in_flight);
    flush_queue(tried);
}

void t_cluster_placement_stats::flush_invalid_queue() {
    flush_queue(invalid);
}

bool t_cluster_placement_stats::in_flight_empty() {
    return (in_flight.empty());
}

t_pb_type* t_cluster_placement_stats::in_flight_type() {
    return (in_flight.begin()->second->pb_graph_node->pb_type);
}

void t_cluster_placement_stats::free_primitives() {
    for (auto& primitive : tried)
        delete primitive.second;

    for (auto& primitive : in_flight)
        delete primitive.second;

    for (auto& primitive : invalid)
        delete primitive.second;

    for (int j = 0; j < num_pb_types; j++) {
        for (auto& primitive : valid_primitives[j]) {
            delete primitive.second;
        }
    }
}

AtomBlockId GridBlock::block_at_location(const t_pl_atom_loc& loc) const {
    const auto& atom_lookup = g_vpr_ctx.atom().lookup;
    t_pl_loc cluster_loc (loc.x, loc.y, loc.sub_tile, loc.layer);
    ClusterBlockId cluster_at_loc = block_at_location(cluster_loc);
    if (cluster_at_loc == EMPTY_BLOCK_ID) {
        return EMPTY_PRIMITIVE_BLOCK_ID;
    } else if (cluster_at_loc == INVALID_BLOCK_ID) {
        return INVALID_PRIMITIVE_BLOCK_ID;
    } else {
        VTR_ASSERT(cluster_at_loc.is_valid());
        const auto& cluster_atoms = g_vpr_ctx.cl_helper().atoms_lookup;
        const auto& atom_list = cluster_atoms.at(cluster_at_loc);
        for (const auto& atom : atom_list) {
            int primitive_pin = atom_lookup.atom_pb_graph_node(atom)->primitive_num;
            t_pl_atom_loc atom_loc(primitive_pin, cluster_loc.x, cluster_loc.y, cluster_loc.sub_tile, cluster_loc.layer);
            if (atom_loc == loc) {
                return atom;
            }
        }
        return EMPTY_PRIMITIVE_BLOCK_ID;
    }
}

t_cluster_placement_primitive* t_cluster_placement_stats::get_cluster_placement_primitive_from_pb_graph_node(const t_pb_graph_node* pb_graph_node) {
    auto it = valid_primitives[pb_graph_node->cluster_placement_type_index].find(pb_graph_node->cluster_placement_primitive_index);
    if (it != valid_primitives[pb_graph_node->cluster_placement_type_index].end())
        return valid_primitives[pb_graph_node->cluster_placement_type_index][pb_graph_node->cluster_placement_primitive_index];

    for (auto itr = tried.find(pb_graph_node->cluster_placement_primitive_index); itr != tried.end(); itr++) {
        if (itr->second->pb_graph_node->cluster_placement_type_index == pb_graph_node->cluster_placement_type_index)
            return itr->second;
    }

    for (auto itr = invalid.find(pb_graph_node->cluster_placement_primitive_index); itr != invalid.end(); itr++) {
        if (itr->second->pb_graph_node->cluster_placement_type_index == pb_graph_node->cluster_placement_type_index)
            return itr->second;
    }

    for (auto itr = in_flight.find(pb_graph_node->cluster_placement_primitive_index); itr != in_flight.end(); itr++) {
        if (itr->second->pb_graph_node->cluster_placement_type_index == pb_graph_node->cluster_placement_type_index)
            return itr->second;
    }

    return nullptr;
}