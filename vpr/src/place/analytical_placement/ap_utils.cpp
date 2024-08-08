
#include "ap_utils.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include "PartialPlacement.h"
#include "ap_netlist.h"
#include "ap_netlist_fwd.h"
#include "atom_netlist.h"
#include "vpr_context.h"
#include "vtr_assert.h"
#include "vtr_log.h"

void print_ap_netlist_stats(const APNetlist& netlist) {
    // Get the number of moveable and fixed blocks
    size_t num_moveable_blocks = 0;
    size_t num_fixed_blocks = 0;
    for (APBlockId blk_id : netlist.blocks()) {
        if (netlist.block_type(blk_id) == APBlockType::MOVEABLE)
            num_moveable_blocks++;
        else
            num_fixed_blocks++;
    }
    // Get the fanout information of nets
    size_t highest_fanout = 0;
    float average_fanout = 0.f;
    for (APNetId net_id : netlist.nets()) {
        size_t net_fanout = netlist.net_pins(net_id).size();
        if (net_fanout > highest_fanout)
            highest_fanout = net_fanout;
        average_fanout += static_cast<float>(net_fanout);
    }
    average_fanout /= static_cast<float>(netlist.nets().size());
    // Print the statistics
    VTR_LOG("Analytical Placement Netlist Statistics:\n");
    VTR_LOG("\tBlocks: %zu\n", netlist.blocks().size());
    VTR_LOG("\t\tMoveable Blocks: %zu\n", num_moveable_blocks);
    VTR_LOG("\t\tFixed Blocks: %zu\n", num_fixed_blocks);
    VTR_LOG("\tNets: %zu\n", netlist.nets().size());
    VTR_LOG("\t\tAverage Fanout: %.2f\n", average_fanout);
    VTR_LOG("\t\tHighest Fanout: %zu\n", highest_fanout);
    VTR_LOG("\tPins: %zu\n", netlist.pins().size());
}

double get_hpwl(const PartialPlacement& placement, const APNetlist& netlist) {
    // NOTE: This gets the HPWL of the netlist and partial placement as it
    //       currently appears. The user should be aware that fractional
    //       positions of blocks are not realistic and the netlist is ignoring
    //       some nets to make the analytical placment problem easier.
    //       The user should use an atom-level HPWL for an accurate result. This
    //       is more used for debugging.
    double hpwl = 0.0;
    for (APNetId net_id : netlist.nets()) {
        double min_x = std::numeric_limits<double>::max();
        double max_x = std::numeric_limits<double>::lowest();
        double min_y = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<double>::lowest();
        for (APPinId pin_id : netlist.net_pins(net_id)) {
            APBlockId blk_id = netlist.pin_block(pin_id);
            max_x = std::max(max_x, placement.block_x_locs[blk_id]);
            min_x = std::min(min_x, placement.block_x_locs[blk_id]);
            max_y = std::max(max_y, placement.block_y_locs[blk_id]);
            min_y = std::min(min_y, placement.block_y_locs[blk_id]);
        }
        VTR_ASSERT(max_x >= min_x && max_y >= min_y);
        hpwl += max_x - min_x + max_y - min_y;
    }
    return hpwl;
}

void unicode_art(const PartialPlacement& placement,
                 const APNetlist& netlist,
                 const DeviceContext& device_ctx) {
    VTR_LOG("unicode_art start\n");
    size_t device_width = device_ctx.grid.width();
    size_t device_height = device_ctx.grid.height();
    size_t board_width = device_width * 5;
    size_t board_height = device_height * 5;
    std::vector<std::vector<int>> board(board_height, std::vector<int>(board_width, 0));
    for (APBlockId blk_id : netlist.blocks()) {
        size_t node_x_relative = std::floor(placement.block_x_locs[blk_id] / (device_width * board_width));
        size_t node_y_relative = std::floor(placement.block_y_locs[blk_id] / (device_height * board_height));
        if (netlist.block_type(blk_id) == APBlockType::MOVEABLE) {
            board[node_x_relative][node_y_relative]++;
        } else {
            board[node_x_relative][node_y_relative] = -1;
        }
    }
    int max = 0;
    for(unsigned y_board_id = 0; y_board_id < board_height; y_board_id++)
        for(unsigned x_board_id = 0; x_board_id < board_width; x_board_id++)
            if (board[x_board_id][y_board_id] > max)
                max = board[x_board_id][y_board_id];
    for(unsigned y_board_id = 0; y_board_id < board_height; y_board_id++){
        for(unsigned x_board_id = 0; x_board_id < board_width; x_board_id++){
            int density = board[y_board_id][x_board_id];
            int density_range = std::floor((double)density/(double)max*6);
            // if (density >=0 && density <=9){
            //     char digit[8] = "0️⃣";
            //     digit[0] += density;
            //     VTR_LOG(digit);
            // }else if(density >= 10){
            //     VTR_LOG("🔟");
            // }else if(density == -1){
            //     VTR_LOG("*️⃣");
            // }else{
            //     VTR_ASSERT(0 && "Unexpected range!");
            // }
            switch (density_range) {
                default:
                VTR_ASSERT(0 && "unexpected range!");
                break;
                case -1:
                VTR_LOG("⬜");
                break;
                case 0:
                VTR_LOG("⬛");
                break;
                case 1:
                VTR_LOG("🟫");
                break;
                case 2:
                VTR_LOG("🟩");
                break;
                case 3:
                VTR_LOG("🟨");
                break;
                case 4:
                VTR_LOG("🟧");
                break;
                case 5:
                VTR_LOG("🟥");
                break;
                case 6:
                VTR_LOG("🟪");
                break;
            }
        }
        VTR_LOG("\n");
    }
    VTR_LOG("unicode_art end\n");
    fflush(stderr);
}

bool export_to_flat_placement_file(const PartialPlacement& placement,
                                   const APNetlist& ap_netlist,
                                   const AtomNetlist& atom_netlist,
                                   const std::string& file_name) {
    // https://doi.org/10.1145/3665283.3665300
    // Primitive Name /t X /t Y /t Subtile /t Site
    std::ofstream flat_placement_file;
    flat_placement_file.open(file_name, std::ios::out);
    if (!flat_placement_file) {
        VTR_LOG_ERROR("Failed to open the flat placement file '%s' to export the PartialPlacement.\n",
                file_name.c_str());
        return false;
    }

    VTR_ASSERT(placement.block_x_locs.size() == ap_netlist.blocks().size());
    // FIXME: Are sites just unique IDs per molecule or do they need to start at
    //        0 per tile?
    //   - No, they are related to t_pb_graph_node; this is incorrect.
    size_t site_idx = 0;
    for (APBlockId blk_id : ap_netlist.blocks()) {
        int mol_pos_x = std::floor(placement.block_x_locs[blk_id]);
        int mol_pos_y = std::floor(placement.block_y_locs[blk_id]);
        int mol_sub_tile = placement.block_sub_tiles[blk_id];
        const t_pack_molecule* mol = ap_netlist.block_molecule(blk_id);
        for (AtomBlockId block_id : mol->atom_block_ids) {
            // Primitive's name
            flat_placement_file << atom_netlist.block_name(block_id) << '\t';
            // Primitive's cluster coordinates
            flat_placement_file << mol_pos_x << '\t' << mol_pos_y << '\t';
            flat_placement_file << mol_sub_tile << '\t';
            // Primitive site index
            flat_placement_file << site_idx << '\n';
        }
        // Increment the site index per molecule so each molecule has a unique
        // index.
        site_idx++;
    }

    flat_placement_file.close();
    return true;
}

