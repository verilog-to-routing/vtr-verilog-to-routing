#include <algorithm>
#include <string>
#include <sstream>
#include <cmath>

#include "vtr_log.h"

#include "histogram.h"


void print_histogram(std::vector<HistogramBucket> histogram) {
    size_t char_width = 80;

    //Determine the maximum count
    size_t max_count = 0.;
    for(HistogramBucket bucket : histogram) {
        max_count = std::max(max_count, bucket.count);
    }

    if(max_count == 0) return; //Nothing to do

    int count_digits = ceil(log10(max_count));

    //Determine the maximum prefix length
    size_t bar_len = char_width
                     - (18 +3) //bucket prefix
                     - count_digits
                     - 2; //-2 for " |" appended after count

    for(size_t ibucket = 0; ibucket < histogram.size(); ++ibucket) {
        vtr::printf("[% 9.2g:% 9.2g) %*zu |", histogram[ibucket].min_value, histogram[ibucket].max_value, count_digits, histogram[ibucket].count);

        size_t num_chars = std::round((double(histogram[ibucket].count) / max_count) * bar_len);
        for(size_t i = 0; i < num_chars; ++i) {
            vtr::printf("*");
        }
        vtr::printf("\n");
    }

}
