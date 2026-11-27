#pragma once
/**
 * @file
 * @brief Declaration of the RoutingNetlist class: the netlist seen by the router.
 *
 * Overview
 * ========
 * The router connects the pins of a netlist through the routing resource graph. Which
 * netlist that is depends on the flow:
 *  - Regular (two-stage) routing routes the nets of the ClusteredNetlist: only the
 *    connections between clustered blocks are routed.
 *  - Flat routing routes the nets of the AtomNetlist: the connections between primitives
 *    are routed, including the parts which are inside the clustered blocks.
 *
 * A RoutingNetlist presents either of the two through a single (non-template) interface,
 * so the router and the code which consumes its results (timing analysis, statistics,
 * routing checks, ...) can be written once.
 *
 * Connectivity
 * ============
 * A RoutingNetlist holds no copy of the netlist it was built from: every query is answered
 * from the source netlist. The pins of a net are returned through a RoutingPinIterator,
 * which converts the pin IDs of the source netlist as they are read. The source netlist
 * must therefore outlive the RoutingNetlist, and its nets, pins and blocks must not be
 * added, removed or re-connected while the RoutingNetlist is in use.
 */

#include <compare>
#include <cstddef>
#include <iterator>
#include <string>

#include "atom_lookup.h"
#include "atom_netlist.h"
#include "clustered_netlist.h"
#include "routing_netlist_fwd.h"
#include "vtr_assert.h"
#include "vtr_range.h"
#include "vtr_strong_id_range.h"

/**
 * @brief Random-access iterator over the pins of a net of the atom or the clustered netlist,
 *        which yields them as RoutingPinIds.
 *
 * It points into the pin storage of the source netlist and converts each pin ID when it is
 * read, so the pins of a net are not copied. Exactly one of its two pointers is set, which
 * one depends on the netlist which is routed; the branch on it goes the same way for the
 * whole run.
 */
class RoutingPinIterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = RoutingPinId;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = RoutingPinId; // Pins are yielded by value

    RoutingPinIterator() = default;

    /// @brief Iterates over pins of the atom netlist, starting at the given one.
    explicit RoutingPinIterator(const AtomPinId* atom_pin)
        : atom_pin_(atom_pin) {}

    /// @brief Iterates over pins of the clustered netlist, starting at the given one.
    explicit RoutingPinIterator(const ClusterPinId* clb_pin)
        : clb_pin_(clb_pin) {}

    RoutingPinId operator*() const {
        return atom_pin_ ? RoutingPinId(size_t(*atom_pin_)) : RoutingPinId(size_t(*clb_pin_));
    }

    RoutingPinId operator[](difference_type n) const { return *(*this + n); }

    RoutingPinIterator& operator+=(difference_type n) {
        if (atom_pin_) {
            atom_pin_ += n;
        } else {
            clb_pin_ += n;
        }
        return *this;
    }

    RoutingPinIterator& operator-=(difference_type n) { return *this += -n; }
    RoutingPinIterator& operator++() { return *this += 1; }
    RoutingPinIterator& operator--() { return *this -= 1; }

    RoutingPinIterator operator++(int) {
        RoutingPinIterator prev = *this;
        ++*this;
        return prev;
    }

    RoutingPinIterator operator--(int) {
        RoutingPinIterator prev = *this;
        --*this;
        return prev;
    }

    friend RoutingPinIterator operator+(RoutingPinIterator it, difference_type n) { return it += n; }
    friend RoutingPinIterator operator+(difference_type n, RoutingPinIterator it) { return it += n; }
    friend RoutingPinIterator operator-(RoutingPinIterator it, difference_type n) { return it -= n; }

    friend difference_type operator-(const RoutingPinIterator& a, const RoutingPinIterator& b) {
        return a.atom_pin_ ? a.atom_pin_ - b.atom_pin_ : a.clb_pin_ - b.clb_pin_;
    }

    friend bool operator==(const RoutingPinIterator& a, const RoutingPinIterator& b) {
        return a.atom_pin_ == b.atom_pin_ && a.clb_pin_ == b.clb_pin_;
    }

    friend std::strong_ordering operator<=>(const RoutingPinIterator& a, const RoutingPinIterator& b) {
        return (a - b) <=> 0;
    }

  private:
    const AtomPinId* atom_pin_ = nullptr;   ///< The current pin, if the atom netlist is routed
    const ClusterPinId* clb_pin_ = nullptr; ///< The current pin, if the clustered netlist is routed
};

class RoutingNetlist {
  public: // Types
    typedef vtr::StrongIdRange<RoutingBlockId> block_range;
    typedef vtr::StrongIdRange<RoutingNetId> net_range;
    typedef vtr::StrongIdRange<RoutingPinId> pin_id_range;
    typedef vtr::Range<RoutingPinIterator> pin_range;

  public: // Constructors
    /**
     * @brief Builds the netlist to route from the atom netlist (flat routing) or from
     *        the clustered netlist (routing between the clustered blocks).
     *
     *  @param atom_nlist   The atom netlist of the design.
     *  @param clb_nlist    The clustered netlist of the design.
     *  @param atom_lookup  The mapping between the atom and the clustered netlists.
     *  @param is_flat      If true the nets of the atom netlist are to be routed,
     *                      otherwise the nets of the clustered netlist.
     */
    RoutingNetlist(const AtomNetlist& atom_nlist,
                   const ClusteredNetlist& clb_nlist,
                   const AtomLookup& atom_lookup,
                   bool is_flat);

  public: // Netlist
    /// @brief Returns true if this is the netlist of a flat routing (the atom netlist is routed).
    bool is_flat() const { return is_flat_; }

    /// @brief Returns all the blocks of the netlist.
    block_range blocks() const {
        size_t num_blocks = is_flat() ? atom_nlist_.blocks().size() : clb_nlist_.blocks().size();
        return block_range(RoutingBlockId(0), RoutingBlockId(num_blocks));
    }

    /// @brief Returns all the nets of the netlist.
    net_range nets() const {
        size_t num_nets = is_flat() ? atom_nlist_.nets().size() : clb_nlist_.nets().size();
        return net_range(RoutingNetId(0), RoutingNetId(num_nets));
    }

    /// @brief Returns all the pins of the netlist.
    pin_id_range pins() const {
        size_t num_pins = is_flat() ? atom_nlist_.pins().size() : clb_nlist_.pins().size();
        return pin_id_range(RoutingPinId(0), RoutingPinId(num_pins));
    }

  public: // Blocks
    /// @brief Returns the name of a block.
    const std::string& block_name(RoutingBlockId blk_id) const {
        if (is_flat()) {
            return atom_nlist_.block_name(atom_block(blk_id));
        }
        return clb_nlist_.block_name(cluster_block(blk_id));
    }

  public: // Pins
    /// @brief Returns the block a pin belongs to.
    RoutingBlockId pin_block(RoutingPinId pin_id) const {
        if (is_flat()) {
            return RoutingBlockId(size_t(atom_nlist_.pin_block(atom_pin(pin_id))));
        }
        return RoutingBlockId(size_t(clb_nlist_.pin_block(cluster_pin(pin_id))));
    }

    /// @brief Returns the index of a pin within its net (0 for the driver, 1 and up for the sinks).
    int pin_net_index(RoutingPinId pin_id) const {
        if (is_flat()) {
            return atom_nlist_.pin_net_index(atom_pin(pin_id));
        }
        return clb_nlist_.pin_net_index(cluster_pin(pin_id));
    }

  public: // Nets
    /// @brief Returns the name of a net.
    const std::string& net_name(RoutingNetId net_id) const {
        if (is_flat()) {
            return atom_nlist_.net_name(atom_net(net_id));
        }
        return clb_nlist_.net_name(cluster_net(net_id));
    }

    /// @brief Returns the pins of a net: the driver (which is invalid if the net has no driver) followed by the sinks.
    pin_range net_pins(RoutingNetId net_id) const {
        if (is_flat()) {
            return make_pin_range_(atom_nlist_.net_pins(atom_net(net_id)));
        }
        return make_pin_range_(clb_nlist_.net_pins(cluster_net(net_id)));
    }

    /// @brief Returns the sink pins of a net.
    pin_range net_sinks(RoutingNetId net_id) const {
        pin_range pins_of_net = net_pins(net_id);
        return pin_range(pins_of_net.begin() + 1, pins_of_net.end());
    }

    /// @brief Returns the pin at the given index of a net (0 is the driver).
    RoutingPinId net_pin(RoutingNetId net_id, int net_pin_index) const {
        pin_range pins_of_net = net_pins(net_id);
        VTR_ASSERT_SAFE_MSG(net_pin_index >= 0 && size_t(net_pin_index) < pins_of_net.size(), "Pin index must be in range");
        return pins_of_net.begin()[net_pin_index];
    }

    /// @brief Returns the block which drives a net (invalid if the net has no driver).
    RoutingBlockId net_driver_block(RoutingNetId net_id) const {
        RoutingPinId driver_pin = net_pin(net_id, 0);
        return driver_pin.is_valid() ? pin_block(driver_pin) : RoutingBlockId::INVALID();
    }

    /// @brief Returns true if the router should not route this net.
    bool net_is_ignored(RoutingNetId net_id) const {
        if (is_flat()) {
            return atom_nlist_.net_is_ignored(atom_net(net_id));
        }
        return clb_nlist_.net_is_ignored(cluster_net(net_id));
    }

    /// @brief Returns true if this net is a global net (e.g. a clock routed on a dedicated network).
    bool net_is_global(RoutingNetId net_id) const {
        if (is_flat()) {
            return atom_nlist_.net_is_global(atom_net(net_id));
        }
        return clb_nlist_.net_is_global(cluster_net(net_id));
    }

  public: // Conversions to the IDs of the atom and clustered netlists
    /// @brief Returns the atom block which a block of a flat routing netlist stands for.
    AtomBlockId atom_block(RoutingBlockId blk_id) const {
        VTR_ASSERT_SAFE(is_flat());
        return AtomBlockId(size_t(blk_id));
    }

    /// @brief Returns the atom pin which a pin of a flat routing netlist stands for.
    AtomPinId atom_pin(RoutingPinId pin_id) const {
        VTR_ASSERT_SAFE(is_flat());
        return AtomPinId(size_t(pin_id));
    }

    /**
     * @brief Returns the atom net which is routed by a net: the net itself in a flat
     *        routing netlist, otherwise the atom net carried by the clustered net.
     */
    AtomNetId atom_net(RoutingNetId net_id) const {
        if (is_flat()) {
            return AtomNetId(size_t(net_id));
        }
        return atom_lookup_.atom_net(cluster_net(net_id));
    }

    /**
     * @brief Returns the clustered block of a block: the block itself in a non-flat
     *        routing netlist, otherwise the cluster which contains the atom block.
     */
    ClusterBlockId cluster_block(RoutingBlockId blk_id) const {
        if (is_flat()) {
            return atom_lookup_.atom_clb(atom_block(blk_id));
        }
        return ClusterBlockId(size_t(blk_id));
    }

    /// @brief Returns the clustered pin which a pin of a non-flat routing netlist stands for.
    ClusterPinId cluster_pin(RoutingPinId pin_id) const {
        VTR_ASSERT_SAFE(!is_flat());
        return ClusterPinId(size_t(pin_id));
    }

    /// @brief Returns the clustered net which a net of a non-flat routing netlist stands for.
    ClusterNetId cluster_net(RoutingNetId net_id) const {
        VTR_ASSERT_SAFE(!is_flat());
        return ClusterNetId(size_t(net_id));
    }

  public: // Conversions from the IDs of the atom and clustered netlists
    /// @brief Returns the block of a non-flat routing netlist which stands for a clustered block.
    RoutingBlockId routing_block(ClusterBlockId clb_blk_id) const {
        VTR_ASSERT_SAFE(!is_flat());
        return RoutingBlockId(size_t(clb_blk_id));
    }

    /**
     * @brief Returns the net which routes a clustered net: the net itself in a non-flat
     *        routing netlist, otherwise the atom net carried by the clustered net.
     */
    RoutingNetId routing_net(ClusterNetId clb_net_id) const {
        if (is_flat()) {
            AtomNetId atom_net_id = atom_lookup_.atom_net(clb_net_id);
            VTR_ASSERT(atom_net_id.is_valid());
            return RoutingNetId(size_t(atom_net_id));
        }
        return RoutingNetId(size_t(clb_net_id));
    }

    /// @brief Returns the net of a flat routing netlist which stands for an atom net.
    RoutingNetId routing_net(AtomNetId atom_net_id) const {
        VTR_ASSERT_SAFE(is_flat());
        return RoutingNetId(size_t(atom_net_id));
    }

  private: // Helpers
    /**
     * @brief Checks that the blocks, pins and nets of the source netlist (either the atom
     *        or the clustered netlist) can be numbered like the source, and that every
     *        net has an entry for its driver.
     */
    template<typename SourceNetlist>
    static void check_source_(const SourceNetlist& src_nlist);

    /**
     * @brief Returns the given pins of a net of the source netlist as a range of RoutingPinIds.
     *
     * The range is never empty: every net has an entry for its driver (see check_source_()).
     */
    template<typename SourcePinRange>
    static pin_range make_pin_range_(const SourcePinRange& src_pins) {
        const auto* first_pin = &*src_pins.begin();
        return pin_range(RoutingPinIterator(first_pin), RoutingPinIterator(first_pin + src_pins.size()));
    }

  private: // Data
    // The atom netlist of the design: the source of this netlist with flat routing.
    const AtomNetlist& atom_nlist_;

    // The clustered netlist of the design: the source of this netlist without flat routing.
    const ClusteredNetlist& clb_nlist_;

    // The mapping between the atom and the clustered netlists.
    const AtomLookup& atom_lookup_;

    // True if this netlist was built from the atom netlist, false if from the clustered netlist.
    bool is_flat_;
};

/**
 * @brief Builds the routing netlist of the design held in the global VPR context.
 *
 *  @param is_flat  If true the netlist of a flat routing (the atom netlist) is built,
 *                  otherwise the netlist routed between the clustered blocks.
 */
RoutingNetlist make_routing_netlist(bool is_flat);
