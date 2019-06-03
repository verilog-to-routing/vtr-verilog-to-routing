#include <queue>
#include "place_delay_model.h"
#include "globals.h"
#include "router_lookahead_map.h"
#include "rr_graph2.h"

#include "timing_place_lookup.h"

#include "vtr_log.h"
#include "vtr_math.h"

/*
 * DeltaDelayModel
 */

float DeltaDelayModel::delay(int from_x, int from_y, int /*from_pin*/, int to_x, int to_y, int /*to_pin*/) const {
    int delta_x = std::abs(from_x - to_x);
    int delta_y = std::abs(from_y - to_y);

    return delays_[delta_x][delta_y];
}

void DeltaDelayModel::dump_echo(std::string filepath) const {
    FILE* f = vtr::fopen(filepath.c_str(), "w");
    fprintf(f, "         ");
    for (size_t dx = 0; dx < delays_.dim_size(0); ++dx) {
        fprintf(f, " %9zu", dx);
    }
    fprintf(f, "\n");
    for (size_t dy = 0; dy < delays_.dim_size(1); ++dy) {
        fprintf(f, "%9zu", dy);
        for (size_t dx = 0; dx < delays_.dim_size(0); ++dx) {
            fprintf(f, " %9.2e", delays_[dx][dy]);
        }
        fprintf(f, "\n");
    }
    vtr::fclose(f);
}

/*
 * OverrideDelayModel
 */
float OverrideDelayModel::delay(int from_x, int from_y, int from_pin, int to_x, int to_y, int to_pin) const {
    //First check to if there is an override delay value
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    t_type_ptr from_type_ptr = grid[from_x][from_y].type;
    t_type_ptr to_type_ptr = grid[to_x][to_y].type;

    t_override override_key;
    override_key.from_type = from_type_ptr->index;
    override_key.from_class = from_type_ptr->pin_class[from_pin];
    override_key.to_type = to_type_ptr->index;
    override_key.to_class = to_type_ptr->pin_class[to_pin];

    //Delay overrides may be different for +/- delta so do not use
    //an absolute delta for the look-up
    override_key.delta_x = to_x - from_x;
    override_key.delta_y = to_y - from_y;

    float delay_val = std::numeric_limits<float>::quiet_NaN();
    auto override_iter = delay_overrides_.find(override_key);
    if (override_iter != delay_overrides_.end()) {
        //Found an override
        delay_val = override_iter->second;
    } else {
        //Fall back to the base delay model if no override was found
        delay_val = base_delay_model_->delay(from_x, from_y, from_pin, to_x, to_y, to_pin);
    }

    return delay_val;
}

void OverrideDelayModel::set_delay_override(int from_type, int from_class, int to_type, int to_class, int delta_x, int delta_y, float delay_val) {
    t_override override_key;
    override_key.from_type = from_type;
    override_key.from_class = from_class;
    override_key.to_type = to_type;
    override_key.to_class = to_class;
    override_key.delta_x = delta_x;
    override_key.delta_y = delta_y;

    auto res = delay_overrides_.insert(std::make_pair(override_key, delay_val));
    if (!res.second) {                 //Key already exists
        res.first->second = delay_val; //Overwrite existing delay
    }
}

void OverrideDelayModel::dump_echo(std::string filepath) const {
    base_delay_model_->dump_echo(filepath);

    FILE* f = vtr::fopen(filepath.c_str(), "a");

    fprintf(f, "\n");
    fprintf(f, "# Delay Overrides\n");
    auto& device_ctx = g_vpr_ctx.device();
    for (auto kv : delay_overrides_) {
        auto override_key = kv.first;
        float delay_val = kv.second;
        fprintf(f, "from_type: %s to_type: %s from_pin_class: %d to_pin_class: %d delta_x: %d delta_y: %d -> delay: %g\n",
                device_ctx.block_types[override_key.from_type].name,
                device_ctx.block_types[override_key.to_type].name,
                override_key.from_class,
                override_key.to_class,
                override_key.delta_x,
                override_key.delta_y,
                delay_val);
    }

    vtr::fclose(f);
}
