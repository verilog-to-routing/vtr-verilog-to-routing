#pragma once
/**
 * @file
 * @brief This file defines the UserRelativeMacros class, which stores
 *        user-specified relative placement macros read from a VPR
 *        constraints file.
 *
 * Overview
 * ========
 * A relative placement macro constrains groups of atoms to be placed at fixed
 * offsets from each other, without pinning them to absolute locations. Each
 * macro consists of one reference group and one or more relative groups:
 * - Atoms within one group must be packed into the same cluster.
 * - Atoms of different groups (of any macro) must be packed into different
 *   clusters.
 * - During placement, each relative group's cluster is placed at its offset
 *   from the reference group's cluster and the whole macro moves as a unit
 *   (it becomes a placement macro, see vpr/src/place/place_macro.h).
 */

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "atom_netlist_fwd.h"
#include "vpr_types.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"

/// @brief A unique identifier for a user-defined relative placement macro.
typedef vtr::StrongId<struct user_relative_macro_id_tag> UserRelativeMacroId;

/**
 * @brief A group of atoms within a user-defined relative placement macro.
 *
 * All atoms of a group must be packed into the same cluster. The offset is
 * relative to the macro's reference group; the reference group itself has a
 * zero offset.
 */
struct UserRelativeGroup {
    /// @brief Atoms belonging to this group (resolved from the name patterns
    ///        in the constraints file).
    std::vector<AtomBlockId> atoms;

    /// @brief Placement offset of this group's cluster relative to the
    ///        reference group's cluster. (0, 0, 0, 0) for the reference group.
    t_pl_offset offset;
};

/**
 * @brief A user-defined relative placement macro.
 *
 * groups[0] is always the reference group (zero offset); the remaining
 * entries are the relative groups.
 */
struct UserRelativeMacro {
    /// @brief Unique name of the macro, used in log and error messages.
    std::string name;

    /// @brief The macro's groups. groups[0] is the reference group.
    std::vector<UserRelativeGroup> groups;
};

/**
 * @brief Stores all user-defined relative placement macros and provides an
 *        O(1) atom -> (macro, group) reverse lookup for the packer.
 *
 * An atom may belong to at most one group across all macros; this invariant
 * is enforced when the constraints file is loaded.
 */
class UserRelativeMacros {
  public:
    /**
     * @brief Add a macro and register its atoms in the reverse lookup.
     *
     * The caller (the constraints file reader) is responsible for validating
     * that no atom belongs to more than one group; this method asserts it.
     *
     *   @return The id of the newly added macro.
     */
    UserRelativeMacroId add_macro(const UserRelativeMacro& macro);

    /**
     * @brief Return the number of stored macros.
     */
    size_t get_num_macros() const;

    /**
     * @brief Return the macro with the given id.
     */
    const UserRelativeMacro& get_macro(UserRelativeMacroId macro_id) const;

    /**
     * @brief Return the (macro id, group index) an atom belongs to, or
     *        (UserRelativeMacroId::INVALID(), -1) if the atom is not part of
     *        any relative placement macro.
     */
    std::pair<UserRelativeMacroId, int> get_atom_group(AtomBlockId blk_id) const;

  private:
    /// @brief All user-defined relative placement macros.
    vtr::vector<UserRelativeMacroId, UserRelativeMacro> macros_;

    /// @brief Reverse lookup: atom -> (macro id, group index within the macro).
    std::unordered_map<AtomBlockId, std::pair<UserRelativeMacroId, int>> atom_to_group_;
};
