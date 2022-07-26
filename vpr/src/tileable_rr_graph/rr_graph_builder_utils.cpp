/************************************************************************
 *  This file contains most utilized functions for rr_graph builders 
 ***********************************************************************/
#include <vector>
#include <algorithm>

/* Headers from vtrutil library */
#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_utils.h"

#include "rr_graph_builder_utils.h"

/* begin namespace openfpga */
namespace openfpga {

/************************************************************************
 * Correct number of routing channel width to be compatible to 
 * uni-directional routing architecture
 ***********************************************************************/
size_t find_unidir_routing_channel_width(const size_t& chan_width) {
  size_t actual_chan_width = chan_width;
  /* Correct the chan_width: it should be an even number */
  if (0 != actual_chan_width % 2) {
    actual_chan_width++; /* increment it to be even */
  }
  VTR_ASSERT(0 == actual_chan_width % 2);

  return actual_chan_width;
}

/************************************************************************
 * Get the class index of a grid pin 
 ***********************************************************************/
int get_grid_pin_class_index(const t_grid_tile& cur_grid,
                             const int pin_index) {
  /* check */
  VTR_ASSERT(pin_index < cur_grid.type->num_pins);
  return cur_grid.type->pin_class[pin_index];
}

/* Deteremine the side of a io grid */
e_side determine_io_grid_pin_side(const vtr::Point<size_t>& device_size, 
                                  const vtr::Point<size_t>& grid_coordinate) {
  /* TOP side IO of FPGA */
  if (device_size.y() == grid_coordinate.y()) {
    return BOTTOM; /* Such I/O has only Bottom side pins */
  } else if (device_size.x() == grid_coordinate.x()) { /* RIGHT side IO of FPGA */
    return LEFT; /* Such I/O has only Left side pins */
  } else if (0 == grid_coordinate.y()) { /* BOTTOM side IO of FPGA */
    return TOP; /* Such I/O has only Top side pins */
  } else if (0 == grid_coordinate.x()) { /* LEFT side IO of FPGA */
    return RIGHT; /* Such I/O has only Right side pins */
  } else if ((grid_coordinate.x() < device_size.x()) && (grid_coordinate.y() < device_size.y())) {
    /* I/O grid in the center grid */
    return NUM_SIDES;
  }
  VTR_LOGF_ERROR(__FILE__, __LINE__,
                 "Invalid coordinate (%lu, %lu) for I/O Grid whose size is (%lu, %lu)!\n",
                grid_coordinate.x(), grid_coordinate.y(),
                device_size.x(), device_size.y());
  exit(1);
}

/* Deteremine the side of a pin of a grid */
std::vector<e_side> find_grid_pin_sides(const t_grid_tile& grid, 
                                        const size_t& pin_id) {
  std::vector<e_side> pin_sides;

  for (const e_side& side : {TOP, RIGHT, BOTTOM, LEFT} ) {
    if (true == grid.type->pinloc[grid.width_offset][grid.height_offset][size_t(side)][pin_id]) {
      pin_sides.push_back(side);
    }
  }

  return pin_sides;
}

/************************************************************************
 * Get a list of pin_index for a grid (either OPIN or IPIN)
 * For IO_TYPE, only one side will be used, we consider one side of pins 
 * For others, we consider all the sides  
 ***********************************************************************/
std::vector<int> get_grid_side_pins(const t_grid_tile& cur_grid, 
                                    const e_pin_type& pin_type, 
                                    const e_side& pin_side, 
                                    const int& pin_width,
                                    const int& pin_height) {
  std::vector<int> pin_list; 
  /* Make sure a clear start */
  pin_list.clear();

  for (int ipin = 0; ipin < cur_grid.type->num_pins; ++ipin) {
    int class_id = cur_grid.type->pin_class[ipin];
    if ( (1 == cur_grid.type->pinloc[pin_width][pin_height][pin_side][ipin]) 
      && (pin_type == cur_grid.type->class_inf[class_id].type) ) {
      pin_list.push_back(ipin);
    }
  }
  return pin_list;
}

/************************************************************************
 * Get the number of pins for a grid (either OPIN or IPIN)
 * For IO_TYPE, only one side will be used, we consider one side of pins 
 * For others, we consider all the sides  
 ***********************************************************************/
size_t get_grid_num_pins(const t_grid_tile& cur_grid, 
                         const e_pin_type& pin_type, 
                         const e_side& io_side) {
  size_t num_pins = 0;

  /* For IO_TYPE sides */
  for (const e_side& side : {TOP, RIGHT, BOTTOM, LEFT}) {
    /* skip unwanted sides */
    if ( (true == is_io_type(cur_grid.type))
      && (side != io_side) && (NUM_SIDES != io_side)) { 
      continue;
    }
    /* Get pin list */
    for (int width = 0; width < cur_grid.type->width; ++width) {
      for (int height = 0; height < cur_grid.type->height; ++height) {
        std::vector<int> pin_list = get_grid_side_pins(cur_grid, pin_type, side, width, height);
        num_pins += pin_list.size();
      }
    } 
  }

  return num_pins;
}

/************************************************************************
 * Get the number of pins for a grid (either OPIN or IPIN)
 * For IO_TYPE, only one side will be used, we consider one side of pins 
 * For others, we consider all the sides  
 ***********************************************************************/
size_t get_grid_num_classes(const t_grid_tile& cur_grid, 
                            const e_pin_type& pin_type) {
  size_t num_classes = 0;

  for (int iclass = 0; iclass < cur_grid.type->num_class; ++iclass) {
    /* Bypass unmatched pin_type */
    if (pin_type != cur_grid.type->class_inf[iclass].type) {
      continue;
    }
    num_classes++;
  }

  return num_classes;
}

/************************************************************************
 * Idenfity if a X-direction routing channel exist in the fabric
 * This could be entirely possible that a routig channel
 * is in the middle of a multi-width and multi-height grid
 *
 * As the chanx always locates on top of a grid with the same coord
 *
 *     +----------+
 *     |   CHANX  |
 *     |  [x][y]  |
 *     +----------+
 *
 *     +----------+
 *     |   Grid   |   height_offset = height - 1
 *     |  [x][y]  |
 *     +----------+
 *
 *     +----------+
 *     |  Grid    |   height_offset = height - 2
 *     | [x][y-1] |
 *     +----------+
 *  If the CHANX is in the middle of a multi-width and multi-height grid
 *  it should locate at a grid whose height_offset is lower than the its height defined in physical_tile
 *  When height_offset == height - 1, it means that the grid is at the top side of this multi-width and multi-height block
 ***********************************************************************/
bool is_chanx_exist(const DeviceGrid& grids,
                    const vtr::Point<size_t>& chanx_coord,
                    const bool& through_channel) {

  if ((1 > chanx_coord.x()) || (chanx_coord.x() > grids.width() - 2)) {
    return false;
  }

  if (chanx_coord.y() > grids.height() - 2) {
    return false;
  }

  if (true == through_channel) {
    return true;
  }

  return (grids[chanx_coord.x()][chanx_coord.y()].height_offset == grids[chanx_coord.x()][chanx_coord.y()].type->height - 1);
}

/************************************************************************
 * Idenfity if a Y-direction routing channel exist in the fabric
 * This could be entirely possible that a routig channel
 * is in the middle of a multi-width and multi-height grid
 *
 * As the chany always locates on right of a grid with the same coord
 *
 * +-----------+  +---------+  +--------+
 * |   Grid    |  |  Grid   |  |  CHANY |
 * | [x-1][y]  |  | [x][y]  |  | [x][y] |
 * +-----------+  +---------+  +--------+
 *  width_offset   width_offset
 *  = width - 2   = width -1
 *  If the CHANY is in the middle of a multi-width and multi-height grid
 *  it should locate at a grid whose width_offset is lower than the its width defined in physical_tile
 *  When height_offset == height - 1, it means that the grid is at the top side of this multi-width and multi-height block
 *
 *  If through channel is allowed, the chany will always exists
 *  unless it falls out of the grid array
 ***********************************************************************/
bool is_chany_exist(const DeviceGrid& grids,
                    const vtr::Point<size_t>& chany_coord,
                    const bool& through_channel) {

  if (chany_coord.x() > grids.width() - 2) {
    return false;
  }

  if ((1 > chany_coord.y()) || (chany_coord.y() > grids.height() - 2)) {
    return false;
  }

  if (true == through_channel) {
    return true;
  }

  return (grids[chany_coord.x()][chany_coord.y()].width_offset == grids[chany_coord.x()][chany_coord.y()].type->width - 1);
}

/************************************************************************
 * Identify if a X-direction routing channel is at the right side of a 
 * multi-height grid
 *
 *     +-----------------+
 *     |                 |
 *     |                 |  +-------------+
 *     |      Grid       |  |   CHANX     |
 *     |    [x-1][y]     |  |   [x][y]    |
 *     |                 |  +-------------+
 *     |                 |
 *     +-----------------+
 ***********************************************************************/
bool is_chanx_right_to_multi_height_grid(const DeviceGrid& grids,
                                         const vtr::Point<size_t>& chanx_coord,
                                         const bool& through_channel) {
  VTR_ASSERT(0 < chanx_coord.x());
  if (1 == chanx_coord.x()) {
    /* This is already the LEFT side of FPGA fabric,
     * it is the same results as chanx is right to a multi-height grid
     */
    return true;
  }
  
  if (false == through_channel) {
    /* We check the left neighbor of chanx, if it does not exist, the chanx is left to a multi-height grid */
    vtr::Point<size_t> left_chanx_coord(chanx_coord.x() - 1, chanx_coord.y());
    if (false == is_chanx_exist(grids, left_chanx_coord)) {
      return true;
    }
  }

  return false;
}

/************************************************************************
 * Identify if a X-direction routing channel is at the left side of a 
 * multi-height grid
 *
 *                            +-----------------+
 *                            |                 |
 *        +---------------+   |                 | 
 *        |    CHANX      |   |      Grid       | 
 *        |    [x][y]     |   |    [x+1][y]     | 
 *        +---------------+   |                 |
 *                            |                 |
 *                            +-----------------+
 ***********************************************************************/
bool is_chanx_left_to_multi_height_grid(const DeviceGrid& grids,
                                        const vtr::Point<size_t>& chanx_coord,
                                        const bool& through_channel) {
  VTR_ASSERT(chanx_coord.x() < grids.width() - 1);
  if (grids.width() - 2 == chanx_coord.x()) {
    /* This is already the RIGHT side of FPGA fabric,
     * it is the same results as chanx is right to a multi-height grid
     */
    return true;
  }
  

  if (false == through_channel) {
    /* We check the right neighbor of chanx, if it does not exist, the chanx is left to a multi-height grid */
    vtr::Point<size_t> right_chanx_coord(chanx_coord.x() + 1, chanx_coord.y());
    if (false == is_chanx_exist(grids, right_chanx_coord)) {
      return true;
    }
  }

  return false;
}

/************************************************************************
 * Identify if a Y-direction routing channel is at the top side of a 
 * multi-width grid
 * 
 *          +--------+
 *          | CHANY  |
 *          | [x][y] | 
 *          +--------+
 *
 *     +-----------------+
 *     |                 |
 *     |                 | 
 *     |      Grid       | 
 *     |    [x-1][y]     | 
 *     |                 | 
 *     |                 |
 *     +-----------------+
 ***********************************************************************/
bool is_chany_top_to_multi_width_grid(const DeviceGrid& grids,
                                      const vtr::Point<size_t>& chany_coord,
                                      const bool& through_channel) {
  VTR_ASSERT(0 < chany_coord.y());
  if (1 == chany_coord.y()) {
    /* This is already the BOTTOM side of FPGA fabric,
     * it is the same results as chany is at the top of a multi-width grid
     */
    return true;
  }
  
  if (false == through_channel) {
    /* We check the bottom neighbor of chany, if it does not exist, the chany is top to a multi-height grid */
    vtr::Point<size_t> bottom_chany_coord(chany_coord.x(), chany_coord.y() - 1);
    if (false == is_chany_exist(grids, bottom_chany_coord)) {
      return true;
    }
  }

  return false;
}

/************************************************************************
 * Identify if a Y-direction routing channel is at the bottom side of a 
 * multi-width grid
 * 
 *     +-----------------+
 *     |                 |
 *     |                 | 
 *     |      Grid       | 
 *     |    [x][y+1]     | 
 *     |                 | 
 *     |                 |
 *     +-----------------+
 *          +--------+
 *          | CHANY  |
 *          | [x][y] | 
 *          +--------+
 *
 ***********************************************************************/
bool is_chany_bottom_to_multi_width_grid(const DeviceGrid& grids,
                                         const vtr::Point<size_t>& chany_coord,
                                         const bool& through_channel) {
  VTR_ASSERT(chany_coord.y() < grids.height() - 1);
  if (grids.height() - 2 == chany_coord.y()) {
    /* This is already the TOP side of FPGA fabric,
     * it is the same results as chany is at the bottom of a multi-width grid
     */
    return true;
  }
  
  if (false == through_channel) {
    /* We check the top neighbor of chany, if it does not exist, the chany is left to a multi-height grid */
    vtr::Point<size_t> top_chany_coord(chany_coord.x(), chany_coord.y() + 1);
    if (false == is_chany_exist(grids, top_chany_coord)) {
      return true;
    }
  }

  return false;
}

/************************************************************************
 * Get the track_id of a routing track w.r.t its coordinator 
 * In tileable routing architecture, the track_id changes SB by SB.
 * Therefore the track_ids are stored in a vector, indexed by the relative coordinator
 * based on the starting point of the track
 * For routing tracks in INC_DIRECTION
 * (xlow, ylow) should be the starting point 
 * 
 * (xlow, ylow)                                      (xhigh, yhigh)
 *  track_id[0]  -------------------------------> track_id[xhigh - xlow + yhigh - ylow]
 *
 * For routing tracks in DEC_DIRECTION
 * (xhigh, yhigh) should be the starting point 
 *
 * (xlow, ylow)                                      (xhigh, yhigh)
 *  track_id[0]  <------------------------------- track_id[xhigh - xlow + yhigh - ylow]
 *
 *
 ***********************************************************************/
short get_rr_node_actual_track_id(const RRGraph& rr_graph,
                                  const RRNodeId& track_rr_node,
                                  const vtr::Point<size_t>& coord,
                                  const vtr::vector<RRNodeId, std::vector<size_t>>& tileable_rr_graph_node_track_ids) {
  vtr::Point<size_t> low_coord(rr_graph.node_xlow(track_rr_node), rr_graph.node_ylow(track_rr_node));
  size_t offset = (int)abs((int)coord.x() -  (int)low_coord.x() + (int)coord.y() - (int)low_coord.y());
  return tileable_rr_graph_node_track_ids[track_rr_node][offset];
}

/************************************************************************
 * Get the ptc of a routing track in the channel where it ends
 * For routing tracks in INC_DIRECTION
 * the ptc is the last of track_ids
 *
 * For routing tracks in DEC_DIRECTION
 * the ptc is the first of track_ids
 ***********************************************************************/
short get_track_rr_node_end_track_id(const RRGraph& rr_graph,
                                     const RRNodeId& track_rr_node,
                                     const vtr::vector<RRNodeId, std::vector<size_t>>& tileable_rr_graph_node_track_ids) {
  /* Make sure we have CHANX or CHANY */
  VTR_ASSERT( (CHANX == rr_graph.node_type(track_rr_node))
           || (CHANY == rr_graph.node_type(track_rr_node)) );
 
  if (Direction::INC == rr_graph.node_direction(track_rr_node)) {
    return tileable_rr_graph_node_track_ids[track_rr_node].back(); 
  }

  VTR_ASSERT(Direction::DEC == rr_graph.node_direction(track_rr_node));
  return tileable_rr_graph_node_track_ids[track_rr_node].front(); 
}

/************************************************************************
 * Find the number of nodes in the same class
 * in a routing resource graph
 ************************************************************************/
short find_rr_graph_num_nodes(const RRGraph& rr_graph,
                              const std::vector<t_rr_type>& node_types) {
  short counter = 0;

  for (const RRNodeId& node : rr_graph.nodes()) {
    /* Bypass the nodes not in the class */
    if (node_types.end() == std::find(node_types.begin(), node_types.end(), rr_graph.node_type(node))) {
      continue;
    }
    counter++; 
  } 

  return counter;
}

/************************************************************************
 * Find the maximum fan-in for a given class of nodes
 * in a routing resource graph
 ************************************************************************/
short find_rr_graph_max_fan_in(const RRGraph& rr_graph,
                               const std::vector<t_rr_type>& node_types) {
  short max_fan_in = 0;

  for (const RRNodeId& node : rr_graph.nodes()) {
    /* Bypass the nodes not in the class */
    if (node_types.end() == std::find(node_types.begin(), node_types.end(), rr_graph.node_type(node))) {
      continue;
    }
    max_fan_in = std::max(rr_graph.node_fan_in(node), max_fan_in); 
  } 

  return max_fan_in;
}

/************************************************************************
 * Find the minimum fan-in for a given class of nodes
 * in a routing resource graph
 ************************************************************************/
short find_rr_graph_min_fan_in(const RRGraph& rr_graph,
                               const std::vector<t_rr_type>& node_types) {
  short min_fan_in = 0;

  for (const RRNodeId& node : rr_graph.nodes()) {
    /* Bypass the nodes not in the class */
    if (node_types.end() == std::find(node_types.begin(), node_types.end(), rr_graph.node_type(node))) {
      continue;
    }
    min_fan_in = std::min(rr_graph.node_fan_in(node), min_fan_in); 
  } 


  return min_fan_in;
}

/************************************************************************
 * Find the average fan-in for a given class of nodes
 * in a routing resource graph
 ************************************************************************/
short find_rr_graph_average_fan_in(const RRGraph& rr_graph,
                                   const std::vector<t_rr_type>& node_types) {
  /* Get the maximum SB mux size */
  size_t sum = 0;
  size_t counter = 0;

  for (const RRNodeId& node : rr_graph.nodes()) {
    /* Bypass the nodes not in the class */
    if (node_types.end() == std::find(node_types.begin(), node_types.end(), rr_graph.node_type(node))) {
      continue;
    }

    sum += rr_graph.node_fan_in(node); 
    counter++;
  } 

  return sum / counter;
}

/************************************************************************
 * Print statistics of multiplexers in a routing resource graph  
 ************************************************************************/
void print_rr_graph_mux_stats(const RRGraph& rr_graph) {

  /* Print MUX size distribution */
  std::vector<t_rr_type> sb_node_types;
  sb_node_types.push_back(CHANX);
  sb_node_types.push_back(CHANY);

  /* Print statistics */
  VTR_LOG("------------------------------------------------\n");
  VTR_LOG("Total No. of Switch Block multiplexer size: %d\n",
          find_rr_graph_num_nodes(rr_graph, sb_node_types));
  VTR_LOG("Maximum Switch Block multiplexer size: %d\n",
          find_rr_graph_max_fan_in(rr_graph, sb_node_types));
  VTR_LOG("Minimum Switch Block multiplexer size: %d\n",
          find_rr_graph_min_fan_in(rr_graph, sb_node_types));
  VTR_LOG("Average Switch Block multiplexer size: %lu\n",
          find_rr_graph_average_fan_in(rr_graph, sb_node_types));
  VTR_LOG("------------------------------------------------\n");

  /* Get the maximum CB mux size */
  std::vector<t_rr_type> cb_node_types(1, IPIN);

  VTR_LOG("------------------------------------------------\n");
  VTR_LOG("Total No. of Connection Block Multiplexer size: %d\n",
          find_rr_graph_num_nodes(rr_graph, cb_node_types));
  VTR_LOG("Maximum Connection Block Multiplexer size: %d\n",
          find_rr_graph_max_fan_in(rr_graph, cb_node_types));
  VTR_LOG("Minimum Connection Block Multiplexer size: %d\n",
          find_rr_graph_min_fan_in(rr_graph, cb_node_types));
  VTR_LOG("Average Connection Block Multiplexer size: %lu\n",
          find_rr_graph_average_fan_in(rr_graph, cb_node_types));
  VTR_LOG("------------------------------------------------\n");
}

} /* end namespace openfpga */
