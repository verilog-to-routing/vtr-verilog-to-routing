#ifndef VPR_PACK_MOLECULE_H
#define VPR_PACK_MOLECULE_H
#include <vector>
#include <iosfwd>

#include "vtr_strong_id.h"
#include "vtr_range.h"
#include "vtr_vector.h"

#include "atom_netlist_fwd.h"

struct pack_molecule_block_id_tag;
struct pack_molecule_pin_id_tag;
struct pack_molecule_edge_id_tag;

typedef vtr::StrongId<pack_molecule_block_id_tag> MoleculeBlockId;
typedef vtr::StrongId<pack_molecule_pin_id_tag> MoleculePinId;
typedef vtr::StrongId<pack_molecule_edge_id_tag> MoleculeEdgeId;

class PackMolecule {
    public: //Types
        enum class BlockType {
            INTERNAL,
            EXTERNAL
        };

        typedef std::vector<MoleculeBlockId>::const_iterator block_iterator;
        typedef std::vector<MoleculePinId>::const_iterator pin_iterator;
        typedef std::vector<MoleculeEdgeId>::const_iterator edge_iterator;

        typedef vtr::Range<block_iterator> block_range;
        typedef vtr::Range<pin_iterator> pin_range;
        typedef vtr::Range<edge_iterator> edge_range;
    
    public: //Accessors
        //General 
        MoleculeBlockId root_block() const;

        //Aggregates
        block_range blocks() const;
        pin_range pins() const;
        edge_range edges() const;

        //Block attributes
        AtomBlockId block_atom(const MoleculeBlockId blk) const;
        BlockType block_type(const MoleculeBlockId blk) const;
        pin_range block_input_pins(const MoleculeBlockId blk) const;
        pin_range block_output_pins(const MoleculeBlockId blk) const;

        //Pin attributes
        MoleculeBlockId pin_block(const MoleculePinId pin) const;
        AtomPinId pin_atom(const MoleculePinId pin) const;
        MoleculeEdgeId pin_edge(const MoleculePinId pin) const;

        //Edge attributes
        MoleculePinId edge_driver(const MoleculeEdgeId edge) const;
        pin_range edge_sinks(const MoleculeEdgeId edge) const;

    public: //Mutators
        MoleculeBlockId create_block(BlockType type, AtomBlockId atom_block);
        MoleculePinId create_pin(MoleculeBlockId block, AtomPinId atom_pin, bool is_input);

        MoleculeEdgeId create_edge(MoleculePinId driver_pin);
        void add_edge_sink(MoleculeEdgeId edge, MoleculePinId sink_pin);

        void set_root_block(MoleculeBlockId root);
    private:
        MoleculeBlockId alloc_block();
        MoleculePinId alloc_pin();
        MoleculeEdgeId alloc_edge();

    private: //Data
        struct Block {
            BlockType type;
            AtomBlockId atom_block;

            std::vector<MoleculePinId> input_pins;
            std::vector<MoleculePinId> output_pins;
        };

        struct Pin {
            MoleculeBlockId block;

            AtomPinId atom_pin;

            MoleculeEdgeId edge;
        };

        struct Edge {
            MoleculePinId driver;
            std::vector<MoleculePinId> sinks;
        };

        MoleculeBlockId root_block_;

        std::vector<MoleculeBlockId> block_ids_;
        std::vector<MoleculePinId> pin_ids_;
        std::vector<MoleculeEdgeId> edge_ids_;

        vtr::vector<MoleculeBlockId,Block> blocks_;
        vtr::vector<MoleculePinId,Pin> pins_;
        vtr::vector<MoleculeEdgeId,Edge> edges_;
};

void write_pack_molecule_dot(std::ostream& os, const PackMolecule& molecule, const AtomNetlist& netlist);

#endif
