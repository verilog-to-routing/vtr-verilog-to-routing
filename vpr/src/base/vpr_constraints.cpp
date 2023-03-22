#include "vpr_constraints.h"
#include "partition.h"
#include "route_constraint.h"
#include <regex>

void VprConstraints::add_constrained_atom(const AtomBlockId blk_id, const PartitionId part_id) {
    auto got = constrained_atoms.find(blk_id);

    /**
     * Each atom can only be in one partition. If the atom is not found in constrained_atoms, it
     * will be added with its partition id.
     * If the atom is already in constrained_atoms, the partition id will be updated.
     */
    if (got == constrained_atoms.end()) {
        constrained_atoms.insert({blk_id, part_id});
    } else {
        got->second = part_id;
    }
}

PartitionId VprConstraints::get_atom_partition(AtomBlockId blk_id) {
    PartitionId part_id;

    auto got = constrained_atoms.find(blk_id);

    if (got == constrained_atoms.end()) {
        return part_id = PartitionId::INVALID(); ///< atom is not in a partition, i.e. unconstrained
    } else {
        return got->second;
    }
}

void VprConstraints::add_partition(Partition part) {
    partitions.push_back(part);
}

Partition VprConstraints::get_partition(PartitionId part_id) {
    return partitions[part_id];
}

std::vector<AtomBlockId> VprConstraints::get_part_atoms(PartitionId part_id) {
    std::vector<AtomBlockId> part_atoms;

    for (auto& it : constrained_atoms) {
        if (it.second == part_id) {
            part_atoms.push_back(it.first);
        }
    }

    return part_atoms;
}

int VprConstraints::get_num_partitions() {
    return partitions.size();
}

PartitionRegion VprConstraints::get_partition_pr(PartitionId part_id) {
    PartitionRegion pr;
    pr = partitions[part_id].get_part_region();
    return pr;
}

void VprConstraints::add_route_constraint(RouteConstraint rc) {
    route_constraints_.insert({rc.net_name(), rc});
    return;
}

const RouteConstraint VprConstraints::get_route_constraint_by_net_name(std::string net_name) {
    RouteConstraint rc;
    auto const& rc_itr = route_constraints_.find(net_name);
    if (rc_itr == route_constraints_.end()) {
        // try regexp
        bool found_thru_regex = false;
        for (auto constraint : route_constraints_) {
            if (std::regex_match(net_name, std::regex(constraint.first))) {
                rc = constraint.second;

                // mark as invalid so write constraint function will not write constraint
                // of regexpr name
                // instead a matched constraint is inserted in
                constraint.second.set_is_valid(false);
                rc.set_net_name(net_name);
                rc.set_is_valid(true);
                route_constraints_.insert({net_name, rc});

                found_thru_regex = true;
                break;
            }
        }
        if (!found_thru_regex) {
            rc.set_net_name("INVALID");
            rc.set_net_type("INVALID");
            rc.set_route_model("INVALID");
            rc.set_is_valid(false);
        }
    } else {
        rc = rc_itr->second;
    }
    return rc;
}

const RouteConstraint VprConstraints::get_route_constraint_by_idx(std::size_t idx) const {
    RouteConstraint rc;
    if ((route_constraints_.size() == 0) || (idx > route_constraints_.size() - 1)) {
        rc.set_net_name("INVALID");
        rc.set_net_type("INVALID");
        rc.set_route_model("INVALID");
        rc.set_is_valid(false);
    } else {
        std::size_t i = 0;
        for (auto const& rc_itr : route_constraints_) {
            if (i == idx) {
                rc = rc_itr.second;
                break;
            }
        }
    }
    return rc;
}

int VprConstraints::get_route_constraint_num(void) const {
    return route_constraints_.size();
}

void print_constraints(FILE* fp, VprConstraints constraints) {
    Partition temp_part;
    std::vector<AtomBlockId> atoms;

    int num_parts = constraints.get_num_partitions();

    fprintf(fp, "\n Number of partitions is %d \n", num_parts);

    for (int i = 0; i < num_parts; i++) {
        PartitionId part_id(i);

        temp_part = constraints.get_partition(part_id);

        fprintf(fp, "\npartition_id: %zu\n", size_t(part_id));
        print_partition(fp, temp_part);

        atoms = constraints.get_part_atoms(part_id);

        int atoms_size = atoms.size();

        fprintf(fp, "\tAtom vector size is %d\n", atoms_size);
        fprintf(fp, "\tIds of atoms in partition: \n");
        for (unsigned int j = 0; j < atoms.size(); j++) {
            AtomBlockId atom_id = atoms[j];
            fprintf(fp, "\t#%zu\n", size_t(atom_id));
        }
    }
}
