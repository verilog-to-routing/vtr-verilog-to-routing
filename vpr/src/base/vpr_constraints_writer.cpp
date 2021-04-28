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

void write_place_constraints(const char* file_name) {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& device_ctx = g_vpr_ctx.device();

    //Fill in all the information needed for putting all atoms in one big partition

    /*test_partition part;

    for (auto blk_id : atom_ctx.nlist.blocks()) {
    	part.atom_names.push_back(atom_ctx.nlist.block_name(blk_id));
    }

    part.x_high = device_ctx.grid.width() - 1;
    part.y_high = device_ctx.grid.height() - 1;

    part.part_name = "my_part";

    part.num_part = 1;

    part.num_atom = atom_ctx.nlist.blocks().size();

    part.num_region = 1;*/

    //Fill in all the info needed for locking atoms to their locations
    VprConstraints constraints;
    Partition part1;
    part1.set_name("mypart1");
    Partition part2;
    part2.set_name("mypart2");

    Region  reg1;
    reg1.set_region_rect(0,0,18,18);
    PartitionRegion pr1;
    pr1.add_to_part_region(reg1);
    part1.set_part_region(pr1);

    Region reg2;
    reg2.set_region_rect(0,0,18,18);
    PartitionRegion pr2;
    pr2.add_to_part_region(reg2);
    part2.set_part_region(pr2);

    constraints.add_partition(part1);
    constraints.add_partition(part2);

    PartitionId pid1(0);
    PartitionId pid2(1);

    AtomBlockId id0(0);
    AtomBlockId id1(1);
    AtomBlockId id2(2);
    constraints.add_constrained_atom(id0,pid1);
    constraints.add_constrained_atom(id1,pid1);
    constraints.add_constrained_atom(id2,pid1);

    AtomBlockId id3(3);
    AtomBlockId id4(4);
    constraints.add_constrained_atom(id3,pid2);
    constraints.add_constrained_atom(id4,pid2);


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

