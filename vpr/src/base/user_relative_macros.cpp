#include "user_relative_macros.h"

#include <utility>

#include "vtr_assert.h"

UserRelativeMacroId UserRelativeMacros::add_macro(t_user_relative_macro macro) {
    UserRelativeMacroId macro_id(macros_.size());

    for (size_t group_idx = 0; group_idx < macro.groups.size(); group_idx++) {
        const t_user_relative_group& group = macro.groups[group_idx];
        VTR_ASSERT_MSG(group.atom_site_paths.empty() || group.atom_site_paths.size() == group.atoms.size(),
                       "A relative placement group must have one atom site path per atom");
        for (size_t atom_idx = 0; atom_idx < group.atoms.size(); atom_idx++) {
            bool first_time = atom_locations_.emplace(group.atoms[atom_idx], t_atom_location{macro_id, group_idx, atom_idx}).second;
            VTR_ASSERT_MSG(first_time, "An atom may belong to at most one relative placement group");
        }
    }

    macros_.push_back(std::move(macro));
    return macro_id;
}

size_t UserRelativeMacros::get_num_macros() const {
    return macros_.size();
}

UserRelativeMacros::macro_range UserRelativeMacros::macros() const {
    return macros_.keys();
}

const t_user_relative_macro& UserRelativeMacros::get_macro(UserRelativeMacroId macro_id) const {
    VTR_ASSERT(macro_id.is_valid() && (size_t)macro_id < macros_.size());
    return macros_[macro_id];
}

std::pair<UserRelativeMacroId, int> UserRelativeMacros::get_atom_group(AtomBlockId blk_id) const {
    // Fast path for the common case of no relative placement macros: callers
    // on hot packer paths may query every atom.
    if (atom_locations_.empty()) {
        return {UserRelativeMacroId::INVALID(), -1};
    }
    auto itr = atom_locations_.find(blk_id);
    if (itr == atom_locations_.end()) {
        return {UserRelativeMacroId::INVALID(), -1};
    }
    return {itr->second.macro_id, (int)itr->second.group_idx};
}

const std::string& UserRelativeMacros::get_atom_locked_site_path(AtomBlockId blk_id) const {
    static const std::string unlocked;
    if (atom_locations_.empty()) {
        return unlocked;
    }
    auto itr = atom_locations_.find(blk_id);
    if (itr == atom_locations_.end()) {
        return unlocked;
    }
    const t_atom_location& location = itr->second;
    const t_user_relative_group& group = macros_[location.macro_id].groups[location.group_idx];
    if (group.atom_site_paths.empty()) {
        // the whole group was authored without site information
        return unlocked;
    }
    return group.atom_site_paths[location.atom_idx];
}

void print_relative_macros(FILE* fp, const UserRelativeMacros& relative_macros) {
    fprintf(fp, "\n Number of relative macros is %zu \n", relative_macros.get_num_macros());

    for (UserRelativeMacroId macro_id : relative_macros.macros()) {
        const t_user_relative_macro& macro = relative_macros.get_macro(macro_id);
        fprintf(fp, "\nrelative_macro_id: %zu name: %s\n", size_t(macro_id), macro.name.c_str());

        for (size_t igroup = 0; igroup < macro.groups.size(); igroup++) {
            const t_user_relative_group& group = macro.groups[igroup];
            fprintf(fp, "\t%s: offset (x %d, y %d, sub_tile %d, layer %d), %zu atom(s)\n",
                    igroup == 0 ? "reference group" : "relative group",
                    group.offset.x, group.offset.y, group.offset.sub_tile, group.offset.layer,
                    group.atoms.size());
            fprintf(fp, "\tIds of atoms in group (with site_path if locked):\n");
            for (size_t iatom = 0; iatom < group.atoms.size(); iatom++) {
                bool locked = !group.atom_site_paths.empty() && !group.atom_site_paths[iatom].empty();
                if (locked) {
                    fprintf(fp, "\t#%zu %s\n", size_t(group.atoms[iatom]), group.atom_site_paths[iatom].c_str());
                } else {
                    fprintf(fp, "\t#%zu\n", size_t(group.atoms[iatom]));
                }
            }
        }
    }
}
