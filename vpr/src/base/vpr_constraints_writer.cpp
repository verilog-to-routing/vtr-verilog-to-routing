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

#include <fstream>
#include "vpr_constraints_writer.h"

void write_vpr_floorplan_constraints(const char* file_name) {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //Fill in all the info needed for locking atoms to their locations
    VprConstraints constraints;

    std::map<ClusterBlockId, std::vector<AtomBlockId>> cluster_atoms;

	for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
		cluster_atoms.insert({blk_id, std::vector<AtomBlockId>()});
	}

	for (auto atom_blk_id : atom_ctx.nlist.blocks()) {
		ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(atom_blk_id);

		cluster_atoms[clb_index].push_back(atom_blk_id);
	}

	int part_id = 0;
    for (auto i = cluster_atoms.begin(); i != cluster_atoms.end(); i++) {
        std::string part_name;
        part_name = cluster_ctx.clb_nlist.block_name(i->first);

        PartitionId partid(part_id);

        Partition part;
        part.set_name(part_name);

        PartitionRegion pr;
        Region reg;

        auto loc = place_ctx.block_locs[i->first].loc;

        reg.set_region_rect(loc.x, loc.y, loc.x, loc.y);
        //reg.set_region_rect(loc.x - exp, loc.y - exp, loc.x + exp, loc.y + exp);

        pr.add_to_part_region(reg);
        part.set_part_region(pr);
        constraints.add_partition(part);

        int num_atoms = i->second.size();

        for (auto j = 0; j < num_atoms; j++) {
            AtomBlockId atom_id = i->second[j];
            constraints.add_constrained_atom(atom_id, partid);
        }
        part_id++;
    }


    VprConstraintsSerializer reader(constraints);

    if (vtr::check_file_name_extension(file_name, ".xml")) {
        std::fstream fp;
        fp.open(file_name, std::fstream::out | std::fstream::trunc);
        fp.precision(std::numeric_limits<float>::max_digits10);
        void* context;
        uxsd::write_vpr_constraints_xml(reader, context, fp);
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "Unknown extension on output %s",
                        file_name);
    }
}

