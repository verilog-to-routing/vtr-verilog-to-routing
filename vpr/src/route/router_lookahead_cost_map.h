#ifndef ROUTER_LOOKAHEAD_COST_MAP_H_
#define ROUTER_LOOKAHEAD_COST_MAP_H_

#include <vector>
#include "router_lookahead_map_utils.h"
#include "vtr_geometry.h"

/**
 * @brief Dense cost maps per source segment and destination nodes
 *
 * This class is intended to collect all methods and members that act on the lookahead cost map.
 * All methods that modify or access the cost maps should belong to this class.
 *
 * The main cost map, containing the cost entries to reach a specific destination in the device is indexed as follows:
 *
 * cost_map_[0][segment_index][delta_x][delta_y]
 *
 * Originally, the first dimension was used to differentiate the lookahead between CHANX and CHANY nodes. It turns out
 * that this distinction leads to worse results when using the extended map lookahead, therefore, this dimension of the cost map
 * has been collapsed, therefore this first dimention is virtually unused, but is kept to have the extended map lookahead as close
 * as possible to the original extended map.
 */
class CostMap {
  public:
    /**
     * @briefSets the total number of segments
     *
     * @param seg_count total segment count.
     */
    void set_counts(size_t seg_count);

    /**
     * @brief Gets the segment index relative to the input node index
     *
     * @param from_node_ind index of the node to search in the segment map
     * @return The index of the segment corresponding to the input node id
     */
    int node_to_segment(int from_node_ind) const;

    /**
     * @brief Queries the cost map to get a lookahead delay cost given the segment index and the x/y deltas to the destination node
     *
     * @param from_seg_index segment index to query the cost map corresponding to the current segment
     * @param delta_x x delta value that is the distance between the current node to the target node
     * @param delta_y y delta value that is the distance between the current node to the target node
     * @return The cost entry corresponding to the current segment and the delta x,y.
     */
    util::Cost_Entry find_cost(int from_seg_index, int delta_x, int delta_y) const;

    /**
     * @brief Sets the cost map given the delay and congestion routing costs
     *
     * @param delay_costs delay costs calculated with the dijkstra expansions
     * @param base_costs congestion costs calculated with the dijkstra expansions
     */
    void set_cost_map(const util::RoutingCosts& delay_costs, const util::RoutingCosts& base_costs);

    /**
     * @brief Gets the cost of an entry that is close to a hole in the cost map
     *
     * @param matrix cost map corresponding to a specific segment index
     * @param cx x location of the cost map entry that needs to be filled
     * @param cy y location of the cost map entry that needs to be filled
     * @param bounds bounds of the cost map corresponding to the current segment
     * @return A pair containing the nearby cost entry and the distance from the current location to the narby cost entry found.
     *
     * The coordinates identifying the cost map location to fill (cx, cy) need to fall within the bounding box provided as input (bounds). If the (cx, cy) point falls out of the bounds, a default cost entry is returned instead.
     */
    std::pair<util::Cost_Entry, int> get_nearby_cost_entry(const vtr::NdMatrix<util::Cost_Entry, 2>& matrix, int cx, int cy, const vtr::Rect<int>& bounds);

    /**
     * @brief Reads the lookahead file
     *
     * @param file input lookahead file
     */
    void read(const std::string& file);

    /**
     * @brief Writes the lookahead file
     *
     * @param file output lookahead file
     */
    void write(const std::string& file) const;

    void print(int iseg) const;
    std::vector<std::pair<int, int>> list_empty() const;

  private:
    vtr::Matrix<vtr::Matrix<util::Cost_Entry>> cost_map_; ///<Cost map containing all the costs computed during the lookahead generation.
                                                          ///<It is indexed as follows: cost_map_[0][segment_index][delta_x][delta_y]
                                                          ///<The first index is always 0 and it is kept to allow future specializations of
                                                          ///<the cost map based on other possible indices.

    vtr::Matrix<std::pair<int, int>> offset_; ///<Offset to specify the bounds of a specific segment map.
                                              ///<It is used to adjust delta values to the segment bounding box.
                                              ///<The offset map is addressed as follows: offset_[0][segment_index]
                                              ///<The values of the matrix are pairs of corresponding to X and Y offsets.

    vtr::Matrix<float> penalty_; ///<Penalty value corresponding to each segment type and used to penalize
                                 ///<delta locations that fall outside of a segment's bounding box.
                                 ///<The penalty map is addressed as follows penalty_[0][segment_index]

    size_t seg_count_ = 0; ///<Total segment count in the architecture

    /**
     * @brief Get penalty delay for a segment type
     * @param matrix cost map for a specific segment type
     * @return Penalty delay value
     */
    float get_penalty(vtr::NdMatrix<util::Cost_Entry, 2>& matrix) const;

    /**
     * @brief Fills the holes in the cost map matrix
     * @param matrix cost map for a specific segment type
     * @param seg_index index of the current segment
     * @param bounding_box_width width of the segment type cost map bounding box
     * @param bounding_box_height height of the segment type cost map bounding box
     * @param delay_penalty penalty corresponding to the current segment cost map
     */
    void fill_holes(vtr::NdMatrix<util::Cost_Entry, 2>& matrix, int seg_index, int bounding_box_width, int bounding_box_height, float delay_penalty);
};

#endif
