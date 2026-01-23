#pragma once

#include <vector>
#include "physical_types.h"


/// @brief Channel width data
/// @details This data structure should only be used for RR graph generation.
///          After RR graph is generated, the number of tracks in each channel
///          is extracted from the RR graph and is stored in `DeviceContext`.
struct t_chan_width {
    /// Maximum channel width between x_max and y_max.
    int max = 0;
    /// Maximum channel width of horizontal channels.
    int x_max = 0;
    /// Maximum channel width of vertical channels.
    int y_max = 0;
    /// Minimum channel width of horizontal channels.
    int x_min = 0;
    /// Minimum channel width of vertical channels.
    int y_min = 0;

    /// Stores the channel width of all horizontal channels and thus goes from [0..grid.height()-1]
    /// (imagine a 2D Cartesian grid with horizontal lines starting at every grid point on a line parallel to the y-axis)
    std::vector<int> x_list;

    /// Stores the channel width of all vertical channels and thus goes from [0..grid.width()-1]
    /// (imagine a 2D Cartesian grid with vertical lines starting at every grid point on a line parallel to the x-axis)
    std::vector<int> y_list;

    bool operator==(const t_chan_width&) const = default;
};

/// @brief Specifies whether global routing or combined global and detailed routing is performed.
enum class e_route_type {
    GLOBAL,
    DETAILED
};

/**
 * @enum e_graph_type
 * @brief Represents the type of routing resource graph
 */
enum class e_graph_type {
    GLOBAL,           ///< One node per channel with wire capacity > 1 and full connectivity
    BIDIR,            ///< Detailed bidirectional routing graph
    UNIDIR,           ///< Detailed unidirectional routing graph (non-tileable)
    UNIDIR_TILEABLE   ///< Tileable unidirectional graph with wire groups in multiples of 2 * L (experimental)
};

/**
 * @typedef t_unified_to_parallel_seg_index
 * @brief Maps indices from the unified segment list to axis-specific segment lists.
 *
 * This map is used to translate indices from the unified segment vector
 * (`segment_inf` in the device context, which contains all segments regardless of axis)
 * to axis-specific segment vectors (`segment_inf_x` or `segment_inf_y` or `segment_inf_z`),
 * based on the segment's parallel axis.
 *
 * Each entry maps a unified segment index to a pair containing:
 *   - The index in the corresponding axis-specific segment vector
 *   - The axis of the segment (X/Y/Z)
 *
 * @see get_parallel_segs for more details.
 */
typedef std::unordered_multimap<size_t, std::pair<size_t, e_parallel_axis>> t_unified_to_parallel_seg_index;
