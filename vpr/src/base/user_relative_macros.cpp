#include "user_relative_macros.h"

#include "vtr_assert.h"

UserRelativeMacroId UserRelativeMacros::add_macro(const UserRelativeMacro& macro) {
    UserRelativeMacroId macro_id(macros_.size());
    macros_.push_back(macro);

    for (size_t group_idx = 0; group_idx < macro.groups.size(); group_idx++) {
        const UserRelativeGroup& group = macro.groups[group_idx];
        VTR_ASSERT_MSG(group.atom_site_paths.empty() || group.atom_site_paths.size() == group.atoms.size(),
                       "A relative placement group must have one atom site path per atom");
        for (size_t iatom = 0; iatom < group.atoms.size(); iatom++) {
            AtomBlockId blk_id = group.atoms[iatom];
            VTR_ASSERT_MSG(atom_to_group_.count(blk_id) == 0,
                           "An atom may belong to at most one relative placement group");
            atom_to_group_[blk_id] = {macro_id, (int)group_idx};
            if (!group.atom_site_paths.empty() && !group.atom_site_paths[iatom].empty()) {
                atom_to_site_path_[blk_id] = group.atom_site_paths[iatom];
            }
        }
    }

    return macro_id;
}

size_t UserRelativeMacros::get_num_macros() const {
    return macros_.size();
}

const UserRelativeMacro& UserRelativeMacros::get_macro(UserRelativeMacroId macro_id) const {
    VTR_ASSERT(macro_id.is_valid() && (size_t)macro_id < macros_.size());
    return macros_[macro_id];
}

std::pair<UserRelativeMacroId, int> UserRelativeMacros::get_atom_group(AtomBlockId blk_id) const {
    // Fast path for the common case of no relative placement macros: callers
    // on hot packer paths may query every atom.
    if (atom_to_group_.empty()) {
        return {UserRelativeMacroId::INVALID(), -1};
    }
    auto itr = atom_to_group_.find(blk_id);
    if (itr == atom_to_group_.end()) {
        return {UserRelativeMacroId::INVALID(), -1};
    }
    return itr->second;
}

const std::string& UserRelativeMacros::get_atom_site_path(AtomBlockId blk_id) const {
    static const std::string unlocked;
    if (atom_to_site_path_.empty()) {
        return unlocked;
    }
    auto itr = atom_to_site_path_.find(blk_id);
    if (itr == atom_to_site_path_.end()) {
        return unlocked;
    }
    return itr->second;
}
