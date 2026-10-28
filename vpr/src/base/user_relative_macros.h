#pragma once
/**
 * @file
 * @brief This file defines the UserRelativeMacros class, which stores
 *        user-specified relative placement macros read from a VPR
 *        constraints file.
 */

#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

#include "atom_netlist_fwd.h"
#include "vpr_types.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"

/// @brief A unique identifier for a user-defined relative placement macro.
typedef vtr::StrongId<struct user_relative_macro_id_tag> UserRelativeMacroId;

/// @brief A unique identifier for a group of a user-defined relative placement
///        macro. Unique across all macros.
typedef vtr::StrongId<struct user_relative_group_id_tag> UserRelativeGroupId;

/**
 * @brief A group of atoms within a user-defined relative placement macro.
 *
 * All atoms of a group must be packed into the same cluster. The offset is
 * relative to the macro's reference group; the reference group itself has a
 * zero offset.
 */
struct t_user_relative_group {
    /// @brief The macro this group belongs to. Set by UserRelativeMacros::add_macro.
    UserRelativeMacroId macro_id;

    /// @brief Atoms belonging to this group (resolved from the name patterns
    ///        in the constraints file).
    std::vector<AtomBlockId> atoms;

    /// @brief The primitive site each atom is locked to: atom_site_paths[i] is
    ///        the site of atoms[i], so there is always one entry per atom. An
    ///        empty string leaves that atom unlocked, i.e. the packer picks its
    ///        site.
    std::vector<std::string> atom_site_paths;

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
struct t_user_relative_macro {
    /// @brief Unique name of the macro, used in log and error messages.
    std::string name;

    /// @brief The macro's groups. groups[0] is the reference group.
    std::vector<UserRelativeGroupId> groups;
};

/**
 * @brief Stores all user-defined relative placement macros and their groups.
 *
 * An atom may belong to at most one group across all macros.
 */
class UserRelativeMacros {
  public:
    typedef vtr::vector<UserRelativeMacroId, t_user_relative_macro>::key_range macro_range;
    typedef vtr::vector<UserRelativeGroupId, t_user_relative_group>::key_range group_range;

    /**
     * @brief Store a macro with the given groups and register its atoms in
     *        the reverse lookup. groups[0] must be the reference group.
     *
     * The macro_id of the given groups is overwritten with the new macro's id.
     *
     * @return The id of the newly added macro.
     */
    UserRelativeMacroId add_macro(const std::string& name, std::vector<t_user_relative_group> groups);

    /**
     * @brief Return the number of stored macros.
     */
    size_t get_num_macros() const;

    /**
     * @brief Return the ids of all stored macros, for range-based iteration.
     */
    macro_range macros() const;

    /**
     * @brief Return the macro with the given id.
     */
    const t_user_relative_macro& get_macro(UserRelativeMacroId macro_id) const;

    /**
     * @brief Return the number of stored groups over all macros.
     */
    size_t get_num_groups() const;

    /**
     * @brief Return the ids of all stored groups, for range-based iteration.
     */
    group_range groups() const;

    /**
     * @brief Return the group with the given id.
     */
    const t_user_relative_group& get_group(UserRelativeGroupId group_id) const;

    /**
     * @brief Return the id of the group an atom belongs to, or an invalid id
     *        if the atom is not part of any relative placement macro.
     */
    UserRelativeGroupId get_atom_group(AtomBlockId blk_id) const;

    /**
     * @brief Return the hierarchical path of the primitive site the given atom
     *        is locked to by its relative placement group, or an empty string
     *        if the atom is in no group or its group leaves it unlocked.
     *
     * The returned reference points into the stored macro and stays valid as
     * long as no macro is added.
     */
    const std::string& get_atom_locked_site_path(AtomBlockId blk_id) const;

  private:
    /// @brief Where an atom sits inside the stored groups.
    struct t_atom_location {
        /// @brief The group the atom belongs to.
        UserRelativeGroupId group_id;
        /// @brief Index into t_user_relative_group::atoms.
        size_t atom_idx;
    };

    /// @brief All user-defined relative placement macros.
    vtr::vector<UserRelativeMacroId, t_user_relative_macro> macros_;

    /// @brief The groups of all macros.
    vtr::vector<UserRelativeGroupId, t_user_relative_group> groups_;

    /// @brief Reverse lookup: atom -> where it sits in groups_. Only atoms that
    ///        belong to a group have an entry.
    std::unordered_map<AtomBlockId, t_atom_location> atom_locations_;
};

/**
 * @brief Print the relative placement macros to an (echo) file.
 */
void print_relative_macros(FILE* fp, const UserRelativeMacros& relative_macros);
