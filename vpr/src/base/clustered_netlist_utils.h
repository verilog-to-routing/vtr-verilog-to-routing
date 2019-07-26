#ifndef CLUSTERED_NETLIST_UTILS_H
#define CLUSTERED_NETLIST_UTILS_H

#include "vtr_vector.h"
#include "vtr_range.h"

#include "vpr_utils.h"
#include "atom_netlist_fwd.h"
#include "clustered_netlist_fwd.h"

class ClusteredPinAtomPinsLookup {
  public:
    typedef std::vector<AtomPinId>::const_iterator atom_pin_iterator;
    typedef typename vtr::Range<atom_pin_iterator> atom_pin_range;

  public:
    ClusteredPinAtomPinsLookup(const ClusteredNetlist& clustered_netlist, const IntraLbPbPinLookup& pb_gpin_lookup);

    atom_pin_range connected_atom_pins(ClusterPinId clustered_pin) const;

  private:
    void init_lookup(const ClusteredNetlist& clustered_netlist, const IntraLbPbPinLookup& pb_gpin_lookup);

  private:
    vtr::vector<ClusterPinId, std::vector<AtomPinId>> clustered_pin_connected_atom_pins_;
};

#endif
