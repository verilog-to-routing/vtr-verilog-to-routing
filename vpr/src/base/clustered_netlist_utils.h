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
    ClusteredPinAtomPinsLookup(const ClusteredNetlist& clustered_netlist, const AtomNetlist& atom_netlist, const IntraLbPbPinLookup& pb_gpin_lookup);

    atom_pin_range connected_atom_pins(ClusterPinId clustered_pin) const;
    ClusterPinId connected_clb_pin(AtomPinId atom_pin) const;

  private:
    void init_lookup(const ClusteredNetlist& clustered_netlist, const AtomNetlist& atom_netlist, const IntraLbPbPinLookup& pb_gpin_lookup);

  private:
    vtr::vector<ClusterPinId, std::vector<AtomPinId>> clustered_pin_connected_atom_pins_;
    vtr::vector<AtomPinId, ClusterPinId> atom_pin_connected_cluster_pin_;
};

/*
 * This lookup is used to see which atoms are in each cluster block.
 * Getting the atoms inside of a cluster is an order k lookup.
 * The data is initialized automatically upon creation of the object.
 * The class should only be used after the clustered netlist is created.
 */
class ClusterAtomsLookup {
  public:
    ClusterAtomsLookup();
    std::vector<AtomBlockId> atoms_in_cluster(ClusterBlockId blk_id);

  private:
    void init_lookup();

  private:
    //Store the atom ids of the atoms inside each cluster
    vtr::vector<ClusterBlockId, std::vector<AtomBlockId>> cluster_atoms;
};
#endif
