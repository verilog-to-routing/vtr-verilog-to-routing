#pragma once

#include <vector>
#include <string_view>

#include "physical_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* special type indexes, necessary for initialization, everything afterwards
 * should use the pointers to these type indices*/
#define EMPTY_TYPE_INDEX 0

// function declarations

/// Loads the given architecture file.
/// @param warn_arch_rr_lookahead When true, emit warnings about architecture
///        file quality (e.g. unexpected file format). Intended for architecture
///        developers; end users can safely leave this off.
void xml_read_arch(std::string_view arch_file,
                   const bool timing_enabled,
                   t_arch* arch,
                   std::vector<t_physical_tile_type>& physical_tile_types,
                   std::vector<t_logical_block_type>& logical_block_types,
                   bool warn_arch_rr_lookahead);

#ifdef __cplusplus
}
#endif
