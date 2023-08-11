#ifndef DEVICE_GRID_ANNOTATION_H
#define DEVICE_GRID_ANNOTATION_H

/********************************************************************
 * Include header files required by the data structure definition
 *******************************************************************/
#include <array>
#include "vtr_geometry.h"
#include "vtr_ndmatrix.h"
#include "physical_types.h"
#include "device_grid.h"

/** @brief This is the data structure to provide additional data for the device grid:
 * - Border of the device grid (check where the empty types cover the perimeters)
 */
class DeviceGridAnnotation {
  public: /* Constructor */
    /** @brief Build the annotation. When shrink is enable, a boundary line is drawn when all the empty types are found between the line and the perimeter. For example
   *      boundry line                     perimeter
   *            v                           v
   * clb        |    empty   empty   empty  | 
   * Otherwise, the boundary line is just the perimeter.
   */
    DeviceGridAnnotation(const DeviceGrid& grid, const bool& shrink);

  private: /* Private mutators */
    void alloc(const DeviceGrid& grid);
    void init(const DeviceGrid& grid, const bool& shrink);

  public: /* Public accessors */
    /** @brief Check if at a given coordinate, a X-direction routing channel should exist or not */
    bool is_chanx_exist(const vtr::Point<size_t>& coord) const;
    bool is_chanx_start(const vtr::Point<size_t>& coord) const;
    bool is_chanx_end(const vtr::Point<size_t>& coord) const;
    /** @brief Check if at a given coordinate, a Y-direction routing channel should exist or not */
    bool is_chany_exist(const vtr::Point<size_t>& coord) const;
    bool is_chany_start(const vtr::Point<size_t>& coord) const;
    bool is_chany_end(const vtr::Point<size_t>& coord) const;

  private: /* Private validators */
    /** @brief Check all the adjacent grid until perimeter. To be an empty zone with a given side, there should be all empty types from the given side to the perimeter */
    bool is_empty_zone(const DeviceGrid& grid, const vtr::Point<size_t>& coord, const e_side& side, const bool& shrink) const;
    vtr::Point<size_t> get_neighbor_coord(const vtr::Point<size_t>& coord, const e_side& side) const;

  private: /* Internal data */
    vtr::NdMatrix<bool, 2> chanx_existence_;
    vtr::NdMatrix<bool, 2> chany_existence_;
};

#endif