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
struct t_user_relative_group {
    /// @brief Atoms belonging to this group (resolved from the name patterns
    ///        in the constraints file).
    std::vector<AtomBlockId> atoms;

    /// @brief The primitive site each atom is locked to: atom_site_paths[i] is
    ///        the site of atoms[i]. An empty string leaves that atom unlocked
    ///        (the packer picks its site); an empty vector leaves the whole
    ///        group unlocked.
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
    std::vector<t_user_relative_group> groups;
};

/**
 * @brief Stores all user-defined relative placement macros.
 *
 * An atom may belong to at most one group across all macros.
 */
class UserRelativeMacros {
  public:
    typedef vtr::vector<UserRelativeMacroId, t_user_relative_macro>::key_range macro_range;

    /**
     * @brief Take ownership of a macro and register its atoms in the reverse
     *        lookup.
     *
     * The macro is moved into this class, which maintains it from then on: the
     * loader builds a macro up and hands it over once it is complete. Pass it
     * with std::move to avoid copying its atom lists and site paths.
     *
     * @return The id of the newly added macro.
     */
    UserRelativeMacroId add_macro(t_user_relative_macro macro);

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
     * @brief Return the (macro id, group index) an atom belongs to, or
     *        (UserRelativeMacroId::INVALID(), -1) if the atom is not part of
     *        any relative placement macro.
     */
    std::pair<UserRelativeMacroId, int> get_atom_group(AtomBlockId blk_id) const;

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
    /// @brief Where an atom sits inside the stored macros.
    struct t_atom_location {
        /// @brief The macro the atom belongs to.
        UserRelativeMacroId macro_id;
        /// @brief Index into t_user_relative_macro::groups.
        size_t group_idx;
        /// @brief Index into t_user_relative_group::atoms.
        size_t atom_idx;
    };

    /// @brief All user-defined relative placement macros.
    vtr::vector<UserRelativeMacroId, t_user_relative_macro> macros_;

    /// @brief Reverse lookup: atom -> its position in macros_. Only holds the
    ///        atoms that belong to a group; the site path of a locked atom is
    ///        read from the macro through it rather than stored a second time.
    std::unordered_map<AtomBlockId, t_atom_location> atom_locations_;
};

/**
 * @brief Print the relative placement macros to an (echo) file.
 */
void print_relative_macros(FILE* fp, const UserRelativeMacros& relative_macros);
