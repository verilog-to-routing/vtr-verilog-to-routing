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

void write_vpr_floorplan_constraints(const char* file_name, int expand, bool subtile, enum constraints_split_factor floorplan_split) {
    if (floorplan_split == HALVES) {
        VTR_LOG("Splitting grid floorplan constraints into halves \n");

        //Split all blocks between two partitions
        VprConstraints constraints_half;

        setup_vpr_floorplan_constraints_halves(constraints_half);

        VprConstraintsSerializer writer_half(constraints_half);

        const char* file_name_half = "half.xml";

        if (vtr::check_file_name_extension(file_name_half, ".xml")) {
            std::fstream fp;
            fp.open(file_name_half, std::fstream::out | std::fstream::trunc);
            fp.precision(std::numeric_limits<float>::max_digits10);
            void* context;
            uxsd::write_vpr_constraints_xml(writer_half, context, fp);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Unknown extension on output %s",
                            file_name);
        }

        //Split half of all blocks between two partitions
        VprConstraints constraints_half_2;

        setup_vpr_floorplan_constraints_halves_half(constraints_half_2);

        VprConstraintsSerializer writer_half_2(constraints_half_2);

        const char* file_name_half_2 = "half_2.xml";

        if (vtr::check_file_name_extension(file_name_half_2, ".xml")) {
            std::fstream fp;
            fp.open(file_name_half_2, std::fstream::out | std::fstream::trunc);
            fp.precision(std::numeric_limits<float>::max_digits10);
            void* context;
            uxsd::write_vpr_constraints_xml(writer_half_2, context, fp);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Unknown extension on output %s",
                            file_name);
        }

    } else if (floorplan_split == QUADRANTS) {
        VTR_LOG("Splitting grid floorplan constraints into quadrants \n");

        //Split all blocks between four partitions
        VprConstraints constraints_quad;

        VTR_LOG("Splitting grid floorplan constraints into quadrants \n");
        setup_vpr_floorplan_constraints_quadrants(constraints_quad);

        VprConstraintsSerializer writer_quad(constraints_quad);

        const char* file_name_quad = "quad.xml";

        if (vtr::check_file_name_extension(file_name_quad, ".xml")) {
            std::fstream fp;
            fp.open(file_name_quad, std::fstream::out | std::fstream::trunc);
            fp.precision(std::numeric_limits<float>::max_digits10);
            void* context;
            uxsd::write_vpr_constraints_xml(writer_quad, context, fp);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Unknown extension on output %s",
                            file_name);
        }

        //Splitting half of all blocks between four partitions
        VprConstraints constraints_quad_2;

        VTR_LOG("Splitting grid floorplan constraints into quadrants \n");
        setup_vpr_floorplan_constraints_quadrants_half(constraints_quad_2);

        VprConstraintsSerializer writer_quad_2(constraints_quad_2);

        const char* file_name_quad_2 = "quad_2.xml";

        if (vtr::check_file_name_extension(file_name_quad_2, ".xml")) {
            std::fstream fp;
            fp.open(file_name_quad_2, std::fstream::out | std::fstream::trunc);
            fp.precision(std::numeric_limits<float>::max_digits10);
            void* context;
            uxsd::write_vpr_constraints_xml(writer_quad_2, context, fp);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Unknown extension on output %s",
                            file_name);
        }

    } else if (floorplan_split == SIXTEENTHS) {
        VTR_LOG("Splitting grid floorplan constraints into sixteenths \n");

        //Splitting all blocks between sixteen partitions
        VprConstraints constraints_six;

        setup_vpr_floorplan_constraints_sixteenths(constraints_six);

        VprConstraintsSerializer writer_six(constraints_six);

        const char* file_name_six = "sixteenth.xml";

        if (vtr::check_file_name_extension(file_name_six, ".xml")) {
            std::fstream fp;
            fp.open(file_name_six, std::fstream::out | std::fstream::trunc);
            fp.precision(std::numeric_limits<float>::max_digits10);
            void* context;
            uxsd::write_vpr_constraints_xml(writer_six, context, fp);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Unknown extension on output %s",
                            file_name);
        }

        //Splitting half of all blocks between sixteen partitions
        VprConstraints constraints_six_2;

        VTR_LOG("Splitting grid floorplan constraints into sixteenths \n");
        setup_vpr_floorplan_constraints_sixteenths_half(constraints_six_2);

        VprConstraintsSerializer writer_six_2(constraints_six_2);

        const char* file_name_six_2 = "sixteenth_2.xml";

        if (vtr::check_file_name_extension(file_name_six_2, ".xml")) {
            std::fstream fp;
            fp.open(file_name_six_2, std::fstream::out | std::fstream::trunc);
            fp.precision(std::numeric_limits<float>::max_digits10);
            void* context;
            uxsd::write_vpr_constraints_xml(writer_six_2, context, fp);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Unknown extension on output %s",
                            file_name);
        }

    } else { //one_spot
        VTR_LOG("Creating floorplan partition of size one for each cluster \n");

        //Put all clusters into partitions of size one
        VprConstraints constraints_fixed;

        setup_vpr_floorplan_constraints(constraints_fixed, expand, subtile);

        VprConstraintsSerializer writer_fixed(constraints_fixed);

        const char* file_name_fixed = "fixed.xml";

        if (vtr::check_file_name_extension(file_name_fixed, ".xml")) {
            std::fstream fp;
            fp.open(file_name_fixed, std::fstream::out | std::fstream::trunc);
            fp.precision(std::numeric_limits<float>::max_digits10);
            void* context;
            uxsd::write_vpr_constraints_xml(writer_fixed, context, fp);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Unknown extension on output %s",
                            file_name);
        }

        //Put half of all clusters into partitions of size one
        VprConstraints constraints_fixed_2;

        setup_vpr_floorplan_constraints_half(constraints_fixed_2, expand, subtile);

        VprConstraintsSerializer writer_fixed_2(constraints_fixed_2);

        const char* file_name_fixed_2 = "fixed_2.xml";

        if (vtr::check_file_name_extension(file_name_fixed_2, ".xml")) {
            std::fstream fp;
            fp.open(file_name_fixed_2, std::fstream::out | std::fstream::trunc);
            fp.precision(std::numeric_limits<float>::max_digits10);
            void* context;
            uxsd::write_vpr_constraints_xml(writer_fixed_2, context, fp);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Unknown extension on output %s",
                            file_name);
        }
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

void setup_vpr_floorplan_constraints_half(VprConstraints& constraints, int expand, bool subtile) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    ClusterAtomsLookup atoms_lookup;

    int part_id = 0;
    int block_count = 0;

    /*
     * For each cluster block, create a partition filled with the atoms that are currently in the cluster.
     * The PartitionRegion will be the location of the block in current placement, modified by the expansion factor.
     * The subtile can also optionally be set in the PartitionRegion, based on the value passed in by the user.
     */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (block_count % 2 == 0) {
            block_count++;
            continue;
        }
        block_count++;

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

void setup_vpr_floorplan_constraints_halves(VprConstraints& constraints) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    ClusterAtomsLookup atoms_lookup;

    int mid_x = device_ctx.grid.width() / 2;
    VTR_LOG("Device grid width is %d, mid_x is %d \n", device_ctx.grid.width(), mid_x);

    Partition left_partition;
    PartitionId left_part_id(0);
    create_partition(left_partition, "left_partition", 0, 0, mid_x - 1, device_ctx.grid.height() - 1);

    Partition right_partition;
    PartitionId right_part_id(1);
    create_partition(right_partition, "right_partition", mid_x, 0, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);

    /*
     * For each cluster block, see whether it belongs to the left partition or right partition
     * by seeing whether the x value is above or below mid_x. Add to the appropriate partition.
     */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {

        std::vector<AtomBlockId> atoms = atoms_lookup.atoms_in_cluster(blk_id);
        int num_atoms = atoms.size();

        if (place_ctx.block_locs[blk_id].loc.x < mid_x) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, left_part_id);
            }
        } else if (place_ctx.block_locs[blk_id].loc.x >= mid_x) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, right_part_id);
            }
        }
    }

    constraints.add_partition(left_partition);
    constraints.add_partition(right_partition);
}

void setup_vpr_floorplan_constraints_halves_half(VprConstraints& constraints) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    ClusterAtomsLookup atoms_lookup;

    int mid_x = device_ctx.grid.width() / 2;
    VTR_LOG("Device grid width is %d, mid_x is %d \n", device_ctx.grid.width(), mid_x);

    Partition left_partition;
    PartitionId left_part_id(0);
    create_partition(left_partition, "left_partition", 0, 0, mid_x - 1, device_ctx.grid.height() - 1);

    Partition right_partition;
    PartitionId right_part_id(1);
    create_partition(right_partition, "right_partition", mid_x, 0, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);

    int block_count = 0;

    /*
     * For each cluster block, see whether it belongs to the left partition or right partition
     * by seeing whether the x value is above or below mid_x. Add to the appropriate partition.
     */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (block_count % 2 == 0) {
            block_count++;
            continue;
        }
        block_count++;

        std::vector<AtomBlockId> atoms = atoms_lookup.atoms_in_cluster(blk_id);
        int num_atoms = atoms.size();

        if (place_ctx.block_locs[blk_id].loc.x < mid_x) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, left_part_id);
            }
        } else if (place_ctx.block_locs[blk_id].loc.x >= mid_x) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, right_part_id);
            }
        }
    }

    constraints.add_partition(left_partition);
    constraints.add_partition(right_partition);
}

void setup_vpr_floorplan_constraints_quadrants(VprConstraints& constraints) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    ClusterAtomsLookup atoms_lookup;

    int mid_x = device_ctx.grid.width() / 2;
    VTR_LOG("Device grid width is %d, mid_x is %d \n", device_ctx.grid.width(), mid_x);

    int mid_y = device_ctx.grid.height() / 2;
    VTR_LOG("Device grid height is %d, mid_y is %d \n", device_ctx.grid.height(), mid_y);

    Partition bottom_left_partition;
    PartitionId bottom_left_part_id(0);
    create_partition(bottom_left_partition, "bottom_left_partition", 0, 0, mid_x - 1, mid_y - 1);

    Partition bottom_right_partition;
    PartitionId bottom_right_part_id(1);
    create_partition(bottom_right_partition, "bottom_right_partition", mid_x, 0, device_ctx.grid.width() - 1, mid_y - 1);

    Partition top_left_partition;
    PartitionId top_left_part_id(2);
    create_partition(top_left_partition, "top_left_partition", 0, mid_y, mid_x - 1, device_ctx.grid.height() - 1);

    Partition top_right_partition;
    PartitionId top_right_part_id(3);
    create_partition(top_right_partition, "top_right_partition", mid_x, mid_y, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);

    /*
     * For each cluster block, see which quadrant it belongs to and add to the appropriate one
     */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        std::vector<AtomBlockId> atoms = atoms_lookup.atoms_in_cluster(blk_id);
        int num_atoms = atoms.size();

        if (place_ctx.block_locs[blk_id].loc.x < mid_x) {
            if (place_ctx.block_locs[blk_id].loc.y < mid_y) {
                for (auto atm = 0; atm < num_atoms; atm++) {
                    AtomBlockId atom_id = atoms[atm];
                    constraints.add_constrained_atom(atom_id, bottom_left_part_id);
                }
            } else {
                for (auto atm = 0; atm < num_atoms; atm++) {
                    AtomBlockId atom_id = atoms[atm];
                    constraints.add_constrained_atom(atom_id, top_left_part_id);
                }
            }
        } else {
            if (place_ctx.block_locs[blk_id].loc.y < mid_y) {
                for (auto atm = 0; atm < num_atoms; atm++) {
                    AtomBlockId atom_id = atoms[atm];
                    constraints.add_constrained_atom(atom_id, bottom_right_part_id);
                }
            } else {
                for (auto atm = 0; atm < num_atoms; atm++) {
                    AtomBlockId atom_id = atoms[atm];
                    constraints.add_constrained_atom(atom_id, top_right_part_id);
                }
            }
        }
    }

    constraints.add_partition(bottom_left_partition);  //0
    constraints.add_partition(bottom_right_partition); //1
    constraints.add_partition(top_left_partition);     //2
    constraints.add_partition(top_right_partition);    //3
}

void setup_vpr_floorplan_constraints_quadrants_half(VprConstraints& constraints) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    ClusterAtomsLookup atoms_lookup;

    int mid_x = device_ctx.grid.width() / 2;
    VTR_LOG("Device grid width is %d, mid_x is %d \n", device_ctx.grid.width(), mid_x);

    int mid_y = device_ctx.grid.height() / 2;
    VTR_LOG("Device grid height is %d, mid_y is %d \n", device_ctx.grid.height(), mid_y);

    Partition bottom_left_partition;
    PartitionId bottom_left_part_id(0);
    create_partition(bottom_left_partition, "bottom_left_partition", 0, 0, mid_x - 1, mid_y - 1);

    Partition bottom_right_partition;
    PartitionId bottom_right_part_id(1);
    create_partition(bottom_right_partition, "bottom_right_partition", mid_x, 0, device_ctx.grid.width() - 1, mid_y - 1);

    Partition top_left_partition;
    PartitionId top_left_part_id(2);
    create_partition(top_left_partition, "top_left_partition", 0, mid_y, mid_x - 1, device_ctx.grid.height() - 1);

    Partition top_right_partition;
    PartitionId top_right_part_id(3);
    create_partition(top_right_partition, "top_right_partition", mid_x, mid_y, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);

    int block_count = 0;
    /*
     * For each cluster block, see which quadrant it belongs to and add to the appropriate one
     */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (block_count % 2 == 0) {
            block_count++;
            continue;
        }
        block_count++;

        std::vector<AtomBlockId> atoms = atoms_lookup.atoms_in_cluster(blk_id);
        int num_atoms = atoms.size();

        if (place_ctx.block_locs[blk_id].loc.x < mid_x) {
            if (place_ctx.block_locs[blk_id].loc.y < mid_y) {
                for (auto atm = 0; atm < num_atoms; atm++) {
                    AtomBlockId atom_id = atoms[atm];
                    constraints.add_constrained_atom(atom_id, bottom_left_part_id);
                }
            } else {
                for (auto atm = 0; atm < num_atoms; atm++) {
                    AtomBlockId atom_id = atoms[atm];
                    constraints.add_constrained_atom(atom_id, top_left_part_id);
                }
            }
        } else {
            if (place_ctx.block_locs[blk_id].loc.y < mid_y) {
                for (auto atm = 0; atm < num_atoms; atm++) {
                    AtomBlockId atom_id = atoms[atm];
                    constraints.add_constrained_atom(atom_id, bottom_right_part_id);
                }
            } else {
                for (auto atm = 0; atm < num_atoms; atm++) {
                    AtomBlockId atom_id = atoms[atm];
                    constraints.add_constrained_atom(atom_id, top_right_part_id);
                }
            }
        }
    }

    constraints.add_partition(bottom_left_partition);  //0
    constraints.add_partition(bottom_right_partition); //1
    constraints.add_partition(top_left_partition);     //2
    constraints.add_partition(top_right_partition);    //3
}

void setup_vpr_floorplan_constraints_sixteenths(VprConstraints& constraints) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    ClusterAtomsLookup atoms_lookup;

    int mid_x = device_ctx.grid.width() / 2;
    VTR_LOG("Device grid width is %d, mid_x is %d \n", device_ctx.grid.width(), mid_x);

    int mid_y = device_ctx.grid.height() / 2;
    VTR_LOG("Device grid height is %d, mid_y is %d \n", device_ctx.grid.height(), mid_y);

    int pre_x;
    if (mid_x % 2 == 0) {
        pre_x = mid_x / 2;
        VTR_LOG("pre_x is %d \n", pre_x);
    } else {
        pre_x = (mid_x / 2) + 1;
        VTR_LOG("pre_x is %d \n", pre_x);
    }

    int post_x = mid_x + pre_x;
    VTR_LOG("post_x is %d \n", post_x);

    int pre_y;
    if (mid_y % 2 == 0) {
        pre_y = mid_y / 2;
        VTR_LOG("pre_x is %d \n", pre_x);
    } else {
        pre_y = (mid_y / 2) + 1;
        VTR_LOG("pre_y is %d \n", pre_y);
    }

    int post_y = mid_y + pre_y;
    VTR_LOG("post_x is %d \n", post_y);

    Partition part0;
    PartitionId partid_0(0);
    create_partition(part0, "part0", 0, 0, pre_x - 1, pre_y - 1);

    Partition part1;
    PartitionId partid_1(1);
    create_partition(part1, "part1", pre_x, 0, mid_x - 1, pre_y - 1);

    Partition part2;
    PartitionId partid_2(2);
    create_partition(part2, "part2", mid_x, 0, post_x - 1, pre_y - 1);

    Partition part3;
    PartitionId partid_3(3);
    create_partition(part3, "part3", post_x, 0, device_ctx.grid.width() - 1, pre_y - 1);

    Partition part4;
    PartitionId partid_4(4);
    create_partition(part4, "part4", 0, pre_y, pre_x - 1, mid_y - 1);

    Partition part5;
    PartitionId partid_5(5);
    create_partition(part5, "part5", pre_x, pre_y, mid_x - 1, mid_y - 1);

    Partition part6;
    PartitionId partid_6(6);
    create_partition(part6, "part6", mid_x, pre_y, post_x - 1, mid_y - 1);

    Partition part7;
    PartitionId partid_7(7);
    create_partition(part7, "part7", post_x, pre_y, device_ctx.grid.width() - 1, mid_y - 1);

    Partition part8;
    PartitionId partid_8(8);
    create_partition(part8, "part8", 0, mid_y, pre_x - 1, post_y - 1);

    Partition part9;
    PartitionId partid_9(9);
    create_partition(part9, "part9", pre_x, mid_y, mid_x - 1, post_y - 1);

    Partition part10;
    PartitionId partid_10(10);
    create_partition(part10, "part10", mid_x, mid_y, post_x - 1, post_y - 1);

    Partition part11;
    PartitionId partid_11(11);
    create_partition(part11, "part11", post_x, mid_y, device_ctx.grid.width() - 1, post_y - 1);

    Partition part12;
    PartitionId partid_12(12);
    create_partition(part12, "part12", 0, post_y, pre_x - 1, device_ctx.grid.height() - 1);

    Partition part13;
    PartitionId partid_13(13);
    create_partition(part13, "part13", pre_x, post_y, mid_x - 1, device_ctx.grid.height() - 1);

    Partition part14;
    PartitionId partid_14(14);
    create_partition(part14, "part14", mid_x, post_y, post_x - 1, device_ctx.grid.height() - 1);

    Partition part15;
    PartitionId partid_15(15);
    create_partition(part15, "part15", post_x, post_y, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);

    int width = device_ctx.grid.width();
    int height = device_ctx.grid.height();

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        std::vector<AtomBlockId> atoms = atoms_lookup.atoms_in_cluster(blk_id);
        int num_atoms = atoms.size();
        int x = place_ctx.block_locs[blk_id].loc.x;
        int y = place_ctx.block_locs[blk_id].loc.y;

        //Check if in grid row 1
        if (x < pre_x && y < pre_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_0);
            }
        } else if (x >= pre_x && x < mid_x && y < pre_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_1);
            }
        } else if (x >= mid_x && x < post_x && y < pre_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_2);
            }
        } else if (x >= post_x && x < width && y < pre_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_3);
            }
        }

        //Check if in grid row 2
        if (x < pre_x && y >= pre_y && y < mid_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_4);
            }
        } else if (x >= pre_x && x < mid_x && y >= pre_y && y < mid_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_5);
            }
        } else if (x >= mid_x && x < post_x && y >= pre_y && y < mid_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_6);
            }
        } else if (x >= post_x && x < width && y >= pre_y && y < mid_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_7);
            }
        }

        //Check if in grid row 3
        if (x < pre_x && y >= pre_y && y >= mid_y && y < post_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_8);
            }
        } else if (x >= pre_x && x < mid_x && y >= mid_y && y < post_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_9);
            }
        } else if (x >= mid_x && x < post_x && y >= mid_y && y < post_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_10);
            }
        } else if (x >= post_x && x < width && y >= mid_y && y < post_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_11);
            }
        }

        //Check if in grid row 4
        if (x < pre_x && y >= post_y && y < height) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_12);
            }
        } else if (x >= pre_x && x < mid_x && y >= post_y && y < height) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_13);
            }
        } else if (x >= mid_x && x < post_x && y >= post_y && y < height) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_14);
            }
        } else if (x >= post_x && x < width && y >= post_y && y < height) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_15);
            }
        }
    }

    constraints.add_partition(part0);
    constraints.add_partition(part1);
    constraints.add_partition(part2);
    constraints.add_partition(part3);

    constraints.add_partition(part4);
    constraints.add_partition(part5);
    constraints.add_partition(part6);
    constraints.add_partition(part7);

    constraints.add_partition(part8);
    constraints.add_partition(part9);
    constraints.add_partition(part10);
    constraints.add_partition(part11);

    constraints.add_partition(part12);
    constraints.add_partition(part13);
    constraints.add_partition(part14);
    constraints.add_partition(part15);
}

void setup_vpr_floorplan_constraints_sixteenths_half(VprConstraints& constraints) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    ClusterAtomsLookup atoms_lookup;

    int mid_x = device_ctx.grid.width() / 2;
    VTR_LOG("Device grid width is %d, mid_x is %d \n", device_ctx.grid.width(), mid_x);

    int mid_y = device_ctx.grid.height() / 2;
    VTR_LOG("Device grid height is %d, mid_y is %d \n", device_ctx.grid.height(), mid_y);

    int pre_x;
    if (mid_x % 2 == 0) {
        pre_x = mid_x / 2;
        VTR_LOG("pre_x is %d \n", pre_x);
    } else {
        pre_x = (mid_x / 2) + 1;
        VTR_LOG("pre_x is %d \n", pre_x);
    }

    int post_x = mid_x + pre_x;
    VTR_LOG("post_x is %d \n", post_x);

    int pre_y;
    if (mid_y % 2 == 0) {
        pre_y = mid_y / 2;
        VTR_LOG("pre_x is %d \n", pre_x);
    } else {
        pre_y = (mid_y / 2) + 1;
        VTR_LOG("pre_y is %d \n", pre_y);
    }

    int post_y = mid_y + pre_y;
    VTR_LOG("post_x is %d \n", post_y);

    Partition part0;
    PartitionId partid_0(0);
    create_partition(part0, "part0", 0, 0, pre_x - 1, pre_y - 1);

    Partition part1;
    PartitionId partid_1(1);
    create_partition(part1, "part1", pre_x, 0, mid_x - 1, pre_y - 1);

    Partition part2;
    PartitionId partid_2(2);
    create_partition(part2, "part2", mid_x, 0, post_x - 1, pre_y - 1);

    Partition part3;
    PartitionId partid_3(3);
    create_partition(part3, "part3", post_x, 0, device_ctx.grid.width() - 1, pre_y - 1);

    Partition part4;
    PartitionId partid_4(4);
    create_partition(part4, "part4", 0, pre_y, pre_x - 1, mid_y - 1);

    Partition part5;
    PartitionId partid_5(5);
    create_partition(part5, "part5", pre_x, pre_y, mid_x - 1, mid_y - 1);

    Partition part6;
    PartitionId partid_6(6);
    create_partition(part6, "part6", mid_x, pre_y, post_x - 1, mid_y - 1);

    Partition part7;
    PartitionId partid_7(7);
    create_partition(part7, "part7", post_x, pre_y, device_ctx.grid.width() - 1, mid_y - 1);

    Partition part8;
    PartitionId partid_8(8);
    create_partition(part8, "part8", 0, mid_y, pre_x - 1, post_y - 1);

    Partition part9;
    PartitionId partid_9(9);
    create_partition(part9, "part9", pre_x, mid_y, mid_x - 1, post_y - 1);

    Partition part10;
    PartitionId partid_10(10);
    create_partition(part10, "part10", mid_x, mid_y, post_x - 1, post_y - 1);

    Partition part11;
    PartitionId partid_11(11);
    create_partition(part11, "part11", post_x, mid_y, device_ctx.grid.width() - 1, post_y - 1);

    Partition part12;
    PartitionId partid_12(12);
    create_partition(part12, "part12", 0, post_y, pre_x - 1, device_ctx.grid.height() - 1);

    Partition part13;
    PartitionId partid_13(13);
    create_partition(part13, "part13", pre_x, post_y, mid_x - 1, device_ctx.grid.height() - 1);

    Partition part14;
    PartitionId partid_14(14);
    create_partition(part14, "part14", mid_x, post_y, post_x - 1, device_ctx.grid.height() - 1);

    Partition part15;
    PartitionId partid_15(15);
    create_partition(part15, "part15", post_x, post_y, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);

    int width = device_ctx.grid.width();
    int height = device_ctx.grid.height();

    int block_count = 0;

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (block_count % 2 == 0) {
            block_count++;
            continue;
        }
        block_count++;

        std::vector<AtomBlockId> atoms = atoms_lookup.atoms_in_cluster(blk_id);
        int num_atoms = atoms.size();
        int x = place_ctx.block_locs[blk_id].loc.x;
        int y = place_ctx.block_locs[blk_id].loc.y;

        //Check if in grid row 1
        if (x < pre_x && y < pre_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_0);
            }
        } else if (x >= pre_x && x < mid_x && y < pre_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_1);
            }
        } else if (x >= mid_x && x < post_x && y < pre_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_2);
            }
        } else if (x >= post_x && x < width && y < pre_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_3);
            }
        }

        //Check if in grid row 2
        if (x < pre_x && y >= pre_y && y < mid_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_4);
            }
        } else if (x >= pre_x && x < mid_x && y >= pre_y && y < mid_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_5);
            }
        } else if (x >= mid_x && x < post_x && y >= pre_y && y < mid_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_6);
            }
        } else if (x >= post_x && x < width && y >= pre_y && y < mid_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_7);
            }
        }

        //Check if in grid row 3
        if (x < pre_x && y >= pre_y && y >= mid_y && y < post_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_8);
            }
        } else if (x >= pre_x && x < mid_x && y >= mid_y && y < post_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_9);
            }
        } else if (x >= mid_x && x < post_x && y >= mid_y && y < post_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_10);
            }
        } else if (x >= post_x && x < width && y >= mid_y && y < post_y) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_11);
            }
        }

        //Check if in grid row 4
        if (x < pre_x && y >= post_y && y < height) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_12);
            }
        } else if (x >= pre_x && x < mid_x && y >= post_y && y < height) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_13);
            }
        } else if (x >= mid_x && x < post_x && y >= post_y && y < height) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_14);
            }
        } else if (x >= post_x && x < width && y >= post_y && y < height) {
            for (auto atm = 0; atm < num_atoms; atm++) {
                AtomBlockId atom_id = atoms[atm];
                constraints.add_constrained_atom(atom_id, partid_15);
            }
        }
    }

    constraints.add_partition(part0);
    constraints.add_partition(part1);
    constraints.add_partition(part2);
    constraints.add_partition(part3);

    constraints.add_partition(part4);
    constraints.add_partition(part5);
    constraints.add_partition(part6);
    constraints.add_partition(part7);

    constraints.add_partition(part8);
    constraints.add_partition(part9);
    constraints.add_partition(part10);
    constraints.add_partition(part11);

    constraints.add_partition(part12);
    constraints.add_partition(part13);
    constraints.add_partition(part14);
    constraints.add_partition(part15);
}

void create_partition(Partition& part, std::string part_name, int xmin, int ymin, int xmax, int ymax) {
    part.set_name(part_name);
    PartitionRegion part_pr;
    Region part_region;
    part_region.set_region_rect(xmin, ymin, xmax, ymax);
    std::vector<Region> part_regions;
    part_regions.push_back(part_region);
    part_pr.set_partition_region(part_regions);
    part.set_part_region(part_pr);
}
