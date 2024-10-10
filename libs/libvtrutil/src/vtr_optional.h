#pragma once

/** 
 * @file
 * @brief std::optional-like interface with optional references.
 * currently: import TartanLlama's optional into the vtr namespace
 * documentation at https://tl.tartanllama.xyz/en/latest/api/optional.html
 * there are three main uses of this:
 * 1. replace pointers when refactoring legacy code
 *   optional<T&> (reference) is in many ways a pointer, it even has * and -> operators,
 *   but it can't be allocated or freed. this property is very helpful in refactoring.
 * 2. explicit alternative for containers
 *   optional<T> (non-reference) allows you to put non-empty-initializable
 *   objects into a container which owns them. it is an alternative to
 *   unique_ptr<T> in that sense, but with a cleaner interface.
 * 3. function return types
 *   returning an optional<T> gives the caller a clear hint to check the return value.
 *
 * Q: why not use std::optional?
 * A: std::optional doesn't allow optional<T&> due to a disagreement about
 * what it means to assign to an optional reference. tl::optional permits this, with
 * "rebind on assignment" behavior. this means opt<T&> acts very similarly to a pointer.
 * Q: why do we need opt<T&>? there's already T*.
 * A: in an ideal world where all pointers are aliases to existing values and nothing
 * else, opt<T&> wouldn't be that necessary. however VPR is full of legacy code where
 * the usual C++ conventions about pointers don't apply.
 * when refactoring such code, turning all pointers into opt<T&> helps a lot.
 * it can't be allocated or freed and doesn't allow pointer arithmetic.
 * in that aspect it acts as a "enforced proper C++ ptr".
 * that's why I think it's worth keeping around in the codebase. */

#include "tl_optional.hpp"

namespace vtr {
template<class T>
using optional = tl::optional<T>;

using nullopt_t = tl::nullopt_t;
static constexpr nullopt_t nullopt = tl::nullopt;

using in_place_t = tl::in_place_t;
static constexpr in_place_t in_place = tl::in_place;

using bad_optional_access = tl::bad_optional_access;
} // namespace vtr
