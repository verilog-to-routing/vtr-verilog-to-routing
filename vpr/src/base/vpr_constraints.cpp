#include "vpr_constraints.h"



void VprConstraints::add_constrained_atom(const AtomBlockId blk_id, const PartitionId part_id){
    placement_constraints_.add_constrained_atom(blk_id, part_id);
}


void VprConstraints::add_partition(Partition part){
    placement_constraints_.add_partition(part);
}


Partition VprConstraints::get_partition(PartitionId part_id){
    return placement_constraints_.get_partition(part_id);
}

std::vector<AtomBlockId> VprConstraints::get_part_atoms(PartitionId part_id){
    return placement_constraints_.get_part_atoms(part_id);
}

int VprConstraints::get_num_partitions(){
    return placement_constraints_.get_num_partitions();
}


void VprConstraints::add_route_constraint(std::string net_name, RoutingScheme route_scheme){
    route_constraints_.add_route_constraint(net_name, route_scheme);
}

const std::pair<std::string, RoutingScheme> VprConstraints::get_route_constraint_by_idx(std::size_t idx) const{
    return route_constraints_.get_route_constraint_by_idx(idx);
}


e_clock_modeling VprConstraints::get_route_model_by_net_name(std::string net_name) const{
    return route_constraints_.get_route_model_by_net_name(net_name);
}

const std::string VprConstraints::get_routing_network_name_by_net_name(std::string net_name) const{
    return route_constraints_.get_routing_network_name_by_net_name(net_name);
}

int VprConstraints::get_num_route_constraints() const{
    return route_constraints_.get_num_route_constraints();
}

const UserPlaceConstraints VprConstraints::place_constraints() const{
    return placement_constraints_;
}

const UserRouteConstraints VprConstraints::route_constraints() const{
    return route_constraints_;
}
