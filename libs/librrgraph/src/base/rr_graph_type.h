#pragma once

#include <vector>
#include "physical_types.h"

struct t_chan_width {
    int max = 0;
    int x_max = 0;
    int y_max = 0;
    int x_min = 0;
    int y_min = 0;
    std::vector<int> x_list;
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
