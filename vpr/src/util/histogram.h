#ifndef VPR_HISTOGRAM_H
#define VPR_HISTOGRAM_H

#include <limits>
#include <vector>

struct HistogramBucket {
    HistogramBucket(float min_val, float max_val, float init_count = 0) noexcept
        : min_value(min_val)
        , max_value(max_val)
        , count(init_count) {}

    float min_value = std::numeric_limits<float>::quiet_NaN();
    float max_value = std::numeric_limits<float>::quiet_NaN();
    size_t count = 0;
};

std::vector<HistogramBucket> build_histogram(std::vector<float> values, size_t num_bins, float min_value = std::numeric_limits<float>::quiet_NaN(), float max_value = std::numeric_limits<float>::quiet_NaN());

void print_histogram(std::vector<HistogramBucket> histogram);

std::vector<std::string> format_histogram(std::vector<HistogramBucket> histogram, size_t width = 80);

#endif
