/**
 * @file
 * @author  Milo Liebster
 * @date    January 2026
 * @brief   The implementation of the PackingSignatureTree class.
 */

#include <algorithm>

#include "atom_netlist.h"
#include "cluster_legalizer.h"
#include "globals.h"
#include "physical_types.h"

#include "packing_signature_tree.h"

void PackingSignatureTree::start_packing_signature(const t_logical_block_type* cluster_logical_block_type) {
    // Return immediately if packing memoization is disabled
    if (!enabled_) return;

    // Reset external IO bookkeeping
    input_nets_.clear();
    output_nets_.clear();
    packed_atoms_.clear();
    checkpoint_input_nets_.clear();
    checkpoint_new_output_nets_.clear();
    checkpoint_decremented_output_nets_.clear();
    checkpoint_new_atoms_.clear();

    routed = false;

    for (size_t i = 0; i < branch_logical_block_types_.size(); i++) {
        if (branch_logical_block_types_[i] != cluster_logical_block_type) continue;
        // Existing cluster type
        at_lcn_ = branches_[i];
        return;
    }

    // New cluster type
    at_lcn_ = new LocationAndConnectivityNode;
    checkpoint_lcn_ = at_lcn_;
    branch_logical_block_types_.push_back(cluster_logical_block_type);
    branches_.push_back(at_lcn_);
}

void PackingSignatureTree::set_checkpoint() {
    // Return immediately if packing memoization is disabled
    if (!enabled_) return;

    checkpoint_lcn_ = at_lcn_;
    checkpoint_input_nets_.clear();
    checkpoint_new_output_nets_.clear();
    checkpoint_decremented_output_nets_.clear();
    checkpoint_new_atoms_.clear();
}

void PackingSignatureTree::rollback_to_checkpoint() {
    at_lcn_ = checkpoint_lcn_;
    VTR_ASSERT(at_lcn_ != nullptr);
    for (auto it = checkpoint_input_nets_.begin(); it != checkpoint_input_nets_.end(); it++) {
        if (input_nets_[it->first].size() == it->second) {
            input_nets_.erase(it->first);
        } else {
            input_nets_[it->first].resize(input_nets_[it->first].size() - it->second);
        }
    }
    for (auto it = checkpoint_decremented_output_nets_.begin(); it != checkpoint_decremented_output_nets_.end(); it++) {
        output_nets_[it->first].external_sinks_count += it->second;
    }
    for (auto net : checkpoint_new_output_nets_) {
        output_nets_.erase(net);
    }
    for (auto atom : checkpoint_new_atoms_) {
        packed_atoms_.erase(atom);
    }
}

void PackingSignatureTree::add_lcn(const t_pb_graph_node* primitive_pb_graph_node, const AtomBlockId atom_block_id) {
    // Return immediately if packing memoization is disabled
    if (!enabled_) return;

    VTR_ASSERT(at_lcn_ != nullptr);

    LocationAndConnectivityNode* new_lcn = this->create_lcn(primitive_pb_graph_node, atom_block_id);

    // Determine whether a path with this LCN already exists.
    // Similar packing primitives are likely to appear close to each other,
    // so iterate over the list in reverse to take better advantage of this.
    for (ssize_t i = at_lcn_->child_lcn.size() - 1; i >= 0; i--) {
        LocationAndConnectivityNode* child_lcn = at_lcn_->child_lcn[i];
        if (*child_lcn == *new_lcn) {
            delete new_lcn;
            at_lcn_ = child_lcn;
            return;
        }
    }

    // This is a new diverging path, so add the LCN to the tree.
    at_lcn_->child_lcn.push_back(new_lcn);
    at_lcn_ = new_lcn;
}

LocationAndConnectivityNode* PackingSignatureTree::create_lcn(const t_pb_graph_node* primitive_pb_graph_node, const AtomBlockId atom_block_id) {
    LocationAndConnectivityNode* lcn = new LocationAndConnectivityNode;
    lcn->primitive_num = primitive_pb_graph_node->primitive_num;

    const AtomNetlist& atom_netlist = g_vpr_ctx.atom().netlist();
    AtomNetlist::pin_range primitive_input_pins = atom_netlist.block_input_pins(atom_block_id);

    // Determine how many sinks each net that the output pins for this block drive.
    // If we find that these sinks are inside the cluster later, we decrement the external sinks count for the net.
    // Once packing is complete, if the value for the net is >0, then we know there are sinks outside of the cluster.
    for (AtomPinId primitive_output_pin_id : atom_netlist.block_output_pins(atom_block_id)) {
        AtomPortId primitive_output_port_id = atom_netlist.pin_port(primitive_output_pin_id);
        AtomNetId primitive_output_net_id = atom_netlist.pin_net(primitive_output_pin_id);

        PstPin primitive_output_lcn_pin = std::make_tuple(primitive_pb_graph_node->primitive_num,
                                                          atom_netlist.port_name(primitive_output_port_id),
                                                          atom_netlist.pin_port_bit(primitive_output_pin_id));

        output_nets_[primitive_output_net_id] = ExternalSinksRecord(
            this->get_pin_mapping(primitive_output_lcn_pin),
            atom_netlist.net_sinks(primitive_output_net_id).size());

        checkpoint_new_output_nets_.push_back(primitive_output_net_id);
    }

    for (AtomPinId primitive_input_pin_id : primitive_input_pins) {
        AtomNetId primitive_input_pin_net_id = atom_netlist.pin_net(primitive_input_pin_id);
        AtomPortId primitive_input_pin_port_id = atom_netlist.pin_port(primitive_input_pin_id);
        AtomBlockId source_atom_block_id = atom_netlist.net_driver_block(primitive_input_pin_net_id);
        VTR_ASSERT(source_atom_block_id != AtomBlockId::INVALID());

        // Create record of groupings of primitive pins driven by the same net.
        // This is used by the final path node to identify pins which are driven by an external net.
        PstPin input_lcn_pin = std::make_tuple(primitive_pb_graph_node->primitive_num,
                                               atom_netlist.port_name(primitive_input_pin_port_id),
                                               atom_netlist.pin_port_bit(primitive_input_pin_id));

        input_nets_[primitive_input_pin_net_id].push_back(this->get_pin_mapping(input_lcn_pin));
        checkpoint_input_nets_[primitive_input_pin_net_id]++;

        // Identify if any atoms that have already been placed drive this new primitive.
        auto got = packed_atoms_.find(source_atom_block_id);
        if (got != packed_atoms_.end()) {
            AtomPinId source_pin_id = atom_netlist.net_driver(primitive_input_pin_net_id);
            AtomPortId source_port_id = atom_netlist.pin_port(source_pin_id);
            int source_primitive_num = got->second;

            PstPin source_lcn_pin = std::make_tuple(source_primitive_num,
                                                    atom_netlist.port_name(source_port_id),
                                                    atom_netlist.pin_port_bit(source_pin_id));
            PstPin sink_lcn_pin = std::make_tuple(lcn->primitive_num,
                                                  atom_netlist.port_name(primitive_input_pin_port_id),
                                                  atom_netlist.pin_port_bit(primitive_input_pin_id));
            PstConnection source_connection{this->get_pin_mapping(source_lcn_pin), this->get_pin_mapping(sink_lcn_pin)};
            lcn->intracluster_sources_to_primitive_inputs.push_back(source_connection);

            ExternalSinksRecord& record = output_nets_[primitive_input_pin_net_id];
            VTR_ASSERT(record.external_sinks_count > 0);
            record.external_sinks_count--;
            checkpoint_decremented_output_nets_[primitive_input_pin_net_id]++;
        }
    }

    // Sort the sources list to ensure that primitives are comparable.
    std::sort(lcn->intracluster_sources_to_primitive_inputs.begin(), lcn->intracluster_sources_to_primitive_inputs.end());

    // Add to packed_atoms_ now to avoid double counting if the atom drives itself
    // (i.e. the atom will show as driving itself in the LCN, but not also as driven by itself)
    packed_atoms_[atom_block_id] = lcn->primitive_num;
    checkpoint_new_atoms_.push_back(atom_block_id);

    // Rather than check all the sinks of this block to see if they are already in the cluster,
    // the upper bound of the search space is reduced if we instead check to see if the source
    // of any of the blocks already packed in this cluster is this block.
    for (auto maybe_sink_atom : packed_atoms_) {
        AtomBlockId maybe_sink_atom_block_id = maybe_sink_atom.first;
        AtomNetlist::pin_range potential_sink_input_pins = atom_netlist.block_input_pins(maybe_sink_atom_block_id);
        int maybe_sink_primitive_num = maybe_sink_atom.second;

        for (AtomPinId potential_sink_pin_id : potential_sink_input_pins) {
            AtomNetId potential_sink_net_id = atom_netlist.pin_net(potential_sink_pin_id);
            AtomBlockId source_atom_block_id = atom_netlist.net_driver_block(potential_sink_net_id);
            VTR_ASSERT(source_atom_block_id != AtomBlockId::INVALID());

            if (source_atom_block_id == atom_block_id) {
                AtomPinId primitive_output_pin_id = atom_netlist.net_driver(potential_sink_net_id);
                AtomPortId primitive_output_port_id = atom_netlist.pin_port(primitive_output_pin_id);
                AtomPinId sink_pin_id = potential_sink_pin_id;
                AtomPortId sink_port_id = atom_netlist.pin_port(sink_pin_id);

                PstPin source_lcn_pin = std::make_tuple(lcn->primitive_num,
                                                        atom_netlist.port_name(primitive_output_port_id),
                                                        atom_netlist.pin_port_bit(primitive_output_pin_id));
                PstPin sink_lcn_pin = std::make_tuple(maybe_sink_primitive_num,
                                                      atom_netlist.port_name(sink_port_id),
                                                      atom_netlist.pin_port_bit(sink_pin_id));
                PstConnection sink_connection{this->get_pin_mapping(source_lcn_pin), this->get_pin_mapping(sink_lcn_pin)};
                lcn->intracluster_sinks_of_primitive_outputs.push_back(sink_connection);

                ExternalSinksRecord& record = output_nets_[potential_sink_net_id];
                VTR_ASSERT(record.external_sinks_count > 0);
                record.external_sinks_count--;
                checkpoint_decremented_output_nets_[potential_sink_net_id]++;

                break;
            }
        }
    }

    return lcn;
}

void PackingSignatureTree::add_ecn(e_ecn_legality legality) {
    // Return immediately if packing memoization is disabled
    if (!enabled_) return;

    ExternalConnectivityNode* ecn = this->create_ecn(legality);

    for (ssize_t i = at_lcn_->child_ecn.size() - 1; i >= 0; i--) {
        if (*at_lcn_->child_ecn[i] == *ecn) {
            delete ecn;
            return;
        }
    }
    at_lcn_->child_ecn.push_back(ecn);
}

ExternalConnectivityNode* PackingSignatureTree::create_ecn(e_ecn_legality legality) {
    ExternalConnectivityNode* new_ecn = new ExternalConnectivityNode(legality);

    for (auto input_net : input_nets_) {
        if (output_nets_.count(input_net.first) != 0) continue; // net is driven from inside cluster; not an external source
        std::sort(input_net.second.begin(), input_net.second.end());
        new_ecn->cluster_inputs.push_back(input_net.second);
    }
    std::sort(new_ecn->cluster_inputs.begin(), new_ecn->cluster_inputs.end(), [](auto a, auto b) { return a < b; });

    for (auto output_net : output_nets_) {
        if (output_net.second.external_sinks_count == 0) continue; // net only drives pins inside cluster
        new_ecn->cluster_outputs.push_back(output_net.second.pin);
    }
    std::sort(new_ecn->cluster_outputs.begin(), new_ecn->cluster_outputs.end());

    return new_ecn;
}

e_ecn_legality PackingSignatureTree::check_legality() {
    // Return immediately if packing memoization is disabled
    if (!enabled_) return e_ecn_legality::UNKNOWN;

    ExternalConnectivityNode* ecn = this->create_ecn();

    for (ssize_t i = at_lcn_->child_ecn.size() - 1; i >= 0; i--) {
        if (*at_lcn_->child_ecn[i] == *ecn) {
            delete ecn;
            return at_lcn_->child_ecn[i]->legality;
        }
    }
    delete ecn;
    return e_ecn_legality::UNKNOWN;
}
