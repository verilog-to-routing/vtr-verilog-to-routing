#include "pack_report.h"

#include "vtr_ostream_guard.h"

#include "vpr_utils.h"
#include "histogram.h"

#include <algorithm>
#include <iostream>
#include <iomanip>

void report_packing_pin_usage(std::ostream& os, const VprContext& ctx) {
    os << "#Packing pin usage report\n";

    auto& cluster_ctx = ctx.clustering();
    auto& device_ctx = ctx.device();

    std::map<t_logical_block_type_ptr, size_t> total_input_pins;
    std::map<t_logical_block_type_ptr, size_t> total_output_pins;
    for (auto const& type : device_ctx.logical_block_types) {
        if (is_empty_type(&type)) continue;

        t_pb_type* pb_type = type.pb_type;

        total_input_pins[&type] = pb_type->num_input_pins + pb_type->num_clock_pins;
        total_output_pins[&type] = pb_type->num_output_pins;
    }

    std::map<t_logical_block_type_ptr, std::vector<float>> inputs_used;
    std::map<t_logical_block_type_ptr, std::vector<float>> outputs_used;

    for (auto blk : cluster_ctx.clb_nlist.blocks()) {
        t_logical_block_type_ptr type = cluster_ctx.clb_nlist.block_type(blk);

        inputs_used[type].push_back(static_cast<float>(cluster_ctx.clb_nlist.block_input_pins(blk).size() + cluster_ctx.clb_nlist.block_clock_pins(blk).size()));
        outputs_used[type].push_back(static_cast<float>(cluster_ctx.clb_nlist.block_output_pins(blk).size()));
    }

    vtr::OsFormatGuard os_guard(os);

    os << std::fixed << std::setprecision(2);

    for (auto const& logical_type : device_ctx.logical_block_types) {
        auto type = &logical_type;
        if (is_empty_type(type)) continue;
        auto inputs_used_it = inputs_used.find(type);
        if (inputs_used_it == inputs_used.end()) continue;

        const std::vector<float>& type_inputs_used = inputs_used_it->second;
        const std::vector<float>& type_outputs_used = outputs_used[type];
        const size_t num_input_pins = total_input_pins[type];
        const size_t num_output_pins = total_output_pins[type];

        float max_inputs = static_cast<float>(std::ranges::max(type_inputs_used));
        float min_inputs = static_cast<float>(std::ranges::min(type_inputs_used));
        float avg_inputs = static_cast<float>(std::accumulate(type_inputs_used.begin(), type_inputs_used.end(), 0.f)) / static_cast<float>(type_inputs_used.size());

        float max_outputs = std::ranges::max(type_outputs_used);
        float min_outputs = std::ranges::min(type_outputs_used);
        float avg_outputs = std::accumulate(type_outputs_used.begin(), type_outputs_used.end(), 0.f) / static_cast<float>(type_outputs_used.size());

        os << "Type: " << type->name << "\n";

        os << "\tInput Pin Usage:\n";
        os << "\t\tMax: " << max_inputs << " (" << max_inputs / static_cast<float>(num_input_pins) << ")"
           << "\n";
        os << "\t\tAvg: " << avg_inputs << " (" << avg_inputs / static_cast<float>(num_input_pins) << ")"
           << "\n";
        os << "\t\tMin: " << min_inputs << " (" << min_inputs / static_cast<float>(num_input_pins) << ")"
           << "\n";

        if (num_input_pins != 0) {
            os << "\t\tHistogram:\n";
            auto input_histogram = build_histogram(type_inputs_used, 10, 0, static_cast<float>(num_input_pins));
            for (const std::string& line : format_histogram(input_histogram)) {
                os << "\t\t" << line << "\n";
            }
        }

        os << "\tOutput Pin Usage:\n";
        os << "\t\tMax: " << max_outputs << " (" << max_outputs / float(num_output_pins) << ")"
           << "\n";
        os << "\t\tAvg: " << avg_outputs << " (" << avg_outputs / float(num_output_pins) << ")"
           << "\n";
        os << "\t\tMin: " << min_outputs << " (" << min_outputs / float(num_output_pins) << ")"
           << "\n";

        if (num_output_pins != 0) {
            os << "\t\tHistogram:\n";

            auto output_histogram = build_histogram(type_outputs_used, 10, 0, static_cast<float>(num_output_pins));
            for (auto line : format_histogram(output_histogram)) {
                os << "\t\t" << line << "\n";
            }
        }
        os << "\n";
    }
}
