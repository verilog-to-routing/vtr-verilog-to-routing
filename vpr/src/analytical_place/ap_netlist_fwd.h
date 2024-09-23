/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Forward declarations of the APNetlist class
 *
 * Forward declares the APNetlist class, and defines common types used by it.
 */

#pragma once

#include "netlist_fwd.h"
#include "vtr_strong_id.h"

// Forward declaration
class APNetlist;

/*
 * Ids
 *
 * The APNetlist uses unique IDs to identify any component of the netlist.
 * To avoid type-conversion errors, we use vtr::StrongIds.
 */

/**
 * @brief A unique identifier for a block in the AP netlist
 */
class APBlockId : public ParentBlockId {
public:
    static constexpr APBlockId INVALID() { return APBlockId(); }
    using ParentBlockId::ParentBlockId;

    friend std::hash<APBlockId>;
};

/**
 * @brief A unique identifier for a net in the AP netlist
 */
class APNetId : public ParentNetId {
public:
    static constexpr APNetId INVALID() { return APNetId(); }
    using ParentNetId::ParentNetId;

    friend std::hash<APNetId>;
};

/**
 * @brief A unique identifier for a port in the AP netlist
 */
class APPortId : public ParentPortId {
public:
    static constexpr APPortId INVALID() { return APPortId(); }
    using ParentPortId::ParentPortId;

    friend std::hash<APPortId>;
};

/**
 * @brief A unique identifier for a pin in the AP netlist
 */
class APPinId : public ParentPinId {
public:
    static constexpr APPinId INVALID() { return APPinId(); }
    using ParentPinId::ParentPinId;

    friend std::hash<APPinId>;
};

/**
 * @brief Specialized std::hash for StrongIds
 *
 * This is needed for std::unordered_map-like containers.
 */
namespace std {
template<>
struct hash<APBlockId> {
    std::hash<ParentBlockId> parent_hash;
    std::size_t operator()(const APBlockId k) const noexcept {
        return parent_hash(k); // Hash with the underlying type
    }
};
template<>
struct hash<APNetId> {
    std::hash<ParentNetId> parent_hash;
    std::size_t operator()(const APNetId k) const noexcept {
        return parent_hash(k); // Hash with the underlying type
    }
};
template<>
struct hash<APPortId> {
    std::hash<ParentPortId> parent_hash;
    std::size_t operator()(const APPortId k) const noexcept {
        return parent_hash(k); // Hash with the underlying type
    }
};
template<>
struct hash<APPinId> {
    std::hash<ParentPinId> parent_hash;
    std::size_t operator()(const APPinId k) const noexcept {
        return parent_hash(k); // Hash with the underlying type
    }
};
} // namespace std

