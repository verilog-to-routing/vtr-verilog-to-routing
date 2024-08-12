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
#include "clustered_netlist_utils.h"

#include <fstream>
#include "vpr_constraints_writer.h"
#include "region.h"
#include "re_cluster_util.h"

/**
 * @brief Create a partition with the given name and a single region.
 *
 * @param part_name The name of the partition to be created.
 * @param region The region that the partition covers.
 * @return A newly created partition with the giver name and region.
 */
static Partition create_partition(const std::string& part_name, const Region& region);

void write_vpr_floorplan_constraints(const char* file_name, int expand, bool subtile, int horizontal_partitions, int vertical_partitions) {
    VprConstraints constraints;

    if (horizontal_partitions != 0 && vertical_partitions != 0) {
        setup_vpr_floorplan_constraints_cutpoints(constraints, horizontal_partitions, vertical_partitions);
    } else {
        setup_vpr_floorplan_constraints_one_loc(constraints, expand, subtile);
    }

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

void setup_vpr_floorplan_constraints_one_loc(VprConstraints& constraints, int expand, bool subtile) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    int part_id = 0;
    /*
     * For each cluster block, create a partition filled with the atoms that are currently in the cluster.
     * The PartitionRegion will be the location of the block in current placement, modified by the expansion factor.
     * The subtile can also optionally be set in the PartitionRegion, based on the value passed in by the user.
     */
    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        const std::string& part_name = cluster_ctx.clb_nlist.block_name(blk_id);
        PartitionId partid(part_id);

        Partition part;
        part.set_name(part_name);

        const auto& loc = place_ctx.block_locs[blk_id].loc;

        PartitionRegion pr;
        Region reg(loc.x - expand, loc.y - expand,
                   loc.x + expand, loc.y + expand, loc.layer);

        if (subtile) {
            reg.set_sub_tile(loc.sub_tile);
        }

        pr.add_to_part_region(reg);
        part.set_part_region(pr);
        constraints.mutable_place_constraints().add_partition(part);

        const std::unordered_set<AtomBlockId>& atoms = cluster_to_atoms(blk_id);

        for (AtomBlockId atom_id : atoms) {
            constraints.mutable_place_constraints().add_constrained_atom(atom_id, partid);
        }
        part_id++;
    }
}

void setup_vpr_floorplan_constraints_cutpoints(VprConstraints& constraints, int horizontal_cutpoints, int vertical_cutpoints) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    const int n_layers = device_ctx.grid.get_num_layers();

    //calculate the cutpoint values according to the grid size
    //load two arrays - one for horizontal cutpoints and one for vertical

    std::vector<int> horizontal_cuts;
    std::vector<int> vertical_cuts;

    // This function has not been tested for multi-layer grids
    VTR_ASSERT(n_layers == 1);
    int horizontal_interval = device_ctx.grid.width() / horizontal_cutpoints;
    VTR_LOG("Device grid width is %d, horizontal interval is %d\n", device_ctx.grid.width(), horizontal_interval);

    unsigned int horizontal_point = horizontal_interval;
    horizontal_cuts.push_back(0);
    int num_horizontal_cuts = 0;
    while (num_horizontal_cuts < horizontal_cutpoints - 1) {
        horizontal_cuts.push_back(horizontal_point);
        horizontal_point = horizontal_point + horizontal_interval;
        num_horizontal_cuts++;
    }
    //Add in the last point after your exit the while loop
    horizontal_cuts.push_back(device_ctx.grid.width());

    int vertical_interval = device_ctx.grid.height() / vertical_cutpoints;
    VTR_LOG("Device grid height is %d, vertical interval is %d\n", device_ctx.grid.height(), vertical_interval);

    unsigned int vertical_point = vertical_interval;
    vertical_cuts.push_back(0);
    int num_vertical_cuts = 0;
    while (num_vertical_cuts < vertical_cutpoints - 1) {
        vertical_cuts.push_back(vertical_point);
        vertical_point = vertical_point + vertical_interval;
        num_vertical_cuts++;
    }
    //Add in the last point after your exit the while loop
    vertical_cuts.push_back(device_ctx.grid.height());

    //Create floorplan regions based on the cutpoints
    std::unordered_map<Region, std::vector<AtomBlockId>> region_atoms;

    for (unsigned int i = 0; i < horizontal_cuts.size() - 1; i++) {
        int xmin = horizontal_cuts[i];
        int xmax = horizontal_cuts[i + 1] - 1;

        for (unsigned int j = 0; j < vertical_cuts.size() - 1; j++) {
            int ymin = vertical_cuts[j];
            int ymax = vertical_cuts[j + 1] - 1;

            Region reg(xmin, ymin, xmax, ymax, 0, n_layers - 1);
            // This function has not been tested for multi-layer grids. An assertion is used earlier to make sure that the grid has only one layer
            std::vector<AtomBlockId> atoms;
            region_atoms.insert({reg, atoms});
        }
    }

    /*
     * For each cluster block, see which region it belongs to, and add its atoms to the
     * appropriate region accordingly
     */
    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        const std::unordered_set<AtomBlockId>& atoms = cluster_to_atoms(blk_id);
        int x = place_ctx.block_locs[blk_id].loc.x;
        int y = place_ctx.block_locs[blk_id].loc.y;
        int width = device_ctx.grid.width();
        int height = device_ctx.grid.height();
        VTR_ASSERT(x >= 0 && x < width);
        VTR_ASSERT(y >= 0 && y < height);
        int xminimum = 0, yminimum = 0, xmaximum = 0, ymaximum = 0;

        for (unsigned int h = 1; h < horizontal_cuts.size(); h++) {
            if (x < horizontal_cuts[h]) {
                xmaximum = horizontal_cuts[h] - 1;
                xminimum = horizontal_cuts[h - 1];
                break;
            }
        }

        for (unsigned int v = 1; v < vertical_cuts.size(); v++) {
            if (y < vertical_cuts[v]) {
                ymaximum = vertical_cuts[v] - 1;
                yminimum = vertical_cuts[v - 1];
                break;
            }
        }

        Region current_reg(xminimum, yminimum, xmaximum, ymaximum, 0, n_layers-1);
        // This function has not been tested for multi-layer grids. An assertion is used earlier to make sure that the grid has only one layer

        auto got = region_atoms.find(current_reg);

        VTR_ASSERT(got != region_atoms.end());

        for (AtomBlockId atom_id : atoms) {
            got->second.push_back(atom_id);
        }
    }

    int num_partitions = 0;
    for (const auto& [region, atoms] : region_atoms) {
        PartitionId partid(num_partitions);
        std::string part_name = "Part" + std::to_string(num_partitions);
        Partition part = create_partition(part_name, region);
        constraints.mutable_place_constraints().add_partition(part);

        for (AtomBlockId blk_id : atoms) {
            constraints.mutable_place_constraints().add_constrained_atom(blk_id, partid);
        }

        num_partitions++;
    }
}

static Partition create_partition(const std::string& part_name, const Region& region) {
    Partition part;

    part.set_name(part_name);
    PartitionRegion part_pr;
    part_pr.set_partition_region({region});
    part.set_part_region(part_pr);

    return part;
}
