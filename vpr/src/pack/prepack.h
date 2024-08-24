/*
 * Prepacking: Group together technology-mapped netlist blocks before packing.
 * This gives hints to the packer on what groups of blocks to keep together
 * during packing.
 *
 * Primary uses: 1) "Forced" packs (eg LUT+FF pair)
 *               2) Carry-chains
 */

#ifndef PREPACK_H
#define PREPACK_H

#include <algorithm>
#include <map>
#include <unordered_map>
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_range.h"

class AtomNetlist;
class AtomBlockId;
struct t_molecule_stats;
struct t_logical_block_type;

/**
 * @brief Class that performs prepacking.
 *
 * This class maintains the prepacking state, allowing the use of molecules
 * (prepacked atoms) while this object exists. After prepacking, every atom will
 * be part of a molecule (with a large number being part of single-atom
 * molecules.
 *
 * Molecules currently come from pack patterns in the architecture file. For
 * example, a 3-bit carry chain in most architectures would turn into a molecule
 * containing the 3 atoms forming the carry chain.
 *
 * To use the prepacker, call the init method with a complete atom netlist.
 * Then maintain this object (do not reset or destroy it) so long as the
 * molecules are needed.
 *
 * // Initialize device and atom netlist
 * // ...
 * Prepacker prepacker;
 * prepacker.init(atom_ctx.nlist, device_ctx.logical_block_types);
 * // ...
 * // Use the prepacked molecules.
 * // ...
 * prepacker.reset(); // Or if the prepacker object is destroyed.
 * // Prepacked molecules can no longer be used beyond this point.
 *
 */
class Prepacker {
    using atom_molecules_const_iterator = std::multimap<AtomBlockId, t_pack_molecule*>::const_iterator;
public:
    // The constructor is default, the init method performs prepacking.
    Prepacker() = default;

    // This class maintains pointers to internal data structures, and as such
    // should not be copied or moved (prevents unsafe accesses).
    Prepacker(const Prepacker&) = delete;
    Prepacker& operator=(const Prepacker&) = delete;

    /**
     * @brief Performs prepacking.
     *
     * Initializes the prepacker by performing prepacking and allocating the
     * necessary data strucutres.
     *
     *  @param atom_nlist           The atom netlist to prepack.
     *  @param logical_block_types  A list of the logical block types on the device.
     */
    void init(const AtomNetlist& atom_nlist, const std::vector<t_logical_block_type> &logical_block_types);

    /**
     * @brief Get the cluster molecules containing the given atom block.
     *
     *  @param blk_id       The atom block to get the molecules of.
     */
    inline t_pack_molecule* get_atom_molecule(AtomBlockId blk_id) const {
        // Internally the prepacker maintains multiple molecules per atom, since
        // some atoms may be part of multiple pack patterns; however, the
        // prepacker should merge these eventually into one single molecule.
        // TODO: If this can be 100% verified, this should be turned into a
        //       debug assert.
        VTR_ASSERT(atom_molecules.count(blk_id) == 1);
        return atom_molecules.find(blk_id)->second;
    }

    /**
     * @brief Get the expected lowest cost physical block graph node for the
     *        given atom block.
     *
     *  @param blk_id       The atom block to get the pb graph node of.
     */
    inline t_pb_graph_node* get_expected_lowest_cost_pb_gnode(AtomBlockId blk_id) const {
        auto iter = expected_lowest_cost_pb_gnode.find(blk_id);
        VTR_ASSERT(iter != expected_lowest_cost_pb_gnode.end());
        t_pb_graph_node* pb_gnode = iter->second;
        VTR_ASSERT(pb_gnode != nullptr);
        return pb_gnode;
    }

    /**
     * @brief Returns the total number of molecules in the prepacker.
     */
    inline size_t get_num_molecules() const {
        size_t num_molecules = 0;
        t_pack_molecule* molecule_head = list_of_pack_molecules;
        for (auto cur_molecule = molecule_head; cur_molecule != nullptr; cur_molecule = cur_molecule->next) {
            ++num_molecules;
        }
        return num_molecules;
    }

    /**
     * @brief Returns all of the molecules as a vector.
     */
    inline std::vector<t_pack_molecule*> get_molecules_vector() const {
        std::vector<t_pack_molecule*> molecules;
        t_pack_molecule* molecule_head = list_of_pack_molecules;
        for (auto cur_molecule = molecule_head; cur_molecule != nullptr; cur_molecule = cur_molecule->next) {
            molecules.push_back(cur_molecule);
        }
        return molecules;
    }

    /**
     * @brief Marks all of the molecules as valid.
     *
     * Within clustering, the valid flag of a molecule is used to signify if any
     * of the atoms in the molecule has been packed into a cluster yet or not.
     * If any atom in the molecule has been packed, the flag will be false.
     *
     * This method is used before clustering to mark all the molecules as
     * unpacked.
     */
    inline void mark_all_molecules_valid() {
        t_pack_molecule* molecule_head = list_of_pack_molecules;
        for (auto cur_molecule = molecule_head; cur_molecule != nullptr; cur_molecule = cur_molecule->next) {
            cur_molecule->valid = true;
        }
    }

    /**
     * @brief Calculates maximum molecule statistics accross all molecules,
     */
    t_molecule_stats calc_max_molecule_stats(const AtomNetlist& netlist) const;

    /**
     * @brief Gets the largest number of blocks (atoms) that any molecule contains.
     */
    inline size_t get_max_molecule_size() const {
        size_t max_molecule_size = 1;
        t_pack_molecule* molecule_head = list_of_pack_molecules;
        for (auto cur_molecule = molecule_head; cur_molecule != nullptr; cur_molecule = cur_molecule->next) {
            max_molecule_size = std::max<size_t>(max_molecule_size, cur_molecule->num_blocks);
        }
        return max_molecule_size;
    }

    /**
     * @brief Resets the prepacker object. Clearing all state.
     *
     * This resets the prepacker, allowing it to prepack again and also freeing
     * any state.
     */
    void reset();

    /// @brief Destructor of the prepacker class. Calls the reset method.
    ~Prepacker() { reset(); }

private:
    /**
     * @brief A linked list of all the packing molecules that are loaded in
     *        prepacking stage.
     *
     * All of the molecules in the prepacker are allocated into this linked list
     * and must be freed eventually.
     *
     * TODO: Should use a vtr::vector instead of a linked list for storage. Then
     *       instead of pointers, IDs can be used to manipulate the molecules
     *       which would be safer.
     */
    t_pack_molecule* list_of_pack_molecules = nullptr;

    /**
     * @brief The molecules associated with each atom block.
     *
     * This map is loaded in the prepacking stage and freed at the very end of
     * vpr flow run. The pointers in this multimap is shared with
     * list_of_pack_molecules.
     */
    std::multimap<AtomBlockId, t_pack_molecule*> atom_molecules;

    /// @brief A map for the expected lowest cost physical block graph node.
    std::unordered_map<AtomBlockId, t_pb_graph_node*> expected_lowest_cost_pb_gnode;

    /// @brief A list of the pack patterns used for prepacking. I think the
    ///        molecules keep pointers to this vector, so this needs to remain
    ///        for the lifetime of the molecules.
    std::vector<t_pack_patterns> list_of_pack_patterns;
};

#endif
