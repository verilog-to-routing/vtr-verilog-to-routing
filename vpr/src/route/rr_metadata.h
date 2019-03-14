#ifndef RR_METADATA_H_
#define RR_METADATA_H_

#include "physical_types.h"

namespace vpr {

const t_metadata_value* rr_node_metadata(int src_node, std::string key);
void add_rr_node_metadata(int src_node, std::string key, std::string value);

const t_metadata_value* rr_edge_metadata(int src_node, int sink_node, short switch_id, std::string key);
void add_rr_edge_metadata(int src_node, int sink_node, short switch_id, std::string key, std::string value);

} // namespace vpr

#endif // RR_METADATA_H_
