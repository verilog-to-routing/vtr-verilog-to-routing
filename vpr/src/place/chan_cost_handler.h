#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    December 2025
 * @brief   Declaration of the channel cost handler class.
 */

#include "vpr_types.h"
#include "vtr_prefix_sum.h"

// Forward declarations.
class DeviceGrid;
class RRGraphView;

/**
 * @brief Manager class for computing the cost factors for channels in different
 *        dimensions.
 */
class ChanCostHandler {
  public:
    ChanCostHandler() = delete;

    /**
     * @brief Constructor for the ChanCostHanlder class.
     *
     * This will pre-compute prefix sum data structures which will make getting
     * the x, y, and z chan cost factors more efficient.
     *
     * @param rr_chanx_width
     *      The horizontal channel width distribution across the device grid.
     * @param rr_chany_width
     *      The vertical channel width distribution across the device grid.
     * @param rr_graph
     *      The Routing Resource Graph of the device.
     * @param grid
     *      The device grid.
     */
    ChanCostHandler(const std::vector<int>& rr_chanx_width,
                    const std::vector<int>& rr_chany_width,
                    const RRGraphView& rr_graph,
                    const DeviceGrid& grid);

    /**
     * @brief Computes the inverse of average channel width for horizontal
     *        channels within a bounding box.
     *
     * @tparam BBT This can be either t_bb or t_2D_bb.
     * @param bb The bounding box for which the inverse of average channel width
     *           within the bounding box is computed.
     * @return The inverse of average channel width for horizontal channels.
     */
    template<typename BBT>
    inline double get_chanx_cost_fac(const BBT& bb) const {
        int total_chanx_width = acc_chanx_width_.get_sum(bb.ymin, bb.ymax);
        double inverse_average_chanx_width = (bb.ymax - bb.ymin + 1.0) / total_chanx_width;
        return inverse_average_chanx_width;
    }

    /**
     * @brief Computes the inverse of average channel width for vertical
     *        channels within a bounding box.
     *
     * @tparam BBT This can be either t_bb or t_2D_bb.
     * @param bb The bounding box for which the inverse of average channel width
     *           within the bounding box is computed.
     * @return The inverse of average channel width for vertical channels.
     */
    template<typename BBT>
    inline double get_chany_cost_fac(const BBT& bb) const {
        int total_chany_width = acc_chany_width_.get_sum(bb.xmin, bb.xmax);
        double inverse_average_chany_width = (bb.xmax - bb.xmin + 1.0) / total_chany_width;
        return inverse_average_chany_width;
    }

    /**
     * @brief Calculate the chanz cost factor based on the inverse of the
     *        average number of inter-die connections in the given bounding box.
     *
     * This cost factor increases the placement cost for blocks that require
     * inter-layer connections in areas with, on average, fewer inter-die
     * connections. If inter-die connections are evenly distributed across
     * tiles, the cost factor will be the same for all bounding boxes, but it
     * will still weight z-directed vs. x- and y-directed connections appropriately.
     *
     * @param bb Bounding box of the net which chanz cost factor is to be calculated
     * @return ChanZ cost factor
     */
    inline double get_chanz_cost_fac(const t_bb& bb) const {
        int num_inter_dir_conn = acc_tile_num_inter_die_conn_.get_sum(bb.xmin,
                                                                      bb.ymin,
                                                                      bb.xmax,
                                                                      bb.ymax);

        if (num_inter_dir_conn == 0)
            return 1.0;

        int bb_num_tiles = (bb.xmax - bb.xmin + 1) * (bb.ymax - bb.ymin + 1);
        return static_cast<double>(bb_num_tiles) / num_inter_dir_conn;
    }

  private:
    /**
     * @brief Matrices below are used to precompute the inverse of the average
     * number of tracks per channel between [subhigh] and [sublow].  Access
     * them as chan?_place_cost_fac(subhigh, sublow).  They are used to
     * speed up the computation of the cost function that takes the length
     * of the net bounding box in each dimension, divided by the average
     * number of tracks in that direction; for other cost functions they
     * will never be used.
     */
    vtr::PrefixSum1D<int> acc_chanx_width_; // [0..grid_width-1]
    vtr::PrefixSum1D<int> acc_chany_width_; // [0..grid_height-1]

    /**
     * @brief The matrix below is used to calculate a chanz_place_cost_fac based on the average channel width in 
     * the cross-die-layer direction over a 2D (x,y) region. We don't assume the inter-die connectivity is the same at all (x,y) locations, so we
     * can't compute the full chanz_place_cost_fac for all possible (xlow,ylow)(xhigh,yhigh) without a 4D array, which would
     * be too big: O(n^2) in circuit size. Instead we compute a prefix sum that stores the number of inter-die connections per layer from
     * (x=0,y=0) to (x,y). Given this, we can compute the average number of inter-die connections over a (xlow,ylow) to (xhigh,yhigh) 
     * region in O(1) (by adding and subtracting 4 entries)
     */
    vtr::PrefixSum2D<int> acc_tile_num_inter_die_conn_; // [0..grid_width-1][0..grid_height-1]
};
