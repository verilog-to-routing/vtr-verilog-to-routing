#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

#include "globals.h"

#include "vtr_assert.h"
#include "vtr_logic.h"
#include "vtr_version.h"

#include "atom_netlist_utils.h"
#include "netlist_writer.h"

#include "fasm.h"


FasmWriterVisitor::FasmWriterVisitor(std::ostream& f) : os_(f) {}

void FasmWriterVisitor::visit_top_impl(const char* top_level_name) {
}

void FasmWriterVisitor::visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) {
}

void FasmWriterVisitor::visit_all_impl(const t_pb_route *top_pb_route, const t_pb* pb,
        const t_pb_graph_node* pb_graph_node) {
}

void FasmWriterVisitor::visit_open_impl(const t_pb* atom) {
}

void FasmWriterVisitor::visit_atom_impl(const t_pb* atom) {
}

void FasmWriterVisitor::finish_impl() {
}
