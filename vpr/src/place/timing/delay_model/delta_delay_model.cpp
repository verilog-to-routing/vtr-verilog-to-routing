
#include "delta_delay_model.h"

#include "compute_delta_delays_utils.h"

void DeltaDelayModel::compute(RouterDelayProfiler& route_profiler,
                              const t_placer_opts& placer_opts,
                              const t_router_opts& router_opts,
                              int longest_length) {
    delays_ = compute_delta_delay_model(route_profiler,
                                        placer_opts,
                                        router_opts,
                                        /*measure_directconnect=*/true,
                                        longest_length,
                                        is_flat_);
}

float DeltaDelayModel::delay(const t_physical_tile_loc& from_loc, int /*from_pin*/,
                             const t_physical_tile_loc& to_loc, int /*to_pin*/) const {
    int delta_x = std::abs(from_loc.x - to_loc.x);
    int delta_y = std::abs(from_loc.y - to_loc.y);

    return delays_[from_loc.layer_num][to_loc.layer_num][delta_x][delta_y];
}

void DeltaDelayModel::dump_echo(std::string filepath) const {
    FILE* f = vtr::fopen(filepath.c_str(), "w");
    fprintf(f, "         ");
    for (size_t from_layer_num = 0; from_layer_num < delays_.dim_size(0); ++from_layer_num) {
        for (size_t to_layer_num = 0; to_layer_num < delays_.dim_size(1); ++to_layer_num) {
            fprintf(f, " %9zu", from_layer_num);
            fprintf(f, "\n");
            for (size_t dx = 0; dx < delays_.dim_size(2); ++dx) {
                fprintf(f, " %9zu", dx);
            }
            fprintf(f, "\n");
            for (size_t dy = 0; dy < delays_.dim_size(3); ++dy) {
                fprintf(f, "%9zu", dy);
                for (size_t dx = 0; dx < delays_.dim_size(2); ++dx) {
                    fprintf(f, " %9.2e", delays_[from_layer_num][to_layer_num][dx][dy]);
                }
                fprintf(f, "\n");
            }
        }
    }
    vtr::fclose(f);
}

