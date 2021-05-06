/*
 * vpr_constraints_writer.cpp
 *
 *      Author: khalid88
 */

#include "vpr_constraints_serializer.h"
#include "vpr_constraints_uxsdcxx.h"

#include "vtr_time.h"

#include "globals.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "echo_files.h"
#include "clustered_netlist_utils.h"

#include <fstream>
#include "vpr_constraints_writer.h"

void write_vpr_floorplan_constraints(const char* file_name, int expand, bool subtile) {
    //Fill in the constraints object to be printed out.
    VprConstraints constraints;

    setup_vpr_floorplan_constraints(constraints, expand, subtile);

    VprConstraintsSerializer writer(constraints);

    if (vtr::check_file_name_extension(file_name, ".xml")) {
        std::fstream fp;
        fp.open(file_name, std::fstream::out | std::fstream::trunc);
        fp.precision(std::numeric_limits<float>::max_digits10);
        void* context;
        uxsd::write_vpr_constraints_xml(writer, context, fp);
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "Unknown extension on output %s",
                        file_name);
    }
}

void setup_vpr_floorplan_constraints(VprConstraints& constraints, int expand, bool subtile) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    ClusterAtomsLookup atoms_lookup;

    int part_id = 0;
    /*
     * For each cluster block, create a partition filled with the atoms that are currently in the cluster.
     * The PartitionRegion will be the location of the block in current placement, modified by the expansion factor.
     * The subtile can also optionally be set in the PartitionRegion, based on the value passed in by the user.
     */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        std::string part_name;
        part_name = cluster_ctx.clb_nlist.block_name(blk_id);
        PartitionId partid(part_id);

        Partition part;
        part.set_name(part_name);

        PartitionRegion pr;
        Region reg;

        auto loc = place_ctx.block_locs[blk_id].loc;

        reg.set_region_rect(loc.x - expand, loc.y - expand, loc.x + expand, loc.y + expand);
        if (subtile) {
            int st = loc.sub_tile;
            reg.set_sub_tile(st);
        }

        pr.add_to_part_region(reg);
        part.set_part_region(pr);
        constraints.add_partition(part);

        std::vector<AtomBlockId> atoms = atoms_lookup.atoms_in_cluster(blk_id);
        int num_atoms = atoms.size();

        for (auto atm = 0; atm < num_atoms; atm++) {
            AtomBlockId atom_id = atoms[atm];
            constraints.add_constrained_atom(atom_id, partid);
        }
        part_id++;
    }
}
