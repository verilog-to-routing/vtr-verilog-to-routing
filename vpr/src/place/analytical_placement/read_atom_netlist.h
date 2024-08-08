
#pragma once

class AtomContext;
class VprConstraints;
class APNetlist;

APNetlist read_atom_netlist(AtomContext& mutable_atom_ctx, const VprConstraints& constraints);

