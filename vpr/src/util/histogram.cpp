#include <algorithm>
#include <string>
#include <sstream>
#include <cmath>

#include "vtr_log.h"
#include "vtr_assert.h"
#include "vtr_util.h"

#include "histogram.h"

std::vector<HistogramBucket> build_histogram(std::vector<float> values, size_t num_bins, float min_value, float max_value) {
    std::vector<HistogramBucket> histogram;

    if (values.empty()) return histogram;

    if (std::isnan(min_value)) {
        min_value = *std::min_element(values.begin(), values.end());
    }
    if (std::isnan(max_value)) {
        max_value = *std::max_element(values.begin(), values.end());
    }

    //Determine the bin size
    float range = max_value - min_value;
    float bin_size = range / num_bins;

    //Create the buckets
    float bucket_min = min_value;
    for (size_t ibucket = 0; ibucket < num_bins; ++ibucket) {
        float bucket_max = bucket_min + bin_size;

        histogram.emplace_back(bucket_min, bucket_max);

        bucket_min = bucket_max;
    }

    //To avoid round-off errors we force the max value of the last bucket equal to the max value
    histogram[histogram.size() - 1].max_value = max_value;

    //Count the values into the buckets
    auto comp = [](const HistogramBucket& bucket, float value) {
        return bucket.max_value < value;
    };
    for (auto value : values) {
        //Find the bucket who's max is less than the current slack

        auto iter = std::lower_bound(histogram.begin(), histogram.end(), value, comp);
        VTR_ASSERT(iter != histogram.end());

        iter->count++;
    }

    return histogram;
}

void print_histogram(std::vector<HistogramBucket> histogram) {
    size_t char_width = 80;

    auto lines = format_histogram(histogram, char_width);

    for (auto line : lines) {
        VTR_LOG("%s\n", line.c_str());
    }
}

std::vector<std::string> format_histogram(std::vector<HistogramBucket> histogram, size_t width) {
    std::vector<std::string> lines;

    //Determine the maximum and total count
    size_t max_count = 0;
    size_t total_count = 0;
    for (const HistogramBucket& bucket : histogram) {
        max_count = std::max(max_count, bucket.count);
        total_count += bucket.count;
    }

    if (max_count == 0) return lines; //Nothing to do

    int count_digits = ceil(log10(max_count));

    //Determine the maximum prefix length
    size_t bar_len = width
                     - (18 + 3) //bucket prefix
                     - count_digits
                     - 7  //percentage
                     - 2; //-2 for " |" appended after count

    for (size_t ibucket = 0; ibucket < histogram.size(); ++ibucket) {
        std::string line;

        float pct = histogram[ibucket].count / float(total_count) * 100;

        line += vtr::string_fmt("[% 9.2g:% 9.2g) %*zu (%4.1f%) |", histogram[ibucket].min_value, histogram[ibucket].max_value, count_digits, histogram[ibucket].count, pct);

        size_t num_chars = std::round((double(histogram[ibucket].count) / max_count) * bar_len);
        for (size_t i = 0; i < num_chars; ++i) {
            line += "*";
        }

        lines.push_back(line);
    }

    return lines;
}
