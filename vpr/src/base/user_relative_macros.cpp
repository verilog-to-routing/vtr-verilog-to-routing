#include "user_relative_macros.h"

#include "vtr_assert.h"

UserRelativeMacroId UserRelativeMacros::add_macro(const UserRelativeMacro& macro) {
    UserRelativeMacroId macro_id(macros_.size());
    macros_.push_back(macro);

    for (size_t group_idx = 0; group_idx < macro.groups.size(); group_idx++) {
        for (AtomBlockId blk_id : macro.groups[group_idx].atoms) {
            VTR_ASSERT_MSG(atom_to_group_.count(blk_id) == 0,
                           "An atom may belong to at most one relative placement group");
            atom_to_group_[blk_id] = {macro_id, (int)group_idx};
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
    auto itr = atom_to_group_.find(blk_id);
    if (itr == atom_to_group_.end()) {
        return {UserRelativeMacroId::INVALID(), -1};
    }
    return itr->second;
}
