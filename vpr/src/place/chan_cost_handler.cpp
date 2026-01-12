/**
 * @file
 * @author  Alex Singer
 * @date    December 2025
 * @brief   The definition of the channel cost handler class.
 */

#include "chan_cost_handler.h"
#include <cmath>
#include "rr_graph_view.h"

ChanCostHandler::ChanCostHandler(const std::vector<int>& rr_chanx_width,
                                 const std::vector<int>& rr_chany_width,
                                 const RRGraphView& rr_graph,
                                 const DeviceGrid& grid) {

    // These arrays contain accumulative channel width between channel zero and
    // the channel specified by the given index. The accumulated channel width
    // is inclusive, meaning that it includes both channel zero and channel `idx`.
    // To compute the total channel width between channels 'low' and 'high', use the
    // following formula:
    //      acc_chan?_width_[high] - acc_chan?_width_[low - 1]
    // This returns the total number of tracks between channels 'low' and 'high',
    // including tracks in these channels.
    acc_chanx_width_ = vtr::PrefixSum1D<int>(grid.height(), [&](size_t y) noexcept {
        int chan_x_width = rr_chanx_width[y];

        // If the number of tracks in a channel is zero, two consecutive elements take the same
        // value. This can lead to a division by zero in get_chanxy_cost_fac_(). To avoid this
        // potential issue, we assume that the channel width is at least 1.
        if (chan_x_width == 0) {
            return 1;
        }

        return chan_x_width;
    });

    acc_chany_width_ = vtr::PrefixSum1D<int>(grid.width(), [&](size_t x) noexcept {
        int chan_y_width = rr_chany_width[x];

        // to avoid a division by zero
        if (chan_y_width == 0) {
            return 1;
        }

        return chan_y_width;
    });

    // If this is a multi-layer (3D) architecture, compute Z-channel cost term.
    if (grid.get_num_layers() > 1) {
        vtr::NdMatrix<float, 2> tile_num_inter_die_conn({grid.width(), grid.height()}, 0.);

        /*
         * Step 1: iterate over the rr-graph, recording how many edges go between layers at each (x,y) location
         * in the device. We count all these edges, regardless of which layers they connect. Then we divide by
         * the number of layers - 1 to get the average cross-layer edge count per (x,y) location -- this mirrors
         * what we do for the horizontal and vertical channels where we assume the channel width doesn't change
         * along the length of the channel. It lets us be more memory-efficient for 3D devices, and could be revisited
         * if someday we have architectures with widely varying connectivity between different layers in a stack.
         */

        /* To calculate the accumulative number of inter-die connections we first need to get the number of
         * inter-die connection per location. To be able to work for the cases that RR Graph is read instead
         * of being made from the architecture file, we calculate this number by iterating over the RR graph. Once
         * tile_num_inter_die_conn is populated, we can start populating acc_tile_num_inter_die_conn_.
         */

        for (const RRNodeId node : rr_graph.nodes()) {
            if (rr_graph.node_type(node) == e_rr_type::CHANZ) {
                int x = rr_graph.node_xlow(node);
                int y = rr_graph.node_ylow(node);
                VTR_ASSERT_SAFE(x == rr_graph.node_xhigh(node) && y == rr_graph.node_yhigh(node));
                tile_num_inter_die_conn[x][y]++;
            }
        }

        int num_layers = grid.get_num_layers();
        for (size_t x = 0; x < grid.width(); x++) {
            for (size_t y = 0; y < grid.height(); y++) {
                tile_num_inter_die_conn[x][y] /= (num_layers - 1);
            }
        }

        // Step 2: Calculate prefix sum of the inter-die connectivity up to and including the channel at (x, y).
        acc_tile_num_inter_die_conn_ = vtr::PrefixSum2D<int>(grid.width(),
                                                             grid.height(),
                                                             [&](size_t x, size_t y) {
                                                                 return static_cast<int>(std::round(tile_num_inter_die_conn[x][y]));
                                                             });
    }
}
