
#pragma once

class AtomContext;
class UserPlaceConstraints;
class APNetlist;

APNetlist read_atom_netlist(AtomContext& mutable_atom_ctx, const UserPlaceConstraints& constraints);

