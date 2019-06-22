#include "channel_stats.h"
#include "route_util.h"
#include "histogram.h"
#include "globals.h"

void print_channel_stats() {
    std::vector<HistogramBucket> histogram;

    auto& device_ctx = g_vpr_ctx.device();

    //Bins by 10%, with final > 1 bin
    histogram.emplace_back(0., 0.1);
    histogram.emplace_back(0.1, 0.2);
    histogram.emplace_back(0.2, 0.3);
    histogram.emplace_back(0.3, 0.4);
    histogram.emplace_back(0.4, 0.5);
    histogram.emplace_back(0.5, 0.6);
    histogram.emplace_back(0.7, 0.8);
    histogram.emplace_back(0.8, 0.9);
    histogram.emplace_back(0.9, 1.0);
    histogram.emplace_back(1.0, std::numeric_limits<float>::infinity());

    auto chanx_usage = calculate_routing_usage(CHANX);
    auto chany_usage = calculate_routing_usage(CHANY);

    auto chanx_avail = calculate_routing_avail(CHANX);
    auto chany_avail = calculate_routing_avail(CHANY);

    auto comp = [](const HistogramBucket& bucket, float value) {
        return bucket.max_value < value;
    };

    float max_util = 0.;
    size_t peak_x = 0.;
    size_t peak_y = 0.;
    for (size_t x = 0; x < device_ctx.grid.width() - 1; ++x) {
        for (size_t y = 0; y < device_ctx.grid.height() - 1; ++y) {
            float chanx_util = routing_util(chanx_usage[x][y], chanx_avail[x][y]);
            float chany_util = routing_util(chanx_usage[x][y], chanx_avail[x][y]);

            for (float util : {chanx_util, chany_util}) {
                //Record peak utilization
                if (util > max_util) {
                    max_util = util;
                    peak_x = x;
                    peak_y = y;
                }

                //Find histogram bin
                auto iter = std::lower_bound(histogram.begin(), histogram.end(), util, comp);
                VTR_ASSERT(iter != histogram.end());

                iter->count++; //Add to bin
            }
        }
    }

    //Makes more sense from high to low, so reverse
    std::reverse(histogram.begin(), histogram.end());

    VTR_LOG("\n");
    VTR_LOG("Routing channel utilization histogram:\n");
    print_histogram(histogram);
    VTR_LOG("Maximum routing channel utilization: %9.2g at (%zu,%zu)\n", max_util, peak_x, peak_y);
}
