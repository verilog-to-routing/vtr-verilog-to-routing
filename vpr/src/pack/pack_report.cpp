#include "pack_report.h"

#include "vtr_ostream_guard.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "histogram.h"

#include <iostream>
#include <iomanip>

void report_packing_pin_usage(std::ostream& os, const VprContext& ctx) {
    os << "#Packing pin usage report\n";

    auto& cluster_ctx = ctx.clustering();
    auto& device_ctx = ctx.device();

    std::map<t_type_ptr, size_t> total_input_pins;
    std::map<t_type_ptr, size_t> total_output_pins;
    for (int itype = 0; itype < device_ctx.num_block_types; ++itype) {
        t_type_ptr type = &device_ctx.block_types[itype];
        if (is_empty_type(type)) continue;

        t_pb_type* pb_type = type->pb_type;

        total_input_pins[type] = pb_type->num_input_pins + pb_type->num_clock_pins;
        total_output_pins[type] = pb_type->num_output_pins;
    }

    std::map<t_type_ptr, std::vector<float>> inputs_used;
    std::map<t_type_ptr, std::vector<float>> outputs_used;

    for (auto blk : cluster_ctx.clb_nlist.blocks()) {
        t_type_ptr type = cluster_ctx.clb_nlist.block_type(blk);

        inputs_used[type].push_back(cluster_ctx.clb_nlist.block_input_pins(blk).size() + cluster_ctx.clb_nlist.block_clock_pins(blk).size());
        outputs_used[type].push_back(cluster_ctx.clb_nlist.block_output_pins(blk).size());
    }

    vtr::OsFormatGuard os_guard(os);

    os << std::fixed << std::setprecision(2);

    for (int itype = 0; itype < device_ctx.num_block_types; ++itype) {
        t_type_ptr type = &device_ctx.block_types[itype];
        if (is_empty_type(type)) continue;
        if (!inputs_used.count(type)) continue;

        float max_inputs = *std::max_element(inputs_used[type].begin(), inputs_used[type].end());
        float min_inputs = *std::min_element(inputs_used[type].begin(), inputs_used[type].end());
        float avg_inputs = std::accumulate(inputs_used[type].begin(), inputs_used[type].end(), 0) / float(inputs_used[type].size());

        float max_outputs = *std::max_element(outputs_used[type].begin(), outputs_used[type].end());
        float min_outputs = *std::min_element(outputs_used[type].begin(), outputs_used[type].end());
        float avg_outputs = std::accumulate(outputs_used[type].begin(), outputs_used[type].end(), 0) / float(outputs_used[type].size());

        os << "Type: " << type->name << "\n";

        os << "\tInput Pin Usage:\n";
        os << "\t\tMax: " << max_inputs << " (" << max_inputs / float(total_input_pins[type]) << ")"
           << "\n";
        os << "\t\tAvg: " << avg_inputs << " (" << avg_inputs / float(total_input_pins[type]) << ")"
           << "\n";
        os << "\t\tMin: " << min_inputs << " (" << min_inputs / float(total_input_pins[type]) << ")"
           << "\n";

        if (total_input_pins[type] != 0) {
            os << "\t\tHistogram:\n";
            auto input_histogram = build_histogram(inputs_used[type], 10, 0, total_input_pins[type]);
            for (auto line : format_histogram(input_histogram)) {
                os << "\t\t" << line << "\n";
            }
        }

        os << "\tOutput Pin Usage:\n";
        os << "\t\tMax: " << max_outputs << " (" << max_outputs / float(total_output_pins[type]) << ")"
           << "\n";
        os << "\t\tAvg: " << avg_outputs << " (" << avg_outputs / float(total_output_pins[type]) << ")"
           << "\n";
        os << "\t\tMin: " << min_outputs << " (" << min_outputs / float(total_output_pins[type]) << ")"
           << "\n";

        if (total_output_pins[type] != 0) {
            os << "\t\tHistogram:\n";

            auto output_histogram = build_histogram(outputs_used[type], 10, 0, total_output_pins[type]);
            for (auto line : format_histogram(output_histogram)) {
                os << "\t\t" << line << "\n";
            }
        }
        os << "\n";
    }
}
