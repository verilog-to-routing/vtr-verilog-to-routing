#include "pack_molecules.h"

#include "atom_netlist.h"
#include "vtr_assert.h"


//General 
MoleculeBlockId PackMolecule::root_block() const {
    return root_block_;
}

AtomBlockId PackMolecule::root_block_atom() const {
    return block_atom(root_block());
}


//Aggregates
PackMolecule::block_range PackMolecule::blocks() const {
    return vtr::make_range(block_ids_);
}

PackMolecule::pin_range PackMolecule::pins() const {
    return vtr::make_range(pin_ids_);
}

PackMolecule::edge_range PackMolecule::edges() const {
    return vtr::make_range(edge_ids_);
}


//Block attributes
AtomBlockId PackMolecule::block_atom(const MoleculeBlockId blk) const {
    return blocks_[blk].atom_block;
}

PackMolecule::BlockType PackMolecule::block_type(const MoleculeBlockId blk) const {
    return blocks_[blk].type;
}

PackMolecule::pin_range PackMolecule::block_input_pins(const MoleculeBlockId blk) const {
    return vtr::make_range(blocks_[blk].input_pins);
}

PackMolecule::pin_range PackMolecule::block_output_pins(const MoleculeBlockId blk) const {
    return vtr::make_range(blocks_[blk].output_pins);
}


//Pin attributes
MoleculeBlockId PackMolecule::pin_block(const MoleculePinId pin) const {
    return pins_[pin].block;
}

AtomPinId PackMolecule::pin_atom(const MoleculePinId pin) const {
    return pins_[pin].atom_pin;
}

MoleculeEdgeId PackMolecule::pin_edge(const MoleculePinId pin) const {
    return pins_[pin].edge;
}


//Edge attributes
MoleculePinId PackMolecule::edge_driver(const MoleculeEdgeId edge) const {
    return edges_[edge].driver;
}

PackMolecule::pin_range PackMolecule::edge_sinks(const MoleculeEdgeId edge) const {
    return vtr::make_range(edges_[edge].sinks);
}

//Creation
MoleculeBlockId PackMolecule::create_block(BlockType type, AtomBlockId atom_block) {
    auto blk_id = alloc_block();
    blocks_[blk_id].type = type;
    blocks_[blk_id].atom_block = atom_block;

    return blk_id;
}

MoleculePinId PackMolecule::create_pin(MoleculeBlockId block, AtomPinId atom_pin, bool is_input) {
    auto pin_id = alloc_pin();
    pins_[pin_id].block = block;
    pins_[pin_id].atom_pin = atom_pin;

    //Update references
    if (is_input) {
        blocks_[block].input_pins.push_back(pin_id);
    } else {
        blocks_[block].output_pins.push_back(pin_id);
    }

    return pin_id;
}

MoleculeEdgeId PackMolecule::create_edge(MoleculePinId driver) {
    auto edge_id = alloc_edge();
    edges_[edge_id].driver = driver;

    //Update references
    VTR_ASSERT(!pins_[driver].edge);
    pins_[driver].edge = edge_id;

    return edge_id;
}

void PackMolecule::add_edge_sink(MoleculeEdgeId edge, MoleculePinId sink) {
    edges_[edge].sinks.push_back(sink);

    //Update references
    VTR_ASSERT(!pins_[sink].edge);
    pins_[sink].edge = edge;
}
void PackMolecule::set_root_block(MoleculeBlockId root) {
    root_block_ = root;
}

MoleculeBlockId PackMolecule::alloc_block() {
    blocks_.emplace_back();
    auto id = MoleculeBlockId(blocks_.size() - 1);
    block_ids_.push_back(id);
    return id;
}

MoleculePinId PackMolecule::alloc_pin() {
    pins_.emplace_back();
    auto id = MoleculePinId(pins_.size() - 1);
    pin_ids_.push_back(id);
    return id;
}

MoleculeEdgeId PackMolecule::alloc_edge() {
    edges_.emplace_back();
    auto id = MoleculeEdgeId(edges_.size() - 1);
    edge_ids_.push_back(id);
    return id;
}

void write_pack_molecule_dot(std::ostream& os, const PackMolecule& molecule, const AtomNetlist& netlist) {
    os << "#Dot file of pack molecule graph\n";
    os << "digraph molecule {\n";

    //Nodes
    for (auto block : molecule.blocks()) {
        os << "N" << size_t(block);
        
        os << " [";

        os << "label=\"";
        os << netlist.block_name(molecule.block_atom(block));
        os << " (#" << size_t(block);
        if (molecule.root_block() == block) {
            os << " [ROOT]";
        }
        os << ")";
        os << "\"";

        if (molecule.block_type(block) == PackMolecule::BlockType::EXTERNAL) {
            os << " shape=invhouse";
        }

        os << "];\n";
    }

    //Edges
    for (auto edge : molecule.edges()) {

        auto molecule_driver_pin = molecule.edge_driver(edge);
        auto molecule_driver_block = molecule.pin_block(molecule_driver_pin);
        auto atom_driver_pin = molecule.pin_atom(molecule_driver_pin);
        size_t isink = 0;
        for (auto molecule_sink_pin : molecule.edge_sinks(edge)) {
            auto atom_sink_pin = molecule.pin_atom(molecule_sink_pin);
            auto molecule_sink_block = molecule.pin_block(molecule_sink_pin);

            os << "N" << size_t(molecule_driver_block) << " -> N" << size_t(molecule_sink_block);
            os << " [";

            os << "label=\"" ;
            os << netlist.pin_name_non_hierarchical(atom_driver_pin);
            os << " (#" << size_t(edge) << "." << isink << ")\\n";
            os << "\\n-> ";
            os << netlist.pin_name_non_hierarchical(atom_sink_pin);
            os << "\"";

            os << "];\n";
        }
    }

    os << "}\n";
}
